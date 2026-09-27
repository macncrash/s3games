#include "game/kilo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace fkilo {
namespace {

constexpr double DT = 1.0 / 60.0;
constexpr double kFinish = 1000.0;
constexpr double kLook = 42.0;
constexpr double kCeiling = 16.8;
constexpr int kWheelN = 6;

struct Mark {
    double x, h;
};

struct WheelDef {
    double x, y, r;
    int kind;  // 0 paddle, 1 lorry, 2 gantry sheave
    float phase;
};

constexpr Mark kLane[] = {
    {0, 7.1},     {90, 8.3},    {160, 8.5},   {230, 6.3},   {340, 6.1},   {430, 8.8},
    {520, 9.2},   {600, 12.6},  {680, 13.4},  {760, 9.4},   {840, 6.8},   {930, 7.6},
    {1100, 8.0},
};
constexpr int kLaneN = int(sizeof kLane / sizeof kLane[0]);

constexpr WheelDef kWheels[kWheelN] = {
    {125, 2.15, 1.85, 0, 0.3f}, {305, 15.15, 3.15, 2, 1.2f}, {470, 1.55, 1.25, 1, 0.6f},
    {640, 5.35, 3.85, 0, 1.7f}, {810, 15.55, 2.95, 2, 0.4f}, {915, 1.45, 1.15, 1, 2.1f},
};

// Samples in meters from the CG. +x is the bow, +y is up. The low point is the ramp wheel.
constexpr double kPts[][2] = {
    {4.4, 0.15}, {2.2, 0.45}, {0.1, 0.35}, {-2.4, 0.22}, {-4.5, 0.18},
    {-3.4, 2.35}, {0.6, 1.85}, {1.6, -0.35}, {1.7, -1.05},
};
constexpr int kPtN = int(sizeof kPts / sizeof kPts[0]);

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

double laneAt(double x) {
    if (x <= kLane[0].x) return kLane[0].h;
    for (int i = 1; i < kLaneN; i++) {
        if (x <= kLane[i].x) {
            double den = kLane[i].x - kLane[i - 1].x;
            double u = den > 1e-6 ? (x - kLane[i - 1].x) / den : 0.0;
            double s = u * u * (3.0 - 2.0 * u);
            return kLane[i - 1].h + (kLane[i].h - kLane[i - 1].h) * s;
        }
    }
    return kLane[kLaneN - 1].h;
}

const char* wheelWord(int kind) {
    if (kind == 0) return "a paddle wheel";
    if (kind == 1) return "a lorry wheel";
    return "a gantry wheel";
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.12f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.18f, 0.55f, 0.22f};
    p.op[1] = {2.f, 0.28f, 0.02f, 0.22f, 0.35f, 0.18f};
    p.op[2] = {3.01f, 0.08f, 0.02f, 0.24f, 0.2f, 0.2f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f};
    p.vol = 0.22f;
    p.tone = 1400.f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (x_ >= 780.0) return 3;
    if (x_ >= 160.0) return 2;
    return 1;
}

int Game::nextWheel() const {
    int best = -1;
    double bestX = 1e9;
    for (int i = 0; i < kWheelN; i++) {
        if (kWheels[i].x < x_ - 6.0) continue;
        if (kWheels[i].x < bestX) {
            bestX = kWheels[i].x;
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
    cue_ = -2;
    chime_ = -1;
    time_ = 0;
    x_ = 0;
    h_ = 7.1;
    v_ = 15.5;
    vy_ = 0;
    att_ = 0.02;
    camX_ = 168;
    camH_ = 9.4;
    camS_ = 5.4;
    shake_ = 0;
}

void Game::startRun() {
    x_ = 0;
    h_ = 7.1;
    v_ = 16.4;
    vy_ = 0;
    att_ = 0;
    time_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    cue_ = -2;
    chime_ = -1;
    puffN_ = 0;
    shake_ = 0;
    for (Puff& p : puffs_) p = {};
    mode_ = Mode::Run;
    camX_ = x_ + 10;
    camH_ = 8.2;
    camS_ = 6.0;
    blip(520.f);
}

void Game::pilot(double& nose) const {
    double target = laneAt(x_ + kLook);
    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& w = kWheels[i];
        double ahead = w.x - x_;
        if (ahead < -4.0 || ahead > 55.0) continue;
        if (w.kind == 2) target = std::min(target, w.y - w.r - 3.35);
        else target = std::max(target, w.y + w.r + 2.55);
    }
    target = clampd(target, 5.2, 14.6);
    double want = clampd((target - h_) * 0.92, -3.4, 3.4);
    nose = clampd((want - vy_) / 4.6, -1.0, 1.0);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "wheels untouched";
    chime_ = 0;
    chimeT_ = 0;
    x_ = std::max(x_, kFinish);
    sys_->rumble(0.16f, 0.06f, 150);
    sys_->setLight(30, 160, 90);
    std::printf("S3 FERRY KILO  CLEAR  finished the kilometer  1000 m  wheels untouched  (%.1f s)\n", time_);
    std::fflush(stdout);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    std::snprintf(whyBuf_, sizeof whyBuf_, "%s", why);
    why_ = whyBuf_;
    shake_ = 0.5f;
    int meters = int(std::clamp(x_, 0.0, kFinish));
    sys_->rumble(0.5f, 0.25f, 180);
    sys_->setLight(170, 40, 20);
    sys_->apu.noiseBurst(0.48f, 220.f, 0.34f);
    std::printf("S3 FERRY KILO  FAIL  %s at %d m\n", why, meters);
    std::fflush(stdout);
}

void Game::physics(double nose) {
    nose = clampd(nose, -1.0, 1.0);
    time_ += DT;
    double path = nose * 0.28;
    double targetVy = path * std::max(v_, 8.0);
    vy_ += (targetVy - vy_) * std::min(1.0, 3.0 * DT);
    double trim = 16.6 - std::max(vy_, 0.0) * 0.40 + std::max(-vy_, 0.0) * 0.12;
    v_ += (trim - v_) * (0.5 * DT);
    v_ = clampd(v_, 0.0, 22.0);
    x_ += v_ * DT;
    h_ += vy_ * DT;
    if (h_ > 16.2) vy_ -= (h_ - 16.2) * 12.0 * DT;
    if (h_ > kCeiling) {
        h_ = kCeiling;
        if (vy_ > 0) vy_ = 0;
    }
    if (!std::isfinite(x_) || !std::isfinite(h_) || !std::isfinite(v_)) {
        fail("lost the channel");
        return;
    }
    att_ += (std::atan2(vy_, std::max(v_, 8.0)) * 0.65 - att_) * 0.22;

    const double ca = std::cos(att_), sa = std::sin(att_);
    double sx[kPtN], sy[kPtN];
    bool bed = false;
    for (int i = 0; i < kPtN; i++) {
        sx[i] = x_ + kPts[i][0] * ca - kPts[i][1] * sa;
        sy[i] = h_ + kPts[i][0] * sa + kPts[i][1] * ca;
        if (sy[i] <= 0.08) bed = true;
    }
    for (int w = 0; w < kWheelN; w++) {
        const WheelDef& wheel = kWheels[w];
        if (std::fabs(x_ - wheel.x) > wheel.r + 8.0) continue;
        for (int i = 0; i < kPtN; i++) {
            double dx = sx[i] - wheel.x, dy = sy[i] - wheel.y;
            if (dx * dx + dy * dy < wheel.r * wheel.r) {
                char buf[48];
                std::snprintf(buf, sizeof buf, "touched %s", wheelWord(wheel.kind));
                fail(buf);
                return;
            }
        }
    }
    if (bed) {
        h_ = std::max(h_, 1.3);
        vy_ = 0;
        fail("wheels touched");
        return;
    }
    if (x_ >= kFinish) {
        win();
        return;
    }
    if (time_ > 95.0) fail("did not finish the kilometer");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    beep_ = 0.07f;
}

void Game::sky() {
    uint16_t zen = gs::rgb4(3, 6, 12);
    uint16_t mid = gs::rgb4(6, 10, 14);
    uint16_t hor = gs::rgb4(12, 13, 11);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 5, 4), 0.45f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(8, 14, 10), 0.35f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.55f ? lerpC(zen, mid, t / 0.55f) : lerpC(mid, hor, (t - 0.55f) / 0.45f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(7, 10, 12));
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

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip, int fog) {
    if (ht < 1.2f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h * 0.5f)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -40) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::sprAnchor(const gs::Mipped& m, float ax, float ay, float sx, float sy, float destH, int pal, int fog) {
    if (destH < 1.5f || m.h < 1) return;
    float sc = destH / float(m.h);
    float w = float(m.w) * sc;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(destH)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(sx - ax * sc)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(sy - ay * sc)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -40) return;
    s.img = m.pick(destH);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal, int fog) {
    if (w < 1.2f || h < 1.2f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(top)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 4 || s.x + s.w < -4 || s.y > gs::SCREEN_H + 4 || s.y + s.h < -4) return;
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !s[0]) return;
    const float adv = 18.f * scale;
    int n = int(std::strlen(s));
    x -= float(n) * adv * 0.5f;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();

    const bool title = mode_ == Mode::Title;
    double shipX = x_;
    double shipH = h_;
    double shipAtt = att_;
    if (title) {
        shipX = 150.0 + std::sin(t_ * 0.55) * 1.2;
        shipH = 7.2 + std::sin(t_ * 1.1) * 0.18;
        shipAtt = 0.03 + std::sin(t_ * 0.7) * 0.03;
    }

    if (title) {
        text("FERRY KILO", 160, 18, 1.05f, PAL_HUD);
        text("FINISH THE KILOMETER", 160, 40, 0.58f, PAL_AMBER);
        text("DO NOT TOUCH A WHEEL", 160, 56, 0.58f, PAL_GOOD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 28, 1.1f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text("TOUCHED", 160, 24, 1.15f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("CLEAR", 160, 20, 1.2f, PAL_GOOD);
        text("WHEELS UNTOUCHED", 160, 44, 0.58f, PAL_HUD);
    }

    double wantS = title ? 5.4 : 6.0;
    double lead = title ? 0.0 : (mode_ == Mode::Win ? 4.0 : 14.0);
    double wantX = title ? 168.0 : shipX + lead;
    double wantH = title ? 9.4 : clampd(shipH * 0.45 + 5.0, 7.4, 12.4);
    if (title) {
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
    } else {
        camX_ += (wantX - camX_) * 0.12;
        camH_ += (wantH - camH_) * 0.12;
        camS_ += (wantS - camS_) * 0.12;
    }
    if (shake_ > 0) {
        camX_ += std::sin(t_ * 41.0) * double(shake_) * 1.6;
        camH_ += std::cos(t_ * 33.0) * double(shake_) * 0.5;
    }

    const float scale = float(camS_);
    const float ax = 132.f;
    const float ay = 112.f;
    auto project = [&](double wx, double wy, float& sx, float& sy) {
        sx = ax + float(wx - camX_) * scale;
        sy = ay - float(wy - camH_) * scale;
    };

    auto drawHullAt = [&](double wx, double wy, double att) {
        int fi = int(std::lround((0.28 - att) / 0.14));
        fi = std::clamp(fi, 0, 4);
        const Hull& hull = art_.hull[fi];
        float sx, sy;
        project(wx, wy, sx, sy);
        float dest = float(hull.img.h) / hull.ppm * scale;
        sprAnchor(hull.img, hull.ax, hull.ay, sx, sy, std::max(10.f, dest), PAL_SHIP);
    };
    drawHullAt(shipX, shipH, shipAtt);

    for (const Puff& p : puffs_) {
        if (p.life <= 0) continue;
        float sx, sy;
        project(p.x, p.y, sx, sy);
        spr(art_.cloud, sx, sy, 8.f + float(1.0 - p.life) * 10.f, PAL_SKY, false, int((1.0 - p.life) * 8));
    }

    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& w = kWheels[i];
        float sx, sy;
        project(w.x, w.y, sx, sy);
        int fr = int(t_ * 6.0 + w.phase * 4.0) & 3;
        float dest = float(2.0 * w.r / double(art_.wheelRim)) * scale;
        int pal = w.kind == 0 ? PAL_PADDLE : w.kind == 1 ? PAL_LORRY : PAL_GANTRY;
        if (w.kind == 0) {
            float mx, my;
            project(w.x, w.y, mx, my);
            float millH = float(w.y + w.r) * scale * (float(art_.mill.h) / art_.millSpan);
            sprAnchor(art_.mill, art_.millAx, art_.millAy, mx, my - dest * 0.15f, std::max(14.f, millH * 0.55f), PAL_PADDLE);
        } else if (w.kind == 1) {
            float bx, by;
            project(w.x, w.y + w.r * 0.15, bx, by);
            spr(art_.bed, bx, by - dest * 0.35f, std::max(8.f, float(w.r) * scale * 1.1f), PAL_LORRY);
        } else {
            float bx, by, tx, ty;
            project(w.x, w.y + w.r + 0.4, bx, by);
            project(w.x, 18.4, tx, ty);
            spr(art_.beam, bx, by, std::max(6.f, 0.7f * scale), PAL_GANTRY, false, 1);
            float cableTop = std::min(by, ty);
            float cableBot = sy - dest * 0.42f;
            if (cableBot > cableTop) sprBox(art_.cable, sx, cableTop, 3.f, cableBot - cableTop, PAL_GANTRY, 1);
        }
        spr(art_.wheel[fr], sx, sy, std::max(10.f, dest), pal);
    }

    {
        float sx, sy;
        project(1000.0, 0.2, sx, sy);
        float poleH = 9.5f * scale;
        spr(art_.pole, sx - 8.f, sy - poleH * 0.42f, poleH, PAL_BANNER);
        spr(art_.pole, sx + 42.f, sy - poleH * 0.42f, poleH, PAL_BANNER);
        float bx, by;
        project(1008.0, 7.4, bx, by);
        spr(art_.banner, bx, by, std::max(12.f, 2.8f * scale), PAL_BANNER);
    }

    float flagX, flagY;
    project(12.0, 0.4, flagX, flagY);
    float flagH = std::max(14.f, 3.4f * scale);
    spr(art_.flag[int(t_ * 3.0) & 1], flagX, flagY - flagH * 0.4f, flagH, PAL_POST);

    const double reeds[] = {40, 210, 360, 540, 700, 880, 990};
    for (double rx : reeds) {
        bool crowded = false;
        for (int i = 0; i < kWheelN; i++)
            if (std::fabs(rx - kWheels[i].x) < kWheels[i].r + 2.5) crowded = true;
        if (crowded) continue;
        float sx, sy;
        project(rx, laneAt(rx) - 1.3, sx, sy);
        float ht = (1.8f + float(int(rx) % 3) * 0.25f) * scale;
        spr(art_.reed, sx, sy - ht * 0.4f, ht, PAL_PIER, rx > 500, 2);
    }

    int gframe = int(t_ * 5.0) & 1;
    const double gulls[] = {220, 510, 760};
    for (int i = 0; i < 3; i++) {
        float sx, sy;
        double gy = laneAt(gulls[i]) + 3.4 + std::sin(t_ * 1.4 + gulls[i]) * 0.3;
        project(gulls[i] + std::sin(t_ * 0.35 + i) * 3.0, gy, sx, sy);
        spr(art_.gull[gframe], sx, sy, 9.f, PAL_HUD, i & 1, 2);
    }

    float left = float(camX_) - (ax + 40.f) / scale;
    float right = float(camX_) + (gs::SCREEN_W - ax + 40.f) / scale;
    float step = 4.2f;
    float start = std::floor(left / step) * step;
    float tile = std::max(10.f, step * scale + 1.f);
    for (float wx = start; wx < right; wx += step) {
        float sx, sy;
        double surf = laneAt(wx) - 1.25;
        project(wx + step * 0.5, surf, sx, sy);
        int rows = 0;
        for (float y = sy; y < gs::SCREEN_H + 2.f && rows < 6; y += tile - 1.f, rows++)
            sprBox(art_.water, sx, y, tile + 1.f, tile, PAL_WATER, rows > 2 ? 4 : 0);
    }
    for (int i = 0; i < 4; i++) {
        float sx = std::fmod(10.f + float(i) * 160.f - float(camX_) * scale * 0.18f, 700.f);
        if (sx < -120.f) sx += 700.f;
        float dummy, gy;
        project(camX_, 0, dummy, gy);
        spr(art_.bank, sx, gy - 6.f, 26.f + float(i % 2) * 6.f, PAL_BANK, false, 9);
    }
    float rail0 = std::floor((left - 2.f) / 18.f) * 18.f;
    for (float wx = rail0; wx < right + 6.f; wx += 18.f) {
        float sx, sy;
        project(wx, 18.2, sx, sy);
        spr(art_.rail, sx, sy, 16.f, PAL_GANTRY, false, 2);
    }
    for (int i = 0; i < 3; i++) {
        float sx = std::fmod(40.f + float(i) * 150.f - float(camX_) * scale * 0.05f + float(t_) * 4.f, 520.f);
        if (sx < -40.f) sx += 520.f;
        spr(art_.cloud, sx, 22.f + float(i) * 14.f, 18.f, PAL_SKY, i & 1, 2);
    }
    spr(art_.sun, 36.f, 18.f, 18.f, PAL_SKY);

    if (title) {
        hudC(20, "UP RISES     DOWN SETTLES", PAL_HUD);
        hudC(21, "THE RAMP WHEEL COUNTS TOO", PAL_AMBER);
        hudC(23, "OVER A PADDLE    UNDER A GANTRY", PAL_HUD);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START SAILS    ESC TITLE", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(24, why_, PAL_BAD);
        hudC(26, "START TRIES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        char buf[40];
        std::snprintf(buf, sizeof buf, "1000 M   %.1f S", time_);
        hudC(24, buf, PAL_GOOD);
        hudC(26, "THE KILOMETER IS FINISHED", PAL_HUD);
    } else {
        char buf[64];
        int meters = int(std::clamp(std::floor(x_), 0.0, kFinish));
        std::snprintf(buf, sizeof buf, "%d/1000 M", meters);
        hud(1, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "HULL %4.1f", std::max(0.0, h_));
        hud(13, 0, buf, h_ < 3.2 ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(26, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "RISE %+4.1f", vy_);
        hud(24, 1, buf, vy_ > 0.35 ? PAL_GOOD : vy_ < -0.35 ? PAL_AMBER : PAL_HUD);

        int nw = nextWheel();
        const char* line = "KEEP THE RAMP CLEAR";
        int pal = PAL_GOOD;
        if (nw >= 0) {
            const WheelDef& w = kWheels[nw];
            int dist = int(std::max(0.0, w.x - x_));
            if (w.kind == 0) std::snprintf(buf, sizeof buf, "OVER THE PADDLE   %d M", dist);
            else if (w.kind == 1) std::snprintf(buf, sizeof buf, "OVER THE LORRY   %d M", dist);
            else std::snprintf(buf, sizeof buf, "UNDER THE GANTRY   %d M", dist);
            line = buf;
            pal = dist < 30 ? PAL_AMBER : PAL_HUD;
        } else if (x_ > 860.0) {
            line = "THE LINE IS AHEAD";
            pal = PAL_GOOD;
        }
        if (h_ < 3.4) {
            line = "RAMP TOO LOW";
            pal = PAL_BAD;
        }
        hudC(2, line, pal);
        hud(1, 26, "UP DOWN TRIM", PAL_HUD);
        hud(27, 26, "START PAUSE", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(7, 10, 12));
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.16f, 0.2f, 0.08f);
    sys.apu.setPatch(0, bellPatch());
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    t_ += DT;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - float(DT));
    if (beep_ > 0.f) {
        beep_ -= float(DT);
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    for (Puff& p : puffs_)
        if (p.life > 0) p.life = std::max(0.0, p.life - DT * 0.7);

    if (chime_ >= 0) {
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        chimeT_ += float(DT);
        if (chimeT_ > 0.16f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.18f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 8) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
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
        if (pad.pressed(gs::BTN_START)) {
            blip(440.f);
            mode_ = Mode::Run;
        } else if (pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        draw();
        sys.apu.noise(0.f, 600.f, false);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    double nose = 0;
    if (bot_) pilot(nose);
    else {
        if (pad.down(gs::BTN_UP)) nose += 1;
        if (pad.down(gs::BTN_DOWN)) nose -= 1;
        if (std::fabs(pad.axisY) > 0.2f) nose = pad.axisY;
        nose = clampd(nose, -1.0, 1.0);
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(330.f);
            draw();
            return;
        }
    }

    int nw = nextWheel();
    physics(nose);
    if (mode_ == Mode::Run && nw != cue_) {
        cue_ = nw;
        if (nw >= 0) blip(kWheels[nw].kind == 2 ? 349.f : 622.f);
    }

    if (mode_ == Mode::Run) {
        float wash = float(std::clamp(v_ / 20.0, 0.12, 1.0)) * 0.04f;
        sys.apu.noise(wash, 480.f + float(v_) * 18.f, false);
        if (h_ < 3.6) sys.apu.tone(2, 140.f, 0.045f);
        else sys.apu.tone(2, 0.f, 0.f);
        bool near = false;
        for (int i = 0; i < kWheelN; i++)
            if (std::fabs(kWheels[i].x - x_) < kWheels[i].r + 10.0) near = true;
        if (h_ < 3.5) sys.setLight(160, 70, 30);
        else if (near) sys.setLight(30, 110, 150);
        else sys.setLight(20, 70, 120);
        if ((sys.frame % 16) == 0) {
            puffs_[puffN_ % 6] = {x_ - 4.6, h_ + 2.1, 0.9};
            puffN_++;
        }
    }
    draw();
}

}  // namespace fkilo
