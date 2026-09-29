#include "lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace headerlock {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHor = 86.f;
constexpr float kCamH = 1.25f;
constexpr float kFocal = 260.f;
constexpr float kRoadHW = 4.4f;
constexpr float kHalfCar = 0.72f;
constexpr float kEnd = 360.f;
constexpr float kHeader = 150.f;
constexpr float kLimit = 28.f;
constexpr float kPost = 0.42f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::bend(float z) const { return 16.f * std::sin(z * 0.013f) + 6.f * std::sin(z * 0.041f); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (z_ < gates_[0].z) return 1;
    if (z_ < kHeader) return 2;
    return 3;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.reset();
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.clear();
    sys.apu.setMaster(0.42f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
}

void Game::begin() {
    z_ = 6.f;
    x_ = 0.35f;
    speed_ = 9.f;
    time_ = 0;
    shake_ = 0;
    why_ = "";
    over_ = false;
    won_ = false;
    lockOpen_ = false;
    chime_ = -1;
    gates_[0] = {72.f, 1.62f, false, false};
    gates_[1] = {96.f, 1.48f, false, false};
    gates_[2] = {120.f, 1.58f, false, false};
    mode_ = Mode::Run;
    blip(330.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.1f);
    beep_ = 0.07f;
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    over_ = true;
    won_ = false;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    speed_ *= 0.25f;
    sys_->apu.noiseBurst(0.45f, 700.f, 0.28f);
    sys_->apu.tone(1, 80.f, 0.22f);
    beep_ = 0.28f;
}

void Game::finish() {
    if (mode_ != Mode::Run) return;
    over_ = true;
    won_ = true;
    mode_ = Mode::Win;
    why_ = "clear";
    chime_ = 0;
    chimeT_ = 0;
}

void Game::pilot(float& thrust, float& steer, float& brake) {
    thrust = steer = brake = 0;
    if (bot_) {
        steer = clampf(-x_ * 2.1f, -1.f, 1.f);
        thrust = 1.f;
        return;
    }
    const gs::Pad& pad = sys_->pad;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    steer += pad.axisX;
    steer = clampf(steer, -1.f, 1.f);
    if (pad.down(gs::BTN_A) || pad.accel > 0.2f) thrust = 1.f;
    if (pad.down(gs::BTN_B) || pad.brake > 0.2f) brake = 1.f;
}

void Game::step(float thrust, float steer, float brake) {
    float accel = thrust * 14.f - 3.2f;
    if (brake > 0.f) accel -= 20.f;
    if (std::fabs(x_) > kRoadHW) accel -= 12.f;
    speed_ = clampf(speed_ + accel * kDt, 0.f, 24.f);
    float rate = steer * (3.6f + speed_ * 0.2f);
    x_ += rate * kDt;
    x_ = clampf(x_, -8.f, 8.f);
    z_ += speed_ * kDt;
    time_ += kDt;

    for (Gate& g : gates_) {
        if (g.crossed || z_ < g.z) continue;
        g.crossed = true;
        const float edge = std::fabs(x_) + kHalfCar;
        if (edge <= g.gap) {
            g.clean = true;
            blip(520.f);
        } else if (std::fabs(x_) < g.gap + kPost + kHalfCar) {
            fail("scraped a gate");
            return;
        } else {
            fail("missed the lock");
            return;
        }
    }
    if (!lockOpen_ && gates_[2].crossed && gates_[0].clean && gates_[1].clean && gates_[2].clean) {
        lockOpen_ = true;
        blip(660.f);
    }
    if (z_ >= kEnd) {
        bool all = true;
        for (const Gate& g : gates_) all = all && g.clean;
        if (!all) {
            fail("missed the lock");
            return;
        }
        finish();
        return;
    }
    if (time_ >= kLimit) fail("missed the end");
}

void Game::project(float wz, float wx, float wy, float& sx, float& sy, float& scale, int& fog) const {
    float dz = wz - z_;
    if (dz < 0.35f) dz = 0.35f;
    scale = kFocal / dz;
    float camX = bend(z_) + x_;
    float jx = (shake_ > 0.f) ? std::sin(time_ * 80.f) * shake_ * 4.f : 0.f;
    sx = 160.f + (wx - camX) * scale + jx;
    sy = kHor + (kCamH - wy) * scale;
    fog = int(clampf((28.f - (sy - kHor)) * 0.35f, 0.f, 12.f));
}

void Game::mark(const gs::Mipped& m, float wz, float wx, float wy, float worldH, int pal) {
    if (m.h < 1 || wz < z_ + 0.4f) return;
    float sx, sy, scale;
    int fog = 0;
    project(wz, wx, wy, sx, sy, scale, fog);
    float h = worldH * scale;
    if (h < 2.f || h > 400.f) return;
    float w = h * float(m.w) / float(m.h);
    if (sx + w < -20.f || sx - w > gs::SCREEN_W + 20.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(w), 1L, 400L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 400L));
    s.x = int16_t(std::lround(sx - w * 0.5f));
    s.y = int16_t(std::lround(sy - h));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(fog);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.roadTime = int(time_ * 60.f);

    if (mode_ == Mode::Title) {
        gs::Sprite s;
        s.img = art_.title.pick(36.f);
        s.w = int16_t(art_.title.w * 3);
        s.h = int16_t(art_.title.h * 3);
        s.x = int16_t(160.f - s.w * 0.5f);
        s.y = 36;
        s.pal = PAL_HUD;
        sys_->vdp.sprite(s);
    }

    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineFog[y] = 0;
        if (y < int(kHor)) {
            float u = y / kHor;
            int r = 4 + int(u * 6.f);
            int g = 7 + int(u * 5.f);
            int b = 12 + int((1.f - u) * 3.f);
            vdp.lineBackdrop[y] = gs::rgb4(std::min(15, r), std::min(15, g), std::min(15, b));
            vdp.road[y].on = false;
            continue;
        }
        vdp.lineBackdrop[y] = gs::rgb4(2, 5, 2);
        float dy = float(y) - kHor;
        float dz = kCamH * kFocal / dy;
        float wz = z_ + dz;
        float camX = bend(z_) + x_;
        gs::RoadLine& r = vdp.road[y];
        r.on = true;
        r.cx = 160.f + (bend(wz) - camX) * kFocal / dz;
        r.hw = kRoadHW * kFocal / dz;
        r.v = wz * 10.f;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(wz / 8.f)) & 1) ? 1 : 0;
        r.style = 1;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
        vdp.lineFog[y] = uint8_t(std::max(0, std::min(12, int((22.f - dy) * 0.4f))));
    }

    // Earlier sprites draw above later ones, so the car goes in first.
    {
        gs::Sprite s;
        s.w = 36;
        s.h = 50;
        s.x = 160 - 18;
        s.y = 168;
        s.img = art_.car.pick(50.f);
        s.pal = PAL_CAR;
        sys_->vdp.sprite(s);
    }
    for (int i = 0; i < 3; ++i) {
        const Gate& g = gates_[i];
        float left = bend(g.z) - g.gap - kPost * 0.5f;
        float right = bend(g.z) + g.gap + kPost * 0.5f;
        mark(art_.post, g.z, left, 0.f, 3.1f, PAL_GATE);
        mark(art_.post, g.z, right, 0.f, 3.1f, PAL_GATE);
    }
    mark(art_.sign, gates_[0].z - 14.f, bend(gates_[0].z - 14.f) - 2.2f, 1.6f, 1.3f, PAL_SIGN);
    for (float tz = z_ + 80.f; tz > z_; tz -= 18.f) {
        if (tz < 8.f) continue;
        mark(art_.tree, tz, bend(tz) - kRoadHW - 1.6f, 0.f, 3.4f, PAL_TREE);
        mark(art_.tree, tz + 9.f, bend(tz + 9.f) + kRoadHW + 1.8f, 0.f, 3.1f, PAL_TREE);
    }
    mark(art_.banner, kEnd, bend(kEnd), 2.4f, 2.2f, PAL_BANNER);

    if (mode_ == Mode::Title) {
        hudC(12, "PASS THE LOCK", PAL_HUD);
        hudC(14, "DO NOT SCRAPE A GATE", PAL_HUD);
        hudC(16, "MISSING THE END FAILS THE LEG", PAL_HUD);
        hudC(20, "A START", PAL_HUD);
    } else {
        char buf[48];
        const char* sector = z_ < kHeader ? "HEADER" : "LEG";
        std::snprintf(buf, sizeof buf, "%s  %4.1f", sector, speed_);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "T %4.1f", time_);
        hud(30, 1, buf, PAL_HUD);
        if (!lockOpen_) hud(1, 2, "LOCK AHEAD", PAL_HUD);
        else hud(1, 2, "LOCK CLEAR", PAL_HUD);
        if (mode_ == Mode::Pause) hudC(12, "PAUSE", PAL_HUD);
        if (mode_ == Mode::Fail) {
            hudC(11, "LEG FAILED", PAL_HUD);
            hudC(13, why_, PAL_HUD);
        }
        if (mode_ == Mode::Win) {
            hudC(11, "LEG CLEAR", PAL_HUD);
            hudC(13, "LOCK PASSED", PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float thrust = 0, steer = 0, brake = 0;
            pilot(thrust, steer, brake);
            step(thrust, steer, brake);
        }
    } else if ((mode_ == Mode::Fail || mode_ == Mode::Win) && !bot_) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Title;
            over_ = false;
        }
    }

    if (mode_ == Mode::Run) {
        float pitch = 70.f + speed_ * 6.5f;
        sys.apu.tone(2, pitch, 0.04f + speed_ * 0.002f);
    } else if (mode_ != Mode::Win) {
        sys.apu.tone(2, 0, 0);
    }

    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.5f);
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        int step = int(chimeT_ / 0.16f);
        if (step != chime_ && step < 4) {
            chime_ = step;
            sys.apu.tone(1, notes[step], 0.15f);
        }
        if (step >= 6) {
            sys.apu.tone(1, 0, 0);
            chime_ = -1;
        }
    }
    draw();
}

}  // namespace headerlock
