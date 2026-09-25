#include "game/putt.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace putt {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float OX = 16.f;
constexpr float OY = 40.f;
constexpr float GW = 288.f;
constexpr float GH = 144.f;
constexpr float BALL_R = 4.f;
constexpr float CUP_R = 10.f;
constexpr float LIP = 112.f;
constexpr float MU = 82.f;
constexpr float MU_SAND = 250.f;
constexpr float STOP_V = 7.f;
constexpr float BOUNCE = 0.35f;
constexpr int NHOLES = 9;

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
};

struct Hole {
    const char* name = "";
    float sx = 0, sy = 0, cx = 0, cy = 0;
    float ax = 0, ay = 0;
    Rect wall[4]{};
    int nw = 0;
    Rect sand[3]{};
    int ns = 0;
    Rect water[3]{};
    int nwater = 0;
};

bool inside(float x, float y, const Rect* r, int n) {
    for (int i = 0; i < n; i++) {
        if (x >= r[i].x && y >= r[i].y && x < r[i].x + r[i].w && y < r[i].y + r[i].h) return true;
    }
    return false;
}

void bounce(Body& b, float nx, float ny) {
    float vn = b.vx * nx + b.vy * ny;
    if (vn < 0) {
        b.vx -= (1.f + BOUNCE) * vn * nx;
        b.vy -= (1.f + BOUNCE) * vn * ny;
        b.bumps++;
    }
}

void collide(Body& b, const Rect& r) {
    if (b.x >= r.x && b.x <= r.x + r.w && b.y >= r.y && b.y <= r.y + r.h) {
        float dl = b.x - r.x;
        float dr = r.x + r.w - b.x;
        float dt = b.y - r.y;
        float db = r.y + r.h - b.y;
        if (dl <= dr && dl <= dt && dl <= db) {
            b.x = r.x - BALL_R;
            bounce(b, -1, 0);
        } else if (dr <= dt && dr <= db) {
            b.x = r.x + r.w + BALL_R;
            bounce(b, 1, 0);
        } else if (dt <= db) {
            b.y = r.y - BALL_R;
            bounce(b, 0, -1);
        } else {
            b.y = r.y + r.h + BALL_R;
            bounce(b, 0, 1);
        }
        return;
    }
    float cx = std::clamp(b.x, r.x, r.x + r.w);
    float cy = std::clamp(b.y, r.y, r.y + r.h);
    float dx = b.x - cx;
    float dy = b.y - cy;
    float d2 = dx * dx + dy * dy;
    if (d2 >= BALL_R * BALL_R) return;
    float d = std::sqrt(d2);
    if (d < 1e-4f) return;
    float nx = dx / d;
    float ny = dy / d;
    b.x += nx * (BALL_R - d);
    b.y += ny * (BALL_R - d);
    bounce(b, nx, ny);
}

void cornerRects(Rect out[4]) {
    out[0] = {0, 0, 32, 16};
    out[1] = {GW - 32, 0, 32, 16};
    out[2] = {0, GH - 16, 32, 16};
    out[3] = {GW - 32, GH - 16, 32, 16};
}

void borderRects(Rect out[4]) {
    out[0] = {-40, -40, 40, GH + 80};
    out[1] = {GW, -40, 40, GH + 80};
    out[2] = {-40, -40, GW + 80, 40};
    out[3] = {-40, GH, GW + 80, 40};
}

void addWall(Hole& h, float x, float y, float w, float hh) {
    if (h.nw < 4) h.wall[h.nw++] = Rect{x, y, w, hh};
}
void addSand(Hole& h, float x, float y, float w, float hh) {
    if (h.ns < 3) h.sand[h.ns++] = Rect{x, y, w, hh};
}
void addWater(Hole& h, float x, float y, float w, float hh) {
    if (h.nwater < 3) h.water[h.nwater++] = Rect{x, y, w, hh};
}

Hole makeHole(const char* name, float sx, float sy, float cx, float cy, float ax, float ay) {
    Hole h;
    h.name = name;
    h.sx = sx;
    h.sy = sy;
    h.cx = cx;
    h.cy = cy;
    h.ax = ax;
    h.ay = ay;
    return h;
}

const Hole* course() {
    static Hole h[NHOLES];
    static int ready = 0;
    if (ready) return h;
    ready = 1;
    h[0] = makeHole("FIRST", 144, 112, 144, 36, 0, 0);
    addSand(h[0], 8, 48, 48, 48);
    addSand(h[0], 232, 48, 48, 48);

    h[1] = makeHole("LEAN", 104, 116, 160, 40, 34, 0);

    h[2] = makeHole("DRAW", 188, 116, 120, 40, -36, 0);

    h[3] = makeHole("GATE", 144, 118, 144, 34, 0, 0);
    addWall(h[3], 0, 64, 112, 16);
    addWall(h[3], 176, 64, 112, 16);

    h[4] = makeHole("ANGLE", 72, 118, 200, 40, 0, 0);
    addWall(h[4], 0, 64, 112, 16);
    addWall(h[4], 176, 64, 112, 16);

    h[5] = makeHole("SAND", 144, 116, 144, 36, 0, 0);
    addSand(h[5], 16, 48, 64, 48);
    addSand(h[5], 208, 48, 64, 48);

    h[6] = makeHole("MOAT", 144, 120, 144, 32, 0, 0);
    addWater(h[6], 0, 64, 112, 32);
    addWater(h[6], 176, 64, 112, 32);

    h[7] = makeHole("RISE", 144, 116, 144, 44, 0, 46);

    h[8] = makeHole("HOME", 92, 118, 180, 40, 28, 16);
    addSand(h[8], 0, 48, 32, 32);
    addSand(h[8], 256, 96, 32, 32);
    return h;
}

const Hole& holeAt(int i) {
    if (i < 0) i = 0;
    if (i >= NHOLES) i = NHOLES - 1;
    return course()[i];
}

std::string hintFor(const Hole& h) {
    if (h.nwater) return "WATER IS NOT THE CUP";
    if (std::fabs(h.ax) > 10.f || std::fabs(h.ay) > 10.f) {
        std::string s;
        if (h.ay > 10.f) s = "UPHILL";
        else if (h.ay < -10.f) s = "DOWNHILL";
        if (h.ax > 10.f) s += s.empty() ? "LEANS RIGHT" : "  LEANS RIGHT";
        else if (h.ax < -10.f) s += s.empty() ? "LEANS LEFT" : "  LEANS LEFT";
        return s;
    }
    if (h.nw) return "THROUGH THE GAP";
    if (h.ns) return "SAND IS NOT THE CUP";
    return "STRAIGHT";
}

float wrapf(float v, float m) {
    float r = std::fmod(v, m);
    if (r < 0) r += m;
    return r;
}

}  // namespace

const char* Game::holeName() const { return holeAt(hole_).name; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 5, 3));
    sys.apu.setMaster(0.7f);
    if (!sys.headless) {
        std::string saved = sys.loadBlob("s3putt-best");
        best_ = saved.empty() ? 0 : std::atoi(saved.c_str());
    }
    hole_ = 0;
    loadHole();
    if (bot_) startRound();
    else mode_ = Mode::Title;
}

void Game::startRound() {
    hole_ = 0;
    cups_ = 0;
    strokes_ = 0;
    won_ = false;
    over_ = false;
    bannerT_ = 0;
    sayT_ = 0;
    loadHole();
    mode_ = Mode::Play;
}

void Game::loadHole() {
    const Hole& h = holeAt(hole_);
    ball_ = Body{};
    ball_.x = h.sx;
    ball_.y = h.sy;
    ball_.rest = true;
    aim_ = std::atan2(h.cy - h.sy, h.cx - h.sx);
    swinging_ = false;
    meter_ = 0;
    meterDir_ = 1.f;
    rolled_ = false;
    rollFrames_ = 0;
    sayT_ = 0;
}

void Game::strike(float ang, float spd) {
    aim_ = ang;
    ball_.vx = std::cos(ang) * spd;
    ball_.vy = std::sin(ang) * spd;
    ball_.rest = false;
    ball_.sunk = false;
    ball_.wet = false;
    ball_.bumps = 0;
    rolled_ = true;
    rollFrames_ = 0;
    swinging_ = false;
    strokes_++;
    if (sys_) sys_->apu.noiseBurst(0.28f, 1700.f, 0.06f);
}

void Game::stepBody(Body& b, float dt) {
    if (b.sunk || b.rest) return;
    const Hole& h = holeAt(hole_);
    b.vx += h.ax * dt;
    b.vy += h.ay * dt;
    float mu = inside(b.x, b.y, h.sand, h.ns) ? MU_SAND : MU;
    float sp = std::hypot(b.vx, b.vy);
    float drop = mu * dt;
    if (sp <= drop) {
        b.vx = b.vy = 0;
    } else {
        float s = (sp - drop) / sp;
        b.vx *= s;
        b.vy *= s;
    }
    b.x += b.vx * dt;
    b.y += b.vy * dt;

    Rect borders[4], corners[4];
    borderRects(borders);
    cornerRects(corners);
    for (int pass = 0; pass < 3; pass++) {
        for (int i = 0; i < 4; i++) collide(b, borders[i]);
        for (int i = 0; i < 4; i++) collide(b, corners[i]);
        for (int i = 0; i < h.nw; i++) collide(b, h.wall[i]);
    }

    if (!std::isfinite(b.x) || !std::isfinite(b.y)) {
        b.x = h.sx;
        b.y = h.sy;
        b.vx = b.vy = 0;
        b.rest = true;
        return;
    }
    if (inside(b.x, b.y, h.water, h.nwater)) {
        b.wet = true;
        b.vx = b.vy = 0;
        b.rest = true;
        return;
    }
    sp = std::hypot(b.vx, b.vy);
    float d = std::hypot(b.x - h.cx, b.y - h.cy);
    if (d <= CUP_R && sp <= LIP) {
        b.sunk = true;
        b.rest = true;
        b.vx = b.vy = 0;
        b.x = h.cx;
        b.y = h.cy;
        return;
    }
    if (sp < STOP_V) {
        b.vx = b.vy = 0;
        b.rest = true;
    }
}

bool Game::findPutt(float x, float y, float& ang, float& spd) {
    const Hole& h = holeAt(hole_);
    float base = std::atan2(h.cy - y, h.cx - x);
    float cur = std::hypot(h.cx - x, h.cy - y);
    int bestN = -1;
    float bestA = base;
    float bestDelta = 1e9f;
    float bestVs[128];
    float missD = 1e9f, missA = base, missV = 90.f;

    auto scan = [&](float a0, float a1, float aStep, float v0, float v1, float vStep, bool allowBump) {
        for (float a = a0; a <= a1 + 1e-4f; a += aStep) {
            float vs[128];
            int n = 0;
            for (float v = v0; v <= v1 + 1e-4f; v += vStep) {
                Body b;
                b.x = x;
                b.y = y;
                b.rest = false;
                b.vx = std::cos(a) * v;
                b.vy = std::sin(a) * v;
                for (int i = 0; i < 220; i++) {
                    stepBody(b, DT);
                    if (b.sunk || b.wet || b.rest) break;
                }
                if (!b.sunk && !b.rest) {
                    b.vx = b.vy = 0;
                    b.rest = true;
                }
                float d = std::hypot(b.x - h.cx, b.y - h.cy);
                if (b.sunk && !b.wet && (allowBump || b.bumps == 0) && n < 128) vs[n++] = v;
                else if (!b.wet && d < missD) {
                    missD = d;
                    missA = a;
                    missV = v;
                }
            }
            float delta = std::fabs(a - base);
            if (n > bestN || (n == bestN && n > 0 && delta < bestDelta)) {
                bestN = n;
                bestA = a;
                bestDelta = delta;
                for (int i = 0; i < n; i++) bestVs[i] = vs[i];
            }
        }
    };

    scan(base - 0.7f, base + 0.7f, 0.025f, 34.f, 220.f, 2.f, false);
    if (bestN <= 0) scan(base - 1.5f, base + 1.5f, 0.04f, 40.f, 210.f, 4.f, true);
    if (bestN > 0) {
        ang = bestA;
        spd = bestVs[bestN / 2];
        return true;
    }
    if (missD < cur - 1.5f) {
        ang = missA;
        spd = missV;
        return false;
    }
    ang = base;
    spd = std::sqrt(std::max(36.f, 2.f * MU * std::max(8.f, cur - 4.f)));
    if (spd > 210.f) spd = 210.f;
    return false;
}

void Game::holeOut() {
    const Hole& h = holeAt(hole_);
    ball_.x = h.cx;
    ball_.y = h.cy;
    ball_.vx = ball_.vy = 0;
    ball_.rest = true;
    ball_.sunk = true;
    cups_++;
    say("CUP", PAL_GREEN, 0.9f);
    chime_ = 3;
    chimeT_ = 0;
    if (sys_ && !sys_->headless) sys_->rumble(0.25f, 0.45f, 90);
    if (cups_ >= NHOLES) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        if (!bot_ && sys_ && !sys_->headless && (best_ == 0 || strokes_ < best_)) {
            best_ = strokes_;
            sys_->saveBlob("s3putt-best", std::to_string(best_));
        }
    } else {
        mode_ = Mode::Banner;
        bannerT_ = 0;
    }
}

void Game::splash() {
    const Hole& h = holeAt(hole_);
    ball_ = Body{};
    ball_.x = h.sx;
    ball_.y = h.sy;
    ball_.rest = true;
    rolled_ = false;
    swinging_ = false;
    say("SPLASH", PAL_WHITE, 0.8f);
    if (sys_) sys_->apu.noiseBurst(0.34f, 240.f, 0.18f);
}

void Game::say(const char* s, int pal, float time) {
    say_ = s;
    sayPal_ = pal;
    sayT_ = time;
}

void Game::chime(float dt) {
    if (!sys_) return;
    if (chime_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0) {
            const float notes[3] = {523.25f, 659.25f, 783.99f};
            sys_->apu.tone(0, notes[3 - chime_], 0.1f);
            chimeT_ = 0.08f;
            chime_--;
            if (chime_ == 0) toneKill_ = 0.16f;
        }
    }
    if (toneKill_ > 0) {
        toneKill_ -= dt;
        if (toneKill_ <= 0) sys_->apu.tone(0, 0, 0);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    int iw = std::max(1, int(std::lround(w)));
    int ih = std::max(1, int(std::lround(h)));
    s.w = int16_t(std::min(iw, 2000));
    s.h = int16_t(std::min(ih, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::patch(const gs::Mipped& m, float x, float y, float w, float h, int pal) {
    const float ts = 16.f;
    if (w < 1.f || h < 1.f) return;
    for (float yy = 0; yy < h - 0.1f; yy += ts) {
        float rh = std::min(ts, h - yy);
        for (float xx = 0; xx < w - 0.1f; xx += ts) {
            float rw = std::min(ts, w - xx);
            spr(m, x + xx + rw * 0.5f, y + yy + rh * 0.5f, std::min(rw, rh), pal, false, false);
        }
    }
}

void Game::flagAt(float wx, float wy) {
    const gs::Mipped& f = art_.flag[(int(t_ * 8.f) & 1)];
    float fh = 32.f;
    float fw = fh * float(f.w) / float(std::max(1, int(f.h)));
    float pin = fw * (4.f / 18.f);
    gs::Sprite s;
    s.h = 32;
    s.w = int16_t(std::max(1, int(std::lround(fw))));
    s.x = int16_t(std::lround(OX + wx - pin));
    s.y = int16_t(std::lround(OY + wy - fh + 6));
    s.img = f.pick(fh);
    s.pal = PAL_CUP;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    v.A.enabled = false;
    v.B.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 24) {
            float u = y / 23.f;
            v.lineBackdrop[y] = gs::rgb4(3 + int(u * 8), 5 + int(u * 5), 12 - int(u * 5));
        } else if (y < 40) {
            v.lineBackdrop[y] = gs::rgb4(5, 3, 1);
        } else if (y < 184) {
            bool band = ((y / 4) & 1) == 0;
            v.lineBackdrop[y] = band ? gs::rgb4(2, 8, 3) : gs::rgb4(3, 11, 4);
        } else if (y < 200) {
            v.lineBackdrop[y] = gs::rgb4(6, 4, 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(1, 2, 3);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    const Hole& h = holeAt(hole_);
    const gs::Mipped& water = art_.water[(int(t_ * 6.f) & 1)];

    if (mode_ == Mode::Title) spr(art_.logo, 160, 12, float(art_.logo.h), PAL_LOGO, false, false);
    if (mode_ == Mode::Win) spr(art_.win, 160, 78, float(art_.win.h), PAL_LOGO, false, false);

    bool showAim = ball_.rest && !ball_.sunk && (mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Title);
    if (showAim) {
        float dx = std::cos(aim_);
        float dy = std::sin(aim_);
        int n = swinging_ ? 3 + int(meter_ * 5.f) : 5;
        for (int i = 1; i <= n; i++) {
            float d = 14.f + i * 7.f;
            spr(art_.dot, OX + ball_.x + dx * d, OY + ball_.y + dy * d, 4.5f, PAL_AIM, false, false);
        }
    }

    float ballH = ball_.sunk ? 6.f : 10.f;
    spr(art_.ball, OX + ball_.x, OY + ball_.y, ballH, PAL_BALL, false, false);
    flagAt(h.cx, h.cy);
    spr(art_.cup, OX + h.cx, OY + h.cy, 16, PAL_CUP, false, false);
    spr(art_.shadow, OX + ball_.x + 2.f, OY + ball_.y + 3.f, 7, PAL_BALL, false, true);

    const float tuftX[6] = {48, 100, 150, 196, 240, 78};
    const float tuftY[6] = {50, 90, 70, 104, 58, 86};
    Rect corners[4];
    cornerRects(corners);
    for (int i = 0; i < 6; i++) {
        float x = tuftX[i] + (h.ax * 0.12f) * t_ + std::sin(t_ * 1.6f + i) * 1.4f;
        float y = tuftY[i] + (h.ay * 0.08f) * t_;
        x = wrapf(x, GW);
        y = wrapf(y, GH);
        if (inside(x, y, h.sand, h.ns) || inside(x, y, h.water, h.nwater) || inside(x, y, h.wall, h.nw)) continue;
        if (inside(x, y, corners, 4)) continue;
        if (std::hypot(x - h.cx, y - h.cy) < 18.f) continue;
        if (std::hypot(x - ball_.x, y - ball_.y) < 12.f) continue;
        spr(art_.tuft, OX + x, OY + y, 10, PAL_HEDGE, false, false);
    }

    for (int i = 0; i < h.ns; i++) patch(art_.sand, OX + h.sand[i].x, OY + h.sand[i].y, h.sand[i].w, h.sand[i].h, PAL_SAND);
    for (int i = 0; i < h.nwater; i++)
        patch(water, OX + h.water[i].x, OY + h.water[i].y, h.water[i].w, h.water[i].h, PAL_WATER);
    for (int i = 0; i < h.nw; i++)
        patch(art_.hedge, OX + h.wall[i].x, OY + h.wall[i].y, h.wall[i].w, h.wall[i].h, PAL_HEDGE);
    for (int i = 0; i < 4; i++) patch(art_.hedge, OX + corners[i].x, OY + corners[i].y, corners[i].w, corners[i].h, PAL_HEDGE);

    patch(art_.wood, 0, 24, 320, 16, PAL_WOOD);
    patch(art_.wood, 0, 184, 320, 16, PAL_WOOD);
    patch(art_.wood, 0, 40, 16, 144, PAL_WOOD);
    patch(art_.wood, 304, 40, 16, 144, PAL_WOOD);

    const float trees[][2] = {{36, 40}, {92, 41}, {168, 39}, {236, 42}, {292, 40}};
    for (int i = 0; i < 5; i++) {
        float th = 34.f;
        spr(art_.tree, trees[i][0], trees[i][1] - th * 0.5f, th, PAL_TREE, i & 1, false);
    }
    spr(art_.sun, 300, 12, 16, PAL_SUN, false, false);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(25, "NINE SHORT HOLES", PAL_GOLD);
        hudC(26, "ONLY THE CUP COUNTS", PAL_WHITE);
        if (best_ > 0) {
            std::snprintf(buf, sizeof buf, "BEST %d", best_);
            hud(1, 25, buf, PAL_GREEN);
        }
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_GOLD);
        else hudC(27, "ARROWS AIM   HOLD Z PUTT", PAL_WHITE);
    } else if (mode_ == Mode::Win) {
        hudC(12, "ONLY THE CUP COUNTS", PAL_WHITE);
        std::snprintf(buf, sizeof buf, "STROKES %d", strokes_);
        hudC(14, buf, PAL_GOLD);
        if (best_ > 0) {
            std::snprintf(buf, sizeof buf, "BEST %d", best_);
            hudC(16, buf, PAL_GREEN);
        }
        hudC(26, "NINE CUPS", PAL_GREEN);
        if (!bot_) hudC(27, "START AGAIN", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "HOLE %d  %s", hole_ + 1, h.name);
        hud(1, 0, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "STROKES %d", strokes_);
        hud(1, 1, buf, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "CUPS %d/9", cups_);
        hud(28, 1, buf, PAL_GREEN);
        hudC(25, hintFor(h), h.nwater ? PAL_RED : PAL_GOLD);
        if (mode_ == Mode::Pause) {
            hudC(12, "PAUSED", PAL_GOLD);
            hudC(27, "START RESUME   ESC TITLE", PAL_WHITE);
        } else if (sayT_ > 0) {
            hudC(12, say_, sayPal_);
        }
        if (mode_ == Mode::Play || mode_ == Mode::Banner) {
            if (swinging_) {
                int n = int(std::lround(meter_ * 16.f));
                int pct = int(std::lround(meter_ * 100.f));
                std::string bar = "POWER ";
                for (int i = 0; i < 16; i++) bar += (i < n) ? '=' : '.';
                std::snprintf(buf, sizeof buf, " %d", pct);
                bar += buf;
                hud(1, 26, bar, PAL_GOLD);
            } else if (ball_.rest && !ball_.sunk) {
                hud(1, 26, "HOLD Z TO PUTT", PAL_WHITE);
            } else if (!ball_.sunk) {
                hud(1, 26, "ROLLING", PAL_GOLD);
            }
            if (sayT_ <= 0 && mode_ == Mode::Play) hud(1, 27, "ARROWS AIM    Z PUTT", PAL_WHITE);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (sayT_ > 0) sayT_ = std::max(0.f, sayT_ - DT);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        ball_.x = holeAt(0).sx;
        ball_.y = holeAt(0).sy;
        ball_.rest = true;
        ball_.sunk = false;
        aim_ = std::atan2(holeAt(0).cy - ball_.y, holeAt(0).cx - ball_.x);
        if (pad.pressed(gs::BTN_START)) startRound();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            hole_ = 0;
            loadHole();
        }
    } else if (mode_ == Mode::Banner) {
        bannerT_ += DT;
        if ((!bot_ && pad.pressed(gs::BTN_START)) || bannerT_ > (bot_ ? 0.28f : 0.85f)) {
            hole_++;
            if (hole_ >= NHOLES) {
                mode_ = Mode::Win;
                won_ = true;
                over_ = true;
            } else {
                loadHole();
                mode_ = Mode::Play;
            }
        }
    } else if (mode_ == Mode::Win) {
        if (!bot_ && pad.pressed(gs::BTN_START)) startRound();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            hole_ = 0;
            loadHole();
            won_ = false;
            over_ = false;
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
            mode_ = Mode::Pause;
        } else if (ball_.sunk) {
            holeOut();
        } else if (ball_.rest) {
            if (bot_) {
                float a = aim_, v = 80.f;
                findPutt(ball_.x, ball_.y, a, v);
                strike(a, v);
            } else {
                float rate = 0.9f;
                if (std::fabs(pad.axisX) > 0.15f) aim_ += pad.axisX * rate * 1.35f * DT;
                else {
                    if (pad.down(gs::BTN_LEFT)) aim_ -= rate * DT;
                    if (pad.down(gs::BTN_RIGHT)) aim_ += rate * DT;
                }
                const float PI = 3.14159265f;
                if (aim_ > PI) aim_ -= PI * 2.f;
                if (aim_ < -PI) aim_ += PI * 2.f;
                bool swing = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO);
                if (swing && !swinging_) {
                    swinging_ = true;
                    meter_ = 0;
                    meterDir_ = 1.f;
                }
                if (swinging_ && swing) {
                    meter_ += meterDir_ * DT / 0.95f;
                    if (meter_ >= 1.f) {
                        meter_ = 1.f;
                        meterDir_ = -1.f;
                    }
                    if (meter_ <= 0.f) {
                        meter_ = 0.f;
                        meterDir_ = 1.f;
                    }
                }
                if (swinging_ && !swing) strike(aim_, 34.f + meter_ * 190.f);
            }
        } else {
            int bumps = ball_.bumps;
            rollFrames_++;
            stepBody(ball_, DT);
            if (ball_.bumps > bumps && !ball_.sunk) sys.apu.noiseBurst(0.1f, 480.f, 0.04f);
            if (ball_.sunk) holeOut();
            else if (ball_.wet) splash();
            else if (ball_.rest || rollFrames_ >= 220) {
                ball_.vx = ball_.vy = 0;
                ball_.rest = true;
                if (rolled_) {
                    float d = std::hypot(ball_.x - holeAt(hole_).cx, ball_.y - holeAt(hole_).cy);
                    if (!bot_ && d > CUP_R && d < 30.f) say("NO CUP", PAL_RED, 0.8f);
                    rolled_ = false;
                }
            }
        }
    }

    chime(DT);
    draw();
}

}  // namespace putt
