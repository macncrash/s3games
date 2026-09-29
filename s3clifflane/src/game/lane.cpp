#include "game/lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace clifflane {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFinish = 680.f;
constexpr float kHalf = 0.42f;
constexpr float kShelf = 3.55f;
constexpr float kRoad = 3.85f;
constexpr float kHorizon = 86.f;
constexpr float kPpm = 42.f;
constexpr float kNear = 1.15f;
constexpr float kLimit = 92.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

float laneCenter(float s) {
    return 0.92f * std::sin(s * 0.038f) + 0.38f * std::sin(s * 0.086f + 0.7f);
}

float laneHalf(float s) { return 1.28f + 0.16f * std::sin(s * 0.051f + 0.4f); }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (s_ >= 560.f) return 3;
    if (margin() < 0.38f) return 2;
    return 1;
}

float Game::margin() const { return laneHalf(s_) - (std::fabs(y_ - laneCenter(s_)) + kHalf); }

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    why_ = "";
    chime_ = -1;
    time_ = 0;
    s_ = 28.f;
    y_ = laneCenter(s_);
    v_ = 0;
    shake_ = 0;
}

void Game::startRun() {
    s_ = 0;
    y_ = laneCenter(0.f);
    v_ = 0;
    time_ = 0;
    gas_ = brake_ = steer_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    chime_ = -1;
    shake_ = 0;
    mode_ = Mode::Run;
    blip(392.f);
}

void Game::pilot(float& gas, float& brake, float& steer) const {
    float now = laneCenter(s_);
    float lead = laneCenter(s_ + 5.f);
    float target = now + (lead - now) * 0.85f;
    float err = target - y_;
    float yawScale = 1.55f + 0.038f * v_;
    float vy = clampf(err * 7.5f, -2.4f, 2.4f);
    steer = clampf(vy / yawScale, -1.f, 1.f);

    float curve = std::fabs(lead - now);
    float vWant = 10.4f;
    if (curve > 0.28f) vWant = 8.2f;
    else if (curve > 0.16f) vWant = 9.2f;
    if (std::fabs(err) > 0.22f) vWant = std::min(vWant, 7.2f);
    if (s_ > kFinish - 36.f) vWant = std::min(vWant, 8.6f);
    gas = v_ < vWant ? 1.f : 0.f;
    brake = v_ > vWant + 0.55f ? 1.f : 0.f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "stayed in the lane";
    chime_ = 0;
    chimeT_ = 0;
    v_ = 0;
    sys_->rumble(0.12f, 0.04f, 120);
    sys_->setLight(40, 150, 80);
    std::printf("S3 CLIFFLANE  WIN  stayed in the lane for the whole leg  (%.1f s)\n", time_);
    std::fflush(stdout);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    why_ = why;
    shake_ = 0.55f;
    sys_->rumble(0.55f, 0.3f, 180);
    sys_->setLight(160, 40, 20);
    sys_->apu.noiseBurst(0.42f, 180.f, 0.36f);
}

void Game::physics(float gas, float brake, float steer) {
    gas_ = clampf(gas, 0.f, 1.f);
    brake_ = clampf(brake, 0.f, 1.f);
    steer_ = clampf(steer, -1.f, 1.f);
    time_ += DT;
    float a = gas_ * 5.4f - brake_ * 11.f - 0.46f * v_;
    v_ = std::max(0.f, v_ + a * DT);
    if (v_ > 15.4f) v_ = 15.4f;
    float prev = s_;
    s_ += v_ * DT;
    float yaw = steer_ * (1.55f + 0.038f * v_);
    y_ = clampf(y_ + yaw * DT, -kShelf, kShelf);
    shake_ = std::max(0.f, shake_ - DT);
    if (brake_ > 0.6f && v_ > 3.f) shake_ = std::max(shake_, 0.08f);

    float off = std::fabs(y_ - laneCenter(s_)) + kHalf;
    if (off > laneHalf(s_) || std::fabs(y_) > kShelf - 0.04f) {
        fail("left the lane");
        return;
    }
    if (prev < kFinish && s_ >= kFinish) {
        win();
        return;
    }
    if (time_ > kLimit) fail("missed the end");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.06f);
    beep_ = 0.08f;
}

bool Game::project(float wz, float wy, float& sx, float& sy, float& ppm) const {
    float dz = wz - s_;
    if (dz < 0.5f || dz > 84.f) return false;
    float n = kNear / dz;
    sy = kHorizon + n * (gs::SCREEN_H - kHorizon);
    ppm = kPpm * n;
    sx = 160.f + (wy - y_) * ppm;
    return sy > -48.f && sy < gs::SCREEN_H + 48.f;
}

void Game::spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip, int fog) {
    if (ht < 1.4f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.f, 420.f)));
    s.h = int16_t(std::lround(clampf(ht, 1.f, 320.f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet - s.h));
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::skyShelf() {
    uint16_t zen = gs::rgb4(3, 5, 8);
    uint16_t mid = gs::rgb4(8, 8, 7);
    uint16_t sea = gs::rgb4(2, 6, 8);
    uint16_t deep = gs::rgb4(1, 3, 6);
    if (mode_ == Mode::Fail) {
        mid = lerpC(mid, gs::rgb4(10, 4, 3), 0.45f);
        sea = lerpC(sea, gs::rgb4(8, 2, 2), 0.4f);
    }
    if (mode_ == Mode::Win) mid = lerpC(mid, gs::rgb4(6, 11, 6), 0.4f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(kHorizon)) {
            float t = y / kHorizon;
            sys_->vdp.lineBackdrop[y] = t < 0.55f ? lerpC(zen, mid, t / 0.55f) : lerpC(mid, sea, (t - 0.55f) / 0.45f);
        } else {
            float t = (y - kHorizon) / (gs::SCREEN_H - kHorizon);
            sys_->vdp.lineBackdrop[y] = lerpC(sea, deep, t);
        }
        sys_->vdp.lineFog[y] = 0;
        gs::RoadLine& r = sys_->vdp.road[y];
        r.on = false;
        if (y <= int(kHorizon)) continue;
        float n = (y - kHorizon) / (gs::SCREEN_H - kHorizon);
        if (n < 0.02f) continue;
        float dz = kNear / n;
        float world = s_ + dz;
        r.on = true;
        r.cx = 160.f - y_ * kPpm * n;
        r.hw = kRoad * kPpm * n;
        r.v = world * 8.f;
        r.pal = PAL_ROAD;
        r.style = gs::ROAD_ROCKY;
        r.band = (int(world * 0.35f) & 1);
        r.left = gs::GROUND_DROP;
        r.right = gs::GROUND_LAND;
        sys_->vdp.lineFog[y] = uint8_t(clampf((1.f - n) * 9.f, 0.f, 8.f));
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = true;
    sys_->vdp.HUD.clear();
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
    skyShelf();
    sys_->vdp.clearSprites();
    auto fogOf = [](float ppm) { return int(clampf(11.f - ppm * 0.2f, 0.f, 12.f)); };

    for (int i = 8; i >= 0; i--) {
        float base = std::floor(s_ / 24.f) * 24.f + i * 24.f;
        float sx, sy, ppm;
        if (project(base, kRoad + 0.2f, sx, sy, ppm))
            spr(art_.crag, sx, sy, ppm * 6.2f, PAL_ROCK, (i & 1) != 0, fogOf(ppm));
        if (project(base + 12.f, -kRoad + 0.5f, sx, sy, ppm))
            spr(art_.scrub, sx, sy, ppm * 1.4f, PAL_ROCK, false, fogOf(ppm));
    }

    float step0 = std::floor(s_ / 10.f) * 10.f;
    for (int i = 9; i >= 0; i--) {
        float z = step0 + i * 10.f;
        float c = laneCenter(z);
        float h = laneHalf(z);
        float sx, sy, ppm;
        if (project(z, c - h, sx, sy, ppm)) spr(art_.stake, sx, sy, ppm * 1.7f, PAL_STAKE, false, fogOf(ppm));
        if (project(z, c + h, sx, sy, ppm)) spr(art_.stake, sx, sy, ppm * 1.7f, PAL_STAKE, true, fogOf(ppm));
        if (project(z + 5.f, laneCenter(z + 5.f), sx, sy, ppm))
            spr(art_.dash, sx, sy, ppm * 0.28f, PAL_PAINT, false, fogOf(ppm));
    }

    float px, py, pp;
    if (project(kFinish, laneCenter(kFinish) - 1.15f, px, py, pp))
        spr(art_.post, px, py, pp * 3.4f, PAL_GATE, false, fogOf(pp));
    if (project(kFinish, laneCenter(kFinish) + 1.15f, px, py, pp))
        spr(art_.post, px, py, pp * 3.4f, PAL_GATE, false, fogOf(pp));
    if (project(kFinish, laneCenter(kFinish), px, py, pp))
        spr(art_.ribbon, px, py - pp * 3.0f, pp * 0.55f, PAL_GATE, false, fogOf(pp));

    float jx = (mode_ == Mode::Run) ? std::sin(time_ * 26.f) * shake_ * 7.f : 0.f;
    spr(art_.wheel, 118.f + jx, 214.f, 28.f, PAL_CART);
    spr(art_.wheel, 202.f + jx, 214.f, 28.f, PAL_CART);
    spr(art_.cart, 160.f + jx, 208.f, 78.f, PAL_CART);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 CLIFF LANE", PAL_AMBER);
        hudC(6, "STAY IN THE LANE", PAL_HUD);
        hudC(8, "FOR THE WHOLE LEG", PAL_HUD);
        hudC(10, "ONE DEPARTURE FAILS IT", PAL_BAD);
        hudC(16, "A GAS   B BRAKE", PAL_HUD);
        hudC(17, "LEFT RIGHT  HOLD THE LANE", PAL_HUD);
        hudC(22, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        hudC(3, "LANE HELD", PAL_GOOD);
        hudC(5, "WHOLE LEG INSIDE", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "LEG %.1f S", time_);
        hudC(8, buf, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(3, "LEG FAILED", PAL_BAD);
        hudC(5, why_, PAL_BAD);
        std::snprintf(buf, sizeof buf, "%4.0f M OF 680", clampf(s_, 0.f, kFinish));
        hudC(8, buf, PAL_HUD);
    } else {
        float left = std::max(0.f, kFinish - s_);
        std::snprintf(buf, sizeof buf, "%4.0f M", left);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(28, 1, buf, PAL_AMBER);
        float m = margin();
        if (s_ > kFinish - 70.f) hudC(2, "HOLD TO THE GATE", m > 0.25f ? PAL_GOOD : PAL_BAD);
        else if (m < 0.28f) hudC(2, "ON THE EDGE", PAL_BAD);
        else hudC(2, "IN THE LANE", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "LEG %4.0f S", kLimit - time_);
        hud(1, 26, buf, time_ > kLimit - 12.f ? PAL_BAD : PAL_HUD);
        hud(22, 26, "A GAS  B BRAKE", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.1f, 0.18f, 0.07f);
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
        static const float notes[] = {330.f, 392.f, 494.f, 659.f};
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
        s_ = 36.f + std::sin(time_ * 0.25f) * 2.f;
        y_ = laneCenter(s_) + std::sin(time_ * 0.7f) * 0.18f;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
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

    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        draw();
        sys.apu.noise(0.f, 400.f, false);
        sys.apu.tone(2, 0.f, 0.f);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    float gas = 0, brake = 0, steer = 0;
    if (bot_) {
        pilot(gas, brake, steer);
    } else {
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_UP) || pad.accel > 0.15f) gas = 1.f;
        if (pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.brake > 0.15f) brake = 1.f;
        if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
        if (std::fabs(pad.axisX) > 0.2f) steer = pad.axisX;
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(220.f);
            draw();
            return;
        }
    }

    physics(gas, brake, steer);
    if (mode_ == Mode::Run) {
        float rpm = 46.f + v_ * 8.f + gas_ * 16.f;
        sys.apu.tone(2, rpm, 0.022f + gas_ * 0.018f);
        sys.apu.noise(brake_ > 0.5f && v_ > 1.f ? 0.07f : 0.01f, 260.f + v_ * 10.f, false);
    } else {
        sys.apu.tone(2, 0.f, 0.f);
        sys.apu.noise(0.f, 400.f, false);
    }
    draw();
}

}  // namespace clifflane
