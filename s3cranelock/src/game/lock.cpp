#include "lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cranelock {
namespace {

constexpr float kCanalL = 96.f;
constexpr float kCanalR = 224.f;
constexpr float kMid = 160.f;
constexpr float kLo = 156.f;
constexpr float kHi = 74.f;
constexpr float kGateT = 10.f;
constexpr float kLeaf = (kCanalR - kCanalL) * 0.5f;
constexpr float kHullW = 26.f;
constexpr float kHullH = 34.f;
constexpr float kHook = 12.f;
constexpr float kJib = 42.f;
constexpr float kCrew = 18.f;
constexpr float kPi = 3.14159265f;

float wrapPi(float a) {
    while (a > kPi) a -= kPi * 2.f;
    while (a < -kPi) a += kPi * 2.f;
    return a;
}

bool boxHit(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return std::abs(ax - bx) * 2.f < aw + bw && std::abs(ay - by) * 2.f < ah + bh;
}

}  // namespace

void Game::blip(float freq) { sys_->apu.tone(0, freq, 0.08f); }

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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.f * scale;
    float w = float(s.size()) * adv;
    x -= w * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    sys.apu.setMaster(0.5f);
    sys.apu.tone(0, 0, 0);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
}

void Game::begin() {
    mode_ = Mode::Shift;
    over_ = false;
    won_ = false;
    openLo_ = openHi_ = false;
    lo_ = hi_ = 0;
    hx_ = kMid;
    hy_ = 186.f;
    ang_ = kPi;
    shiftT_ = 0;
    t_ = 0;
    why_ = "the other crew took the lock";
    blip(220);
}

bool Game::hitsGate(float gx0, float gx1, float gy) const {
    if (gx1 - gx0 < 1.f) return false;
    float hkx = hx_ + std::sin(ang_) * kJib;
    float hky = hy_ - std::cos(ang_) * kJib;
    float cx = (gx0 + gx1) * 0.5f;
    float cw = gx1 - gx0;
    if (boxHit(hx_, hy_, kHullW, kHullH, cx, gy, cw, kGateT)) return true;
    if (boxHit(hkx, hky, kHook, kHook, cx, gy, cw, kGateT)) return true;
    return false;
}

void Game::scrape() {
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = "scraped a gate";
    sys_->apu.noiseBurst(0.2f, 1400.f, 0.15f);
}

void Game::botPlan(float& ax, float& ay, float& slew, bool& open) {
    float want = kPi;
    float d = wrapPi(want - ang_);
    slew = d > 0.08f ? 1.f : d < -0.08f ? -1.f : 0.f;
    float dx = kMid - hx_;
    ax = std::abs(dx) < 1.5f ? 0.f : std::clamp(dx / 10.f, -1.f, 1.f);
    ay = 0;
    open = false;
    bool aligned = std::abs(d) < 0.12f && std::abs(dx) < 5.f;
    auto gate = [&](float gateY, float gap, bool& ask) {
        float wait = gateY + 28.f;
        if (gap < 0.97f) {
            if (hy_ > wait + 2.f) ay = -1.f;
            else if (hy_ < wait - 2.f) ay = 1.f;
            if (aligned && hy_ > gateY + 18.f && hy_ < gateY + 40.f) ask = true;
        } else {
            ay = -1.f;
        }
    };
    float hkx = hx_ + std::sin(ang_) * kJib;
    float hky = hy_ - std::cos(ang_) * kJib;
    bool lowerClear = hy_ + kHullH * 0.5f < kLo - kGateT && hky + kHook * 0.5f < kLo - kGateT;
    if (!lowerClear) {
        gate(kLo, lo_, open);
        return;
    }
    bool upperClear = hy_ + kHullH * 0.5f < kHi - kGateT && hky + kHook * 0.5f < kHi - kGateT;
    if (!upperClear) {
        gate(kHi, hi_, open);
        (void)hkx;
        return;
    }
    ay = hy_ > 22.f ? -1.f : 0.f;
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    float ax = 0, ay = 0, slew = 0;
    bool open = false;
    if (bot_) {
        botPlan(ax, ay, slew, open);
    } else {
        if (std::abs(pad.axisX) > 0.2f) ax = pad.axisX;
        else if (pad.down(gs::BTN_RIGHT)) ax = 1;
        else if (pad.down(gs::BTN_LEFT)) ax = -1;
        if (std::abs(pad.axisY) > 0.2f) ay = -pad.axisY;
        else if (pad.down(gs::BTN_UP)) ay = -1;
        else if (pad.down(gs::BTN_DOWN)) ay = 1;
        if (pad.down(gs::BTN_Z)) slew = 1;
        if (pad.down(gs::BTN_X)) slew = -1;
        open = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
    }
    ang_ = wrapPi(ang_ + slew * 2.35f * dt);
    hx_ += ax * 76.f * dt;
    hy_ += ay * 70.f * dt;
    float halfW = kHullW * 0.5f;
    hx_ = std::clamp(hx_, kCanalL + halfW + 2.f, kCanalR - halfW - 2.f);
    hy_ = std::clamp(hy_, 16.f, 206.f);

    auto ask = [&](float gateY, bool& flag, float gap) {
        if (!open || flag || gap >= 1.f) return;
        if (hy_ > gateY && hy_ < gateY + 46.f && std::abs(hx_ - kMid) < 36.f) flag = true;
    };
    bool hullPastLo = hy_ + kHullH * 0.5f < kLo - 6.f;
    ask(kLo, openLo_, lo_);
    if (hullPastLo) ask(kHi, openHi_, hi_);
    if (openLo_) lo_ = std::min(1.f, lo_ + 0.62f * dt);
    if (openHi_) hi_ = std::min(1.f, hi_ + 0.62f * dt);

    float loLen = kLeaf * (1.f - lo_);
    float hiLen = kLeaf * (1.f - hi_);
    if (hitsGate(kCanalL, kCanalL + loLen, kLo) || hitsGate(kCanalR - loLen, kCanalR, kLo) ||
        hitsGate(kCanalL, kCanalL + hiLen, kHi) || hitsGate(kCanalR - hiLen, kCanalR, kHi)) {
        scrape();
        return;
    }

    float hky = hy_ - std::cos(ang_) * kJib;
    float gateTop = kHi - kGateT * 0.5f - 1.f;
    bool clear = hy_ + kHullH * 0.5f < gateTop && hky + kHook * 0.5f < gateTop;
    if (clear) {
        mode_ = Mode::Win;
        over_ = true;
        won_ = true;
        why_ = "passed the lock";
        sys_->apu.tone(1, 523.f, 0.12f);
        return;
    }
    if (shiftT_ >= kCrew) {
        mode_ = Mode::Fail;
        over_ = true;
        won_ = false;
        why_ = "the other crew took the lock";
        sys_->apu.tone(1, 110.f, 0.1f);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        float u = y / float(gs::SCREEN_H);
        int b = 7 + int((1.f - u) * 5.f);
        int g = 8 + int(std::sin((y + t_ * 30.f) * 0.08f) * 1.2f);
        v.lineBackdrop[y] = gs::rgb4(2, std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        v.lineFog[y] = 0;
    }
    v.setFogColor(gs::rgb4(2, 6, 10));

    float hky = hy_ - std::cos(ang_) * kJib;
    float hkx = hx_ + std::sin(ang_) * kJib;

    if (mode_ == Mode::Title) {
        text("S3 CRANE LOCK", 160, 28, 1.15f, PAL_AMBER);
        text("PASS THE LOCK", 160, 52, 0.7f, PAL_HUD);
        text("DO NOT SCRAPE A GATE", 160, 70, 0.55f, PAL_RED);
        text("ARROWS MOVE", 160, 96, 0.5f, PAL_GREEN);
        text("Z X SLEW THE JIB", 160, 112, 0.5f, PAL_GREEN);
        text("C OPENS THE GATE", 160, 128, 0.5f, PAL_GREEN);
        text("THE OTHER CREW IS THE CLOCK", 160, 156, 0.48f, PAL_AMBER);
        text("START", 160, 184, 0.85f, int(t_ * 2.f) % 2 ? PAL_HUD : PAL_GREEN);
        return;
    }

    spr(art_.berth, kMid, 28, 22, PAL_MARK);
    for (int i = 0; i < 8; i++) {
        spr(art_.stone, kCanalL - 14, 16.f + i * 28.f, 30, PAL_STONE);
        spr(art_.stone, kCanalR + 14, 16.f + i * 28.f, 30, PAL_STONE);
    }

    auto leafOnly = [&](float gateY, float len, bool right) {
        if (len < 1.2f) return;
        gs::Sprite s;
        s.img = art_.gate.pick(16);
        s.pal = PAL_GATE;
        s.h = 16;
        s.w = int16_t(std::lround(len));
        s.x = int16_t(std::lround(right ? kCanalR - len : kCanalL));
        s.y = int16_t(std::lround(gateY - 8));
        s.hflip = right;
        v.sprite(s);
    };
    float loLen = kLeaf * (1.f - lo_);
    float hiLen = kLeaf * (1.f - hi_);
    leafOnly(kLo, loLen, false);
    leafOnly(kLo, loLen, true);
    leafOnly(kHi, hiLen, false);
    leafOnly(kHi, hiLen, true);

    for (int i = 1; i <= 6; i++) {
        float u = i / 7.f;
        spr(art_.bead, hx_ + (hkx - hx_) * u, hy_ + (hky - hy_) * u, 7, PAL_HULL);
    }
    spr(art_.hook, hkx, hky, kHook + 2.f, PAL_HOOK);
    spr(art_.hull, hx_, hy_, kHullH, PAL_HULL, std::sin(ang_) < 0);

    float crewU = std::clamp(shiftT_ / kCrew, 0.f, 1.f);
    spr(art_.crew, 28.f + crewU * 264.f, 12.f, 12, PAL_CREW);

    char buf[48];
    float left = std::max(0.f, kCrew - shiftT_);
    std::snprintf(buf, sizeof buf, "OTHER CREW  %4.1f", left);
    hud(1, 0, buf, left < 5.f ? PAL_RED : PAL_AMBER);
    if (mode_ == Mode::Shift) {
        hud(1, 26, "Z/X SLEW   C GATE", PAL_HUD);
        if (!openLo_ && lo_ < 1.f) hudC(24, "LOWER GATE SHUT", PAL_AMBER);
        else if (lo_ < 0.97f) hudC(24, "LOWER GATE MOVING", PAL_GREEN);
        else if (!openHi_ && hy_ + 20.f < kLo) hudC(24, "UPPER GATE SHUT", PAL_AMBER);
        else if (hi_ < 0.97f && openHi_) hudC(24, "UPPER GATE MOVING", PAL_GREEN);
    } else if (mode_ == Mode::Fail) {
        hudC(12, why_, PAL_RED);
        hudC(14, "START TO RETRY", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(12, "LOCK CLEAR", PAL_GREEN);
        hudC(14, "AHEAD OF THE OTHER CREW", PAL_AMBER);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    t_ += dt;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START)) begin();
    } else if (mode_ == Mode::Shift) {
        shiftT_ += dt;
        update(dt);
    } else if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Title;
        over_ = false;
        won_ = false;
    }
    if (mode_ != Mode::Title && (int(t_ * 8.f) % 8) == 0) sys.apu.tone(0, 0, 0);
    draw();
}

}  // namespace cranelock
