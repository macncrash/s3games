#include "game/game.h"

#include "game/world.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace cliffbuoy {
namespace {

constexpr float PI = 3.14159265f;
constexpr float TAU = PI * 2.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float wrapAng(float a) {
    while (a > PI) a -= TAU;
    while (a < -PI) a += TAU;
    return a;
}

struct Wp {
    float x, y, arrive;
    bool crawl;
};

// Port loops around the three marks, then a slow berth on the same dock.
const Wp kRoute[] = {
    {108.f, 196.f, 12.f, false}, {108.f, 150.f, 12.f, false}, {108.f, 108.f, 12.f, false},
    {96.f, 74.f, 12.f, false},   {64.f, 66.f, 12.f, false},   {40.f, 86.f, 12.f, false},
    {40.f, 128.f, 12.f, false},  {70.f, 156.f, 14.f, false},  {120.f, 176.f, 14.f, false},
    {210.f, 150.f, 14.f, false}, {250.f, 122.f, 12.f, false}, {286.f, 92.f, 12.f, false},
    {268.f, 48.f, 12.f, false},  {210.f, 52.f, 14.f, false},  {190.f, 92.f, 12.f, false},
    {126.f, 100.f, 12.f, false}, {122.f, 152.f, 12.f, false}, {170.f, 166.f, 12.f, false},
    {204.f, 136.f, 14.f, false}, {190.f, 178.f, 12.f, true},  {160.f, 196.f, 8.f, true},
};
constexpr int kRouteN = int(sizeof(kRoute) / sizeof(kRoute[0]));

}  // namespace

int Game::headingFrame() const {
    float a = heading_;
    while (a < 0) a += TAU;
    while (a >= TAU) a -= TAU;
    int i = int((a + PI / 8.f) / (PI / 4.f));
    return i & 7;
}

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.08f);
    melodyT_ = 0.07f;
    melody_ = -2;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::begin() {
    for (int i = 0; i < NBUOY; i++) cleared_[i] = false;
    next_ = 0;
    armed_ = false;
    sweep_ = 0;
    wp_ = 0;
    x_ = START_X;
    y_ = START_Y;
    heading_ = START_H;
    speed_ = 0;
    hold_ = 0;
    legT_ = 0;
    won_ = false;
    over_ = false;
    melody_ = -1;
    why_ = "the leg ran out";
    mode_ = Mode::Run;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    const uint16_t water = gs::rgb4(1, 5, 9);
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.lineBackdrop[y] = water;
    sys.vdp.setFogColor(gs::rgb4(3, 6, 9));
    x_ = START_X;
    y_ = START_Y;
    heading_ = START_H;
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::steerOf(float& steer, float& throttle, float& brake) const {
    const gs::Pad& p = sys_->pad;
    steer = p.axisX;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    steer = clampf(steer, -1.f, 1.f);
    throttle = p.accel;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A)) throttle = 1.f;
    if (p.axisY > 0.25f) throttle = std::max(throttle, p.axisY);
    brake = p.brake;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) brake = 1.f;
    if (p.axisY < -0.25f) brake = std::max(brake, -p.axisY);
}

void Game::botDrive(float& steer, float& throttle, float& brake) {
    steer = 0;
    throttle = 0;
    brake = 0;
    if (mode_ != Mode::Run || wp_ >= kRouteN) {
        brake = 1.f;
        return;
    }
    const Wp& w = kRoute[wp_];
    float dx = w.x - x_;
    float dy = w.y - y_;
    float dist = std::sqrt(dx * dx + dy * dy);
    float err = wrapAng(std::atan2(dy, dx) - heading_);
    steer = clampf(err * 3.2f, -1.f, 1.f);
    float ae = std::fabs(err);
    float cap = w.crawl ? (dist > 16.f ? 22.f : 8.f) : (ae > 0.7f ? 22.f : ae > 0.3f ? 40.f : 56.f);
    if (speed_ > cap) brake = 1.f;
    else throttle = w.crawl && dist < 12.f ? 0.f : 1.f;
    if (dist < w.arrive && ae < 0.8f) wp_++;
}

void Game::update(float dt) {
    if (mode_ != Mode::Run) return;
    legT_ += dt;
    float steer = 0, throttle = 0, brake = 0;
    if (bot_) botDrive(steer, throttle, brake);
    else steerOf(steer, throttle, brake);

    heading_ = wrapAng(heading_ + steer * 2.5f * dt);
    float drive = throttle * 92.f;
    if (brake > 0.f) {
        if (std::fabs(speed_) < 6.f && throttle < 0.15f) {
            drive = 0.f;
            speed_ = 0.f;
        } else {
            drive += (speed_ >= 0.f ? -160.f : 160.f) * brake;
        }
    }
    speed_ += (drive - speed_ * 0.85f) * dt;
    speed_ = clampf(speed_, -24.f, 64.f);
    x_ += std::cos(heading_) * speed_ * dt;
    y_ += std::sin(heading_) * speed_ * dt;

    if (std::fabs(speed_) > 8.f) sys_->apu.tone(1, 46.f + std::fabs(speed_) * 0.7f, 0.04f);
    else sys_->apu.tone(1, 0, 0);

    if (x_ < 18.f || x_ > 302.f || y_ < 20.f || y_ > 214.f) {
        mode_ = Mode::Fail;
        over_ = true;
        why_ = "left the cove";
        sys_->apu.tone(1, 0, 0);
        sys_->apu.noiseBurst(0.45f, 500.f, 0.35f);
        return;
    }
    for (int i = 0; i < NBUOY; i++) {
        float dx = x_ - BUOY_X[i];
        float dy = y_ - BUOY_Y[i];
        if (dx * dx + dy * dy < HIT_R * HIT_R) {
            mode_ = Mode::Fail;
            over_ = true;
            why_ = "struck a buoy";
            sys_->apu.tone(1, 0, 0);
            sys_->apu.noiseBurst(0.5f, 700.f, 0.3f);
            sys_->rumble(0.6f, 0.3f, 160);
            return;
        }
    }
    if (legT_ > LEG_LIMIT) {
        mode_ = Mode::Fail;
        over_ = true;
        why_ = "the leg ran out";
        sys_->apu.tone(1, 0, 0);
        return;
    }

    if (next_ < NBUOY) {
        float dx = x_ - BUOY_X[next_];
        float dy = y_ - BUOY_Y[next_];
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > RING_IN && dist < RING_OUT) {
            float ang = std::atan2(dy, dx);
            if (!armed_) {
                armed_ = true;
                lastAng_ = ang;
                sweep_ = 0;
            } else {
                sweep_ -= wrapAng(ang - lastAng_);
                lastAng_ = ang;
            }
        } else if (dist >= RING_OUT && armed_ && sweep_ < SWEEP_NEED * 0.45f) {
            armed_ = false;
            sweep_ = 0;
        }
        if (armed_ && sweep_ >= SWEEP_NEED && (dist > 46.f || sweep_ >= 3.1f)) {
            cleared_[next_] = true;
            next_++;
            armed_ = false;
            sweep_ = 0;
            blip(620.f + float(next_) * 70.f);
            sys_->rumble(0.15f, 0.3f, 50);
        }
    }

    bool home = next_ >= NBUOY && x_ > DOCK_L + 6.f && x_ < DOCK_R - 6.f && y_ > DOCK_T + 8.f && y_ < DOCK_B - 2.f;
    if (home && std::fabs(speed_) < 16.f) hold_ += dt;
    else hold_ = 0;
    if (hold_ > 0.3f) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        melody_ = 0;
        melodyT_ = 0;
        sys_->apu.tone(1, 0, 0);
        sys_->rumble(0.2f, 0.45f, 120);
    }
}

void Game::chime(float dt) {
    static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
    if (melody_ < 0 || melody_ >= 4) return;
    if (melodyT_ <= 0.f) sys_->apu.tone(0, notes[melody_], 0.08f);
    melodyT_ += dt;
    if (melodyT_ > 0.16f) {
        melodyT_ = 0;
        melody_++;
        if (melody_ >= 4) sys_->apu.tone(0, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    t_ += dt;
    if (melody_ == -2) {
        melodyT_ -= dt;
        if (melodyT_ <= 0.f) {
            melody_ = -1;
            sys_->apu.tone(0, 0, 0);
        }
    }
    if ((mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Fail) && startPressed()) begin();
    if (mode_ == Mode::Win || mode_ == Mode::Fail) chime(dt);
    else if (mode_ == Mode::Run) update(dt);
    draw();
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();

    float bob = std::sin(t_ * 2.2f) * 1.6f;
    if (mode_ == Mode::Title) spr(art_.banner, 160.f, 48.f, float(art_.banner.h), PAL_BANNER);

    spr(art_.skiff[headingFrame()], x_, y_, 34.f, PAL_SKIFF);
    spr(art_.shadow, x_, y_ + 10.f, 14.f, PAL_SKIFF, false, true);
    if (std::fabs(speed_) > 6.f) {
        float wx = x_ - std::cos(heading_) * 16.f;
        float wy = y_ - std::sin(heading_) * 16.f;
        spr(art_.wake, wx, wy, 8.f + std::fabs(speed_) * 0.05f, PAL_WAKE);
    }
    for (int i = 0; i < NBUOY; i++) {
        const gs::Mipped& pic = cleared_[i] ? art_.buoyDone : art_.buoy;
        spr(pic, BUOY_X[i], BUOY_Y[i] + bob, 16.f, PAL_BUOY);
        if (!cleared_[i] && i == next_ && mode_ != Mode::Title)
            spr(art_.mark, BUOY_X[i], BUOY_Y[i] - 16.f + bob, 10.f, PAL_MARK);
    }

    if (mode_ == Mode::Title) {
        hudC(10, "ROUND THE BUOYS", PAL_WHITE);
        hudC(12, "RETURN TO THE SAME DOCK", PAL_AMBER);
        hudC(14, "LEAVE EACH MARK TO PORT", PAL_WHITE);
        hudC(16, "A STRIKE FAILS THE LEG", PAL_RED);
        hudC(18, "LEFT RIGHT STEERS", PAL_WHITE);
        hudC(19, "UP DRIVES   DOWN BRAKES", PAL_WHITE);
        hudC(22, "PRESS START", PAL_GREEN);
        return;
    }

    hud(1, 0, "S3 CLIFFBUOY", PAL_AMBER);
    char line[40];
    std::snprintf(line, sizeof line, "BUOYS %d OF %d", next_, NBUOY);
    hud(24, 0, line, PAL_WHITE);
    int clock = std::max(0, int(std::ceil(LEG_LIMIT - legT_)));
    std::snprintf(line, sizeof line, "LEG %d", clock);
    hud(1, 1, line, clock < 20 ? PAL_RED : PAL_WHITE);

    if (mode_ == Mode::Win) {
        hudC(6, "DOCKED", PAL_GREEN);
        hudC(8, "BUOYS ROUNDED", PAL_WHITE);
        hudC(10, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(6, "LEG FAILED", PAL_RED);
        hudC(8, why_, PAL_WHITE);
        hudC(10, "PRESS START", PAL_AMBER);
    } else if (next_ >= NBUOY) {
        hud(1, 26, "BERTH ON THE DOCK AND STOP", PAL_GREEN);
    } else {
        hud(1, 26, "ROUND THE LIT BUOY TO PORT", PAL_WHITE);
    }
}

}  // namespace cliffbuoy
