#include "game/kilo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace gkilo {
namespace {

constexpr double DT = 1.0 / 60.0;
constexpr double kFinish = 1000.0;
constexpr double kLook = 40.0;
constexpr double kCeiling = 17.6;
constexpr int kWheelN = 6;

struct Mark {
    double x, h;
};

struct WheelDef {
    double x, y, r;
    int kind;  // 0 mill, 1 cart, 2 hanging
    float phase;
};

// Flats sit on the safe side of each wheel. Transitions are the empty air between them.
constexpr Mark kLane[] = {
    {0, 9.0},    {50, 8.5},   {120, 8.2},  {150, 7.75}, {240, 7.75}, {310, 11.4}, {370, 15.35},
    {440, 15.35}, {510, 11.0}, {575, 7.80}, {680, 7.80}, {750, 11.5}, {820, 15.40}, {900, 15.40},
    {1100, 15.40},
};
constexpr int kLaneN = int(sizeof kLane / sizeof kLane[0]);

constexpr WheelDef kWheels[kWheelN] = {
    {90, 1.40, 1.30, 1, 0.2f},   {190, 17.00, 4.20, 2, 1.1f}, {400, 6.15, 4.85, 0, 0.4f},
    {610, 17.05, 4.25, 2, 2.0f}, {660, 1.38, 1.28, 1, 0.8f},  {855, 6.20, 4.90, 0, 1.6f},
};

// Body samples, meters from the CG. +x nose, +y up. The low point is the wheel.
constexpr double kPts[][2] = {
    {3.15, 0.05}, {1.6, 0.35}, {0.2, 0.25}, {-1.4, 0.12}, {-3.2, 0.15},
    {-3.15, 1.90}, {0.55, 0.80}, {0.45, -0.55}, {0.45, -1.18},
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
    if (kind == 0) return "a mill wheel";
    if (kind == 1) return "a cart wheel";
    return "a hanging wheel";
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.18f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.16f, 0.62f, 0.2f};
    p.op[1] = {2.f, 0.32f, 0.02f, 0.2f, 0.4f, 0.16f};
    p.op[2] = {3.f, 0.1f, 0.02f, 0.22f, 0.22f, 0.2f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f};
    p.vol = 0.2f;
    p.tone = 1700.f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (x_ >= 820.0) return 3;
    if (x_ >= 150.0) return 2;
    return 1;
}

int Game::nextWheel() const {
    int best = -1;
    double bestX = 1e9;
    for (int i = 0; i < kWheelN; i++) {
        if (kWheels[i].x < x_ - 4.0) continue;
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
    h_ = 9;
    v_ = 16.8;
    vy_ = 0;
    att_ = -0.04;
    camX_ = 169;
    camH_ = 12.2;
    camS_ = 5.2;
    shake_ = 0;
}

void Game::startRun() {
    x_ = 0;
    h_ = 9;
    v_ = 16.8;
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
    mode_ = Mode::Fly;
    camX_ = x_ + 8;
    camH_ = 8.5;
    camS_ = 6.1;
    blip(640.f);
}

void Game::pilot(double& nose) const {
    double target = laneAt(x_ + kLook);
    double want = clampd((target - h_) * 0.95, -3.6, 3.6);
    nose = clampd((want - vy_) / 4.8, -1.0, 1.0);
}

void Game::win() {
    if (mode_ != Mode::Fly) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "wheels untouched";
    chime_ = 0;
    chimeT_ = 0;
    x_ = std::max(x_, kFinish);
    sys_->rumble(0.18f, 0.08f, 140);
    sys_->setLight(40, 180, 70);
    std::printf("S3 GLIDER KILO  CLEAR  finished the kilometer  1000 m  wheels untouched  (%.1f s)\n", time_);
    std::fflush(stdout);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Fly) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    std::snprintf(whyBuf_, sizeof whyBuf_, "%s", why);
    why_ = whyBuf_;
    shake_ = 0.45f;
    int meters = int(std::clamp(x_, 0.0, kFinish));
    sys_->rumble(0.55f, 0.28f, 180);
    sys_->setLight(180, 30, 20);
    sys_->apu.noiseBurst(0.5f, 280.f, 0.32f);
    std::printf("S3 GLIDER KILO  FAIL  %s at %d m\n", why, meters);
    std::fflush(stdout);
}

void Game::physics(double nose) {
    nose = clampd(nose, -1.0, 1.0);
    time_ += DT;
    double path = nose * 0.30;
    double targetVy = path * std::max(v_, 8.0);
    vy_ += (targetVy - vy_) * std::min(1.0, 3.2 * DT);
    double trim = 16.8 - std::max(vy_, 0.0) * 0.55 + std::max(-vy_, 0.0) * 0.20;
    v_ += (trim - v_) * (0.45 * DT);
    v_ = clampd(v_, 0.0, 24.0);
    x_ += v_ * DT;
    h_ += vy_ * DT;
    if (h_ > 17.2) vy_ -= (h_ - 17.2) * 14.0 * DT;
    if (h_ > kCeiling) {
        h_ = kCeiling;
        if (vy_ > 0) vy_ = 0;
    }
    if (!std::isfinite(x_) || !std::isfinite(h_) || !std::isfinite(v_)) {
        fail("lost the air");
        return;
    }
    att_ += (std::atan2(vy_, std::max(v_, 8.0)) - att_) * 0.28;

    const double ca = std::cos(att_), sa = std::sin(att_);
    double sx[kPtN], sy[kPtN];
    bool ground = false;
    for (int i = 0; i < kPtN; i++) {
        sx[i] = x_ + kPts[i][0] * ca - kPts[i][1] * sa;
        sy[i] = h_ + kPts[i][0] * sa + kPts[i][1] * ca;
        if (sy[i] <= 0.04) ground = true;
    }
    for (int w = 0; w < kWheelN; w++) {
        const WheelDef& wheel = kWheels[w];
        if (std::fabs(x_ - wheel.x) > wheel.r + 6.0) continue;
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
    if (ground) {
        h_ = std::max(h_, 1.15);
        vy_ = 0;
        fail("wheels touched");
        return;
    }
    if (x_ >= kFinish) {
        win();
        return;
    }
    if (time_ > 100.0) fail("did not finish the kilometer");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.055f);
    beep_ = 0.07f;
}

void Game::sky() {
    uint16_t zen = gs::rgb4(4, 7, 13);
    uint16_t mid = gs::rgb4(8, 12, 15);
    uint16_t hor = gs::rgb4(13, 14, 12);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 6, 5), 0.4f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(11, 15, 10), 0.28f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.62f ? lerpC(zen, mid, t / 0.62f) : lerpC(mid, hor, (t - 0.62f) / 0.38f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(9, 11, 12));
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
    float left = sx - ax * sc;
    float top = sy - ay * sc;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(destH)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(left)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(top)), -8000L, 8000L));
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
        shipX = 158.0 + std::sin(t_ * 0.7) * 0.6;
        shipH = 8.35 + std::sin(t_ * 1.3) * 0.22;
        shipAtt = -0.05 + std::sin(t_ * 0.8) * 0.04;
    }

    if (title) {
        text("GLIDER KILO", 160, 18, 1.05f, PAL_HUD);
        text("FINISH THE KILOMETER", 160, 40, 0.62f, PAL_AMBER);
        text("DO NOT TOUCH A WHEEL", 160, 58, 0.62f, PAL_GOOD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 28, 1.1f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text("TOUCHED", 160, 26, 1.15f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("CLEAR", 160, 22, 1.2f, PAL_GOOD);
        text("WHEELS UNTOUCHED", 160, 46, 0.62f, PAL_HUD);
    }

    double wantS = title ? 5.2 : 6.1;
    double lead = title ? 0.0 : (mode_ == Mode::Win ? 2.0 : 12.0);
    double wantX = title ? 169.0 : shipX + lead;
    double wantH = title ? 12.2 : clampd(shipH * 0.55 + 4.2, 8.0, 13.2);
    if (title) {
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
    } else {
        camX_ += (wantX - camX_) * 0.14;
        camH_ += (wantH - camH_) * 0.14;
        camS_ += (wantS - camS_) * 0.14;
    }
    if (shake_ > 0) {
        camX_ += std::sin(t_ * 47.0) * double(shake_) * 1.4;
        camH_ += std::cos(t_ * 39.0) * double(shake_) * 0.6;
    }

    const float scale = float(camS_);
    const float ax = 148.f;
    const float ay = 116.f;
    auto project = [&](double wx, double wy, float& sx, float& sy) {
        sx = ax + float(wx - camX_) * scale;
        sy = ay - float(wy - camH_) * scale;
    };

    auto drawShipAt = [&](double wx, double wy, double att) {
        int fi = int(std::lround((0.40 - att) / 0.20));
        fi = std::clamp(fi, 0, 4);
        const Ship& ship = art_.ship[fi];
        float sx, sy;
        project(wx, wy, sx, sy);
        float dest = float(ship.img.h) / ship.ppm * scale;
        sprAnchor(ship.img, ship.ax, ship.ay, sx, sy, std::max(8.f, dest), PAL_SHIP);
    };

    drawShipAt(shipX, shipH, shipAtt);

    for (const Puff& p : puffs_) {
        if (p.life <= 0) continue;
        float sx, sy;
        project(p.x, p.y, sx, sy);
        spr(art_.cloud, sx, sy, 7.f + float(1.0 - p.life) * 8.f, PAL_SKY, false, int((1.0 - p.life) * 8));
    }

    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& w = kWheels[i];
        float sx, sy;
        project(w.x, w.y, sx, sy);
        int fr = int(t_ * 7.0 + w.phase * 3.0) & 3;
        float dest = float(2.0 * w.r / double(art_.wheelRim)) * scale;
        int pal = w.kind == 0 ? PAL_MILL : w.kind == 1 ? PAL_CART : PAL_HANG;
        if (w.kind == 0) {
            float axleX, axleY;
            project(w.x, w.y, axleX, axleY);
            float towerH = float(w.y) * scale * (float(art_.tower.h) / art_.towerSpan);
            sprAnchor(art_.tower, art_.towerAx, art_.towerAy, axleX, axleY, std::max(12.f, towerH), PAL_MILL, 0);
        } else if (w.kind == 1) {
            float bx, by;
            project(w.x - 0.2, w.y + 0.15, bx, by);
            spr(art_.bed, bx, by, std::max(8.f, float(w.r) * scale * 0.85f), PAL_CART, false, 0);
        } else {
            float bx, by, tx, ty;
            project(w.x, w.y + w.r + 0.35, bx, by);
            project(w.x, 19.2, tx, ty);
            spr(art_.beam, bx, by, std::max(6.f, 0.55f * scale), PAL_HANG, false, 1);
            float cableTop = std::min(by, ty);
            float cableBot = sy - dest * 0.5f + 4.f;
            if (cableBot > cableTop) sprBox(art_.cable, sx, cableTop, 3.f, cableBot - cableTop, PAL_HANG, 1);
        }
        spr(art_.wheel[fr], sx, sy, std::max(10.f, dest), pal, false, 0);
    }

    if (!title && shipH < 16.0) {
        float sx, sy;
        project(shipX, 0.05, sx, sy);
        float sh = std::clamp(1.6f * scale + float(shipH) * 0.35f, 4.f, 28.f);
        int fog = int(std::min(14.0, shipH * 0.7));
        spr(art_.shade, sx, sy, sh, PAL_SHADE, false, fog);
    }

    const double posts[] = {200, 400, 600, 800};
    for (double px : posts) {
        float sx, sy;
        project(px, 0, sx, sy);
        float ht = 2.4f * scale;
        spr(art_.pole, sx, sy - ht * 0.5f, ht, PAL_POST, false, 1);
    }
    {
        float sx, sy;
        project(1000.0, 0, sx, sy);
        float poleH = 10.5f * scale;
        spr(art_.pole, sx - 10.f, sy - poleH * 0.45f, poleH, PAL_BANNER, false, 0);
        spr(art_.pole, sx + 46.f, sy - poleH * 0.45f, poleH, PAL_BANNER, false, 0);
        float bx, by;
        project(1006.0, 8.6, bx, by);
        spr(art_.banner, bx, by, std::max(14.f, 3.4f * scale), PAL_BANNER, false, 0);
    }

    float sockX, sockY;
    project(18.0, 0, sockX, sockY);
    int sock = int(t_ * 4.0) & 1;
    float sockH = std::max(12.f, 3.1f * scale);
    spr(art_.sock[sock], sockX, sockY - sockH * 0.42f, sockH, PAL_POST, false, 0);

    const double trees[] = {36, 124, 248, 336, 472, 548, 724, 792, 948, 1024};
    for (double tx : trees) {
        bool crowded = false;
        for (int i = 0; i < kWheelN; i++)
            if (std::fabs(tx - kWheels[i].x) < kWheels[i].r + 3.0) crowded = true;
        if (crowded) continue;
        float sx, sy;
        project(tx, 0, sx, sy);
        float ht = (2.6f + float(int(tx) % 3) * 0.35f) * scale;
        spr(art_.tree, sx, sy - ht * 0.46f, ht, PAL_TREE, tx > 500, 2);
    }

    const double birds[] = {260, 530, 770};
    int bframe = int(t_ * 6.0) & 1;
    for (int i = 0; i < 3; i++) {
        float sx, sy;
        double by = 5.2 + std::sin(t_ * 1.7 + birds[i]) * 0.35;
        project(birds[i] + std::sin(t_ * 0.4 + i) * 2.0, by, sx, sy);
        spr(art_.bird[bframe], sx, sy, 8.f + float(i), PAL_HUD, i & 1, 3);
    }

    float left = float(camX_) - (ax + 30.f) / scale;
    float right = float(camX_) + (gs::SCREEN_W - ax + 40.f) / scale;
    float gY;
    {
        float dummy;
        project(camX_, 0, dummy, gY);
    }
    float step = 3.4f;
    float start = std::floor(left / step) * step;
    float tile = std::max(8.f, step * scale + 1.f);
    for (float wx = start; wx < right; wx += step) {
        float sx = ax + (wx + step * 0.5f - float(camX_)) * scale;
        int rows = 0;
        for (float y = gY; y < gs::SCREEN_H + 2.f && rows < 7; y += tile - 1.f, rows++)
            sprBox(art_.grass, sx, y, tile + 1.f, tile, PAL_GRASS, rows > 2 ? 3 : 0);
    }

    for (int i = 0; i < 5; i++) {
        float sx = std::fmod(20.f + float(i) * 150.f - float(camX_) * scale * 0.22f, 780.f);
        if (sx < -140.f) sx += 780.f;
        spr(art_.hill, sx, gY - 8.f, 28.f + float(i % 2) * 8.f, PAL_HILL, false, 8);
    }
    float deckLeft = std::floor((left - 4.f) / 16.f) * 16.f;
    for (float wx = deckLeft; wx < right + 8.f; wx += 16.f) {
        float sx, sy;
        project(wx, 19.4, sx, sy);
        spr(art_.deck, sx, sy + std::sin(t_ * 0.3f + wx * 0.05f) * 1.5f, 22.f + float(int(wx) & 7), PAL_SKY, false, 1);
    }
    for (int i = 0; i < 4; i++) {
        float sx = std::fmod(30.f + float(i) * 130.f - float(camX_) * scale * 0.06f + float(t_) * 6.f, 560.f);
        if (sx < -50.f) sx += 560.f;
        spr(art_.cloud, sx, 26.f + float(i % 3) * 12.f, 16.f + float(i % 2) * 4.f, PAL_SKY, i & 1, 2);
    }
    spr(art_.sun, 28.f, 22.f, 20.f, PAL_SKY, false, 0);

    if (title) {
        hudC(20, "UP CLIMBS     DOWN DIVES", PAL_HUD);
        hudC(21, "YOUR WHEEL COUNTS TOO", PAL_AMBER);
        hudC(23, "OVER A MILL    UNDER A HANGING WHEEL", PAL_HUD);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START FLIES    ESC TITLE", PAL_AMBER);
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
        std::snprintf(buf, sizeof buf, "ALT %4.1f", std::max(0.0, h_));
        hud(14, 0, buf, h_ < 3.0 ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(26, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "VS %+4.1f", vy_);
        hud(26, 1, buf, vy_ > 0.4 ? PAL_GOOD : vy_ < -0.4 ? PAL_AMBER : PAL_HUD);

        int nw = nextWheel();
        const char* line = "KEEP THE WHEEL UP";
        int pal = PAL_GOOD;
        if (nw >= 0) {
            const WheelDef& w = kWheels[nw];
            int dist = int(std::max(0.0, w.x - x_));
            if (w.kind == 0) std::snprintf(buf, sizeof buf, "OVER THE MILL   %d M", dist);
            else if (w.kind == 1) std::snprintf(buf, sizeof buf, "OVER THE CART   %d M", dist);
            else std::snprintf(buf, sizeof buf, "UNDER THE WHEEL   %d M", dist);
            line = buf;
            pal = dist < 28 ? PAL_AMBER : PAL_HUD;
        } else if (x_ > 860.0) {
            line = "THE LINE IS AHEAD";
            pal = PAL_GOOD;
        }
        if (h_ < 3.2) {
            line = "WHEEL TOO LOW";
            pal = PAL_BAD;
        }
        hudC(2, line, pal);
        hud(1, 26, "UP DOWN FLY", PAL_HUD);
        hud(28, 26, "START PAUSE", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(9, 11, 12));
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.14f, 0.22f, 0.1f);
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
        if (p.life > 0) p.life = std::max(0.0, p.life - DT * 0.8);

    if (chime_ >= 0) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        chimeT_ += float(DT);
        if (chimeT_ > 0.14f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.2f);
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
            blip(520.f);
            mode_ = Mode::Fly;
        } else if (pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        draw();
        sys.apu.noise(0.f, 800.f, false);
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
        if (std::fabs(pad.axisY) > 0.18f) nose = pad.axisY;
        nose = clampd(nose, -1.0, 1.0);
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(400.f);
            draw();
            return;
        }
    }

    int nw = nextWheel();
    physics(nose);
    if (mode_ == Mode::Fly && nw != cue_) {
        cue_ = nw;
        if (nw >= 0) blip(kWheels[nw].kind == 2 ? 392.f : 698.f);
    }

    if (mode_ == Mode::Fly) {
        float wind = float(std::clamp(v_ / 22.0, 0.15, 1.0)) * 0.045f;
        sys.apu.noise(wind, 700.f + float(v_) * 28.f, false);
        if (h_ < 3.4) sys.apu.tone(2, 160.f, 0.04f);
        else sys.apu.tone(2, 0.f, 0.f);
        bool near = false;
        for (int i = 0; i < kWheelN; i++)
            if (std::fabs(kWheels[i].x - x_) < kWheels[i].r + 8.0) near = true;
        if (h_ < 3.2 || (near && std::fabs(h_ - laneAt(x_)) > 2.4)) sys.setLight(170, 90, 30);
        else if (near) sys.setLight(40, 90, 170);
        else sys.setLight(50, 80, 150);
        if ((sys.frame % 18) == 0 && std::fabs(vy_) > 2.2) {
            puffs_[puffN_ % 6] = {x_ - 3.2, h_ - 0.2, 0.8};
            puffN_++;
        }
    }
    draw();
}

}  // namespace gkilo
