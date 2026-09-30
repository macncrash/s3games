#include "game/buoy.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace buoy {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kAccel = 260.f;
constexpr float kDrag = 1.35f;
constexpr float kClock = 52.f;
constexpr float kRingIn = 26.f;
constexpr float kRingOut = 150.f;
constexpr float kHit = 16.f;
constexpr float kOrbit = 74.f;
constexpr float kDockX = 0.f;
constexpr float kDockY = -48.f;
constexpr int kBuoyPal[3] = {PAL_BUOY, PAL_BUOY2, PAL_BUOY3};

float wrapPi(float a) {
    while (a > kPi) a -= kPi * 2.f;
    while (a < -kPi) a += kPi * 2.f;
    return a;
}

float hypot2(float x, float y) { return std::sqrt(x * x + y * y); }

}  // namespace

void Game::begin() {
    const float bx[3] = {230.f, 70.f, -210.f};
    const float by[3] = {150.f, 400.f, 190.f};
    for (int i = 0; i < 3; i++) {
        marks_[i].x = bx[i];
        marks_[i].y = by[i];
        marks_[i].done = false;
    }
    got_ = 0;
    lives_ = 3;
    why_ = "";
    over_ = false;
    won_ = false;
    t_ = 0;
    left_ = kClock;
    x_ = kDockX;
    y_ = kDockY;
    vx_ = 0;
    vy_ = 0;
    lap_ = 0;
    haveAng_ = false;
    settle_ = 0;
    invuln_ = 1.2f;
    heading_ = kPi * 0.5f;
    camX_ = x_;
    camY_ = y_;
    mode_ = Mode::Play;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.62f);
    sys.apu.setEcho(0.12f, 0.22f, 0.14f);
    begin();
    if (!bot_) mode_ = Mode::Title;
}

void Game::human(float& sx, float& sy) {
    const gs::Pad& p = sys_->pad;
    sx = 0;
    sy = 0;
    if (p.down(gs::BTN_LEFT)) sx -= 1.f;
    if (p.down(gs::BTN_RIGHT)) sx += 1.f;
    if (p.down(gs::BTN_DOWN)) sy -= 1.f;
    if (p.down(gs::BTN_UP)) sy += 1.f;
    if (std::fabs(p.axisX) + std::fabs(p.axisY) > 0.2f) {
        sx = p.axisX;
        sy = p.axisY;
    }
}

void Game::pilot(float& sx, float& sy) {
    float tx = kDockX;
    float ty = kDockY;
    float want = 0.f;
    if (got_ < 3) {
        const Mark& m = marks_[got_];
        float dx = x_ - m.x;
        float dy = y_ - m.y;
        float r = hypot2(dx, dy);
        float a = std::atan2(dy, dx);
        float lead = (r < 120.f) ? 0.85f : 0.15f;
        float ta = a + lead;
        float rad = (r > 130.f) ? std::min(r - 20.f, kOrbit) : kOrbit;
        if (r < 50.f) rad = kOrbit;
        tx = m.x + std::cos(ta) * rad;
        ty = m.y + std::sin(ta) * rad;
        want = 150.f;
    } else {
        float dx = kDockX - x_;
        float dy = kDockY - y_;
        float d = hypot2(dx, dy);
        tx = kDockX;
        ty = kDockY;
        want = d > 50.f ? 130.f : 0.f;
    }
    float dx = tx - x_;
    float dy = ty - y_;
    float d = std::max(8.f, hypot2(dx, dy));
    float wx = dx / d * want;
    float wy = dy / d * want;
    sx = std::clamp((wx - vx_) / 140.f, -1.f, 1.f);
    sy = std::clamp((wy - vy_) / 140.f, -1.f, 1.f);
}

void Game::crash() {
    lives_ -= 1;
    if (sys_) {
        sys_->apu.noiseBurst(0.45f, 1400.f, 9.f);
        sys_->rumble(0.9f, 0.5f, 180);
    }
    if (lives_ <= 0) {
        mode_ = Mode::Fail;
        over_ = true;
        won_ = false;
        why_ = "drink";
        return;
    }
    x_ = kDockX;
    y_ = kDockY + 30.f;
    vx_ = 0;
    vy_ = 0;
    lap_ = 0;
    haveAng_ = false;
    invuln_ = 1.4f;
    settle_ = 0;
    left_ = std::max(0.f, left_ - 4.f);
}

void Game::win() {
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "dock";
    if (sys_) sys_->rumble(0.25f, 0.5f, 200);
}

void Game::clockOut() {
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = "clock";
    left_ = 0;
}

void Game::roundBuoy(float dt) {
    (void)dt;
    if (got_ >= 3) return;
    Mark& m = marks_[got_];
    float dx = x_ - m.x;
    float dy = y_ - m.y;
    float r = hypot2(dx, dy);
    if (r < kHit && invuln_ <= 0.f) {
        crash();
        return;
    }
    if (r >= kRingIn && r <= kRingOut) {
        float a = std::atan2(dy, dx);
        if (haveAng_) lap_ += wrapPi(a - lastAng_);
        lastAng_ = a;
        haveAng_ = true;
        if (lap_ >= kPi * 2.f - 0.15f) {
            m.done = true;
            got_ += 1;
            lap_ = 0;
            haveAng_ = false;
            if (sys_) sys_->apu.tone(0, 520.f + got_ * 80.f, 0.22f);
        }
    }
}

void Game::tryDock(float dt) {
    if (got_ < 3 || mode_ != Mode::Play) {
        settle_ = 0;
        return;
    }
    float d = hypot2(x_ - kDockX, y_ - kDockY);
    float sp = hypot2(vx_, vy_);
    if (d < 34.f && sp < 22.f) {
        settle_ += dt;
        if (settle_ >= 0.35f) win();
    } else {
        settle_ = 0;
    }
}

void Game::physics(float dt, float sx, float sy) {
    if (invuln_ > 0.f) invuln_ -= dt;
    sx = std::clamp(sx, -1.f, 1.f);
    sy = std::clamp(sy, -1.f, 1.f);
    vx_ += sx * kAccel * dt;
    vy_ += sy * kAccel * dt;
    float damp = std::exp(-kDrag * dt);
    vx_ *= damp;
    vy_ *= damp;
    x_ += vx_ * dt;
    y_ += vy_ * dt;
    float sp = hypot2(vx_, vy_);
    if (sp > 12.f) heading_ = std::atan2(vy_, vx_);
    if (sys_) {
        float rotor = 0.04f + sp * 0.00035f + (std::fabs(sx) + std::fabs(sy)) * 0.03f;
        sys_->apu.tone(1, 78.f + sp * 0.35f, rotor);
        sys_->apu.noise(0.015f + sp * 0.00008f, 900.f + sp, false);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) begin();
        float bob = std::sin(t_ * 1.6f) * 6.f;
        x_ = kDockX + bob;
        y_ = kDockY + 10.f;
        camX_ += (x_ - camX_) * 0.08f;
        camY_ += (y_ + 40.f - camY_) * 0.08f;
        draw();
        return;
    }
    if (mode_ == Mode::Play) {
        float sx = 0, sy = 0;
        if (bot_) pilot(sx, sy);
        else human(sx, sy);
        physics(kDt, sx, sy);
        roundBuoy(kDt);
        if (mode_ == Mode::Play) tryDock(kDt);
        if (mode_ == Mode::Play) {
            left_ -= kDt;
            if (left_ <= 0.f) clockOut();
        }
        if (mode_ == Mode::Play && (int(t_ * 4.f) % 8) == 0) sys.apu.tone(0, 0, 0);
        float k = 1.f - std::exp(-kDt * 4.2f);
        camX_ += (x_ - camX_) * k;
        camY_ += (y_ - camY_) * k;
    } else {
        sys.apu.tone(1, 0, 0);
        sys.apu.noise(0, 0, false);
    }
    draw();
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldW, float worldH, int pal, bool flip, float ax,
                 float ay) {
    if (worldW < 0.4f || worldH < 0.4f || m.w < 1) return;
    float sx = 160.f + (wx - camX_);
    float sy = 120.f - (wy - camY_);
    float left = sx - (flip ? (1.f - ax) : ax) * worldW;
    float top = sy - ay * worldH;
    if (left > gs::SCREEN_W + 8 || top > gs::SCREEN_H + 8 || left + worldW < -8 || top + worldH < -8) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(worldW), 1L, 400L);
    long sh = std::clamp(std::lround(worldH), 1L, 400L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(left), -400L, 800L));
    s.y = int16_t(std::clamp(std::lround(top), -400L, 600L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    long sw = std::lround(w);
    long sh = std::lround(h);
    s.w = int16_t(std::clamp(sw, 1L, 400L));
    s.h = int16_t(std::clamp(sh, 1L, 200L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s && s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wave = 0.5f + 0.5f * std::sin(y * 0.18f + t_ * 1.7f + camX_ * 0.02f);
        int g = 5 + int(wave * 3.f);
        int b = 8 + (y % 5 == 0 ? 2 : 0);
        v.lineBackdrop[y] = gs::rgb4(1, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 28, float(art_.title.h), PAL_BANNER);
        hudC(8, "ROUND THE BUOYS", PAL_HUD);
        hudC(10, "SAME DOCK  BEAT THE CREW", PAL_HUD);
        hudC(24, "STICK FLIES   START", PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        spr(art_.docked, 160, 36, float(art_.docked.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped& word = why_[0] == 'c' ? art_.late : art_.dipped;
        spr(word, 160, 36, float(word.h), PAL_ALERT);
    } else {
        char buf[32];
        int secs = int(std::ceil(std::max(0.f, left_) - 1e-4f));
        std::snprintf(buf, sizeof(buf), "CREW %d:%02d", secs / 60, secs % 60);
        hud(1, 1, buf, left_ < 12.f ? PAL_ALERT : PAL_HUD);
        std::snprintf(buf, sizeof(buf), "BUOY %d/3", got_);
        hud(30, 1, buf, PAL_BANNER);
        hud(1, 26, "LIVES", PAL_HUD);
        hud(7, 26, lives_ >= 3 ? "---" : lives_ == 2 ? "--" : lives_ == 1 ? "-" : "", PAL_WIN);
    }

    bool blink = invuln_ > 0.f && mode_ == Mode::Play && (int(t_ * 12.f) & 1);
    if (!blink && mode_ != Mode::Fail) {
        float c = std::cos(heading_);
        float s = std::sin(heading_);
        place(art_.shadow, x_ + 6.f, y_ - 8.f, 28.f, 14.f, PAL_DIM, false, 0.5f, 0.5f);
        place(art_.rotor, x_, y_, 46.f, 46.f, PAL_ROTOR, false, 0.5f, 0.5f);
        place(art_.heli, x_ + c * 2.f, y_ + s * 2.f, 36.f, 32.f, PAL_HELI, c < 0, 0.5f, 0.5f);
        if (hypot2(vx_, vy_) > 20.f) {
            place(art_.wake, x_ - c * 22.f, y_ - s * 22.f, 22.f, 12.f, PAL_WAKE, false, 0.5f, 0.5f);
        }
    }

    for (int i = 0; i < 3; i++) {
        const Mark& m = marks_[i];
        float bob = std::sin(t_ * 2.4f + i) * 2.f;
        int pal = m.done ? PAL_WIN : (i == got_ ? kBuoyPal[i] : PAL_DIM);
        place(art_.buoy, m.x, m.y + bob, 22.f, 26.f, pal, false, 0.5f, 0.85f);
        place(art_.flag, m.x + 8.f, m.y + 16.f + bob, 14.f, 14.f, m.done ? PAL_WIN : PAL_FLAG, false, 0.1f, 0.9f);
        if (!m.done && i == got_ && mode_ == Mode::Play) {
            float a = t_ * 1.2f + lap_;
            float mx = m.x + std::cos(a) * 40.f;
            float my = m.y + std::sin(a) * 40.f;
            place(art_.wake, mx, my, 10.f, 6.f, PAL_MARK, false, 0.5f, 0.5f);
        }
    }

    place(art_.dock, kDockX, kDockY, 92.f, 42.f, got_ == 3 ? PAL_WIN : PAL_DOCK, false, 0.5f, 0.5f);
    place(art_.shore, kDockX, kDockY - 36.f, 180.f, 34.f, PAL_SHORE, false, 0.5f, 1.f);

    for (int i = 0; i < 3; i++) {
        float gx = std::fmod(80.f + i * 140.f + t_ * (18.f + i * 4.f), 520.f) - 180.f;
        float gy = 80.f + i * 90.f + std::sin(t_ + i) * 10.f;
        place(art_.gull, gx, gy, 16.f, 8.f, PAL_HUD, false, 0.5f, 0.5f);
    }
}

}  // namespace buoy
