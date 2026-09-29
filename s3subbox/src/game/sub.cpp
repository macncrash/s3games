#include "game/sub.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "version.h"

namespace subbox {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHullL = 6.4f;
constexpr float kHullH = 2.15f;
constexpr float kStop = 0.55f;
constexpr float kHoldNeed = 0.40f;
constexpr float kFloor = -30.f;
constexpr float kCeil = 22.f;
struct Leg {
    float bx, by, hw, hh;
    float current;
    float clock;
};

constexpr int kLegs = 3;

const Leg kLeg[kLegs] = {
    {210.f, 2.f, 20.f, 8.5f, 9.f, 34.f},
    {248.f, -8.f, 17.f, 7.0f, 13.f, 32.f},
    {270.f, 10.f, 15.f, 6.2f, 16.f, 30.f},
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::beginLeg() {
    const Leg& L = kLeg[leg_];
    x_ = 24.f;
    y_ = 0.f;
    vx_ = 0.f;
    vy_ = 0.f;
    hold_ = 0.f;
    still_ = 0.f;
    clock_ = L.clock;
    banner_ = 1.1f;
    why_.clear();
    camX_ = x_;
    camY_ = y_;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.resize(64, 32);
    for (int y = 0; y < sys.vdp.B.h; y++)
        for (int x = 0; x < sys.vdp.B.w; x++) sys.vdp.B.set(x, y, gs::entry(art_.waterTile, PAL_WATER));
    sys.vdp.setFogColor(gs::rgb4(0, 2, 5));
    sys.apu.setMaster(0.6f);
    sys.apu.setEcho(0.18f, 0.22f, 0.10f);
    leg_ = 0;
    raceT_ = 0.f;
    won_ = false;
    over_ = false;
    beginLeg();
    mode_ = bot_ ? Mode::Dive : Mode::Title;
}

void Game::controls(float& thrust, float& ballast) {
    const gs::Pad& p = sys_->pad;
    thrust = 0.f;
    ballast = 0.f;
    if (p.down(gs::BTN_RIGHT) || p.down(gs::BTN_C)) thrust += 1.f;
    if (p.down(gs::BTN_LEFT) || p.down(gs::BTN_B)) thrust -= 1.f;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A)) ballast += 1.f;
    if (p.down(gs::BTN_DOWN)) ballast -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) thrust = clampf(thrust + p.axisX, -1.f, 1.f);
    if (std::fabs(p.axisY) > 0.18f) ballast = clampf(ballast + p.axisY, -1.f, 1.f);
    if (p.accel > 0.15f) thrust = std::max(thrust, p.accel);
    if (p.brake > 0.15f) thrust = std::min(thrust, -p.brake);
}

bool Game::hullInside() const {
    const Leg& L = kLeg[leg_];
    const float lx[4] = {-kHullL, kHullL, kHullL, -kHullL};
    const float ly[4] = {-kHullH, -kHullH, kHullH, kHullH};
    for (int i = 0; i < 4; i++) {
        float wx = x_ + lx[i];
        float wy = y_ + ly[i];
        if (wx < L.bx - L.hw || wx > L.bx + L.hw) return false;
        if (wy < L.by - L.hh || wy > L.by + L.hh) return false;
    }
    return true;
}

float Game::noseX() const { return x_ + kHullL; }

void Game::pilot(float& thrust, float& ballast) {
    const Leg& L = kLeg[leg_];
    const float dx = L.bx - x_;
    const float dy = L.by - y_;
    const bool in = hullInside();
    if (in) {
        thrust = clampf(-vx_ * 0.28f, -1.f, 1.f);
        ballast = clampf((dy * 0.6f - vy_) * 0.3f, -1.f, 1.f);
        if (x_ > L.bx + L.hw * 0.25f) thrust = -0.85f;
        if (x_ < L.bx - L.hw * 0.35f) thrust = 0.45f;
        return;
    }
    float want = 18.f;
    if (dx < 80.f) want = clampf(dx * 0.20f, 2.5f, 12.f);
    if (dx < 36.f) want = clampf((L.bx - L.hw * 0.15f - x_) * 0.22f, 0.f, 6.f);
    if (x_ > L.bx - L.hw) want = clampf(L.bx - x_, -1.f, 3.f);
    if (std::fabs(dy) > L.hh * 0.45f && dx < 48.f) want = std::min(want, 4.f);
    thrust = clampf((want - vx_) * 0.22f, -1.f, 1.f);
    ballast = clampf((dy * 1.15f - vy_) * 0.28f, -1.f, 1.f);
}

void Game::update(float dt) {
    if (banner_ > 0.f) banner_ -= dt;
    float thrust = 0.f, ballast = 0.f;
    if (banner_ <= 0.f) {
        if (bot_) pilot(thrust, ballast);
        else controls(thrust, ballast);
    }

    const Leg& L = kLeg[leg_];
    float ax = thrust * 42.f;
    float ay = ballast * 30.f;
    if (thrust * vx_ < 0.f) ax += thrust * 18.f;
    vx_ += ax * dt;
    vy_ += ay * dt;
    vx_ -= vx_ * 0.95f * dt;
    vy_ -= vy_ * 1.35f * dt;
    float approach = (L.bx - L.hw) - x_;
    float fade = approach < 46.f ? clampf(approach / 46.f, 0.f, 1.f) : 1.f;
    if (x_ > L.bx - L.hw) fade = 0.f;
    vx_ += L.current * fade * dt;
    vy_ += (leg_ == 2 ? 2.4f : 0.f) * fade * dt;
    vx_ = clampf(vx_, -16.f, 28.f);
    vy_ = clampf(vy_, -14.f, 14.f);
    x_ += vx_ * dt;
    y_ += vy_ * dt;
    if (y_ < kFloor) {
        y_ = kFloor;
        vy_ = std::fabs(vy_) * 0.25f;
    }
    if (y_ > kCeil) {
        y_ = kCeil;
        vy_ = -std::fabs(vy_) * 0.25f;
    }
    if (noseX() > L.bx + L.hw) {
        x_ = L.bx + L.hw - kHullL;
        vx_ = std::min(vx_, -1.5f);
    }

    raceT_ += dt;
    clock_ -= dt;
    const bool in = hullInside();
    const float sp = std::hypot(vx_, vy_);
    if (in && sp < kStop) hold_ += dt;
    else hold_ = 0.f;
    if (!in && sp < kStop * 0.75f && raceT_ > 1.6f && x_ > 70.f) still_ += dt;
    else still_ = 0.f;

    if (hold_ >= kHoldNeed) {
        if (leg_ + 1 < kLegs) {
            leg_++;
            beginLeg();
            sys_->apu.tone(0, 392.f, 0.10f);
            tone_ = 0.16f;
        } else {
            mode_ = Mode::Win;
            won_ = true;
            over_ = true;
            why_ = "stopped inside the box";
            sys_->apu.tone(0, 523.f, 0.14f);
            tone_ = 0.28f;
        }
    } else if (noseX() >= L.bx + L.hw - 0.05f && !in) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "missed the end";
    } else if (still_ > 1.05f) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "stopped short of the box";
    } else if (clock_ <= 0.f) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "the leg ran out";
        clock_ = 0.f;
    }

    if (tone_ > 0.f) {
        tone_ -= dt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0, 0);
    } else if (mode_ == Mode::Dive && std::fabs(thrust) > 0.2f) {
        sys_->apu.tone(1, 48.f + std::fabs(vx_) * 3.1f, 0.035f);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::place(const gs::Mipped& m, float wx, float wy, float h, int pal, bool hflip) {
    if (h < 1.2f || m.h < 1) return;
    float sx = (wx - camX_) + gs::SCREEN_W * 0.5f;
    float sy = gs::SCREEN_H * 0.5f - (wy - camY_);
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(sx - s.w * 0.5f));
    s.y = int16_t(std::lround(sy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 64 || s.x + s.w < -64 || s.y > gs::SCREEN_H + 64 || s.y + s.h < -64) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = hflip;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    const Leg& L = kLeg[leg_];
    float lookX = x_ + 36.f;
    float lookY = y_ * 0.65f + L.by * 0.35f;
    if (mode_ == Mode::Title) {
        lookX = 130.f;
        lookY = 2.f;
    }
    camX_ += (lookX - camX_) * 0.10f;
    camY_ += (lookY - camY_) * 0.10f;

    for (int row = 0; row < gs::SCREEN_H; row++) {
        float wy = camY_ + (gs::SCREEN_H * 0.5f - row);
        float depth = clampf((kCeil - wy) / (kCeil - kFloor), 0.f, 1.f);
        int r = int(1 + (1.f - depth) * 2.f);
        int g = int(3 + (1.f - depth) * 6.f);
        int b = int(6 + (1.f - depth) * 7.f);
        v.lineBackdrop[row] = gs::rgb4(r, g, b);
        v.road[row].on = false;
        v.lineFog[row] = uint8_t(depth > 0.72f ? int((depth - 0.72f) * 28.f) : 0);
    }
    v.B.scroll(int(-camX_ * 0.35f), int(camY_ * 0.2f));

    for (int i = 0; i < 18; i++) {
        float rx = -40.f + i * 28.f + ((i * 17) % 9);
        place(art_.rock, rx, kFloor + 1.5f, 16.f + (i % 3) * 3.f, PAL_ROCK);
        if (i % 2 == 0) place(art_.kelp, rx + 8.f, kFloor + 10.f, 22.f, PAL_KELP);
    }
    place(art_.box, L.bx + 2.f, L.by, L.hh * 2.15f, PAL_BOX);
    place(art_.lamp, L.bx + L.hw - 3.f, L.by + L.hh - 2.f, 8.f, PAL_BUB);

    float phase = t_ * 1.7f;
    for (int i = 0; i < 5; i++) {
        float bx = x_ - 10.f - i * 7.f - std::fmod(phase * (8.f + i), 18.f);
        float by = y_ + std::sin(phase + i) * 2.4f + (i - 2) * 1.3f;
        place(art_.bubble, bx, by, 4.f + (i % 3), PAL_BUB);
    }
    bool flip = vx_ < -0.4f;
    place(art_.sub, x_, y_, 28.f, PAL_SUB, flip);

    char line[64];
    if (mode_ == Mode::Title) {
        hudC(6, "S3 SUB BOX", PAL_HUD);
        hudC(9, "STOP INSIDE THE BOX", PAL_HUD);
        hudC(11, "MISSING THE END FAILS THE LEG", PAL_HUD);
        hudC(16, "ARROWS  THRUST AND BALLAST", PAL_HUD);
        hudC(18, "ENTER TO DIVE", PAL_HUD);
        hudC(26, S3_VERSION_STRING, PAL_HUD);
        return;
    }
    std::snprintf(line, sizeof(line), "LEG %d/%d", leg_ + 1, kLegs);
    hud(1, 1, line, PAL_HUD);
    std::snprintf(line, sizeof(line), "SPD %02.0f", std::hypot(vx_, vy_));
    hud(12, 1, line, PAL_HUD);
    std::snprintf(line, sizeof(line), "CLK %04.1f", std::max(0.f, clock_));
    hud(24, 1, line, PAL_HUD);
    if (banner_ > 0.f && mode_ == Mode::Dive) {
        std::snprintf(line, sizeof(line), "LEG %d  DOCK IN THE BOX", leg_ + 1);
        hudC(12, line, PAL_HUD);
    }
    if (hullInside() && mode_ == Mode::Dive) hudC(24, "INSIDE", PAL_HUD);
    if (mode_ == Mode::Win) {
        hudC(10, "BERTHED", PAL_HUD);
        hudC(12, why_, PAL_HUD);
    } else if (mode_ == Mode::Lose) {
        hudC(10, "LEG FAILED", PAL_HUD);
        hudC(12, why_, PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_C)) {
            leg_ = 0;
            raceT_ = 0.f;
            beginLeg();
            mode_ = Mode::Dive;
        }
    } else if (mode_ == Mode::Dive) {
        update(kDt);
    } else if (mode_ == Mode::Lose) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
            leg_ = 0;
            raceT_ = 0.f;
            won_ = false;
            over_ = false;
            beginLeg();
            mode_ = Mode::Dive;
        }
    }
    draw();
}

}  // namespace subbox
