#include "pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace mailpass {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kLen = 780.f;
constexpr float kClock = 32.f;
constexpr float kHalfVan = 1.02f;
constexpr float kHorizon = 76.f;
constexpr float kNear = 7.2f;
constexpr float kPpm = 38.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    auto ch = [](uint16_t c, int sh) { return (c >> sh) & 15; };
    int r = int(ch(a, 8) + (ch(b, 8) - ch(a, 8)) * t);
    int g = int(ch(a, 4) + (ch(b, 4) - ch(a, 4)) * t);
    int bl = int(ch(a, 0) + (ch(b, 0) - ch(a, 0)) * t);
    return gs::rgb4(r, g, bl);
}

}  // namespace

float Game::centerAt(float s) const {
    return 3.6f * std::sin(s * 0.016f) + 1.8f * std::sin(s * 0.037f + 0.8f);
}

float Game::halfAt(float s) const {
    float n = s / kLen;
    float pinch = std::exp(-std::pow((n - 0.58f) / 0.10f, 2.f));
    float shelf = std::exp(-std::pow((n - 0.30f) / 0.055f, 2.f));
    return 6.6f - pinch * 2.35f - shelf * 1.15f;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (s_ > kLen * 0.82f) return 3;
    if (s_ > kLen * 0.48f) return 2;
    return 1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
    time_ = 0;
    storm_ = kClock;
    s_ = 40.f;
    y_ = centerAt(s_);
    v_ = 0;
    vy_ = 0;
    shake_ = 0;
    chime_ = -1;
}

void Game::startRun() {
    s_ = 18.f;
    y_ = centerAt(s_);
    v_ = 8.f;
    vy_ = 0;
    time_ = 0;
    storm_ = kClock;
    shake_ = 0;
    why_ = "";
    over_ = false;
    won_ = false;
    chime_ = -1;
    mode_ = Mode::Run;
    blip(220.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.08f);
    beep_ = 0.08f;
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    over_ = true;
    won_ = false;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    v_ *= 0.2f;
    sys_->apu.noiseBurst(0.4f, 700.f, 0.24f);
    sys_->apu.tone(0, 70.f, 0.2f);
    beep_ = 0.28f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    over_ = true;
    won_ = true;
    mode_ = Mode::Win;
    chime_ = 0;
    chimeT_ = 0;
    why_ = "clear";
}

void Game::pilot(float& gas, float& steer) const {
    const gs::Pad& pad = sys_->pad;
    gas = 0;
    steer = 0;
    if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.accel > 0.2f) gas = 1.f;
    if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B) || pad.brake > 0.2f) gas = -0.7f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (std::fabs(pad.axisX) > 0.18f) steer = pad.axisX;
    if (!bot_) return;
    float look = 28.f + v_ * 0.55f;
    float aim = centerAt(s_ + look);
    float slope = (centerAt(s_ + 10.f) - centerAt(s_)) / 10.f;
    float err = aim - y_;
    steer = clampf(err * 0.42f + (slope * v_ - vy_) * 0.08f, -1.f, 1.f);
    float room = halfAt(s_ + 16.f) - kHalfVan - std::fabs(y_ - centerAt(s_ + 16.f));
    gas = room < 1.6f ? 0.55f : 1.f;
}

void Game::physics(float gas, float steer) {
    float prev = s_;
    float accel = gas > 0.f ? 26.f : 34.f;
    v_ += gas * accel * DT;
    v_ *= std::exp(-DT * (gas > 0.2f ? 0.35f : 1.1f));
    v_ = clampf(v_, 0.f, 38.f);
    float grip = 9.5f + v_ * 0.35f;
    vy_ += (steer * grip - vy_ * 4.2f) * DT;
    s_ += v_ * DT;
    y_ += vy_ * DT;
    storm_ -= DT;
    time_ += DT;

    float worst = 99.f;
    for (int i = -1; i <= 1; i++) {
        float ps = s_ + float(i) * 2.2f;
        float room = halfAt(ps) - std::fabs(y_ - centerAt(ps)) - kHalfVan;
        worst = std::min(worst, room);
    }
    if (worst < 0.f) {
        fail("the van hit the rock");
        return;
    }
    if (storm_ <= 0.f && prev < kLen) {
        fail("the storm closed the pass");
        return;
    }
    if (s_ >= kLen) {
        if (storm_ <= 0.f) fail("the storm closed the pass");
        else win();
    }
}

bool Game::project(float wz, float wy, float& sx, float& sy, float& ppm) const {
    float dz = wz - s_;
    if (dz < 0.6f || dz > 90.f) return false;
    float n = kNear / dz;
    sy = kHorizon + n * (gs::SCREEN_H - kHorizon);
    ppm = kPpm * n;
    float camY = y_;
    sx = 160.f + (wy - camY) * ppm;
    float bend = (centerAt(wz) - centerAt(s_)) * ppm * 0.35f;
    sx += bend;
    return sy > -30.f && sy < gs::SCREEN_H + 20.f;
}

void Game::spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip, int fog) {
    if (ht < 1.3f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.f, 400.f)));
    s.h = int16_t(std::lround(clampf(ht, 1.f, 300.f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet - s.h));
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::skyRoad() {
    float gloom = clampf(1.f - storm_ / kClock, 0.f, 1.f);
    if (mode_ == Mode::Title) gloom = 0.12f;
    uint16_t zen = lerpC(gs::rgb4(2, 3, 7), gs::rgb4(3, 3, 4), gloom);
    uint16_t mid = lerpC(gs::rgb4(6, 7, 10), gs::rgb4(5, 5, 6), gloom);
    uint16_t hor = lerpC(gs::rgb4(11, 10, 9), gs::rgb4(7, 7, 8), gloom);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(10, 3, 3), 0.45f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(6, 11, 7), 0.4f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.32f ? lerpC(zen, mid, t / 0.32f) : lerpC(mid, hor, (t - 0.32f) / 0.68f);
        gs::RoadLine& r = sys_->vdp.road[y];
        r.on = false;
        sys_->vdp.lineFog[y] = uint8_t(gloom > 0.45f ? int((gloom - 0.45f) * 12.f) : 0);
        if (y <= int(kHorizon)) continue;
        float n = (y - kHorizon) / (gs::SCREEN_H - kHorizon);
        if (n < 0.02f) continue;
        float dz = kNear / n;
        float world = s_ + dz;
        float c = centerAt(world);
        float hw = halfAt(world);
        r.on = true;
        r.cx = 160.f + (c - y_) * kPpm * n;
        r.hw = hw * kPpm * n;
        r.v = world * 6.f;
        r.pal = PAL_ROAD;
        r.style = gs::ROAD_ROCKY;
        r.band = (int(world * 0.35f) & 3) == 0 ? 1 : 0;
        r.left = gs::GROUND_SNOWWALL;
        r.right = gs::GROUND_SNOWWALL;
        int fog = int(clampf((1.f - n) * 9.f + gloom * 4.f, 0.f, 14.f));
        sys_->vdp.lineFog[y] = uint8_t(std::max<int>(sys_->vdp.lineFog[y], fog));
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = true;
    sys_->vdp.HUD.clear();
    sys_->vdp.roadTime = int(time_ * 60.f);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::draw() {
    skyRoad();
    sys_->vdp.clearSprites();
    auto fogOf = [](float ppm) { return int(clampf(11.f - ppm * 0.18f, 0.f, 14.f)); };

    for (int i = 12; i >= 0; i--) {
        float base = std::floor(s_ / 22.f) * 22.f + float(i) * 22.f;
        float sx, sy, ppm;
        float left = centerAt(base) - halfAt(base) - 1.1f;
        float right = centerAt(base) + halfAt(base) + 1.1f;
        if (project(base, left, sx, sy, ppm)) spr(art_.pine, sx, sy, ppm * 4.2f, PAL_PINE, false, fogOf(ppm));
        if (project(base + 11.f, right, sx, sy, ppm))
            spr(art_.pine, sx, sy, ppm * 3.6f, PAL_PINE, true, fogOf(ppm));
        if ((int(base / 22.f) & 1) && project(base + 6.f, left + 0.4f, sx, sy, ppm))
            spr(art_.cairn, sx, sy, ppm * 1.5f, PAL_CAIRN, false, fogOf(ppm));
        if (project(base + 16.f, right - 0.3f, sx, sy, ppm))
            spr(art_.sack, sx, sy, ppm * 1.1f, PAL_SACK, false, fogOf(ppm));
    }

    float ax, ay, ap;
    if (project(kLen, centerAt(kLen), ax, ay, ap)) spr(art_.arch, ax, ay, ap * 5.5f, PAL_SIGN, false, fogOf(ap));
    if (project(kLen - 8.f, centerAt(kLen - 8.f) - halfAt(kLen - 8.f), ax, ay, ap))
        spr(art_.sign, ax, ay, ap * 2.4f, PAL_SIGN, false, fogOf(ap));

    float jx = (mode_ == Mode::Run) ? std::sin(time_ * 31.f) * (0.4f + shake_ * 5.f) : 0.f;
    spr(art_.hood, 160.f + jx, 226.f, 92.f, PAL_VAN);

    char buf[72];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 MAILVAN PASS", PAL_ICE);
        hudC(6, "CLEAR THE PASS", PAL_HUD);
        hudC(8, "BEFORE THE STORM CLOCK", PAL_BAD);
        hudC(10, "KEEP THE VAN OFF THE ROCK", PAL_HUD);
        hudC(16, "A GAS    LEFT RIGHT", PAL_HUD);
        hudC(22, "PRESS START", PAL_ICE);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_ICE);
    } else if (mode_ == Mode::Win) {
        hudC(3, "PASS CLEAR", PAL_GOOD);
        hudC(5, "STORM STILL BEHIND YOU", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "CLOCK LEFT  %.1f S", storm_);
        hudC(8, buf, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(3, "PASS CLOSED", PAL_BAD);
        hudC(5, why_, PAL_BAD);
        std::snprintf(buf, sizeof buf, "%3.0f OF %3.0f", clampf(s_, 0.f, kLen), kLen);
        hudC(8, buf, PAL_HUD);
    } else {
        float left = std::max(0.f, kLen - s_);
        std::snprintf(buf, sizeof buf, "%4.0f M", left);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.0f", v_);
        hud(28, 1, buf, PAL_ICE);
        if (s_ > kLen * 0.82f) hudC(2, "THE ARCH", PAL_GOOD);
        else if (s_ > kLen * 0.48f) hudC(2, "THE PINCH", PAL_BAD);
        else hudC(2, "THE CLIMB", PAL_ICE);
        int pal = storm_ < 8.f ? PAL_BAD : PAL_HUD;
        std::snprintf(buf, sizeof buf, "STORM %4.0f S", std::max(0.f, storm_));
        hud(1, 26, buf, pal);
        hud(24, 26, "A GAS", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.reset();
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.05f, 0.1f, 0.05f);
    if (bot_) startRun();
    else showTitle();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    if (chime_ >= 0) {
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        chimeT_ += DT;
        if (chimeT_ > 0.14f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.18f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 8) chime_ = -1;
        }
    }

    if (mode_ == Mode::Title) {
        time_ += DT;
        s_ = 48.f + std::sin(time_ * 0.4f) * 2.f;
        y_ = centerAt(s_) + std::sin(time_ * 0.7f) * 0.25f;
        storm_ = kClock;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || (bot_ && time_ > 0.15f))
            startRun();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
        return;
    }

    if (mode_ == Mode::Pause) {
        draw();
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT);
        draw();
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) showTitle();
        return;
    }

    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        draw();
        return;
    }
    float gas = 0, steer = 0;
    pilot(gas, steer);
    physics(gas, steer);
    if (mode_ == Mode::Run && storm_ < 9.f) sys.apu.noise(0.035f, 280.f + (9.f - storm_) * 30.f);
    else if (mode_ != Mode::Fail) sys.apu.noise(0, 0);
    draw();
}

}  // namespace mailpass
