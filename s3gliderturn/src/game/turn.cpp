#include "game/turn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace gliderturn {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kTurnX[3] = {70.f, 138.f, 206.f};
constexpr int kTurnDir[3] = {-1, 1, -1};
constexpr float kHalf = 17.f;
constexpr float kBandLo = 8.2f;
constexpr float kBandHi = 15.8f;
constexpr float kEnd = 262.f;
constexpr float kGateLo = 4.8f;
constexpr float kGateHi = 8.6f;
constexpr float kTip = 0.96f;
constexpr float kCount = 0.40f;
constexpr float kArcRate = 1.75f;
constexpr float kLegLimit = 36.f;
constexpr float kCruise = 12.f;
constexpr const char* kTurnName[3] = {"TURN 1", "TURN 2", "TURN 3"};
constexpr const char* kNum[3] = {"1", "2", "3"};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float liftOf(float v) { return clampf((v - 8.f) / 12.f, 0.22f, 1.05f); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.12f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.16f, 0.55f, 0.22f};
    p.op[1] = {2.f, 0.28f, 0.02f, 0.2f, 0.3f, 0.18f};
    p.op[2] = {3.f, 0.08f, 0.02f, 0.22f, 0.2f, 0.2f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f};
    p.vol = 0.2f;
    p.tone = 1800.f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (next_ >= 3) return 3;
    if (next_ > 0 || arc_ > 0.08f) return 2;
    return 1;
}

int Game::wingFrame() const {
    float b = clampf(bank_, -0.92f, 0.92f);
    int fi = int(std::lround((b + 0.92f) / 0.3067f));
    return std::clamp(fi, 0, 6);
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    toast_ = "";
    toastT_ = 0.f;
    chime_ = -1;
    next_ = 0;
    arc_ = 0.45f;
    overB_ = 0.f;
    pegT_ = 0.f;
    snap_ = true;
    x_ = kTurnX[0] - 4.f;
    h_ = 12.2f;
    v_ = 16.f;
    vy_ = 0.f;
    bank_ = -0.62f;
    nose_ = 0.f;
    spoil_ = 0.f;
    stick_ = 0.f;
    camX_ = x_ + 8.f;
    camH_ = 8.5f;
    camS_ = 4.1f;
}

void Game::startRun() {
    x_ = 16.f;
    h_ = kCruise;
    v_ = 17.4f;
    vy_ = 0.f;
    bank_ = 0.f;
    nose_ = 0.f;
    spoil_ = 0.f;
    stick_ = 0.f;
    overB_ = 0.f;
    pegT_ = 0.f;
    arc_ = 0.f;
    next_ = 0;
    legT_ = 0.f;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    toast_ = "";
    toastT_ = 0.f;
    chime_ = -1;
    puffN_ = 0;
    shake_ = 0.f;
    for (Puff& p : puffs_) p = {};
    mode_ = Mode::Fly;
    snap_ = true;
    camX_ = x_ + 8.f;
    camH_ = h_;
    camS_ = 4.4f;
    blip(620.f);
}

void Game::pilot(float& nose, float& spoil, float& stick) const {
    float distEnd = kEnd - x_;
    float targetH = kCruise;
    float wantV = 16.2f;
    if (next_ >= 3) {
        float dist = std::max(0.f, distEnd);
        targetH = 6.55f + dist * 0.078f;
        wantV = 15.2f;
    }
    float want = 0.f;
    if (next_ < 3) {
        float dx = x_ - kTurnX[next_];
        float dir = float(kTurnDir[next_]);
        if (std::fabs(dx) <= kHalf) want = dir * 0.72f;
        else if (dx < -kHalf && dx > -(kHalf + 12.f)) {
            float u = (dx + kHalf + 12.f) / 12.f;
            want = dir * 0.72f * u;
        }
    }
    // Stay under a pegged stick. Only a full digital lock is allowed to creep into a tip.
    stick = clampf(want / 0.74f + (want - bank_) * 1.15f, -0.94f, 0.94f);

    float altErr = targetH - h_;
    float lift = std::max(liftOf(v_), 0.25f);
    float sw = clampf(altErr * 1.15f - vy_ * 0.9f, -2.1f, 1.8f);
    if (next_ >= 3 && distEnd < 22.f) sw = clampf(altErr * 1.45f - vy_ * 1.15f, -1.7f, 1.5f);
    spoil = 0.f;
    if (v_ > wantV + 0.35f && altErr > -0.45f) spoil = clampf((v_ - wantV) / 4.2f, 0.f, 0.6f);
    nose = sw / (4.8f * lift);
    if (spoil > 0.f) nose += spoil * 3.4f / (4.8f * lift);
    nose = clampf(nose, -1.f, 1.f);
}

void Game::win() {
    if (mode_ != Mode::Fly) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "clean";
    banner_ = "CLEAN";
    toast_ = "";
    vy_ = 0.f;
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.3f, 0.12f, 180);
    sys_->setLight(40, 180, 90);
}

void Game::fail(const char* why, const char* banner) {
    if (mode_ != Mode::Fly) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    banner_ = banner;
    if (h_ < 0.f) h_ = 0.f;
    shake_ = 1.f;
    sys_->rumble(0.6f, 0.3f, 200);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.45f, 380.f, 0.32f);
}

void Game::physics(float nose, float spoil, float stick) {
    if (mode_ != Mode::Fly) return;
    nose_ = clampf(nose, -1.f, 1.f);
    spoil_ = clampf(spoil, 0.f, 1.f);
    stick_ = clampf(stick, -1.f, 1.f);
    legT_ += kDt;

    float mag = std::fabs(stick_);
    float sgn = 0.f;
    if (stick_ > 0.02f) sgn = 1.f;
    else if (stick_ < -0.02f) sgn = -1.f;
    // A held digital lock creeps past the safe bank, so the wing can still tip.
    bool pegged = mag > 0.97f && std::fabs(bank_) > 0.60f && stick_ * bank_ > 0.f;
    if (pegged) {
        pegT_ += kDt;
        if (pegT_ > 0.75f) overB_ = std::min(1.f, overB_ + 0.38f * kDt);
    } else {
        pegT_ = std::max(0.f, pegT_ - 2.5f * kDt);
        overB_ = std::max(0.f, overB_ - 1.5f * kDt);
    }
    float gust = 0.f;
    if (next_ < 3) {
        float d = std::fabs(x_ - kTurnX[next_]);
        if (d < kHalf && h_ > kBandLo - 1.5f && h_ < kBandHi + 1.5f)
            gust = (1.f - d / kHalf) * 0.13f * float(kTurnDir[next_]);
    }
    float urged = stick_ * 0.74f;
    float target = urged + sgn * overB_ * 0.42f + gust;
    bank_ += (target - bank_) * std::min(1.f, 3.4f * kDt);

    float lift = liftOf(v_);
    float stall = std::max(0.f, 11.f - v_) * 0.65f;
    float vyCmd = nose_ * 4.8f * lift - spoil_ * 3.4f - stall;
    vy_ += (vyCmd - vy_) * std::min(1.f, 3.5f * kDt);
    v_ += (-0.22f - spoil_ * 2.0f + 0.15f * std::max(-vy_, 0.f) - 0.10f * std::max(vy_, 0.f)) * kDt;
    v_ = clampf(v_, 0.f, 26.f);
    x_ += v_ * kDt;
    h_ += vy_ * kDt;

    if (!std::isfinite(x_) || !std::isfinite(h_) || !std::isfinite(v_) || !std::isfinite(bank_)) {
        fail("tipped", "TIPPED");
        return;
    }
    if (std::fabs(bank_) > kTip) {
        fail("tipped", "TIPPED");
        return;
    }
    float reach = 0.30f + std::fabs(bank_) * 3.6f;
    if (h_ < reach && std::fabs(bank_) > 0.20f) {
        h_ = std::max(0.f, h_);
        fail("tipped", "TIPPED");
        return;
    }
    if (h_ <= 0.10f) {
        h_ = 0.f;
        fail("missed the end", "MISSED");
        return;
    }
    if (v_ < 7.f && h_ > 2.f) {
        fail("stalled", "STALL");
        return;
    }

    if (next_ < 3) {
        float dx = std::fabs(x_ - kTurnX[next_]);
        bool inH = h_ >= kBandLo && h_ <= kBandHi;
        float signedB = bank_ * float(kTurnDir[next_]);
        if (dx <= kHalf && inH && signedB > kCount && std::fabs(bank_) < kTip) {
            arc_ += signedB * kArcRate * kDt;
            if (arc_ >= 1.f) {
                toast_ = kTurnName[next_];
                toastT_ = 1.15f;
                blip(520.f + float(next_) * 90.f);
                sys_->rumble(0.18f, 0.06f, 70);
                next_++;
                arc_ = 0.f;
                overB_ = 0.f;
                pegT_ = 0.f;
            }
        } else if (x_ > kTurnX[next_] + kHalf) {
            fail("missed the turn", "NO TURN");
            return;
        }
    }

    if (x_ >= kEnd) {
        if (next_ < 3) fail("missed the turn", "NO TURN");
        else if (h_ < kGateLo || h_ > kGateHi) fail("missed the end", "MISSED");
        else win();
        return;
    }
    if (legT_ > kLegLimit) fail("missed the end", "MISSED");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.055f);
    beep_ = 0.08f;
}

void Game::audio() {
    if (mode_ != Mode::Fly) {
        sys_->apu.noise(0.f, 800.f, false);
        sys_->apu.tone(2, 0.f, 0.f);
        return;
    }
    float wind = clampf((v_ - 8.f) / 16.f, 0.f, 1.f) * (spoil_ > 0.4f ? 0.07f : 0.034f);
    sys_->apu.noise(wind, 780.f + v_ * 28.f, false);
    bool counting = false;
    if (next_ < 3) {
        float signedB = bank_ * float(kTurnDir[next_]);
        counting = std::fabs(x_ - kTurnX[next_]) <= kHalf && h_ >= kBandLo && h_ <= kBandHi && signedB > kCount;
    }
    if (counting) sys_->apu.tone(2, 280.f + arc_ * 520.f, 0.035f);
    else if (v_ < 12.f) sys_->apu.tone(2, 150.f + std::max(0.f, -vy_) * 16.f, 0.028f);
    else sys_->apu.tone(2, 0.f, 0.f);

    if (std::fabs(bank_) > 0.86f || overB_ > 0.45f) sys_->setLight(180, 40, 28);
    else if (counting) sys_->setLight(180, 120, 30);
    else if (next_ >= 3) sys_->setLight(40, 160, 90);
    else sys_->setLight(40, 90, 160);
}

void Game::sky() {
    uint16_t zen = gs::rgb4(3, 5, 10);
    uint16_t mid = gs::rgb4(8, 10, 14);
    uint16_t hor = gs::rgb4(14, 8, 4);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 3, 3), 0.45f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(10, 14, 7), 0.35f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.58f ? lerpC(zen, mid, t / 0.58f) : lerpC(mid, hor, (t - 0.58f) / 0.42f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(8, 7, 8));
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip, int fog) {
    if (ht < 1.2f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h * 0.5f)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -8) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::sprAnchor(const gs::Mipped& m, float ax, float ay, float sx, float sy, float destH, int pal) {
    if (destH < 1.5f || m.h < 1) return;
    float sc = destH / float(m.h);
    float w = float(m.w) * sc;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(destH)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(sx - ax * sc)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(sy - ay * sc)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -8) return;
    s.img = m.pick(destH);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal) {
    if (w < 1.2f || h < 1.2f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(top)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 4 || s.x + s.w < -4 || s.y > gs::SCREEN_H + 4 || s.y + s.h < -4) return;
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    const float adv = 17.f * scale;
    x -= float(std::strlen(s)) * adv * 0.5f;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, std::max(8.f, g.h * scale), pal, false);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();

    const bool framing = mode_ == Mode::Win || mode_ == Mode::Fail;
    float span = framing ? 48.f : clampf(46.f + std::max(0.f, h_ - 6.f) * 1.15f, 44.f, 78.f);
    float wantS = 250.f / span;
    float lead = (mode_ == Mode::Fly && !framing) ? clampf(v_ * 0.32f, 2.f, 9.f) : 0.f;
    float wantX = x_ + lead;
    float wantH = framing ? h_ * 0.45f + 3.f : std::max(4.f, h_ * 0.42f + 2.4f);
    if (mode_ == Mode::Title) {
        wantX = x_ + 8.f;
        wantH = 4.4f;
        wantS = 4.15f;
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
    } else if (snap_) {
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
        snap_ = false;
    } else {
        camX_ += (wantX - camX_) * 0.14f;
        camH_ += (wantH - camH_) * 0.14f;
        camS_ += (wantS - camS_) * 0.12f;
    }
    if (shake_ > 0.f) {
        shx_ = std::sin(legT_ * 86.f) * shake_ * 4.f;
        shy_ = std::cos(legT_ * 64.f) * shake_ * 3.f;
        shake_ *= 0.9f;
        if (shake_ < 0.05f) shake_ = 0.f;
    } else {
        shx_ = shy_ = 0.f;
    }

    const float scale = camS_;
    const float ax = 168.f + shx_;
    const float ay = 112.f + shy_;
    auto project = [&](float wx, float wy, float& sx, float& sy) {
        sx = ax + (wx - camX_) * scale;
        sy = ay - (wy - camH_) * scale;
    };

    if (mode_ == Mode::Title) {
        text("GLIDER TURN", 160, 20, 1.0f, PAL_HUD);
        text("THREE TURNS", 160, 44, 0.58f, PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 24, 1.05f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(banner_, 160, 22, 1.05f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("CLEAN", 160, 20, 1.15f, PAL_GOOD);
    } else if (toastT_ > 0.f && toast_[0]) {
        text(toast_, 160, 22, 0.7f, PAL_GOOD);
    }

    const Ship& ship = art_.ship[wingFrame()];
    float gsx, gsy;
    project(x_, h_, gsx, gsy);
    float dest = float(ship.img.h) / ship.ppm * scale;
    sprAnchor(ship.img, ship.ax, ship.ay, gsx, gsy, std::max(12.f, dest), PAL_SHIP);

    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        float sx, sy;
        project(p.x, p.y, sx, sy);
        spr(art_.dust, sx, sy, 8.f + (1.f - p.life) * 12.f, PAL_DUST, false, int((1.f - p.life) * 10));
    }

    for (int i = 0; i < 3; i++) {
        int pal = PAL_AMBER;
        if (i < next_) pal = PAL_GOOD;
        else if (i == next_ && mode_ != Mode::Title) pal = ((sys_->frame / 12) & 1) ? PAL_HUD : PAL_AMBER;
        float x0s, y0s, x1s, y1s;
        project(kTurnX[i] - kHalf, 0.f, x0s, y0s);
        project(kTurnX[i] + kHalf, 0.f, x1s, y1s);
        float bandH = std::max(4.f, 0.55f * scale);
        sprBox(art_.stripe, (x0s + x1s) * 0.5f, y0s - bandH, std::max(6.f, x1s - x0s), bandH, pal);

        float px, py, hx, hy;
        project(kTurnX[i], 0.f, px, py);
        project(kTurnX[i], kBandHi, hx, hy);
        float poleH = py - hy;
        if (poleH > 8.f) spr(art_.pole, px, hy + poleH * 0.5f, poleH, PAL_PYLON, false, 0);
        float armH = std::clamp(0.7f * scale, 6.f, 16.f);
        float lx, ly;
        project(kTurnX[i] + 1.2f, kBandLo, lx, ly);
        spr(art_.arm, lx, ly, armH, PAL_PYLON, false, 0);
        project(kTurnX[i] + 1.2f, kBandHi, lx, ly);
        spr(art_.arm, lx, ly, armH, PAL_PYLON, false, 0);
        float flagH = std::clamp(1.5f * scale, 10.f, 28.f);
        spr(art_.flag, px + flagH * 0.2f, hy - flagH * 0.15f, flagH, PAL_PYLON, kTurnDir[i] < 0, 0);
        text(kNum[i], px, hy - flagH - 8.f, 0.48f, i < next_ ? PAL_GOOD : PAL_HUD);

        if (i == next_ && arc_ > 0.02f) {
            int lit = std::clamp(int(arc_ * 5.f + 0.001f), 0, 5);
            for (int k = 0; k < 5; k++) {
                float cx = kTurnX[i] - 8.f + float(k) * 3.2f;
                float qx, qy;
                project(cx, kCruise + 2.4f, qx, qy);
                spr(art_.chev, qx, qy, std::max(6.f, 0.55f * scale), k < lit ? PAL_GOOD : PAL_CHEV, kTurnDir[i] < 0, 0);
            }
        }
    }

    float ex, ey, topx, topy, sillx, silly;
    project(kEnd, 0.f, ex, ey);
    project(kEnd, kGateHi + 2.6f, topx, topy);
    float postH = ey - topy;
    if (postH > 8.f) spr(art_.post, ex, topy + postH * 0.5f, postH, PAL_END, false, 0);
    float banH = std::clamp(1.6f * scale, 12.f, 36.f);
    spr(art_.banner, ex + banH * 0.35f, topy, banH, PAL_END, false, 0);
    project(kEnd + 1.4f, kGateLo, sillx, silly);
    spr(art_.arm, sillx, silly, std::clamp(0.7f * scale, 6.f, 16.f), PAL_END, false, 0);

    float c0, cy0, c1, cy1;
    project(kEnd - 1.5f, 0.f, c0, cy0);
    project(kEnd + 8.f, 0.f, c1, cy1);
    sprBox(art_.check, (c0 + c1) * 0.5f, cy0 - std::max(4.f, 0.45f * scale), std::max(8.f, c1 - c0),
           std::max(4.f, 0.7f * scale), PAL_END);

    int sock = int(anim_ * 6.f) % 3;
    if (sock < 0) sock = 0;
    float sx, sy;
    project(kEnd + 14.f, 0.f, sx, sy);
    float sockH = std::clamp(2.6f * scale, 14.f, 52.f);
    spr(art_.sock[sock], sx, sy - sockH * 0.42f, sockH, PAL_PYLON, false, 0);

    project(30.f, 0.f, sx, sy);
    float hut = std::clamp(3.1f * scale, 16.f, 60.f);
    spr(art_.bothy, sx, sy - hut * 0.42f, hut, PAL_HOUSE, false, 1);

    const float trees[] = {24.f, 48.f, 96.f, 118.f, 160.f, 184.f, 232.f, 248.f, 278.f};
    for (float tx : trees) {
        float px, py;
        project(tx, 0.f, px, py);
        float ht = std::clamp(4.4f * scale, 16.f, 84.f);
        int fog = tx > kEnd ? 5 : 0;
        spr(art_.pine, px, py - ht * 0.46f, ht, PAL_PINE, tx > 140.f, fog);
    }
    const float rocks[] = {58.f, 126.f, 198.f};
    for (float rx : rocks) {
        float px, py;
        project(rx, 0.f, px, py);
        float ht = std::clamp(1.8f * scale, 10.f, 36.f);
        spr(art_.rock, px - 10.f, py - ht * 0.3f, ht, PAL_ROCK, false, 1);
    }

    float left = camX_ - (ax + 20.f) / scale;
    float right = camX_ + (gs::SCREEN_W - ax + 20.f) / scale;
    float gStep = std::clamp(26.f / scale, 3.4f, 8.f);
    float g0 = std::floor(left / gStep) * gStep;
    for (float wx = g0; wx < right; wx += gStep) {
        float px, py;
        project(wx + gStep * 0.5f, 0.f, px, py);
        float sw = gStep * scale + 1.5f;
        int pal = (wx > kEnd - 6.f && wx < kEnd + 22.f) ? PAL_FIELD : PAL_GRASS;
        float tile = 24.f;
        int rows = 0;
        for (float y = py; y < gs::SCREEN_H + 2.f && rows < 5; y += tile - 1.f, rows++)
            sprBox(art_.grass, px, y, sw, tile, pal);
    }

    if (h_ < 18.f) {
        float px, py;
        project(x_, 0.f, px, py);
        float sh = std::clamp((2.2f + h_ * 0.08f) * scale * 0.35f, 3.f, 26.f);
        float skew = bank_ * scale * 1.4f;
        spr(art_.shade, px + skew, py, sh, PAL_DUST, false, int(std::min(12.f, h_ * 0.7f)));
    }

    for (int i = 0; i < 2; i++) {
        float bx = 40.f + float(i) * 48.f + std::sin(anim_ * 0.8f + float(i)) * 5.f;
        float by = 9.f + float(i) * 2.2f;
        float px, py;
        project(bx, by, px, py);
        int fr = int(anim_ * 3.f + float(i)) & 1;
        spr(art_.gull[fr], px, py, std::max(6.f, 0.65f * scale), PAL_SKY, i & 1, 2);
    }

    for (int i = 0; i < 3; i++) {
        float sxr = std::fmod(30.f + float(i) * 180.f - camX_ * scale * 0.16f, 700.f);
        if (sxr < -90.f) sxr += 700.f;
        spr(art_.ridge, sxr, 148.f, 26.f + float(i % 2) * 8.f, PAL_FAR, false, 8);
    }
    for (int i = 0; i < 4; i++) {
        float sxc = std::fmod(20.f + float(i) * 120.f - camX_ * scale * 0.05f + anim_ * 5.f, 520.f);
        if (sxc < -50.f) sxc += 520.f;
        spr(art_.cloud, sxc, 26.f + float(i % 3) * 14.f, 13.f + float(i % 2) * 4.f, PAL_SKY, i & 1, 1);
    }
    spr(art_.sun, 286.f, 34.f, 22.f, PAL_SKY, false, 0);

    if (mode_ == Mode::Title) {
        hudC(16, "LEFT AND RIGHT BANK THE WING", PAL_HUD);
        hudC(17, "UP CLIMBS     DOWN DIVES", PAL_HUD);
        hudC(18, "Z  C  SPACE   SPOILER", PAL_AMBER);
        hudC(20, "MAKE THE THREE TURNS", PAL_HUD);
        hudC(21, "WITHOUT TIPPING THE WING", PAL_GOOD);
        hudC(22, "THEN FLY THE END OF THE LEG", PAL_HUD);
        hudC(23, "MISSING THE END FAILS THE LEG", PAL_AMBER);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START FLIES    ESC TITLE", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(24, why_, PAL_BAD);
        hudC(26, "START TRIES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(23, "THREE TURNS, NO TIP", PAL_GOOD);
        char buf[40];
        std::snprintf(buf, sizeof buf, "%.1f S", legT_);
        hudC(25, buf, PAL_HUD);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "ALT %4.1f", h_);
        int ap = (next_ < 3 && h_ >= kBandLo && h_ <= kBandHi) ? PAL_GOOD : PAL_HUD;
        hud(1, 1, buf, ap);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(12, 1, buf, v_ < 10.f ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "BNK %+5.0f", bank_ * 100.f);
        int bp = std::fabs(bank_) > 0.86f ? PAL_BAD : PAL_HUD;
        hud(23, 1, buf, bp);
        if (spoil_ > 0.4f) hud(33, 1, "SPOILER", PAL_AMBER);

        char bar[20];
        const int n = 11;
        int pos = int(std::lround((bank_ + 1.f) * 0.5f * float(n - 1)));
        pos = std::clamp(pos, 0, n - 1);
        bar[0] = 'L';
        bar[1] = ' ';
        for (int i = 0; i < n; i++) {
            char c = '-';
            if (i == 0 || i == n - 1) c = '!';
            else if (i == n / 2) c = '+';
            if (i == pos) c = 'O';
            bar[2 + i] = c;
        }
        bar[2 + n] = ' ';
        bar[3 + n] = 'R';
        bar[4 + n] = 0;
        hud(12, 2, bar, bp);

        const char* line = "TO THE TURN";
        int pal = PAL_HUD;
        if (next_ >= 3) {
            float dist = std::max(0.f, kEnd - x_);
            if (dist < 28.f && (h_ < kGateLo || h_ > kGateHi)) {
                line = "GATE HEIGHT";
                pal = PAL_BAD;
            } else if (dist < 8.f && h_ >= kGateLo && h_ <= kGateHi) {
                line = "THE END";
                pal = PAL_GOOD;
            } else {
                std::snprintf(buf, sizeof buf, "END %3.0f M", dist);
                line = buf;
                pal = PAL_AMBER;
            }
        } else {
            float dx = kTurnX[next_] - x_;
            bool inX = std::fabs(x_ - kTurnX[next_]) <= kHalf;
            bool inH = h_ >= kBandLo && h_ <= kBandHi;
            float signedB = bank_ * float(kTurnDir[next_]);
            if (inX && inH && signedB > kCount) {
                line = "HOLD THE BANK";
                pal = PAL_GOOD;
            } else if (inX && inH) {
                line = kTurnDir[next_] < 0 ? "BANK LEFT" : "BANK RIGHT";
                pal = PAL_AMBER;
            } else if (inX) {
                line = "WRONG HEIGHT";
                pal = PAL_BAD;
            } else if (std::fabs(bank_) > 0.8f) {
                line = "EASE THE BANK";
                pal = PAL_BAD;
            } else {
                std::snprintf(buf, sizeof buf, "TURN %d   %3.0f M", next_ + 1, std::max(0.f, dx));
                line = buf;
            }
        }
        hudC(26, line, pal);
        if (next_ < 3 && arc_ > 0.02f) {
            int pips = std::clamp(int(arc_ * 5.f + 0.001f), 0, 5);
            std::snprintf(buf, sizeof buf, "ARC %d/5", pips);
            hudC(25, buf, PAL_GOOD);
        } else if (overB_ > 0.25f) {
            hudC(25, "TIP", PAL_BAD);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(8, 7, 8));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.2f, 0.1f);
    sys.apu.setPatch(0, hornPatch());
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ != Mode::Pause) anim_ += kDt;
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    if (toastT_ > 0.f) toastT_ = std::max(0.f, toastT_ - kDt);
    for (Puff& p : puffs_)
        if (p.life > 0.f) p.life = std::max(0.f, p.life - kDt);

    if (chime_ >= 0) {
        static const float notes[] = {494.f, 659.f, 784.f, 988.f};
        chimeT_ += kDt;
        if (chimeT_ > 0.13f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.2f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0.f;
            if (chime_ > 7) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        bank_ = -0.38f - 0.28f * (0.5f + 0.5f * std::sin(anim_ * 1.3f));
        h_ = 12.1f + std::sin(anim_ * 1.1f) * 0.35f;
        x_ = kTurnX[0] - 3.5f;
        v_ = 16.f;
        vy_ = 0.f;
        arc_ = 0.35f + 0.25f * (0.5f + 0.5f * std::sin(anim_ * 0.8f));
        next_ = 0;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            blip(700.f);
            startRun();
        } else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
        return;
    }

    if (mode_ == Mode::Pause) {
        draw();
        if (pad.pressed(gs::BTN_START)) {
            blip(560.f);
            mode_ = Mode::Fly;
        } else if (pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        audio();
        draw();
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    float nose = 0.f, spoil = 0.f, stick = 0.f;
    if (bot_) {
        pilot(nose, spoil, stick);
    } else {
        if (pad.down(gs::BTN_UP)) nose += 1.f;
        if (pad.down(gs::BTN_DOWN)) nose -= 1.f;
        if (std::fabs(pad.axisY) > 0.15f) nose = pad.axisY;
        nose = clampf(nose, -1.f, 1.f);
        if (pad.down(gs::BTN_LEFT)) stick -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) stick += 1.f;
        if (std::fabs(pad.axisX) > 0.15f) stick = pad.axisX;
        stick = clampf(stick, -1.f, 1.f);
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO) ||
            pad.down(gs::BTN_X) || pad.down(gs::BTN_Z))
            spoil = 1.f;
        if (pad.accel > 0.08f) spoil = std::max(spoil, pad.accel);
        if (pad.brake > 0.08f) spoil = std::max(spoil, pad.brake);
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(400.f);
            draw();
            return;
        }
    }

    physics(nose, spoil, stick);
    if (mode_ == Mode::Fly && spoil_ > 0.55f && (sys.frame % 5) == 0) {
        puffs_[puffN_ % 8] = {x_ - 2.4f, h_ - 0.3f, 0.4f};
        puffN_++;
    }
    audio();
    draw();
}

}  // namespace gliderturn
