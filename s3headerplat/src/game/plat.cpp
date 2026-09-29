#include "plat.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace headerplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHor = 88.f;
constexpr float kCamH = 1.22f;
constexpr float kFocal = 250.f;
constexpr float kRoadHW = 4.2f;
constexpr float kPlat = 188.f;
constexpr float kAlign = 0.95f;
constexpr float kZWin = 0.9f;
constexpr float kPast = 2.2f;
constexpr float kStop = 0.42f;
constexpr float kHold = 0.36f;
constexpr float kLimit = 26.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::bend(float z) const { return 9.f * std::sin(z * 0.018f); }

bool Game::level() const {
    return std::fabs(z_ - kPlat) <= kZWin && std::fabs(x_) <= kAlign && speed_ <= kStop;
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
    z_ = 18.f;
    x_ = 0.4f;
    speed_ = 0.f;
}

void Game::begin() {
    z_ = 8.f;
    x_ = 0.55f;
    speed_ = 8.f;
    time_ = 0;
    hold_ = 0;
    bad_ = 0;
    shake_ = 0;
    why_ = "";
    over_ = false;
    won_ = false;
    chime_ = -1;
    mode_ = Mode::Run;
    blip(360.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.1f);
    beep_ = 0.08f;
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    over_ = true;
    won_ = false;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    speed_ *= 0.2f;
    sys_->apu.noiseBurst(0.42f, 640.f, 0.26f);
    sys_->apu.tone(1, 74.f, 0.2f);
    beep_ = 0.26f;
}

void Game::finish() {
    if (mode_ != Mode::Run) return;
    over_ = true;
    won_ = true;
    mode_ = Mode::Win;
    why_ = "level";
    chime_ = 0;
    chimeT_ = 0;
    speed_ = 0;
}

void Game::pilot(float& thrust, float& steer, float& brake) {
    thrust = steer = brake = 0;
    if (bot_) {
        steer = clampf(-x_ * 2.8f, -1.f, 1.f);
        float err = kPlat - z_;
        if (err > 10.f) {
            float want = 16.f;
            if (speed_ > want + 0.3f) brake = 1.f;
            else if (speed_ < want - 0.5f) thrust = 1.f;
        } else if (err > 0.35f) {
            float want = std::sqrt(std::max(0.2f, err) * 6.5f);
            if (speed_ > want + 0.25f) brake = 1.f;
            else if (speed_ < want - 0.35f) thrust = 0.7f;
        } else if (err > -0.05f) {
            if (speed_ > 0.2f) brake = 1.f;
            else if (err > 0.08f) thrust = 0.35f;
        } else {
            brake = speed_ > 0.05f ? 1.f : 0.f;
        }
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
    float drag = 2.1f + speed_ * 0.12f;
    float accel = thrust * 15.f - drag;
    if (brake > 0.f) accel -= brake * 20.f;
    if (std::fabs(x_) > kRoadHW) accel -= 8.f;
    if (speed_ < 0.04f && accel < 0.f) speed_ = 0.f;
    else speed_ = clampf(speed_ + accel * kDt, 0.f, 22.f);

    x_ += steer * (2.4f + speed_ * 0.12f) * kDt;
    x_ = clampf(x_, -7.5f, 7.5f);
    z_ += speed_ * kDt;
    time_ += kDt;

    if (z_ > kPlat + kPast) {
        fail("missed the end");
        return;
    }
    if (level()) {
        hold_ += kDt;
        bad_ = 0;
        if (hold_ >= kHold) finish();
    } else {
        hold_ = 0;
        if (speed_ <= kStop) {
            bad_ += kDt;
            if (bad_ >= 0.7f) {
                if (z_ > kPlat) fail("missed the end");
                else fail("not level with the platform");
            }
        } else {
            bad_ = 0;
        }
    }
    if (mode_ == Mode::Run && time_ >= kLimit) fail("missed the end");
}

void Game::project(float wz, float wx, float wy, float& sx, float& sy, float& scale, int& fog) const {
    float dz = wz - z_;
    if (dz < 0.4f) dz = 0.4f;
    scale = kFocal / dz;
    float camX = bend(z_) + x_;
    float jx = (shake_ > 0.f) ? std::sin(time_ * 70.f) * shake_ * 3.f : 0.f;
    sx = 160.f + (wx - camX) * scale + jx;
    sy = kHor + (kCamH - wy) * scale;
    fog = int(clampf((26.f - (sy - kHor)) * 0.32f, 0.f, 12.f));
}

void Game::mark(const gs::Mipped& m, float wz, float wx, float wy, float worldH, int pal) {
    if (m.h < 1 || wz < z_ + 0.45f) return;
    float sx, sy, scale;
    int fog = 0;
    project(wz, wx, wy, sx, sy, scale, fog);
    float h = worldH * scale;
    if (h < 2.f || h > 420.f) return;
    float w = h * float(m.w) / float(m.h);
    if (sx + w < -24.f || sx - w > gs::SCREEN_W + 24.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(w), 1L, 420L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 420L));
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
        s.img = art_.title.pick(32.f);
        s.w = int16_t(art_.title.w * 2);
        s.h = int16_t(art_.title.h * 2);
        s.x = int16_t(160.f - s.w * 0.5f);
        s.y = 28;
        s.pal = PAL_HUD;
        vdp.sprite(s);
    }

    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineFog[y] = 0;
        if (y < int(kHor)) {
            float u = y / kHor;
            int r = 5 + int(u * 5.f);
            int g = 8 + int(u * 4.f);
            int b = 11 + int((1.f - u) * 2.f);
            vdp.lineBackdrop[y] = gs::rgb4(std::min(15, r), std::min(15, g), std::min(15, b));
            vdp.road[y].on = false;
            continue;
        }
        vdp.lineBackdrop[y] = gs::rgb4(3, 5, 2);
        float dy = float(y) - kHor;
        float dz = kCamH * kFocal / dy;
        float wz = z_ + dz;
        float camX = bend(z_) + x_;
        gs::RoadLine& r = vdp.road[y];
        r.on = true;
        r.cx = 160.f + (bend(wz) - camX) * kFocal / dz;
        r.hw = kRoadHW * kFocal / dz;
        r.v = wz * 9.f;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(wz / 7.f)) & 1) ? 1 : 0;
        bool deck = wz > kPlat - 6.f && wz < kPlat + 4.f;
        r.style = deck ? 0 : 1;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
        vdp.lineFog[y] = uint8_t(std::max(0, std::min(12, int((20.f - dy) * 0.38f))));
    }

    {
        gs::Sprite s;
        s.w = 34;
        s.h = 48;
        s.x = 160 - 17;
        s.y = 166;
        s.img = art_.car.pick(48.f);
        s.pal = PAL_CAR;
        vdp.sprite(s);
    }

    float cx = bend(kPlat);
    mark(art_.dock, kPlat, cx, 0.15f, 1.35f, PAL_DOCK);
    mark(art_.post, kPlat - 1.2f, cx - 2.4f, 0.f, 2.8f, PAL_POST);
    mark(art_.post, kPlat - 1.2f, cx + 2.4f, 0.f, 2.8f, PAL_POST);
    mark(art_.sign, kPlat - 28.f, bend(kPlat - 28.f) - 2.6f, 1.5f, 1.15f, PAL_SIGN);
    for (float tz = z_ + 70.f; tz > z_; tz -= 16.f) {
        if (tz < 6.f || std::fabs(tz - kPlat) < 8.f) continue;
        mark(art_.tree, tz, bend(tz) - kRoadHW - 1.5f, 0.f, 3.0f, PAL_TREE);
        mark(art_.tree, tz + 7.f, bend(tz + 7.f) + kRoadHW + 1.7f, 0.f, 2.6f, PAL_TREE);
    }

    if (mode_ == Mode::Title) {
        hudC(12, "STOP LEVEL WITH THE PLATFORM", PAL_HUD);
        hudC(14, "MISSING THE END FAILS THE LEG", PAL_HUD);
        hudC(18, "A GAS   B BRAKE   START", PAL_HUD);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "HEADER  %4.1f", speed_);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "T %4.1f", time_);
        hud(30, 1, buf, PAL_HUD);
        float gap = kPlat - z_;
        if (level()) hud(1, 2, "LEVEL", PAL_HUD);
        else if (gap > 1.5f) {
            std::snprintf(buf, sizeof buf, "PLATFORM %4.0f", gap);
            hud(1, 2, buf, PAL_HUD);
        } else if (std::fabs(x_) > kAlign) {
            hud(1, 2, "OFF THE DECK", PAL_HUD);
        } else if (speed_ > kStop) {
            hud(1, 2, "STILL ROLLING", PAL_HUD);
        } else {
            hud(1, 2, "SHORT", PAL_HUD);
        }
        if (mode_ == Mode::Pause) hudC(12, "PAUSE", PAL_HUD);
        if (mode_ == Mode::Fail) {
            hudC(11, "LEG FAILED", PAL_HUD);
            hudC(13, why_, PAL_HUD);
        }
        if (mode_ == Mode::Win) {
            hudC(11, "LEG CLEAR", PAL_HUD);
            hudC(13, "LEVEL WITH THE PLATFORM", PAL_HUD);
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
        float pitch = 64.f + speed_ * 6.f;
        sys.apu.tone(2, pitch, 0.035f + speed_ * 0.002f);
    } else if (mode_ != Mode::Win) {
        sys.apu.tone(2, 0, 0);
    }

    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.6f);
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {349.f, 440.f, 523.f, 698.f};
        int stepN = int(chimeT_ / 0.15f);
        if (stepN != chime_ && stepN < 4) {
            chime_ = stepN;
            sys.apu.tone(1, notes[stepN], 0.14f);
        }
        if (stepN >= 6) {
            sys.apu.tone(1, 0, 0);
            chime_ = -1;
        }
    }

    draw();
}

}  // namespace headerplat
