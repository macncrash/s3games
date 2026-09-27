#include "ferry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ferrypass {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr float END_Y = 236.f;
constexpr float CLOCK = 46.f;
constexpr float ZOOM = 7.4f;
constexpr float STORM_V = 4.15f;
constexpr float MARGIN = 0.35f;

float gauss(float y, float c, float w) {
    float d = (y - c) / w;
    return std::exp(-d * d);
}

float centerAt(float y) {
    return 4.2f * std::sin(y * 0.022f) + 1.5f * std::sin(y * 0.047f + 0.5f);
}

float halfAt(float y) {
    float w = 10.2f - 2.2f * gauss(y, 78.f, 22.f) - 2.3f * gauss(y, 164.f, 20.f);
    return std::max(w, 7.6f);
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
    stormY_ = -28.f;
    x_ = centerAt(8.f);
    y_ = 8.f;
    hdg_ = 0;
    speed_ = 1.2f;
    yawV_ = 0;
    thrust_ = 0;
    steer_ = 0;
    camX_ = x_;
    camY_ = y_;
}

void Game::fail(const char* why) {
    if (over_) return;
    over_ = true;
    won_ = false;
    why_ = why;
    mode_ = Mode::Fail;
    sys_->apu.tone(0, 0.f, 0.f);
    sys_->apu.noiseBurst(0.45f, 380.f, 0.28f);
}

void Game::win() {
    if (over_) return;
    over_ = true;
    won_ = true;
    why_ = "";
    mode_ = Mode::Win;
    sys_->apu.tone(0, 0.f, 0.f);
    sys_->apu.tone(1, 523.f, 0.12f);
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

void Game::pilot() {
    float look = y_ + 16.f + speed_ * 1.1f;
    float err = centerAt(look) - x_;
    float want = std::clamp(err * 0.28f, -0.5f, 0.5f);
    steer_ = std::clamp((want - hdg_) * 4.2f - yawV_ * 1.4f, -1.f, 1.f);
    thrust_ = std::fabs(hdg_) > 0.55f ? 0.45f : 1.f;
}

void Game::drive(float dt) {
    speed_ += (thrust_ * 7.4f - speed_ * 0.92f) * dt;
    yawV_ += steer_ * 4.8f * dt;
    yawV_ *= 0.86f;
    hdg_ += yawV_ * dt;
    if (hdg_ > 1.2f) hdg_ = 1.2f;
    if (hdg_ < -1.2f) hdg_ = -1.2f;
    x_ += std::sin(hdg_) * speed_ * dt;
    y_ += std::cos(hdg_) * speed_ * dt;
}

bool Game::rockHit() const {
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    const float along[3] = {0.92f, 0.f, -0.92f};
    const float side[2] = {-0.92f, 0.92f};
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
    if (bot_) pilot();
    else {
        const gs::Pad& pad = sys_->pad;
        thrust_ = pad.down(gs::BTN_UP) ? 1.f : (pad.down(gs::BTN_DOWN) ? -0.55f : 0.15f);
        if (pad.accel > 0.05f) thrust_ = pad.accel;
        if (pad.brake > 0.05f) thrust_ = -pad.brake;
        steer_ = pad.axisX;
        if (pad.down(gs::BTN_LEFT)) steer_ = -1.f;
        if (pad.down(gs::BTN_RIGHT)) steer_ = 1.f;
    }
    drive(DT);

    if (rockHit()) {
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
    camX_ += (x_ - camX_) * 0.1f;
    camY_ += (y_ - camY_) * 0.12f;
    float hum = mode_ == Mode::Play ? 0.05f + std::min(speed_, 8.f) * 0.008f : 0.f;
    sys_->apu.tone(0, 62.f + std::max(speed_, 0.f) * 9.f, hum);
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
    if (px < -80.f || px > gs::SCREEN_W + 80.f || py < -80.f || py > gs::SCREEN_H + 80.f) return;
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
    float stormScr = sy(stormY_);
    uint16_t water = gs::rgb4(2, 7, 11);
    uint16_t storm = gs::rgb4(3, 3, 6);
    uint16_t lip = gs::rgb4(8, 8, 11);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (float(y) > stormScr + 6.f) vdp.lineBackdrop[y] = storm;
        else if (float(y) > stormScr - 2.f) vdp.lineBackdrop[y] = lip;
        else vdp.lineBackdrop[y] = water;
        vdp.lineFog[y] = float(y) > stormScr ? 6 : 0;
    }

    float y0 = camY_ - 22.f;
    float y1 = camY_ + 24.f;
    for (float wy = std::floor(y0 / 8.f) * 8.f; wy < y1; wy += 8.f) {
        float c = centerAt(wy);
        float h = halfAt(wy);
        place(art_.cliff, c - h - 2.4f, wy, 5.2f, PAL_CLIFF);
        place(art_.cliff, c + h + 2.4f, wy, 5.2f, PAL_CLIFF, true);
        place(art_.scrub, c - h - 0.6f, wy + 2.f, 1.4f, PAL_CLIFF);
        place(art_.rock, c - h + 0.2f, wy + 4.f, 1.5f, PAL_CLIFF);
        place(art_.rock, c + h - 0.2f, wy + 1.f, 1.3f, PAL_CLIFF, true);
    }
    place(art_.hut, centerAt(40.f) - halfAt(40.f) - 3.2f, 40.f, 3.2f, PAL_HUT);
    place(art_.lamp, centerAt(70.f) - halfAt(70.f) + 0.4f, 70.f, 2.2f, PAL_LIGHT);
    place(art_.lamp, centerAt(156.f) + halfAt(156.f) - 0.4f, 156.f, 2.2f, PAL_LIGHT);
    place(art_.post, centerAt(END_Y) - 3.2f, END_Y, 2.6f, PAL_TAPE);
    place(art_.post, centerAt(END_Y) + 3.2f, END_Y, 2.6f, PAL_TAPE);
    place(art_.tape, centerAt(END_Y), END_Y + 0.15f, 0.7f, PAL_TAPE);

    float gx = centerAt(camY_ + 10.f) + std::sin(playT_ * 0.7f) * 3.f;
    place(art_.gull[int(playT_ * 3.f) & 1], gx, camY_ + 9.f, 0.9f, PAL_GULL);

    const gs::Mipped& hull = art_.hull[yawOf(hdg_)];
    float hh = float(hull.h) * ((kHalfL * 2.f) * ZOOM / float(kHullPx));
    if (speed_ > 0.4f) {
        place(art_.foam, x_ - std::sin(hdg_) * kHalfL * 0.85f, y_ - std::cos(hdg_) * kHalfL * 0.85f, 1.3f, PAL_FOAM);
    }
    spr(hull, sx(x_), sy(y_), hh, PAL_FERRY);

    int left = int(stormLeft());
    if (left < 0) left = 0;
    char line[64];
    std::snprintf(line, sizeof line, "STORM %02d:%02d", left / 60, left % 60);
    hud(1, 1, line, left < 10 ? PAL_ALERT : PAL_HUD);
    hud(28, 1, "FERRY PASS", PAL_DIM);
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 74, 26, PAL_BANNER);
        hudC(15, "CLEAR THE PASS", PAL_HUD);
        hudC(17, "BEFORE THE STORM CLOCK", PAL_HUD);
        hudC(19, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        hudC(24, "UP THROTTLE   ARROWS STEER", PAL_DIM);
    } else if (mode_ == Mode::Pause) {
        spr(art_.paused, 160, 100, 24, PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        spr(art_.clearWord, 160, 84, 30, PAL_WIN);
        hudC(16, "THE PASS IS CLEAR", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        spr(art_.missed, 160, 84, 18, PAL_ALERT);
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
        } else step();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        begin();
    }
    draw();
    if (mode_ == Mode::Win) sys.apu.tone(1, 0.f, 0.f);
}

}  // namespace ferrypass
