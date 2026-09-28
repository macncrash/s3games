#include "game/kilo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cranekilo {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFinish = 1000.f;
constexpr float kRear = 2.6f;
constexpr float kNose = 6.8f;  // boom tip
constexpr float kHalf = 0.82f;
constexpr float kHookHalf = 0.34f;
constexpr float kClear = 0.20f;
constexpr float kLane = 2.05f;
constexpr float kRoad = 2.65f;
constexpr float kEndGate = 0.72f;
constexpr float kHorizon = 78.f;
constexpr float kPpm = 42.f;
constexpr float kNear = 1.05f;
constexpr float kLimit = 96.f;
constexpr int kWheelN = 6;

struct WheelDef {
    float s, y, r;
};

// Each wheel leaves one side of the lane open for the crane and its hook.
constexpr WheelDef kWheels[kWheelN] = {
    {140.f, 1.32f, 0.58f}, {300.f, -1.38f, 0.64f}, {450.f, 0.20f, 0.46f},
    {610.f, -1.22f, 0.70f}, {760.f, 1.28f, 0.56f}, {900.f, -0.40f, 0.50f},
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
    hook_ = 0.1f;
    v_ = 0;
    shake_ = 0;
}

void Game::startRun() {
    s_ = 0;
    y_ = 0.f;
    hook_ = 0.f;
    v_ = 0;
    time_ = 0;
    gas_ = brake_ = steer_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    chime_ = -1;
    shake_ = 0;
    mode_ = Mode::Run;
    blip(440.f);
}

void Game::pilot(float& gas, float& brake, float& steer) const {
    float target = 0.f;
    int w = nextWheel();
    bool threading = false;
    if (w >= 0) {
        const WheelDef& wheel = kWheels[w];
        float ahead = wheel.s - s_;
        if (ahead < 52.f && ahead > -kNose) {
            threading = true;
            float need = wheel.r + std::max(kHalf, kHookHalf) + kClear;
            float left = wheel.y - need;
            float right = wheel.y + need;
            bool leftOk = left >= -kLane + 0.05f;
            bool rightOk = right <= kLane - 0.05f;
            if (leftOk && rightOk) target = (std::fabs(hook_ - left) < std::fabs(hook_ - right)) ? left : right;
            else if (leftOk) target = left;
            else target = right;
            target = clampf(target, -kLane, kLane);
        }
    }
    if (s_ > 940.f && !threading) target = 0.f;

    float err = target - hook_;
    steer = clampf(err * 2.8f, -1.f, 1.f);

    float vWant = 14.2f;
    if (threading) {
        float ahead = kWheels[w].s - (s_ + kNose);
        if (std::fabs(err) > 0.55f && ahead < 18.f) vWant = 8.5f;
        else if (std::fabs(err) > 0.28f) vWant = 11.5f;
    }
    if (s_ > 960.f) vWant = std::min(vWant, 11.f);
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
    sys_->rumble(0.15f, 0.05f, 140);
    sys_->setLight(40, 170, 70);
    std::printf("S3 CRANEKILO  WIN  finished the kilometer without touching wheels  (%.1f s)\n", time_);
    std::fflush(stdout);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    why_ = why;
    shake_ = 0.5f;
    sys_->rumble(0.5f, 0.28f, 170);
    sys_->setLight(180, 30, 20);
    sys_->apu.noiseBurst(0.46f, 240.f, 0.32f);
}

void Game::physics(float gas, float brake, float steer) {
    gas_ = clampf(gas, 0.f, 1.f);
    brake_ = clampf(brake, 0.f, 1.f);
    steer_ = clampf(steer, -1.f, 1.f);
    time_ += DT;
    float a = gas_ * 6.4f - brake_ * 13.f - 0.42f * v_;
    v_ = std::max(0.f, v_ + a * DT);
    if (v_ > 17.5f) v_ = 17.5f;
    float prev = s_;
    s_ += v_ * DT;
    float yaw = steer_ * (1.55f + 0.04f * v_);
    y_ = clampf(y_ + yaw * DT, -kLane - 0.12f, kLane + 0.12f);
    // The hook lags the slew, so a late correction still clips a wheel.
    hook_ += (y_ - hook_) * std::min(1.f, DT * 3.2f);
    shake_ = std::max(0.f, shake_ - DT);
    if (brake_ > 0.6f && v_ > 3.f) shake_ = std::max(shake_, 0.07f);

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
        bool hook = hits(std::fabs(hook_ - wheel.y), kHookHalf, wheel.r) && farS > s_ + kNose * 0.55f;
        if (body || hook) {
            fail("touched a wheel");
            return;
        }
    }

    if (prev < kFinish && s_ >= kFinish) {
        if (std::fabs(y_) > kEndGate || std::fabs(hook_) > kEndGate) fail("missed the end");
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
    if (dz < 0.55f || dz > 78.f) return false;
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
    uint16_t mid = gs::rgb4(5, 6, 8);
    uint16_t hor = gs::rgb4(9, 8, 6);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(10, 3, 3), 0.5f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(6, 11, 6), 0.45f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.30f ? lerpC(zen, mid, t / 0.30f) : lerpC(mid, hor, (t - 0.30f) / 0.70f);
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
        r.v = world * 7.f;
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
    auto fogOf = [](float ppm) { return int(clampf(12.f - ppm * 0.22f, 0.f, 13.f)); };

    for (int i = 10; i >= 0; i--) {
        float base = std::floor(s_ / 18.f) * 18.f + i * 18.f;
        float sx, sy, ppm;
        if (project(base, -kRoad - 0.7f, sx, sy, ppm))
            spr(art_.shed, sx, sy, ppm * 5.2f, PAL_YARD, false, fogOf(ppm));
        if (project(base + 8.f, kRoad + 0.8f, sx, sy, ppm))
            spr(art_.shed, sx, sy, ppm * 4.6f, PAL_YARD, true, fogOf(ppm));
        if (project(base + 4.f, -kRoad - 0.15f, sx, sy, ppm))
            spr(art_.lamp, sx, sy, ppm * 3.1f, PAL_GATE, false, fogOf(ppm));
    }

    for (int i = kWheelN - 1; i >= 0; i--) {
        const WheelDef& wheel = kWheels[i];
        float sx, sy, ppm;
        if (!project(wheel.s, wheel.y, sx, sy, ppm)) continue;
        int fog = fogOf(ppm);
        spr(art_.axle, sx, sy - ppm * wheel.r * 0.15f, ppm * 0.7f, PAL_YARD, wheel.y < 0, fog);
        spr(art_.wheel, sx, sy, ppm * wheel.r * 2.2f, PAL_WHEEL, false, fog);
    }

    float px, py, pp;
    if (project(kFinish, -1.15f, px, py, pp)) spr(art_.post, px, py, pp * 4.2f, PAL_GATE, false, fogOf(pp));
    if (project(kFinish, 1.15f, px, py, pp)) spr(art_.post, px, py, pp * 4.2f, PAL_GATE, false, fogOf(pp));
    if (project(kFinish, 0.f, px, py, pp)) spr(art_.banner, px, py - pp * 3.8f, pp * 0.9f, PAL_GATE, false, fogOf(pp));

    float jx = (mode_ == Mode::Run) ? std::sin(time_ * 28.f) * shake_ * 8.f : 0.f;
    float sway = (hook_ - y_) * 46.f + std::sin(time_ * 2.1f) * 3.f;
    spr(art_.boom, 160.f + jx, 214.f, 168.f, PAL_CRANE);
    spr(art_.hook, 160.f + sway + jx, 168.f, 36.f, PAL_HOOK);
    spr(art_.dash, 160.f + jx, 224.f, 58.f, PAL_CRANE);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 CRANE KILO", PAL_AMBER);
        hudC(6, "FINISH THE KILOMETER", PAL_HUD);
        hudC(8, "DO NOT TOUCH A WHEEL", PAL_BAD);
        hudC(10, "MISSING THE END FAILS THE LEG", PAL_AMBER);
        hudC(16, "A GAS   B BRAKE", PAL_HUD);
        hudC(17, "LEFT RIGHT  SLEW THE BOOM", PAL_HUD);
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
        if (s_ > 950.f && (w < 0 || kWheels[w].s < s_)) {
            hudC(2, "TAKE THE END", std::fabs(hook_) < kEndGate ? PAL_GOOD : PAL_BAD);
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
        static const float notes[] = {349.f, 440.f, 523.f, 698.f};
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
        y_ = std::sin(time_ * 0.5f) * 0.28f;
        hook_ = y_ + std::sin(time_ * 1.3f) * 0.18f;
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
            blip(240.f);
            draw();
            return;
        }
    }

    physics(gas, brake, steer);
    if (mode_ == Mode::Run) {
        float rpm = 55.f + v_ * 11.f + gas_ * 24.f;
        sys.apu.tone(2, rpm, 0.028f + gas_ * 0.028f);
        sys.apu.noise(brake_ > 0.5f && v_ > 1.f ? 0.08f : 0.012f, 420.f + v_ * 16.f, false);
        bool tight = false;
        int w = nextWheel();
        if (w >= 0) {
            float dist = kWheels[w].s - s_;
            tight = dist < 16.f && dist > -2.f &&
                    std::fabs(hook_ - kWheels[w].y) < kWheels[w].r + kHalf + 0.4f;
        }
        sys.setLight(tight ? 170 : 40, tight ? 50 : 130, tight ? 20 : 30);
    }
    draw();
}

}  // namespace cranekilo
