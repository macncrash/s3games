#include "game/header.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace headerbox {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kBoxX = 210.f;
constexpr float kBoxY = 18.f;
constexpr float kBoxHW = 30.f;
constexpr float kBoxHH = 22.f;
constexpr float kHullL = 8.2f;
constexpr float kHullW = 3.6f;
constexpr float kStop = 0.42f;
constexpr float kHoldNeed = 0.45f;
constexpr float kClock = 28.f;
// Wind blows toward +Y (a beam reach when the bow points east).
constexpr float kWindFrom = -kPi * 0.5f;

float wrapPi(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float sailDrive(float heading) {
    float into = std::fabs(wrapPi(heading - (kWindFrom + kPi)));
    if (into < 0.50f) return 0.f;
    float beam = std::sin(into);
    float run = into > 2.15f ? 0.62f : 0.f;
    return std::max(beam, run);
}

}  // namespace

void Game::begin() {
    x_ = 28.f;
    y_ = 0.f;
    heading_ = 0.15f;
    speed_ = 0.f;
    hold_ = 0.f;
    still_ = 0.f;
    raceT_ = 0.f;
    clock_ = kClock;
    won_ = false;
    over_ = false;
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
    sys.vdp.setFogColor(gs::rgb4(1, 4, 8));
    sys.apu.setMaster(0.65f);
    sys.apu.setEcho(0.12f, 0.18f, 0.08f);
    begin();
    if (bot_) mode_ = Mode::Sail;
    else mode_ = Mode::Title;
}

void Game::controls(float& gas, float& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    gas = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (p.axisX > 0.2f || p.axisX < -0.2f) steer = clampf(steer + p.axisX, -1.f, 1.f);
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C)) gas += 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) gas -= 1.f;
}

void Game::pilot(float& gas, float& steer) {
    const float dx = kBoxX - x_;
    const float dy = kBoxY - y_;
    const float dist = std::hypot(dx, dy);
    const float fx = std::cos(heading_);
    const float fy = std::sin(heading_);
    const float along = fx * dx + fy * dy;
    const float side = -fy * dx + fx * dy;

    if (hullInside() && std::fabs(speed_) < kStop + 0.35f) {
        gas = 0.f;
        steer = 0.f;
        if (std::fabs(speed_) > kStop) gas = speed_ > 0.f ? -0.55f : 0.35f;
        return;
    }

    float aim = std::atan2(dy, dx);
    float err = wrapPi(aim - heading_);
    float desired = std::min(46.f, 12.f + dist * 0.22f);
    if (dist < 46.f) {
        desired = clampf(dist * 0.38f, 4.f, 16.f);
        if (std::fabs(side) > 6.f) desired = std::min(desired, 8.f);
    }
    if (dist < 16.f) {
        err = wrapPi(0.f - heading_) * 0.35f;
        desired = clampf(along * 0.55f, -6.f, 8.f);
    } else if (std::fabs(err) > 0.65f) {
        desired = std::min(desired, 10.f);
    }
    steer = clampf(err * 1.8f, -1.f, 1.f);
    float drive = std::max(0.25f, sailDrive(heading_));
    float gap = desired - speed_;
    gas = clampf(gap / (48.f * drive), -1.f, 1.f);
}

bool Game::hullInside() const {
    const float c = std::cos(heading_);
    const float s = std::sin(heading_);
    const float lx[4] = {-kHullL, kHullL, kHullL, -kHullL};
    const float ly[4] = {-kHullW, -kHullW, kHullW, kHullW};
    for (int i = 0; i < 4; i++) {
        float wx = x_ + lx[i] * c - ly[i] * s;
        float wy = y_ + lx[i] * s + ly[i] * c;
        if (wx < kBoxX - kBoxHW || wx > kBoxX + kBoxHW) return false;
        if (wy < kBoxY - kBoxHH || wy > kBoxY + kBoxHH) return false;
    }
    return true;
}

void Game::update(float dt) {
    float gas = 0.f, steer = 0.f;
    if (bot_) pilot(gas, steer);
    else controls(gas, steer);

    float drive = sailDrive(heading_);
    if (gas > 0.f) speed_ += gas * drive * 52.f * dt;
    else speed_ += gas * 28.f * dt;
    float drag = 1.15f + (gas <= 0.05f ? 1.35f : 0.f) + (drive < 0.05f && gas > 0.f ? 2.4f : 0.f);
    speed_ -= speed_ * drag * dt;
    speed_ = clampf(speed_, -10.f, 52.f);
    float turn = (1.15f + std::min(std::fabs(speed_), 24.f) * 0.045f) * (std::fabs(speed_) < 1.2f ? 0.35f : 1.f);
    heading_ = wrapPi(heading_ + steer * turn * dt);
    x_ += std::cos(heading_) * speed_ * dt;
    y_ += std::sin(heading_) * speed_ * dt;

    raceT_ += dt;
    clock_ -= dt;
    bool in = hullInside();
    if (in && std::fabs(speed_) < kStop) hold_ += dt;
    else hold_ = 0.f;
    if (!in && std::fabs(speed_) < kStop * 0.7f) still_ += dt;
    else still_ = 0.f;

    if (hold_ >= kHoldNeed) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        why_ = "stopped inside the box";
        sys_->apu.tone(0, 523.f, 0.12f);
        tone_ = 0.25f;
    } else if (still_ > 1.15f) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "stopped short of the box";
    } else if (clock_ <= 0.f) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "the other crew took the box";
        clock_ = 0.f;
    }

    if (tone_ > 0.f) {
        tone_ -= dt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0, 0);
    } else if (mode_ == Mode::Sail && gas > 0.2f && drive > 0.2f) {
        sys_->apu.tone(1, 70.f + std::fabs(speed_) * 2.2f, 0.03f);
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

void Game::place(const gs::Mipped& m, float wx, float wy, float h, int pal) {
    if (h < 1.2f || m.h < 1) return;
    float sx = (wx - camX_) + gs::SCREEN_W * 0.5f;
    float sy = gs::SCREEN_H * 0.5f - (wy - camY_);
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(sx - s.w * 0.5f));
    s.y = int16_t(std::lround(sy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float lookX = x_, lookY = (y_ + kBoxY) * 0.35f;
    if (mode_ == Mode::Title) {
        lookX = 120.f;
        lookY = 10.f;
    }
    camX_ += (lookX - camX_) * 0.12f;
    camY_ += (lookY - camY_) * 0.12f;

    for (int row = 0; row < gs::SCREEN_H; row++) {
        float wy = camY_ + (gs::SCREEN_H * 0.5f - row);
        int band = int(std::floor(wy / 16.f)) & 1;
        v.lineBackdrop[row] = band ? gs::rgb4(1, 6, 11) : gs::rgb4(2, 7, 12);
        v.road[row].on = false;
        v.lineFog[row] = 0;
    }
    v.B.scroll(int(-camX_) , int(camY_));

    place(art_.box, kBoxX, kBoxY, 56.f, PAL_BOX);
    const float px[4] = {-30.f, 30.f, 30.f, -30.f};
    const float py[4] = {-22.f, -22.f, 22.f, 22.f};
    for (int i = 0; i < 4; i++) place(art_.post, kBoxX + px[i], kBoxY + py[i], 16.f, PAL_POST);

    if (std::fabs(speed_) > 2.f) {
        place(art_.wake, x_ - std::cos(heading_) * 16.f, y_ - std::sin(heading_) * 16.f, 10.f + std::fabs(speed_) * 0.15f,
              PAL_WATER);
    }
    int frame = int(std::floor((heading_ / kTau) * 16.f + 16.5f)) & 15;
    place(art_.boat[frame], x_, y_, 36.f, PAL_BOAT);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(6, "S3 HEADER BOX", PAL_HUD);
        hudC(9, "STOP INSIDE THE BOX", PAL_HUD);
        hudC(12, "THE HEADER HAS ONE JOB", PAL_HUD);
        hudC(18, "ENTER TO CAST OFF", PAL_HUD);
        hudC(21, "ARROWS STEER   Z SHEETS IN", PAL_HUD);
    } else if (mode_ == Mode::Sail) {
        std::snprintf(line, sizeof(line), "CLOCK %04.1f", clock_);
        hud(1, 1, line, clock_ < 8.f ? PAL_HUD : PAL_HUD);
        hud(1, 2, hullInside() ? "IN THE BOX" : "OUTSIDE", PAL_HUD);
        std::snprintf(line, sizeof(line), "WAY %.0f", speed_);
        hud(30, 1, line, PAL_HUD);
        hudC(26, "STOP THE HULL INSIDE", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(10, "STOPPED", PAL_HUD);
        hudC(12, "INSIDE THE BOX", PAL_HUD);
    } else {
        hudC(10, "LEG FAILED", PAL_HUD);
        hudC(12, why_, PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) {
            begin();
            mode_ = Mode::Sail;
            sys.apu.tone(0, 330.f, 0.08f);
            tone_ = 0.1f;
        }
    } else if (mode_ == Mode::Sail) {
        update(kDt);
    } else if ((mode_ == Mode::Win || mode_ == Mode::Lose) &&
               (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) {
        begin();
        mode_ = Mode::Title;
    }
    draw();
}

}  // namespace headerbox
