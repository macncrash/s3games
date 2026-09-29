#include "game/kilo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace mailkilo {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFinish = 1000.f;
constexpr float kRear = 1.2f;
constexpr float kNose = 2.4f;
constexpr float kHalf = 0.42f;
constexpr float kClear = 0.26f;
constexpr float kLane = 1.62f;
constexpr float kRoad = 2.45f;
constexpr float kEndGate = 0.70f;
constexpr float kHorizon = 86.f;
constexpr float kPpm = 46.f;
constexpr float kNear = 0.95f;
constexpr float kLimit = 80.f;
constexpr int kWheelN = 7;

struct WheelDef {
    float s, y, r;
};

// Loose cart wheels. Each one leaves one side of the lane open.
constexpr WheelDef kWheels[kWheelN] = {
    {110.f, 1.12f, 0.58f}, {245.f, -1.18f, 0.66f}, {375.f, 0.92f, 0.52f}, {505.f, -0.28f, 0.72f},
    {635.f, 1.28f, 0.60f}, {755.f, -1.02f, 0.64f}, {875.f, 0.38f, 0.48f},
};

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
    if (s_ >= 820.f) return 3;
    if (s_ >= 180.f) return 2;
    return 1;
}

int Game::nextWheel() const {
    int best = -1;
    float bestS = 1e9f;
    for (int i = 0; i < kWheelN; i++) {
        if (kWheels[i].s + kWheels[i].r < s_ - kRear) continue;
        if (kWheels[i].s < bestS) {
            bestS = kWheels[i].s;
            best = i;
        }
    }
    return best;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    why_ = "";
    chime_ = -1;
    time_ = 0;
    s_ = 22.f;
    y_ = 0.1f;
    v_ = 0;
    shake_ = 0;
}

void Game::startRun() {
    s_ = 0;
    y_ = 0.f;
    v_ = 0;
    time_ = 0;
    gas_ = brake_ = steer_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    chime_ = -1;
    shake_ = 0;
    mode_ = Mode::Run;
    blip(480.f);
}

void Game::pilot(float& gas, float& brake, float& steer) const {
    float target = 0.f;
    int w = nextWheel();
    bool threading = false;
    if (w >= 0) {
        const WheelDef& wheel = kWheels[w];
        float ahead = wheel.s - s_;
        if (ahead < 48.f && ahead > -kNose) {
            threading = true;
            float need = wheel.r + kHalf + kClear;
            float left = wheel.y - need;
            float right = wheel.y + need;
            bool leftOk = left >= -kLane;
            bool rightOk = right <= kLane;
            if (leftOk && rightOk) target = (std::fabs(y_ - left) < std::fabs(y_ - right)) ? left : right;
            else if (leftOk) target = left;
            else target = right;
            target = clampf(target, -kLane, kLane);
        }
    }
    if (s_ > 920.f && !threading) target = 0.f;

    float err = target - y_;
    steer = clampf(err * 3.6f, -1.f, 1.f);

    float vWant = 17.2f;
    if (threading) {
        float ahead = kWheels[w].s - s_;
        if (std::fabs(err) > 0.5f && ahead < 18.f) vWant = 10.5f;
        else if (std::fabs(err) > 0.22f) vWant = 14.f;
    }
    if (s_ > 950.f) vWant = std::min(vWant, 13.f);
    gas = v_ < vWant ? 1.f : 0.f;
    brake = v_ > vWant + 1.1f ? 1.f : 0.f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "wheels untouched";
    chime_ = 0;
    chimeT_ = 0;
    v_ = 0;
    sys_->rumble(0.16f, 0.06f, 150);
    sys_->setLight(40, 170, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    why_ = why;
    shake_ = 0.5f;
    sys_->rumble(0.52f, 0.3f, 180);
    sys_->setLight(180, 30, 20);
    sys_->apu.noiseBurst(0.48f, 260.f, 0.34f);
}

void Game::physics(float gas, float brake, float steer) {
    gas_ = clampf(gas, 0.f, 1.f);
    brake_ = clampf(brake, 0.f, 1.f);
    steer_ = clampf(steer, -1.f, 1.f);
    time_ += DT;
    float a = gas_ * 8.4f - brake_ * 15.f - 0.55f * v_;
    v_ = std::max(0.f, v_ + a * DT);
    if (v_ > 20.f) v_ = 20.f;
    float prev = s_;
    s_ += v_ * DT;
    float yaw = steer_ * (1.25f + 0.045f * v_);
    y_ = clampf(y_ + yaw * DT, -kLane - 0.12f, kLane + 0.12f);
    shake_ = std::max(0.f, shake_ - DT);
    if (brake_ > 0.6f && v_ > 4.f) shake_ = std::max(shake_, 0.07f);

    if (time_ > kLimit) {
        fail("the other crew took the route");
        return;
    }

    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& wheel = kWheels[i];
        float along0 = prev - kRear;
        float along1 = s_ + kNose;
        if (along1 < wheel.s - wheel.r || along0 > wheel.s + wheel.r) continue;
        float nearS = std::max(wheel.s - wheel.r, s_ - kRear);
        float farS = std::min(wheel.s + wheel.r, s_ + kNose);
        if (farS < nearS) continue;
        if (std::fabs(y_ - wheel.y) < wheel.r + kHalf - 0.02f) {
            fail("touched a wheel");
            return;
        }
    }

    if (prev < kFinish && s_ >= kFinish) {
        if (std::fabs(y_) > kEndGate) fail("missed the end");
        else win();
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.06f);
    beep_ = 0.08f;
}

bool Game::project(float wz, float wy, float& sx, float& sy, float& ppm) const {
    float dz = wz - s_;
    if (dz < 0.5f || dz > 72.f) return false;
    float n = kNear / dz;
    sy = kHorizon + n * (gs::SCREEN_H - kHorizon);
    ppm = kPpm * n;
    sx = 160.f + (wy - y_) * ppm;
    return sy > -40.f && sy < gs::SCREEN_H + 40.f;
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

void Game::skyRoad() {
    uint16_t zen = gs::rgb4(2, 3, 6);
    uint16_t mid = gs::rgb4(5, 5, 8);
    uint16_t hor = gs::rgb4(9, 7, 5);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(10, 3, 3), 0.5f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(6, 11, 6), 0.45f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.34f ? lerpC(zen, mid, t / 0.34f) : lerpC(mid, hor, (t - 0.34f) / 0.66f);
        sys_->vdp.lineFog[y] = 0;
        gs::RoadLine& r = sys_->vdp.road[y];
        r.on = false;
        if (y <= int(kHorizon)) continue;
        float n = (y - kHorizon) / (gs::SCREEN_H - kHorizon);
        if (n < 0.012f) continue;
        float dz = kNear / n;
        float world = s_ + dz;
        r.on = true;
        r.cx = 160.f - y_ * kPpm * n;
        r.hw = kRoad * kPpm * n;
        r.v = world * 7.5f;
        r.pal = PAL_ROAD;
        r.style = 1;
        r.band = (int(world * 0.5f) & 3) == 0 ? 1 : 0;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
        sys_->vdp.lineFog[y] = uint8_t(clampf((1.f - n) * 10.f, 0.f, 9.f));
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
    auto fogOf = [](float ppm) { return int(clampf(12.f - ppm * 0.2f, 0.f, 13.f)); };

    for (int i = 10; i >= 0; i--) {
        float base = std::floor(s_ / 18.f) * 18.f + i * 18.f;
        float sx, sy, ppm;
        if (project(base, -kRoad - 0.4f, sx, sy, ppm))
            spr(art_.box, sx, sy, ppm * 2.4f, PAL_BOX, false, fogOf(ppm));
        if (project(base + 8.f, kRoad + 0.45f, sx, sy, ppm))
            spr(art_.sack, sx, sy, ppm * 1.6f, PAL_SACK, true, fogOf(ppm));
        if (project(base + 4.f, -kRoad + 0.15f, sx, sy, ppm))
            spr(art_.lamp, sx, sy, ppm * 2.5f, PAL_LAMP, false, fogOf(ppm));
    }

    for (int i = kWheelN - 1; i >= 0; i--) {
        const WheelDef& wheel = kWheels[i];
        float sx, sy, ppm;
        if (!project(wheel.s, wheel.y, sx, sy, ppm)) continue;
        spr(art_.wheel, sx, sy, ppm * wheel.r * 2.2f, PAL_WHEEL, false, fogOf(ppm));
    }

    float px, py, pp;
    if (project(kFinish, -1.1f, px, py, pp)) spr(art_.post, px, py, pp * 3.6f, PAL_POST, false, fogOf(pp));
    if (project(kFinish, 1.1f, px, py, pp)) spr(art_.post, px, py, pp * 3.6f, PAL_POST, false, fogOf(pp));
    if (project(kFinish, 0.f, px, py, pp)) spr(art_.banner, px, py - pp * 3.3f, pp * 0.9f, PAL_POST, false, fogOf(pp));

    float jx = (mode_ == Mode::Run) ? std::sin(time_ * 28.f) * shake_ * 6.f : 0.f;
    spr(art_.hood, 160.f + jx, 224.f, 96.f, PAL_VAN);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 MAIL VAN", PAL_AMBER);
        hudC(6, "FINISH THE KILOMETER", PAL_HUD);
        hudC(8, "DO NOT TOUCH A WHEEL", PAL_BAD);
        hudC(10, "THE CLOCK IS THE OTHER CREW", PAL_AMBER);
        hudC(16, "A GAS   B BRAKE", PAL_HUD);
        hudC(17, "LEFT RIGHT  KEEP OFF THE WHEELS", PAL_HUD);
        hudC(22, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        hudC(3, "KILOMETER CLEAR", PAL_GOOD);
        hudC(5, "WHEELS UNTOUCHED", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "BEFORE THE OTHER CREW  %.1f S", time_);
        hudC(8, buf, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(3, "LEG FAILED", PAL_BAD);
        hudC(5, why_, PAL_BAD);
        std::snprintf(buf, sizeof buf, "%4.0f M OF 1000", clampf(s_, 0.f, kFinish));
        hudC(8, buf, PAL_HUD);
    } else {
        float left = std::max(0.f, kFinish - s_);
        std::snprintf(buf, sizeof buf, "%4.0f M", left);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(28, 1, buf, PAL_AMBER);
        int w = nextWheel();
        if (s_ > 930.f && (w < 0 || kWheels[w].s < s_)) {
            hudC(2, "TAKE THE END", std::fabs(y_) < kEndGate ? PAL_GOOD : PAL_BAD);
        } else if (w >= 0) {
            float dist = kWheels[w].s - s_;
            if (dist > 0.5f) {
                std::snprintf(buf, sizeof buf, "WHEEL  %3.0f M", dist);
                hudC(2, buf, PAL_AMBER);
            } else {
                hudC(2, "WHEEL", PAL_BAD);
            }
        }
        float clock = std::max(0.f, kLimit - time_);
        std::snprintf(buf, sizeof buf, "CREW %4.0f S", clock);
        hud(1, 26, buf, clock < 12.f ? PAL_BAD : PAL_HUD);
        hud(22, 26, "A GAS  B BRAKE", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.74f);
    sys.apu.setEcho(0.06f, 0.12f, 0.06f);
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
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.2f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 8) chime_ = -1;
        }
    }

    if (mode_ == Mode::Title) {
        time_ += DT;
        s_ = 28.f + std::sin(time_ * 0.35f) * 1.4f;
        y_ = std::sin(time_ * 0.5f) * 0.3f;
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
            blip(260.f);
            draw();
            return;
        }
    }

    physics(gas, brake, steer);
    if (mode_ == Mode::Run) {
        float rpm = 64.f + v_ * 13.f + gas_ * 28.f;
        sys.apu.tone(2, rpm, 0.024f + gas_ * 0.028f);
        sys.apu.noise(brake_ > 0.5f && v_ > 1.f ? 0.07f : 0.012f, 480.f + v_ * 16.f, false);
        bool tight = false;
        int w = nextWheel();
        if (w >= 0) {
            float dist = kWheels[w].s - s_;
            tight = dist < 14.f && dist > -1.f && std::fabs(y_ - kWheels[w].y) < kWheels[w].r + kHalf + 0.4f;
        }
        sys.setLight(tight ? 180 : 50, tight ? 40 : 130, tight ? 20 : 30);
    }
    draw();
}

}  // namespace mailkilo
