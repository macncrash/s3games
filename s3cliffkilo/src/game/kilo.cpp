#include "game/kilo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cliffkilo {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFinish = 1000.f;
constexpr float kRear = 1.8f;
constexpr float kNose = 3.4f;
constexpr float kHalf = 0.48f;
constexpr float kPanHalf = 0.30f;
constexpr float kClear = 0.18f;
constexpr float kLane = 1.42f;
constexpr float kShelf = 1.70f;
constexpr float kRoad = 2.15f;
constexpr float kEndGate = 0.58f;
constexpr float kHorizon = 86.f;
constexpr float kPpm = 46.f;
constexpr float kNear = 1.15f;
constexpr float kLimit = 96.f;
constexpr int kWheelN = 6;

struct WheelDef {
    float s, y, r;
};

// Iron sheaves bolted into the cliff. Each one leaves one side of the shelf open.
constexpr WheelDef kWheels[kWheelN] = {
    {155.f, 1.02f, 0.50f}, {305.f, -1.08f, 0.56f}, {455.f, 0.08f, 0.38f},
    {605.f, -0.98f, 0.62f}, {760.f, 1.12f, 0.48f}, {900.f, -0.18f, 0.42f},
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
    if (s_ >= 190.f) return 2;
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
    s_ = 36.f;
    y_ = -0.15f;
    pan_ = -0.05f;
    v_ = 0;
    shake_ = 0;
}

void Game::startRun() {
    s_ = 0;
    y_ = 0.f;
    pan_ = 0.f;
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
    float target = 0.f;
    int w = nextWheel();
    bool threading = false;
    if (w >= 0) {
        const WheelDef& wheel = kWheels[w];
        float ahead = wheel.s - s_;
        if (ahead < 58.f && ahead > -kNose) {
            threading = true;
            float need = wheel.r + std::max(kHalf, kPanHalf) + kClear;
            float left = wheel.y - need;
            float right = wheel.y + need;
            bool leftOk = left >= -kLane + 0.04f;
            bool rightOk = right <= kLane - 0.04f;
            if (leftOk && rightOk) target = (std::fabs(pan_ - left) < std::fabs(pan_ - right)) ? left : right;
            else if (leftOk) target = left;
            else target = right;
            target = clampf(target, -kLane, kLane);
        }
    }
    if (s_ > 930.f && !threading) target = 0.f;

    float err = target - pan_;
    steer = clampf(err * 3.1f, -1.f, 1.f);

    float vWant = 13.6f;
    if (threading) {
        float ahead = kWheels[w].s - (s_ + kNose);
        if (std::fabs(err) > 0.50f && ahead < 16.f) vWant = 7.8f;
        else if (std::fabs(err) > 0.26f) vWant = 10.6f;
    }
    if (s_ > 955.f) vWant = std::min(vWant, 10.f);
    gas = v_ < vWant ? 1.f : 0.f;
    brake = v_ > vWant + 1.0f ? 1.f : 0.f;
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
    sys_->rumble(0.12f, 0.04f, 120);
    sys_->setLight(40, 150, 80);
    std::printf("S3 CLIFFKILO  WIN  finished the kilometer without touching wheels  (%.1f s)\n", time_);
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
    float a = gas_ * 5.6f - brake_ * 12.f - 0.48f * v_;
    v_ = std::max(0.f, v_ + a * DT);
    if (v_ > 16.2f) v_ = 16.2f;
    float prev = s_;
    s_ += v_ * DT;
    float yaw = steer_ * (1.35f + 0.035f * v_);
    y_ = clampf(y_ + yaw * DT, -kShelf, kShelf);
    // The pannier swings after the mule, so a late dodge still clips a sheave.
    pan_ += (y_ - pan_) * std::min(1.f, DT * 3.0f);
    shake_ = std::max(0.f, shake_ - DT);
    if (brake_ > 0.6f && v_ > 3.f) shake_ = std::max(shake_, 0.08f);

    if (std::fabs(y_) > kShelf - 0.02f || std::fabs(pan_) > kShelf - 0.02f) {
        fail("left the shelf");
        return;
    }

    auto hits = [&](float lat, float half, float r) { return lat < r + half - 0.02f; };

    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& wheel = kWheels[i];
        float along0 = prev - kRear;
        float along1 = s_ + kNose;
        if (along1 < wheel.s - wheel.r || along0 > wheel.s + wheel.r) continue;
        float nearS = std::max(wheel.s - wheel.r, s_ - kRear);
        float farS = std::min(wheel.s + wheel.r, s_ + kNose);
        if (farS < nearS) continue;
        bool body = hits(std::fabs(y_ - wheel.y), kHalf, wheel.r);
        bool pan = hits(std::fabs(pan_ - wheel.y), kPanHalf, wheel.r) && farS > s_ + kNose * 0.45f;
        if (body || pan) {
            fail("touched a wheel");
            return;
        }
    }

    if (prev < kFinish && s_ >= kFinish) {
        if (std::fabs(y_) > kEndGate || std::fabs(pan_) > kEndGate) fail("missed the end");
        else win();
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

    for (int i = 9; i >= 0; i--) {
        float base = std::floor(s_ / 22.f) * 22.f + i * 22.f;
        float sx, sy, ppm;
        if (project(base, kRoad + 0.35f, sx, sy, ppm))
            spr(art_.crag, sx, sy, ppm * 6.4f, PAL_ROCK, (i & 1) != 0, fogOf(ppm));
        if (project(base + 11.f, kRoad - 0.35f, sx, sy, ppm))
            spr(art_.scrub, sx, sy, ppm * 1.5f, PAL_ROCK, false, fogOf(ppm));
    }

    for (int i = kWheelN - 1; i >= 0; i--) {
        const WheelDef& wheel = kWheels[i];
        float sx, sy, ppm;
        if (!project(wheel.s, wheel.y, sx, sy, ppm)) continue;
        int fog = fogOf(ppm);
        spr(art_.hub, sx, sy - ppm * wheel.r * 0.2f, ppm * 0.55f, PAL_ROCK, wheel.y < 0, fog);
        spr(art_.sheave, sx, sy, ppm * wheel.r * 2.15f, PAL_WHEEL, false, fog);
    }

    float px, py, pp;
    if (project(kFinish, -0.95f, px, py, pp)) spr(art_.post, px, py, pp * 3.6f, PAL_GATE, false, fogOf(pp));
    if (project(kFinish, 0.95f, px, py, pp)) spr(art_.post, px, py, pp * 3.6f, PAL_GATE, false, fogOf(pp));
    if (project(kFinish, 0.f, px, py, pp)) spr(art_.ribbon, px, py - pp * 3.2f, pp * 0.7f, PAL_GATE, false, fogOf(pp));

    float jx = (mode_ == Mode::Run) ? std::sin(time_ * 26.f) * shake_ * 7.f : 0.f;
    float sway = (pan_ - y_) * 52.f;
    spr(art_.mule, 160.f + jx, 220.f, 92.f, PAL_MULE);
    spr(art_.pan, 160.f + sway + jx * 0.6f, 188.f, 34.f, PAL_PAN);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 CLIFF KILO", PAL_AMBER);
        hudC(6, "FINISH THE KILOMETER", PAL_HUD);
        hudC(8, "DO NOT TOUCH A WHEEL", PAL_BAD);
        hudC(10, "MISSING THE END FAILS THE LEG", PAL_AMBER);
        hudC(16, "A GAS   B BRAKE", PAL_HUD);
        hudC(17, "LEFT RIGHT  HOLD THE SHELF", PAL_HUD);
        hudC(22, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        hudC(3, "KILOMETER CLEAR", PAL_GOOD);
        hudC(5, "WHEELS UNTOUCHED", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "LEG %.1f S", time_);
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
        if (s_ > 945.f && (w < 0 || kWheels[w].s < s_)) {
            hudC(2, "TAKE THE END", std::fabs(pan_) < kEndGate ? PAL_GOOD : PAL_BAD);
        } else if (w >= 0) {
            float dist = kWheels[w].s - s_;
            if (dist > 0.5f) {
                std::snprintf(buf, sizeof buf, "WHEEL  %3.0f M", dist);
                hudC(2, buf, PAL_AMBER);
            } else {
                hudC(2, "WHEEL", PAL_BAD);
            }
        }
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
        s_ = 40.f + std::sin(time_ * 0.3f) * 1.6f;
        y_ = std::sin(time_ * 0.45f) * 0.22f;
        pan_ = y_ + std::sin(time_ * 1.4f) * 0.16f;
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
        float rpm = 48.f + v_ * 9.f + gas_ * 18.f;
        sys.apu.tone(2, rpm, 0.024f + gas_ * 0.02f);
        sys.apu.noise(brake_ > 0.5f && v_ > 1.f ? 0.07f : 0.01f, 280.f + v_ * 12.f, false);
    } else {
        sys.apu.tone(2, 0.f, 0.f);
        sys.apu.noise(0.f, 400.f, false);
    }
    draw();
}

}  // namespace cliffkilo
