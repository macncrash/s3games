#include "game/lotbann.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace lotbann {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kWorldW = 1400.0f;
constexpr float kWorldH = 800.0f;
constexpr float kSpawnX = 110.0f;
constexpr float kSpawnY = 72.0f;
constexpr float kGutter = 64.0f;
constexpr float kFloorY = 748.0f;
constexpr float kBannerX = 1260.0f;
constexpr float kBannerY = 742.0f;
constexpr float kHomeX0 = 28.0f, kHomeX1 = 210.0f;
constexpr float kHomeY0 = 28.0f, kHomeY1 = 132.0f;
constexpr float kSpeed = 148.0f;
constexpr float kCarry = 124.0f;
constexpr float kBody = 9.0f;
constexpr float kClock = 90.0f;
constexpr float kGrab = 30.0f;

float wrapX(float x) {
    while (x < 0) x += kWorldW;
    while (x >= kWorldW) x -= kWorldW;
    return x;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (!has_) return 1;
    if (py_ > 420.0f) return 2;
    return 3;
}

void Game::resetRun() {
    const float y0[4] = {156, 326, 496, 646};
    const float dir[4] = {1, -1, 1, -1};
    const float spd[4] = {118, 136, 124, 108};
    const float seed[4][2] = {{280, 980}, {160, 860}, {440, 1140}, {80, 780}};
    stall_.clear();
    for (int i = 0; i < 4; i++) {
        lane_[i].y0 = y0[i];
        lane_[i].y1 = y0[i] + 58.0f;
        lane_[i].dir = dir[i];
        lane_[i].speed = spd[i];
        lane_[i].carX[0] = seed[i][0];
        lane_[i].carX[1] = seed[i][1];
    }
    const float rowY[3] = {236, 406, 576};
    for (float y : rowY) {
        for (float x = 150.0f; x < 1320.0f; x += 96.0f) {
            bool alley = (x > 450.0f && x < 560.0f) || (x > 940.0f && x < 1060.0f);
            if (alley) continue;
            stall_.push_back({x, y, 70.0f, 34.0f});
        }
    }
    px_ = kSpawnX;
    py_ = kSpawnY;
    face_ = 1;
    has_ = false;
    lives_ = 3;
    clock_ = kClock;
    inv_ = shake_ = 0;
    playT_ = 0;
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    blip(440.0f, 0.05f, 0.08f);
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    over_ = true;
    shake_ = 0.3f;
    sys_->rumble(0.2f, 0.55f, 160);
    sys_->setLight(40, 140, 50);
    blip(660.0f, 0.08f, 0.22f);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    sys_->rumble(0.5f, 0.2f, 200);
    sys_->setLight(140, 20, 16);
    blip(110.0f, 0.08f, 0.25f);
}

bool Game::blocked(float x, float y) const {
    if (x < 16.0f || y < 16.0f || x > kWorldW - 16.0f || y > kWorldH - 16.0f) return true;
    for (const Box& b : stall_) {
        if (x + kBody > b.x && x - kBody < b.x + b.w && y + kBody > b.y && y - kBody < b.y + b.h) return true;
    }
    return false;
}

bool Game::laneUnsafe(int i) const {
    const Lane& L = lane_[i];
    for (float t = 0; t <= 0.75f; t += 0.05f) {
        for (int c = 0; c < 2; c++) {
            float cx = wrapX(L.carX[c] + L.dir * L.speed * t);
            if (std::abs(cx - px_) < 60.0f) return true;
        }
    }
    return false;
}

bool Game::crossBlocked(float yGoal) const {
    for (int i = 0; i < 4; i++) {
        const Lane& L = lane_[i];
        bool in = py_ > L.y0 - kBody && py_ < L.y1 + kBody;
        if (in) return false;
        float lo = std::min(py_, yGoal);
        float hi = std::max(py_, yGoal);
        if (hi > L.y0 && lo < L.y1 && laneUnsafe(i)) return true;
    }
    return false;
}

void Game::hit(int lane) {
    lives_--;
    inv_ = 1.15f;
    shake_ = 0.25f;
    has_ = false;
    const Lane& L = lane_[lane];
    float above = L.y0 - 18.0f;
    float below = L.y1 + 18.0f;
    py_ = (kBannerY - py_ > py_ - kSpawnY) ? above : below;
    if (blocked(px_, py_)) py_ = (py_ < L.y0) ? above : below;
    sys_->rumble(0.7f, 0.3f, 120);
    blip(90.0f, 0.07f, 0.12f);
    if (lives_ <= 0) lose();
}

void Game::bot(bool& left, bool& right, bool& up, bool& down) {
    float goalX = kGutter;
    float goalY = kFloorY;
    if (!has_) {
        if (py_ < kFloorY - 8.0f) {
            goalX = kGutter;
            goalY = kFloorY;
        } else {
            goalX = kBannerX;
            goalY = kFloorY;
        }
    } else if (px_ > kGutter + 24.0f && py_ > 400.0f) {
        goalX = kGutter;
        goalY = kFloorY;
    } else {
        goalX = kGutter;
        goalY = 70.0f;
    }
    if (std::abs(px_ - goalX) > 6.0f) {
        if (px_ < goalX) right = true;
        else left = true;
        return;
    }
    if (std::abs(py_ - goalY) <= 6.0f) return;
    if (crossBlocked(goalY)) return;
    if (py_ < goalY) down = true;
    else up = true;
}

void Game::stepPlay(bool left, bool right, bool up, bool down) {
    playT_ += DT;
    clock_ -= DT;
    if (clock_ <= 0) {
        clock_ = 0;
        lose();
        return;
    }
    if (inv_ > 0) inv_ -= DT;

    for (Lane& L : lane_) {
        for (int c = 0; c < 2; c++) L.carX[c] = wrapX(L.carX[c] + L.dir * L.speed * DT);
    }

    float dx = (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f);
    float dy = (down ? 1.0f : 0.0f) - (up ? 1.0f : 0.0f);
    float m = std::hypot(dx, dy);
    if (m > 0) {
        dx /= m;
        dy /= m;
    }
    if (dx != 0) face_ = dx > 0 ? 1 : -1;
    float spd = has_ ? kCarry : kSpeed;
    float nx = px_ + dx * spd * DT;
    float ny = py_ + dy * spd * DT;
    if (!blocked(nx, py_)) px_ = nx;
    if (!blocked(px_, ny)) py_ = ny;

    if (inv_ <= 0 && mode_ == Mode::Play) {
        for (int i = 0; i < 4; i++) {
            const Lane& L = lane_[i];
            if (py_ < L.y0 || py_ > L.y1) continue;
            for (int c = 0; c < 2; c++) {
                if (std::abs(L.carX[c] - px_) < 40.0f) {
                    hit(i);
                    return;
                }
            }
        }
    }

    if (!has_ && std::hypot(px_ - kBannerX, py_ - kBannerY) < kGrab) {
        has_ = true;
        blip(720.0f, 0.07f, 0.1f);
        sys_->rumble(0.15f, 0.4f, 70);
        sys_->setLight(160, 40, 24);
    }
    if (has_ && px_ > kHomeX0 && px_ < kHomeX1 && py_ > kHomeY0 && py_ < kHomeY1) win();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    resetRun();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys.apu.tone(0, 0, 0);
    }
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT);

    bool left = false, right = false, up = false, down = false;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        titleHold_++;
        bool go = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C);
        if (bot_ && titleHold_ > 8) go = true;
        if (go) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        if (bot_) bot(left, right, up, down);
        else {
            left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.3f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.3f;
            up = pad.down(gs::BTN_UP) || pad.axisY > 0.3f;
            down = pad.down(gs::BTN_DOWN) || pad.axisY < -0.3f;
        }
        if (mode_ == Mode::Play) stepPlay(left, right, up, down);
    } else if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START) || (bot_ && t_ > 0)) {
        if (!bot_) {
            mode_ = Mode::Title;
            titleHold_ = 0;
            over_ = false;
        }
    }

    camX_ = std::clamp(px_ - 160.0f, 0.0f, kWorldW - gs::SCREEN_W);
    camY_ = std::clamp(py_ - 100.0f, 0.0f, kWorldH - gs::SCREEN_H);
    if (mode_ == Mode::Title) {
        camX_ = 0;
        camY_ = 0;
    }
    draw();
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - camX_ - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - camY_ - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 8 || s.y > gs::SCREEN_H + 8 || s.x + s.w < -8 || s.y + s.h < -8) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    float width = 0;
    for (unsigned char c : s) {
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.0f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale + scale;
    }
    if (align == 0) x -= width * 0.5f;
    else if (align > 0) x -= width;
    for (unsigned char c : s) {
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            x += 8.0f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        gs::Sprite sp;
        float h = float(g.h) * scale;
        float w = float(g.w) * scale;
        sp.w = int16_t(std::lround(w));
        sp.h = int16_t(std::lround(h));
        sp.x = int16_t(std::lround(x));
        sp.y = int16_t(std::lround(y - h * 0.5f));
        sp.img = g.pick(h);
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        x += w + scale;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int shade = 3 + (y > 160 ? 1 : 0);
        vdp.lineBackdrop[y] = gs::rgb4(shade, shade, shade + 1);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    auto mark = [&](float x, float y, float w, float h) { spr(art_.dash, x, y, h, PAL_MARK, false); (void)w; };

    for (int i = 0; i < 4; i++) {
        float y0 = lane_[i].y0;
        float y1 = lane_[i].y1;
        for (float x = 20; x < kWorldW; x += 36.0f) {
            mark(x, y0, 22, 3);
            mark(x, y1, 22, 3);
        }
    }
    spr(art_.pad, 118, 78, 40, PAL_MARK, false);
    spr(art_.booth, 120, 70, 52, PAL_BOOTH, false);
    const float lamps[][2] = {{200, 120}, {420, 300}, {760, 470}, {1100, 300}, {1280, 640}, {240, 700}};
    for (auto p : lamps) spr(art_.lamp, p[0], p[1], 46, PAL_LAMP, false);

    int shade = 0;
    for (const Box& b : stall_) {
        int pal = (shade++ % 3 == 0) ? PAL_CAR : (shade % 3 == 1) ? PAL_CARB : PAL_CARC;
        spr(art_.car, b.x + b.w * 0.5f, b.y + b.h * 0.5f, 30, pal, false);
    }
    static const int trafficPal[4] = {PAL_CAR, PAL_CARB, PAL_CARC, PAL_CAR};
    for (int i = 0; i < 4; i++) {
        float cy = (lane_[i].y0 + lane_[i].y1) * 0.5f;
        for (int c = 0; c < 2; c++) spr(art_.car, lane_[i].carX[c], cy, 28, trafficPal[i], lane_[i].dir < 0);
    }

    if (!has_) spr(art_.banner, kBannerX, kBannerY - 8, 52, PAL_BANNER, false);
    float bob = std::sin(playT_ * 10.0f) * 1.5f;
    spr(art_.runner, px_, py_ + bob, 26, PAL_PLAYER, face_ < 0);
    if (has_) spr(art_.banner, px_ + face_ * 10.0f, py_ - 16, 28, PAL_BANNER, face_ < 0);
    if (inv_ > 0 && (int(inv_ * 16) & 1)) spr(art_.puff, px_, py_, 22, PAL_MARK, false);

    if (mode_ == Mode::Title) {
        text("S3 LOTBANN", 160, 78, 1.15f, PAL_HUD, 0);
        text("BRING THE BANNER BACK", 160, 108, 0.55f, PAL_HUD, 0);
        text("ARROWS MOVE", 160, 140, 0.45f, PAL_HUD, 0);
        text("A TO STEP ONTO THE LOT", 160, 158, 0.45f, PAL_HUD, 0);
        hud(1, 26, S3_VERSION_STRING, PAL_HUD);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "LIVES %d", lives_);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "TIME %d", int(std::ceil(clock_)));
        hud(30, 1, buf, clock_ < 15.0f ? PAL_HUD : PAL_HUD);
        hud(1, 26, has_ ? "BANNER IN HAND" : "BANNER IS ACROSS THE LOT", PAL_HUD);
        if (mode_ == Mode::Pause) hudC(13, "PAUSED", PAL_HUD);
        if (mode_ == Mode::Victory) {
            hudC(11, "BANNER IS BACK", PAL_HUD);
            hudC(13, "THE LOT KEEPS IT", PAL_HUD);
        }
        if (mode_ == Mode::Over) {
            hudC(11, "THE LOT LOST THE BANNER", PAL_HUD);
            hudC(13, "THAT IS A LOSS", PAL_HUD);
        }
    }
}

}  // namespace lotbann
