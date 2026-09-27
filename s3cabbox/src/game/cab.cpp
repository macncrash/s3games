#include "game/cab.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cabbox {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kBox0 = 56.f;
constexpr float kBox1 = 74.f;
constexpr float kRear = 1.2f;
constexpr float kNose = 3.4f;
constexpr float kBoxY = 0.55f;
constexpr float kLane = 0.42f;
constexpr float kStop = 0.28f;
constexpr float kHoldNeed = 0.40f;
constexpr float kHorizon = 78.f;
constexpr float kPpm = 48.f;
constexpr float kNear = 0.82f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05f) return 3;
    if ((s_ + kNose) > kBox0 && (s_ - kRear) < kBox1) return 2;
    return 1;
}

bool Game::hullInside() const {
    return (s_ - kRear) >= kBox0 - 0.05f && (s_ + kNose) <= kBox1 + 0.05f && std::fabs(y_ - kBoxY) <= kLane;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    why_ = "";
    hold_ = 0;
    chime_ = -1;
    time_ = 0;
    s_ = 18.f;
    y_ = 0.f;
    v_ = 0;
}

void Game::startRun() {
    s_ = 0;
    y_ = 0;
    v_ = 0;
    hold_ = 0;
    time_ = 0;
    gas_ = brake_ = steer_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    chime_ = -1;
    mode_ = Mode::Run;
    blip(520.f);
}

void Game::pilot(float& gas, float& brake, float& steer) const {
    float lo = kBox0 + kRear;
    float hi = kBox1 - kNose;
    float target = (lo + hi) * 0.5f;
    float dist = target - s_;
    float vWant = clampf(std::sqrt(std::max(0.f, dist) * 6.2f), 0.f, 12.f);
    gas = 0;
    brake = 0;
    if (dist < 0.12f) brake = 1.f;
    else if (v_ > vWant + 0.12f) brake = 1.f;
    else if (v_ < vWant - 0.35f) gas = 1.f;
    else gas = 0.35f;
    steer = clampf((kBoxY - y_) * 3.2f, -1.f, 1.f);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "stopped inside the box";
    v_ = 0;
    chime_ = 0;
    chimeT_ = 0;
    sys_->rumble(0.18f, 0.06f, 100);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    why_ = why;
    sys_->rumble(0.5f, 0.25f, 150);
    sys_->setLight(180, 30, 20);
    sys_->apu.noiseBurst(0.4f, 480.f, 0.25f);
}

void Game::physics(float gas, float brake, float steer) {
    gas_ = clampf(gas, 0.f, 1.f);
    brake_ = clampf(brake, 0.f, 1.f);
    steer_ = clampf(steer, -1.f, 1.f);
    time_ += DT;
    float a = gas_ * 6.6f - brake_ * 10.4f - 0.42f * v_;
    v_ = std::max(0.f, v_ + a * DT);
    if (v_ > 15.f) v_ = 15.f;
    s_ += v_ * DT;
    float yaw = steer_ * (0.62f + 0.07f * v_);
    y_ = clampf(y_ + yaw * DT, -2.4f, 2.4f);
    shake_ = std::max(0.f, shake_ - DT);
    if (brake_ > 0.6f && v_ > 2.f) shake_ = 0.08f;

    bool inside = hullInside();
    if (s_ + kNose > kBox1 + 0.2f) {
        fail("missed the end");
        return;
    }
    if (v_ <= kStop && inside) hold_ += DT;
    else hold_ = 0;
    if (hold_ >= kHoldNeed) {
        win();
        return;
    }
    if (v_ <= 0.06f && !inside && time_ > 0.4f) {
        if (s_ + kNose < kBox0) fail("short of the box");
        else if (std::fabs(y_ - kBoxY) > kLane) fail("out of the lane");
        else fail("stopped outside the box");
        return;
    }
    if (time_ > 32.f) fail("too late");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.07f);
    beep_ = 0.08f;
}

bool Game::project(float wz, float wy, float& sx, float& sy, float& ppm) const {
    float dz = wz - s_;
    if (dz < 0.45f || dz > 48.f) return false;
    float n = kNear / dz;
    sy = kHorizon + n * (gs::SCREEN_H - kHorizon);
    ppm = kPpm * n;
    sx = 160.f + (wy - y_) * ppm;
    return sy > -40.f && sy < gs::SCREEN_H + 20.f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip, int fog) {
    if (ht < 1.5f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.f, 400.f)));
    s.h = int16_t(std::lround(clampf(ht, 1.f, 300.f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::skyRoad() {
    uint16_t zen = gs::rgb4(3, 5, 10);
    uint16_t mid = gs::rgb4(8, 10, 13);
    uint16_t hor = gs::rgb4(13, 11, 8);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 5, 4), 0.4f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(10, 14, 8), 0.35f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.36f ? lerpC(zen, mid, t / 0.36f) : lerpC(mid, hor, (t - 0.36f) / 0.64f);
        sys_->vdp.lineFog[y] = 0;
        gs::RoadLine& r = sys_->vdp.road[y];
        r.on = false;
        if (y <= int(kHorizon)) continue;
        float n = (y - kHorizon) / (gs::SCREEN_H - kHorizon);
        if (n < 0.012f) continue;
        float dz = kNear / n;
        float world = s_ + dz;
        bool bay = world >= kBox0 && world <= kBox1;
        r.on = true;
        r.cx = 160.f - y_ * kPpm * n;
        r.hw = 3.6f * kPpm * n;
        r.v = world * 26.f;
        r.pal = bay ? PAL_BAY : PAL_ROAD;
        r.style = 1;
        r.band = (int(world) & 1) ? 1 : 0;
        r.left = 0;
        r.right = 0;
        int fog = int(clampf((1.f - n) * 10.f, 0.f, 8.f));
        sys_->vdp.lineFog[y] = uint8_t(fog);
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
    skyRoad();
    sys_->vdp.clearSprites();

    for (int i = 6; i >= 0; i--) {
        float wz = std::floor(s_ / 14.f) * 14.f + i * 14.f;
        float sx, sy, ppm;
        if (project(wz, -4.6f, sx, sy, ppm)) {
            int fog = int(clampf(10.f - ppm * 0.15f, 0.f, 12.f));
            spr(art_.block, sx, sy - ppm * 2.2f, ppm * 4.4f, PAL_CITY, false, fog);
        }
        if (project(wz + 6.f, 4.7f, sx, sy, ppm)) {
            int fog = int(clampf(10.f - ppm * 0.15f, 0.f, 12.f));
            spr(art_.block, sx, sy - ppm * 2.0f, ppm * 3.8f, PAL_CITY, true, fog);
        }
        if (project(wz + 3.f, -3.7f, sx, sy, ppm)) spr(art_.lamp, sx, sy - ppm * 1.6f, ppm * 2.4f, PAL_POST, false, 2);
    }

    const float postsX[2] = {kBox0, kBox1};
    const float postsY[2] = {kBoxY - 1.15f, kBoxY + 1.15f};
    for (float px : postsX)
        for (float py : postsY) {
            float sx, sy, ppm;
            if (!project(px, py, sx, sy, ppm)) continue;
            spr(art_.post, sx, sy - ppm * 1.3f, ppm * 2.6f, PAL_POST);
        }

    float fareS, fareY, fareP;
    if (project((kBox0 + kBox1) * 0.5f, kBoxY, fareS, fareY, fareP))
        spr(art_.fare, fareS, fareY - fareP * 2.4f, fareP * 0.7f, 8);

    float jx = (mode_ == Mode::Run) ? std::sin(time_ * 31.f) * shake_ * 6.f : 0.f;
    spr(art_.dash, 160.f + jx, 196.f, 78.f, PAL_CAB);
    int wi = int(s_ * 1.7f) & 3;
    if (mode_ != Mode::Run) wi = 0;
    spr(art_.wheel[wi], 160.f + steer_ * 10.f + jx, 176.f, 52.f, PAL_WHEEL);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 CAB BOX", PAL_AMBER);
        hudC(6, "STOP INSIDE THE BOX", PAL_HUD);
        hudC(8, "MISS THE END, THE LEG FAILS", PAL_BAD);
        hudC(16, "A GAS   B BRAKE", PAL_HUD);
        hudC(17, "LEFT RIGHT  AIM THE BAY", PAL_HUD);
        hudC(22, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        hudC(3, "STOPPED", PAL_GOOD);
        hudC(5, "INSIDE THE BOX", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "LEG %.1f S", time_);
        hudC(8, buf, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(3, "LEG FAILED", PAL_BAD);
        hudC(5, why_, PAL_BAD);
    } else {
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(1, 1, buf, PAL_HUD);
        float nose = s_ + kNose;
        if (nose < kBox0) {
            std::snprintf(buf, sizeof buf, "BOX %3.0f M", kBox0 - nose);
            hudC(2, buf, PAL_AMBER);
        } else if (hullInside()) {
            hudC(2, "IN THE BOX  STOP", PAL_GOOD);
        } else if (std::fabs(y_ - kBoxY) > kLane) {
            hudC(2, y_ < kBoxY ? "BAY IS RIGHT" : "BAY IS LEFT", PAL_AMBER);
        } else {
            hudC(2, "HOLD IN THE BAY", PAL_AMBER);
        }
        if (hold_ > 0.02f) {
            int n = std::clamp(int(hold_ / kHoldNeed * 5.f + 0.001f), 0, 5);
            std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
            hudC(3, buf, PAL_GOOD);
        }
        hudC(26, "A GAS  B BRAKE  ARROWS STEER", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.08f, 0.16f, 0.08f);
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
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        chimeT_ += DT;
        if (chimeT_ > 0.14f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.22f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 8) chime_ = -1;
        }
    }

    if (mode_ == Mode::Title) {
        time_ += DT;
        s_ = 18.f + std::sin(time_ * 0.4f) * 0.4f;
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
            blip(300.f);
            draw();
            return;
        }
    }

    physics(gas, brake, steer);
    if (mode_ == Mode::Run) {
        float rpm = 70.f + v_ * 18.f + gas_ * 40.f;
        sys.apu.tone(2, rpm, 0.03f + gas_ * 0.03f);
        sys.apu.noise(brake_ > 0.5f && v_ > 1.f ? 0.08f : 0.015f, 700.f + v_ * 30.f, false);
        if (hullInside()) sys.setLight(40, 160, 70);
        else sys.setLight(40, 70, 150);
    }
    draw();
}

}  // namespace cabbox
