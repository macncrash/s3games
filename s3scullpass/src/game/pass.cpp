#include "pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace scullpass {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr float END_Y = 214.f;
constexpr float CLOCK = 62.f;
constexpr float ZOOM = 9.2f;
constexpr float STORM_V = 3.05f;
constexpr float MARGIN = 0.22f;

float gauss(float y, float c, float w) {
    float d = (y - c) / w;
    return std::exp(-d * d);
}

float centerAt(float y) {
    return 3.1f * std::sin(y * 0.028f) + 1.15f * std::sin(y * 0.061f + 0.7f);
}

float halfAt(float y) {
    float w = 6.15f - 1.35f * gauss(y, 62.f, 16.f) - 1.45f * gauss(y, 148.f, 15.f);
    return std::max(w, 4.55f);
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(3, 3, 6));
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    why_ = "";
    playT_ = 0;
    stormY_ = -40.f;
    x_ = centerAt(6.f);
    y_ = 6.f;
    hdg_ = 0;
    speed_ = 1.4f;
    yawV_ = 0;
    steer_ = 0;
    camX_ = x_;
    camY_ = y_;
    strokeCd_ = 0;
    catch_ = 0;
    wantStroke_ = false;
    wasStroke_ = false;
}

void Game::fail(const char* why) {
    if (over_) return;
    over_ = true;
    won_ = false;
    why_ = why;
    mode_ = Mode::Fail;
    sys_->apu.tone(0, 0.f, 0.f);
    sys_->apu.noiseBurst(0.4f, 320.f, 0.26f);
}

void Game::win() {
    if (over_) return;
    over_ = true;
    won_ = true;
    why_ = "";
    mode_ = Mode::Win;
    sys_->apu.tone(0, 0.f, 0.f);
    sys_->apu.tone(1, 659.f, 0.12f);
}

float Game::stormLeft() const { return std::max(0.f, CLOCK - playT_); }
float Game::bowY() const { return y_ + std::cos(hdg_) * kHalfL; }
float Game::sternY() const { return y_ - std::cos(hdg_) * kHalfL; }
float Game::sx(float wx) const { return gs::SCREEN_W * 0.5f + (wx - camX_) * ZOOM; }
float Game::sy(float wy) const { return gs::SCREEN_H * 0.5f - (wy - camY_) * ZOOM; }
int Game::yawOf(float h) const {
    int i = int(std::lround(-h / (PI / 4.f)));
    return (i % 8 + 8) % 8;
}

void Game::stroke() {
    if (strokeCd_ > 0) return;
    speed_ = std::min(speed_ + 1.55f, 6.4f);
    strokeCd_ = 11;
    catch_ = 8;
    sys_->apu.tone(2, 180.f, 0.06f);
}

void Game::pilot() {
    float look = y_ + 14.f + speed_ * 1.4f;
    float err = centerAt(look) - x_;
    float want = std::clamp(err * 0.34f, -0.55f, 0.55f);
    steer_ = std::clamp((want - hdg_) * 5.0f - yawV_ * 1.6f, -1.f, 1.f);
    wantStroke_ = speed_ < 4.85f;
}

void Game::drive(float dt) {
    speed_ += (0.15f - speed_ * 1.05f) * dt;
    if (speed_ < 0.f) speed_ = 0.f;
    yawV_ += steer_ * (2.2f + speed_ * 0.55f) * dt;
    yawV_ *= 0.84f;
    hdg_ += yawV_ * dt;
    hdg_ = std::clamp(hdg_, -1.15f, 1.15f);
    x_ += std::sin(hdg_) * speed_ * dt;
    y_ += std::cos(hdg_) * speed_ * dt;
}

bool Game::wallHit() const {
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    const float along[3] = {1.f, 0.15f, -0.9f};
    const float side[2] = {-1.f, 1.f};
    for (float a : along) {
        for (float b : side) {
            float wx = x_ + fx * a * kHalfL + rx * b * kHalfB;
            float wy = y_ + fy * a * kHalfL + ry * b * kHalfB;
            if (std::fabs(wx - centerAt(wy)) > halfAt(wy) - MARGIN) return true;
        }
    }
    return false;
}

void Game::step() {
    playT_ += DT;
    stormY_ += STORM_V * DT;
    if (strokeCd_ > 0) strokeCd_--;
    if (catch_ > 0) catch_--;
    if (bot_) pilot();
    else {
        const gs::Pad& pad = sys_->pad;
        steer_ = pad.axisX;
        if (pad.down(gs::BTN_LEFT)) steer_ = -1.f;
        if (pad.down(gs::BTN_RIGHT)) steer_ = 1.f;
        wantStroke_ = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_UP) || pad.accel > 0.2f;
        if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) speed_ *= 0.96f;
    }
    bool edge = wantStroke_ && !wasStroke_;
    wasStroke_ = wantStroke_;
    if (bot_) {
        if (wantStroke_) stroke();
    } else if (edge) {
        stroke();
    }
    drive(DT);

    if (wallHit()) {
        fail("missed the end");
        return;
    }
    if (stormY_ >= sternY() || playT_ >= CLOCK) {
        fail("missed the end");
        return;
    }
    if (bowY() >= END_Y) {
        float c = centerAt(END_Y);
        if (std::fabs(x_ - c) > halfAt(END_Y) - kHalfB) fail("missed the end");
        else win();
        return;
    }
    camX_ += (x_ - camX_) * 0.12f;
    camY_ += ((y_ + 2.2f) - camY_) * 0.14f;
    if (catch_ == 0) sys_->apu.tone(2, 0.f, 0.f);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(m.h, 1));
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 400L);
    long sh = std::clamp(std::lround(h), 1L, 400L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::lround(cx - sw * 0.5f));
    s.y = int16_t(std::lround(cy - sh * 0.5f));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip) {
    float px = sx(wx), py = sy(wy);
    if (px < -90.f || px > gs::SCREEN_W + 90.f || py < -90.f || py > gs::SCREEN_H + 90.f) return;
    spr(m, px, py, worldH * ZOOM, pal, flip);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        int x = col + i;
        if (x < 0 || x > 39) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.clear();
    vdp.B.clear();

    float stormScr = sy(stormY_);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.road[y].on = false;
        float wy = camY_ + (gs::SCREEN_H * 0.5f - float(y)) / ZOOM;
        int band = int(std::floor(wy * 0.7f)) & 1;
        uint16_t water = band ? gs::rgb4(2, 6, 10) : gs::rgb4(1, 5, 9);
        uint16_t lip = gs::rgb4(6, 7, 10);
        uint16_t storm = gs::rgb4(3, 3, 6);
        if (float(y) > stormScr + 4.f) vdp.lineBackdrop[y] = storm;
        else if (float(y) > stormScr - 2.f) vdp.lineBackdrop[y] = lip;
        else vdp.lineBackdrop[y] = water;
        vdp.lineFog[y] = float(y) > stormScr ? 5 : 0;
    }

    int y0 = int(std::floor(camY_ - 16.f));
    int y1 = int(std::ceil(camY_ + 18.f));
    for (int k = y0; k <= y1; k++) {
        if ((k & 3) != 0) continue;
        float wy = float(k);
        float c = centerAt(wy);
        float h = halfAt(wy);
        place(art_.cliff, c - h - 1.6f, wy, 3.4f, PAL_CLIFF, false);
        place(art_.cliff, c + h + 1.6f, wy, 3.4f, PAL_CLIFF, true);
        if ((k & 7) == 0) {
            place(art_.pine, c - h - 3.1f, wy + 1.2f, 2.6f, PAL_PINE);
            place(art_.pine, c + h + 3.1f, wy + 0.4f, 2.4f, PAL_PINE);
        }
    }

    float tapeY = END_Y;
    if (sy(tapeY) > -20.f && sy(tapeY) < gs::SCREEN_H + 20.f) {
        float c = centerAt(tapeY);
        float h = halfAt(tapeY);
        place(art_.post, c - h + 0.3f, tapeY, 2.2f, PAL_TAPE);
        place(art_.post, c + h - 0.3f, tapeY, 2.2f, PAL_TAPE);
        for (float u = -h + 0.8f; u < h - 0.4f; u += 2.4f) place(art_.tape, c + u, tapeY, 0.55f, PAL_TAPE);
    }

    int yaw = yawOf(hdg_);
    float hh = 7.2f * ZOOM * (kHalfL * 2.f) / 52.f;
    float ox = std::cos(hdg_) * 1.15f;
    float oy = std::sin(hdg_) * 1.15f;
    float reach = catch_ > 0 ? 1.35f : 1.7f;
    place(art_.oar, x_ - ox * reach, y_ + oy * 0.2f, 1.15f, PAL_OAR, hdg_ > 0);
    place(art_.oar, x_ + ox * reach, y_ - oy * 0.2f, 1.15f, PAL_OAR, hdg_ < 0);
    spr(art_.hull[yaw], sx(x_), sy(y_), hh, PAL_HULL);

    int left = int(stormLeft());
    if (left < 0) left = 0;
    char line[64];
    std::snprintf(line, sizeof line, "STORM %02d:%02d", left / 60, left % 60);
    hud(1, 1, line, left < 12 ? PAL_ALERT : PAL_HUD);
    hud(27, 1, "SCULL PASS", PAL_DIM);
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 78, 28, PAL_BANNER);
        hudC(15, "CLEAR THE PASS", PAL_HUD);
        hudC(17, "BEFORE THE STORM CLOCK", PAL_HUD);
        hudC(19, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        hudC(24, "A STROKE    ARROWS STEER", PAL_DIM);
    } else if (mode_ == Mode::Pause) {
        spr(art_.paused, 160, 100, 24, PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        spr(art_.clearWord, 160, 84, 32, PAL_WIN);
        hudC(16, "THE PASS IS CLEAR", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        spr(art_.missed, 160, 84, 22, PAL_ALERT);
        hudC(16, "MISSED THE END", PAL_ALERT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || bot_) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            sys.apu.tone(0, 0.f, 0.f);
            sys.apu.tone(2, 0.f, 0.f);
        } else step();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        begin();
    }
    draw();
    if (mode_ == Mode::Win) sys.apu.tone(1, 0.f, 0.f);
}

}  // namespace scullpass
