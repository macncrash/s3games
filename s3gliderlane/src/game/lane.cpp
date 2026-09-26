#include "game/lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace glane {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kEnd = 204.f;
constexpr float kCrew0 = 18.f;
constexpr float kTrim = 16.4f;
constexpr float kGate = 22.f;
constexpr float kScale = 11.5f;
constexpr float kAx = 146.f;
constexpr float kAy = 102.f;
constexpr float kLead = 4.f;
constexpr float kNoseK = 4.5f;
constexpr float kBias = 0.22f;
constexpr float kSpoilVy = 2.7f;
constexpr float kStallV = 11.f;
constexpr float kStallK = 0.62f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float sqr(float v) { return v * v; }

float liftOf(float v) { return clampf((v - 7.2f) / 12.5f, 0.24f, 1.f); }

float gustOf(float x, float t) { return 0.5f * std::sin(x * 0.21f) * std::sin(t * 0.77f + 0.6f); }

float laneMid(float x) {
    float u = clampf(x, 0.f, kEnd);
    return 10.6f + 2.05f * std::sin(u * 0.030f + 0.35f) + 0.82f * std::sin(u * 0.071f + 1.9f);
}

float laneHalf(float x) {
    float t = clampf(x / kEnd, 0.f, 1.f);
    float a = std::exp(-sqr((t - 0.34f) / 0.075f));
    float b = std::exp(-sqr((t - 0.66f) / 0.065f));
    return 5.75f - 1.15f * a - 1.30f * b;
}

bool inLane(float x, float h) {
    float mid = laneMid(x);
    float half = laneHalf(x);
    return h >= mid - half && h <= mid + half;
}

bool tightAt(float x) { return laneHalf(x) < 5.02f; }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch chimePatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.12f;
    p.op[0] = {1.f, 0.9f, 0.01f, 0.22f, 0.55f, 0.25f};
    p.op[1] = {2.f, 0.22f, 0.02f, 0.28f, 0.3f, 0.22f};
    p.op[2] = {3.5f, 0.08f, 0.02f, 0.3f, 0.2f, 0.24f};
    p.op[3] = {0.5f, 0.18f, 0.02f, 0.4f, 0.4f, 0.3f};
    p.vol = 0.2f;
    p.tone = 1800.f;
    p.echo = 0.18f;
    return p;
}

}  // namespace

float Game::margin() const {
    float x = clampf(x_, 0.f, kEnd);
    return laneHalf(x) - std::fabs(h_ - laneMid(x));
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (x_ > kEnd - 16.f) return 3;
    if (x_ > 58.f) return 2;
    return 1;
}

int Game::frameFor(float att) const {
    int fi = int(std::lround((0.30f - att) / 0.15f));
    return std::clamp(fi, 0, 4);
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    legT_ = 0.f;
    crew_ = kCrew0;
    x_ = 46.f;
    h_ = laneMid(x_);
    v_ = kTrim;
    vy_ = 0.f;
    att_ = 0.04f;
    crewX_ = 28.f;
    shake_ = 0.f;
}

void Game::startRun() {
    x_ = 6.f;
    h_ = laneMid(x_);
    v_ = kTrim;
    float slope = (laneMid(x_ + 3.f) - laneMid(x_ - 3.f)) / 6.f;
    vy_ = slope * v_ + gustOf(x_, 0.f);
    att_ = clampf(slope * 1.4f, -0.2f, 0.2f);
    nose_ = 0.f;
    spoil_ = 0.f;
    legT_ = 0.f;
    crew_ = kCrew0;
    crewX_ = 0.f;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    spoilArm_ = false;
    near_ = false;
    tight_ = false;
    gate_ = false;
    crewSec_ = 99;
    puffN_ = 0;
    shake_ = 0.f;
    for (Puff& p : puffs_) p = {};
    mode_ = Mode::Fly;
    blip(680.f);
}

void Game::pilot(float& nose, float& spoil) const {
    float lift = std::max(liftOf(v_), 0.24f);
    float stall = std::max(0.f, kStallV - v_) * kStallK;
    float gust = gustOf(x_, legT_);
    float lead = clampf(v_ * 0.28f + 1.5f, 4.f, 8.f);
    float err = laneMid(x_ + lead) - h_;
    float slope = (laneMid(x_ + 3.f) - laneMid(x_ - 3.f)) / 6.f;
    float wantVy = slope * std::max(v_, 8.f) + clampf(err * 0.9f, -2.6f, 2.6f);
    float here = laneMid(x_) - h_;
    float edge = laneHalf(x_) - std::fabs(here);
    if (edge < 2.f) {
        float push = (2.f - edge) * 1.7f;
        if (here >= 0.f) wantVy = std::max(wantVy, push);
        else wantVy = std::min(wantVy, -push);
    }
    float maxUp = (kNoseK - kBias) * lift - stall;
    float maxDn = (-kNoseK - kBias) * lift - stall - kSpoilVy;
    if (maxUp < maxDn + 0.4f) maxUp = maxDn + 0.4f;
    wantVy = clampf(wantVy, maxDn + 0.1f, maxUp - 0.05f);
    float vyCmd = wantVy - gust;

    spoil = 0.f;
    if (v_ > kTrim + 1.1f && wantVy < 1.2f) spoil = clampf((v_ - kTrim - 0.4f) / 5.f, 0.f, 0.4f);
    auto noseFor = [&](float sp) {
        float n = (vyCmd + sp * kSpoilVy + stall + kBias * lift) / (kNoseK * lift);
        return clampf(n, -1.f, 1.f);
    };
    nose = noseFor(spoil);
    float pred = (nose * kNoseK - kBias) * lift - spoil * kSpoilVy - stall;
    if (pred > vyCmd + 0.25f) {
        spoil = clampf(spoil + (pred - vyCmd) / kSpoilVy, 0.f, 1.f);
        nose = noseFor(spoil);
    } else if (pred < vyCmd - 0.3f && spoil > 0.f) {
        spoil = clampf(spoil - (vyCmd - pred) / kSpoilVy, 0.f, 1.f);
        nose = noseFor(spoil);
    }
}

void Game::win() {
    if (mode_ != Mode::Fly) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "in the lane";
    banner_ = "IN THE LANE";
    x_ = kEnd;
    vy_ = 0.f;
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.28f, 0.1f, 160);
    sys_->setLight(40, 180, 90);
}

void Game::fail(const char* why, const char* banner) {
    if (mode_ != Mode::Fly) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    banner_ = banner;
    shake_ = 1.f;
    sys_->rumble(0.55f, 0.28f, 180);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.4f, 380.f, 0.28f);
}

void Game::physics(float nose, float spoil) {
    if (mode_ != Mode::Fly) return;
    nose_ = clampf(nose, -1.f, 1.f);
    spoil_ = clampf(spoil, 0.f, 1.f);
    float gust = gustOf(x_, legT_);
    float lift = liftOf(v_);
    float stall = std::max(0.f, kStallV - v_) * kStallK;
    float vyCmd = (nose_ * kNoseK - kBias) * lift - spoil_ * kSpoilVy - stall;
    vy_ += (vyCmd + gust - vy_) * std::min(1.f, 3.4f * kDt);
    v_ += ((kTrim - v_) * 0.16f - spoil_ * 2.8f - 0.10f * std::max(vy_, 0.f)) * kDt;
    v_ = clampf(v_, 0.f, 26.f);
    x_ += v_ * kDt;
    h_ += vy_ * kDt;
    legT_ += kDt;
    crew_ = std::max(0.f, kCrew0 - legT_);
    crewX_ = kEnd * (1.f - crew_ / kCrew0);

    if (!std::isfinite(x_) || !std::isfinite(h_) || !std::isfinite(v_)) {
        fail("lost the air", "LOST");
        return;
    }
    float at = std::min(x_, kEnd);
    if (!inLane(at, h_)) {
        fail("left the lane", "LEFT THE LANE");
        return;
    }
    if (!gate_ && x_ >= kGate) {
        gate_ = true;
        blip(740.f);
    }
    if (x_ >= kEnd) {
        x_ = kEnd;
        win();
        return;
    }
    if (legT_ >= kCrew0) {
        fail("the other crew", "OTHER CREW");
        return;
    }

    bool tight = tightAt(x_);
    if (tight && !tight_) {
        tight_ = true;
        blip(520.f);
        sys_->rumble(0.12f, 0.04f, 40);
    } else if (!tight && tight_) {
        tight_ = false;
    }
    float edge = laneHalf(x_) - std::fabs(h_ - laneMid(x_));
    if (!near_ && edge < 1.15f) {
        near_ = true;
        blip(170.f);
    } else if (near_ && edge > 1.8f) {
        near_ = false;
    }
    if (crew_ < 6.f) {
        int sec = int(std::floor(crew_));
        if (sec != crewSec_) {
            crewSec_ = sec;
            blip(crew_ < 3.f ? 210.f : 320.f);
        }
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    beep_ = 0.07f;
}

void Game::audio() {
    if (mode_ != Mode::Fly) {
        sys_->apu.noise(0.f, 800.f, false);
        sys_->apu.tone(2, 0.f, 0.f);
        if (mode_ == Mode::Win) sys_->setLight(40, 180, 90);
        else if (mode_ == Mode::Fail) sys_->setLight(180, 36, 24);
        return;
    }
    float wind = clampf((v_ - 8.f) / 16.f, 0.f, 1.f) * (spoil_ > 0.45f ? 0.07f : 0.03f);
    sys_->apu.noise(wind, 780.f + v_ * 28.f, false);
    if (std::fabs(vy_) > 0.6f) sys_->apu.tone(2, 240.f + vy_ * 36.f, 0.022f);
    else sys_->apu.tone(2, 0.f, 0.f);
    float edge = laneHalf(x_) - std::fabs(h_ - laneMid(x_));
    if (crew_ < 5.f) sys_->setLight(170, 48, 32);
    else if (edge < 1.4f) sys_->setLight(180, 120, 30);
    else if (tight_) sys_->setLight(70, 130, 180);
    else sys_->setLight(30, 140, 130);
}

void Game::sky() {
    uint16_t zen = gs::rgb4(3, 5, 11);
    uint16_t mid = gs::rgb4(7, 10, 14);
    uint16_t hor = gs::rgb4(14, 11, 8);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 5, 4), 0.45f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(12, 15, 9), 0.3f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.58f ? lerpC(zen, mid, t / 0.58f) : lerpC(mid, hor, (t - 0.58f) / 0.42f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(7, 8, 11));
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

void Game::sprAnchor(const gs::Mipped& m, float ax, float ay, float sx, float sy, float destH, int pal, int fog) {
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
    s.fog = uint8_t(std::clamp(fog, 0, 16));
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
    const gs::Mipped& sample = art_.glyph['A' - 32];
    const float adv = std::max(8.f, float(sample.w) * scale * 0.92f);
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

    float scale = kScale;
    float camX = x_ + kLead;
    float camH = laneMid(clampf(x_, 0.f, kEnd));
    float ax0 = kAx;
    float ay0 = kAy;
    if (mode_ == Mode::Title) {
        scale = 9.4f;
        camX = x_ + 1.5f;
        camH = laneMid(x_);
        ax0 = 156.f;
        ay0 = 118.f;
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        camX = kEnd - 2.f;
        camH = laneMid(kEnd);
    }
    if (shake_ > 0.f) {
        shx_ = std::sin(anim_ * 90.f) * shake_ * 4.f;
        shy_ = std::cos(anim_ * 70.f) * shake_ * 3.f;
        shake_ *= 0.9f;
        if (shake_ < 0.04f) shake_ = 0.f;
    } else {
        shx_ = shy_ = 0.f;
    }
    const float ax = ax0 + shx_;
    const float ay = ay0 + shy_;
    auto project = [&](float wx, float wy, float& sx, float& sy) {
        sx = ax + (wx - camX) * scale;
        sy = ay - (wy - camH) * scale;
    };

    if (mode_ == Mode::Title) {
        text("GLIDER LANE", 160, 18, 1.0f, PAL_HUD);
        text("STAY IN THE LANE", 160, 40, 0.55f, PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 18, 1.05f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(banner_, 160, 18, 0.95f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("IN THE LANE", 160, 18, 0.95f, PAL_GOOD);
    }

    auto drawShip = [&](float wx, float wy, float att, int pal, int fog, float mul) {
        const Wing& wing = art_.wing[frameFor(att)];
        float sx, sy;
        project(wx, wy, sx, sy);
        float dest = float(wing.img.h) / wing.ppm * scale * mul;
        sprAnchor(wing.img, wing.ax, wing.ay, sx, sy, std::max(8.f, dest), pal, fog);
    };
    drawShip(x_, h_, att_, PAL_SHIP, 0, 1.f);
    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        float sx, sy;
        project(p.x, p.y, sx, sy);
        spr(art_.dust, sx, sy, 6.f + (1.f - p.life / 0.4f) * 8.f, PAL_DUST, false, int((1.f - p.life) * 8.f));
    }
    if (crewX_ > -6.f && crewX_ < kEnd + 4.f) {
        float cs = (laneMid(crewX_ + 3.f) - laneMid(crewX_ - 3.f)) / 6.f;
        drawShip(crewX_, laneMid(crewX_), clampf(cs * 1.5f, -0.28f, 0.28f), PAL_CREW, 5, 0.92f);
    }

    float left = camX - (ax + 24.f) / scale;
    float right = camX + (gs::SCREEN_W - ax + 24.f) / scale;
    const float gates[2] = {kGate, kEnd};
    for (float gx : gates) {
        float yLo = laneMid(gx) - laneHalf(gx) - 0.35f;
        float yHi = laneMid(gx) + laneHalf(gx) + 1.15f;
        float xL = gx - 0.4f, xR = gx + 0.4f;
        for (float px : {xL, xR}) {
            float sx, sy0, sy1;
            project(px, yHi, sx, sy0);
            project(px, yLo, sx, sy1);
            sprBox(art_.post, sx, sy0, std::max(5.f, 0.42f * scale), std::max(8.f, sy1 - sy0), PAL_TIGHT);
        }
        float s0, s1, sy;
        project(xL, yHi - 0.15f, s0, sy);
        project(xR, yHi - 0.15f, s1, sy);
        float bar = std::max(6.f, 0.5f * scale);
        sprBox(art_.rail, (s0 + s1) * 0.5f, sy - bar * 0.5f, std::fabs(s1 - s0) + 2.f, bar, PAL_TIGHT);
        project(xL, yLo + 0.2f, s0, sy);
        project(xR, yLo + 0.2f, s1, sy);
        sprBox(art_.rail, (s0 + s1) * 0.5f, sy - bar * 0.5f, std::fabs(s1 - s0) + 2.f, bar, PAL_TIGHT);
        float signX, signY;
        project(gx, yHi + 0.35f, signX, signY);
        const gs::Mipped& sign = (gx > kEnd - 1.f) ? art_.signEnd : art_.signLane;
        spr(sign, signX, signY - 16.f, std::max(18.f, 2.3f * scale), PAL_SIGN, false, 0);
    }

    float step = 2.2f;
    float x0 = std::floor(left / step) * step;
    for (float wx = x0; wx < right; wx += step) {
        if (wx < -4.f || wx > kEnd + 8.f) continue;
        int pal = tightAt(wx) ? PAL_TIGHT : PAL_RAIL;
        float xa = wx, xb = wx + step;
        float ceilA = laneMid(xa) + laneHalf(xa) + 0.22f;
        float ceilB = laneMid(xb) + laneHalf(xb) + 0.22f;
        float floorA = laneMid(xa) - laneHalf(xa) - 0.22f;
        float floorB = laneMid(xb) - laneHalf(xb) - 0.22f;
        float s0, t0, s1, t1;
        project(xa, ceilA, s0, t0);
        project(xb, ceilB, s1, t1);
        float ht = std::max(5.f, 0.4f * scale);
        sprBox(art_.rail, (s0 + s1) * 0.5f, (t0 + t1) * 0.5f - ht * 0.5f, std::fabs(s1 - s0) + 1.5f, ht, pal);
        project(xa, floorA, s0, t0);
        project(xb, floorB, s1, t1);
        sprBox(art_.rail, (s0 + s1) * 0.5f, (t0 + t1) * 0.5f - ht * 0.5f, std::fabs(s1 - s0) + 1.5f, ht, pal);
    }

    for (float px = 8.f; px < kEnd; px += 16.f) {
        if (px < left - 2.f || px > right + 2.f) continue;
        if (std::fabs(px - kGate) < 8.f || std::fabs(px - kEnd) < 8.f) continue;
        float top = laneMid(px) + laneHalf(px);
        float bot = laneMid(px) - laneHalf(px);
        float sx, sy0, sy1;
        project(px, top + 1.25f, sx, sy0);
        project(px, top + 0.15f, sx, sy1);
        sprBox(art_.post, sx, sy0, std::max(4.f, 0.26f * scale), std::max(7.f, sy1 - sy0), PAL_RAIL);
        spr(art_.flag, sx + 6.f, sy0 + 2.f, 9.f, PAL_RAIL, false, 0);
        project(px, bot - 0.15f, sx, sy0);
        project(px, bot - 1.25f, sx, sy1);
        sprBox(art_.post, sx, sy0, std::max(4.f, 0.26f * scale), std::max(7.f, sy1 - sy0), PAL_RAIL);
        spr(art_.flag, sx + 6.f, sy1 - 2.f, 9.f, PAL_RAIL, false, 0);
    }

    for (float cx = 4.f; cx < kEnd; cx += 8.f) {
        if (cx < left - 1.f || cx > right + 1.f) continue;
        float sx, sy;
        project(cx, laneMid(cx), sx, sy);
        int pal = tightAt(cx) ? PAL_TIGHT : PAL_RAIL;
        spr(art_.chev, sx, sy, std::max(7.f, 0.7f * scale), pal, false, 0);
    }

    const float gullX[4] = {34.f, 86.f, 132.f, 176.f};
    for (int i = 0; i < 4; i++) {
        float gx = gullX[i] + std::sin(anim_ * 0.6f + float(i)) * 1.5f;
        float gy = laneMid(gx) + laneHalf(gx) + 2.6f + float(i % 2) * 0.4f;
        float sx, sy;
        project(gx, gy, sx, sy);
        int fr = int(anim_ * 3.f + float(i)) & 1;
        spr(art_.gull[fr], sx, sy, std::max(7.f, 0.65f * scale), PAL_SKY, i & 1, 1);
    }

    int sock = int(anim_ * 4.f) % 3;
    if (sock < 0) sock = 0;
    float sx, sy;
    project(kEnd + 3.2f, 0.f, sx, sy);
    float sockH = std::max(12.f, 3.6f * scale);
    spr(art_.sock[sock], sx, sy - sockH * 0.45f, sockH, PAL_RAIL, false, 1);
    project(kEnd + 6.5f, 0.f, sx, sy);
    float shedH = std::max(14.f, 3.1f * scale);
    spr(art_.shed, sx, sy - shedH * 0.45f, shedH, PAL_SHED, false, 1);

    const float reeds[] = {14.f, 36.f, 58.f, 92.f, 128.f, 162.f, 188.f, 210.f};
    for (float rx : reeds) {
        if (rx < left - 2.f || rx > right + 2.f) continue;
        float px, py;
        project(rx, 0.2f, px, py);
        if (py < ay + 20.f || py > gs::SCREEN_H + 20.f) continue;
        float ht = std::max(10.f, 2.4f * scale);
        spr(art_.reed, px, py - ht * 0.4f, ht, PAL_REED, rx > 100.f, 2);
    }

    float gStep = 4.2f;
    float g0 = std::floor(left / gStep) * gStep;
    for (float wx = g0; wx < right + gStep; wx += gStep) {
        float s0, y0, s1, y1;
        project(wx, 1.3f, s0, y0);
        project(wx + gStep, 0.f, s1, y1);
        if (y0 > gs::SCREEN_H + 6.f || y0 < ay + 24.f) continue;
        sprBox(art_.bank, (s0 + s1) * 0.5f, y0, std::max(3.f, s1 - s0 + 1.f), std::max(4.f, y1 - y0), PAL_BANK);
        float depth = std::min(float(gs::SCREEN_H + 4.f), y1 + 36.f);
        if (depth - y1 > 3.f)
            sprBox(art_.water, (s0 + s1) * 0.5f, y1, std::max(3.f, s1 - s0 + 1.f), depth - y1, PAL_WATER);
    }

    for (int i = 0; i < 3; i++) {
        float hx = std::fmod(20.f + float(i) * 150.f - camX * scale * 0.12f, 520.f);
        if (hx < -80.f) hx += 520.f;
        spr(art_.hill, hx, 196.f, 26.f + float(i % 2) * 6.f, PAL_HILL, false, 8);
    }
    for (int i = 0; i < 4; i++) {
        float cx = std::fmod(30.f + float(i) * 120.f - camX * scale * 0.05f + anim_ * 8.f, 460.f);
        if (cx < -40.f) cx += 460.f;
        spr(art_.cloud, cx, 22.f + float(i % 3) * 14.f, 12.f + float(i % 2) * 4.f, PAL_SKY, i & 1, 1);
    }
    spr(art_.sun, 292.f, 20.f, 18.f, PAL_SKY, false, 0);

    if (mode_ == Mode::Title) {
        int cs = int(kCrew0);
        char buf[48];
        std::snprintf(buf, sizeof buf, "CREW CLOCK  %d:%02d", cs / 60, cs % 60);
        hudC(16, "UP CLIMB     DOWN DIVE", PAL_HUD);
        hudC(18, "Z X C SPACE  SPOILER", PAL_HUD);
        hudC(20, "OUT OF THE LANE LOSES THE LEG", PAL_AMBER);
        hudC(21, buf, PAL_GOOD);
        hudC(23, "BEAT THE OTHER CREW", PAL_HUD);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START FLIES    ESC TITLE", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(23, why_, PAL_BAD);
        hudC(25, "START TRIES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(22, "THE WHOLE LEG, STILL IN THE LANE", PAL_GOOD);
        char buf[48];
        std::snprintf(buf, sizeof buf, "%.1f S", legT_);
        hudC(24, buf, PAL_HUD);
        int leftS = int(std::floor(crew_ + 1e-3f));
        std::snprintf(buf, sizeof buf, "CREW HAD %d S", leftS);
        hudC(25, buf, PAL_AMBER);
    } else {
        char buf[48];
        float err = h_ - laneMid(x_);
        float half = laneHalf(x_);
        std::snprintf(buf, sizeof buf, "LANE %+4.1f", err);
        int lp = std::fabs(err) < half * 0.35f ? PAL_GOOD : (std::fabs(err) > half - 1.3f ? PAL_AMBER : PAL_HUD);
        hud(1, 1, buf, lp);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(13, 1, buf, v_ < 12.f ? PAL_BAD : PAL_HUD);
        int csec = int(std::ceil(crew_ - 1e-4f));
        if (csec < 0) csec = 0;
        std::snprintf(buf, sizeof buf, "CREW %d:%02d", csec / 60, csec % 60);
        hud(24, 1, buf, crew_ < 6.f ? PAL_BAD : (crew_ < 10.f ? PAL_AMBER : PAL_HUD));

        const char* line = "IN THE LANE";
        int pal = PAL_GOOD;
        float leftM = kEnd - x_;
        if (near_) {
            line = "NEAR THE RAIL";
            pal = PAL_AMBER;
        } else if (crewX_ > x_ + 2.f) {
            line = "CREW AHEAD";
            pal = PAL_BAD;
        } else if (tight_) {
            line = "TIGHT LANE";
            pal = PAL_AMBER;
        } else if (leftM < 30.f) {
            std::snprintf(buf, sizeof buf, "GATE %3.0f M", std::max(0.f, leftM));
            line = buf;
            pal = PAL_GOOD;
        } else {
            std::snprintf(buf, sizeof buf, "END %3.0f M", std::max(0.f, leftM));
            line = buf;
            pal = PAL_HUD;
        }
        hudC(2, line, pal);
        if (spoil_ > 0.45f) hud(33, 2, "SPOILER", PAL_AMBER);
        hudC(26, "UP DOWN FLY     Z SPOILER", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(7, 8, 11));
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.12f, 0.18f, 0.1f);
    sys.apu.setPatch(0, chimePatch());
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
    for (Puff& p : puffs_)
        if (p.life > 0.f) p.life = std::max(0.f, p.life - kDt);

    if (chime_ >= 0) {
        static const float notes[] = {349.f, 440.f, 523.f, 698.f};
        chimeT_ += kDt;
        if (chimeT_ > 0.14f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.2f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0.f;
            if (chime_ > 7) chime_ = -1;
        }
    }

    if (mode_ == Mode::Title) {
        x_ = 46.f + std::sin(anim_ * 0.25f) * 2.f;
        h_ = laneMid(x_) + std::sin(anim_ * 0.9f) * 1.1f;
        att_ = std::sin(anim_ * 0.7f) * 0.08f;
        v_ = kTrim;
        crewX_ = x_ - 16.f;
        crew_ = kCrew0;
        sys.setLight(40, 120, 140);
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            blip(740.f);
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
            blip(620.f);
            mode_ = Mode::Fly;
        } else if (pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (mode_ == Mode::Win) {
            h_ = laneMid(kEnd) + std::sin(anim_ * 1.1f) * 0.28f;
            att_ += (0.f - att_) * 0.08f;
            x_ = kEnd;
        }
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

    float nose = 0.f, spoil = 0.f;
    if (bot_) {
        pilot(nose, spoil);
    } else {
        if (pad.down(gs::BTN_UP)) nose += 1.f;
        if (pad.down(gs::BTN_DOWN)) nose -= 1.f;
        if (std::fabs(pad.axisY) > 0.18f) nose = pad.axisY;
        nose = clampf(nose, -1.f, 1.f);
        bool wantSpoil = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO) ||
                         pad.down(gs::BTN_X) || pad.down(gs::BTN_Z) || pad.accel > 0.08f || pad.brake > 0.08f;
        if (!spoilArm_) {
            if (!wantSpoil) spoilArm_ = true;
        } else if (wantSpoil) {
            spoil = 1.f;
        }
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(420.f);
            draw();
            return;
        }
    }

    float path = std::atan2(vy_, std::max(8.f, v_));
    float wantAtt = path * 0.62f + nose * 0.05f;
    att_ += (wantAtt - att_) * 0.3f;
    att_ = clampf(att_, -0.42f, 0.42f);

    physics(nose, spoil);
    if (mode_ == Mode::Fly && spoil_ > 0.4f && (sys.frame % 5) == 0) {
        puffs_[puffN_ % 6] = {x_ - 3.4f, h_ - 0.05f, 0.4f};
        puffN_++;
    }
    audio();
    draw();
}

}  // namespace glane
