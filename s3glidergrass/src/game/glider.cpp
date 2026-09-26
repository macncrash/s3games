#include "glider.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace ggrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kStartX = 20.f;
constexpr float kStartH = 30.f;
constexpr float kStartV = 22.f;
constexpr float kTree1 = 148.f;
constexpr float kCanopy = 8.6f;
constexpr float kGrass0 = 196.f;
constexpr float kGrass1 = 420.f;
constexpr float kHullNose = 4.2f;
constexpr float kHullTail = 5.8f;
constexpr float kAim = 250.f;
constexpr float kCrew0 = 38.f;
constexpr float kStop = 0.10f;
constexpr float kHoldNeed = 0.50f;
constexpr float kHard = -4.9f;
constexpr float kMinV = 8.8f;
constexpr float kPitch = 6.5f;
constexpr float kTrim = 1.9f;
constexpr float kSpoilSink = 5.2f;

float clamp(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float liftOf(float v) { return clamp((v - 8.f) / 14.f, 0.30f, 1.06f); }

float stallOf(float v) { return std::max(0.f, 12.2f - v) * 0.75f; }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.16f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.18f, 0.7f, 0.2f};
    p.op[1] = {2.f, 0.28f, 0.02f, 0.22f, 0.35f, 0.18f};
    p.op[2] = {3.01f, 0.08f, 0.02f, 0.24f, 0.2f, 0.2f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f};
    p.vol = 0.2f;
    p.tone = 1700.f;
    return p;
}

}  // namespace

bool Game::onGrass() const { return x_ >= kGrass0 && x_ <= kGrass1; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (ground_ && v_ <= 1.2f) return 3;
    if (x_ + kHullNose >= kGrass0 && x_ - kHullTail <= kGrass1) return 2;
    return 1;
}

int Game::wingFrame(float att) const {
    int fi = int(std::lround((0.36f - att) / 0.18f));
    return std::clamp(fi, 0, 4);
}

void Game::rivalAt(float& rx, float& rh) const {
    float u = 1.f - clamp(crew_ / kCrew0, 0.f, 1.f);
    float s = u * u * (3.f - 2.f * u);
    float x0 = kGrass0 + 36.f;
    float x1 = kGrass1 - 22.f;
    rx = x0 + (x1 - x0) * s;
    rh = 16.f * (1.f - s) + 0.4f * s;
}

void Game::begin() {
    x_ = kStartX;
    h_ = kStartH;
    v_ = kStartV;
    vy_ = -2.6f;
    att_ = -0.12f;
    nose_ = 0.f;
    spoil_ = 0.f;
    ground_ = false;
    hold_ = 0.f;
    race_ = 0.f;
    crew_ = kCrew0;
    won_ = false;
    over_ = false;
    spoilWas_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    chimeT_ = 0.f;
    puffN_ = 0;
    lastSec_ = -1;
    shake_ = 0.f;
    for (Puff& p : puffs_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    snap_ = true;
    camX_ = 178.f;
    camH_ = 4.8f;
    camS_ = 4.3f;
    anchorY_ = 168.f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Fly;
    snap_ = true;
    blip(640.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.055f);
    beep_ = 0.07f;
}

void Game::win() {
    if (mode_ != Mode::Fly) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    v_ = 0.f;
    vy_ = 0.f;
    h_ = 0.f;
    why_ = "full stop";
    banner_ = "FULL STOP";
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.22f, 0.08f, 140);
    sys_->setLight(40, 180, 70);
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
    sys_->rumble(0.55f, 0.32f, 180);
    sys_->setLight(180, 30, 20);
    sys_->apu.noiseBurst(0.46f, 480.f, 0.3f);
}

void Game::pilot(float& nose, float& spoil) const {
    if (ground_) {
        nose = 0.f;
        spoil = 1.f;
        return;
    }
    if (x_ < kTree1 + 6.f && h_ < kCanopy + 3.5f) {
        nose = 1.f;
        spoil = 0.f;
        return;
    }
    float want, pathVy;
    if (x_ < kAim) {
        float u = clamp((x_ - kStartX) / (kAim - kStartX), 0.f, 1.f);
        want = kStartH * (1.f - u) + 0.45f * u;
        pathVy = -(kStartH - 0.45f) / (kAim - kStartX) * std::max(v_, 10.f);
    } else {
        want = 0.35f;
        pathVy = -1.4f;
    }
    float err = want - h_;
    float sw = pathVy + clamp(err * 0.85f - (vy_ - pathVy) * 0.35f, -2.f, 2.2f);
    sw = clamp(sw, -4.2f, 3.2f);
    if (h_ < 4.2f) sw = std::max(sw, -1.55f);
    if (h_ < 2.1f) sw = std::max(sw, -1.0f);
    if (h_ < 1.05f) sw = std::max(sw, -0.58f);
    float ahead = kGrass1 - kHullNose - 8.f - x_;
    if (ahead < 50.f && h_ > 0.6f) {
        float tt = std::max(0.45f, ahead / std::max(v_, 12.f));
        float need = -h_ / tt;
        sw = std::min(sw, std::max(need, -2.4f));
        if (h_ < 1.3f) sw = std::max(sw, -0.7f);
    }
    spoil = (v_ > 27.f && h_ > 5.f) ? 0.5f : 0.f;
    if (h_ < 4.5f) spoil = 0.f;
    float lift = liftOf(v_);
    float stall = stallOf(v_);
    nose = (sw + kTrim * lift + spoil * kSpoilSink + stall) / (kPitch * std::max(lift, 0.25f));
    nose = clamp(nose, -1.f, 1.f);
}

void Game::physics(float nose, float spoil) {
    nose_ = clamp(nose, -1.f, 1.f);
    spoil_ = clamp(spoil, 0.f, 1.f);
    race_ += kDt;
    crew_ = std::max(0.f, crew_ - kDt);

    if (!ground_) {
        float lift = liftOf(v_);
        float stall = stallOf(v_);
        float vyCmd = (nose_ * kPitch - kTrim) * lift - spoil_ * kSpoilSink - stall;
        vy_ += (vyCmd - vy_) * std::min(1.f, 5.f * kDt);
        v_ += (-0.12f - 0.20f * std::max(vy_, 0.f) + 0.035f * std::max(-vy_, 0.f) - spoil_ * 1.2f) * kDt;
        v_ = clamp(v_, 0.f, 34.f);
        x_ += std::max(0.f, v_ * 0.98f) * kDt;
        h_ += vy_ * kDt;
        if (!std::isfinite(x_) || !std::isfinite(h_) || !std::isfinite(v_)) {
            fail("lost the air", "LOST");
            return;
        }
        if (x_ < kTree1 && h_ < kCanopy) {
            fail("in the trees", "TREES");
            return;
        }
        if (h_ <= 0.04f) {
            float hitVy = vy_;
            h_ = 0.f;
            if (x_ > kGrass1) {
                fail("in the creek", "THE CREEK");
                return;
            }
            if (x_ < kGrass0) {
                fail("not the grass", "NOT GRASS");
                return;
            }
            if (hitVy < kHard) {
                fail("landed too hard", "TOO HARD");
                return;
            }
            if (v_ < kMinV) {
                fail("too slow", "TOO SLOW");
                return;
            }
            ground_ = true;
            vy_ = 0.f;
            shake_ = 0.5f;
            sys_->rumble(0.3f, 0.12f, 80);
            sys_->apu.noiseBurst(0.2f, 220.f, 0.16f);
            puffs_[puffN_ % 8] = {x_ - 1.2f, 0.45f};
            puffN_++;
        } else if (x_ > kGrass1) {
            fail("missed the grass", "MISSED");
            return;
        }
    } else {
        float decel = 0.55f + spoil_ * 8.8f;
        v_ = std::max(0.f, v_ - decel * kDt);
        if (v_ < 0.05f) v_ = 0.f;
        x_ += v_ * kDt;
        h_ = 0.f;
        vy_ = 0.f;
        if (x_ > kGrass1) {
            fail("ran off the grass", "RAN OFF");
            return;
        }
        if (x_ < kGrass0) {
            fail("not the grass", "NOT GRASS");
            return;
        }
        if (v_ <= kStop) {
            hold_ += kDt;
            if (hold_ >= kHoldNeed) {
                win();
                return;
            }
        } else {
            hold_ = 0.f;
        }
    }
    if (mode_ == Mode::Fly && crew_ <= 0.f) fail("the other crew has the grass", "THEIR GRASS");
}

void Game::aimCamera(bool scenic) {
    if (scenic) {
        camX_ = 178.f;
        camH_ = 4.8f;
        camS_ = 4.3f;
        anchorY_ = 168.f;
        return;
    }
    float hh = std::max(0.f, h_);
    float wantS = clamp(5.15f + (5.f - hh) * 0.05f, 4.7f, 6.0f);
    float lead = 0.f;
    if (mode_ == Mode::Fly && !ground_) lead = clamp(v_ * 0.32f, 3.5f, 11.f);
    else if (mode_ == Mode::Fly && ground_) lead = 1.2f;
    float wantX = x_ + lead;
    float wantH = std::max(1.4f, hh * 0.88f + 0.2f);
    float wantA = clamp(152.f - hh * 2.7f, 62.f, 152.f);
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        wantS = 5.1f;
        wantX = x_;
        wantH = 2.2f;
        wantA = 132.f;
    }
    if (snap_) {
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
        anchorY_ = wantA;
        snap_ = false;
        return;
    }
    float k = 0.16f;
    camX_ += (wantX - camX_) * k;
    camH_ += (wantH - camH_) * k;
    camS_ += (wantS - camS_) * k;
    anchorY_ += (wantA - anchorY_) * k;
}

void Game::sky() {
    uint16_t zen = gs::rgb4(4, 7, 13);
    uint16_t mid = gs::rgb4(7, 12, 15);
    uint16_t hor = gs::rgb4(12, 15, 8);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(13, 7, 5), 0.4f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(10, 15, 8), 0.35f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.55f ? lerpC(zen, mid, t / 0.55f) : lerpC(mid, hor, (t - 0.55f) / 0.45f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = true;
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
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -80) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::sprAnchor(const Ship& ship, float sx, float sy, float destH, int pal) {
    const gs::Mipped& m = ship.img;
    if (destH < 1.5f || m.h < 1) return;
    float sc = destH / float(m.h);
    float w = float(m.w) * sc;
    float left = sx - ship.ax * sc;
    float top = sy - ship.ay * sc;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(destH)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(left)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(top)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 12 || s.x + s.w < -12 || s.y > gs::SCREEN_H + 12 || s.y + s.h < -40) return;
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
    if (s.x > gs::SCREEN_W + 4 || s.x + s.w < -4 || s.y > gs::SCREEN_H || s.y + s.h < 0) return;
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !s[0]) return;
    const float adv = 18.f * scale;
    x -= float(std::strlen(s)) * adv * 0.5f;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::project(float wx, float wy, float ax, float& sx, float& sy) const {
    float shy = 0.f;
    if (shake_ > 0.f) shy = std::cos(t_ * 41.f) * 2.4f * shake_;
    sx = ax + (wx - camX_) * camS_;
    if (shake_ > 0.f) sx += std::sin(t_ * 47.f) * 3.2f * shake_;
    sy = anchorY_ + shy - (wy - camH_) * camS_;
}

void Game::draw(float cx, float ch, float catt, bool craft) {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();
    const float scale = camS_;
    const float ax = 150.f;

    if (mode_ == Mode::Title) {
        text("S3 GLIDER GRASS", 160, 12, 0.72f, PAL_HUD);
        text("LAND AND STOP", 160, 30, 0.5f, PAL_GOOD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 22, 1.05f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(banner_, 160, 20, 0.95f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("FULL STOP", 160, 18, 1.05f, PAL_GOOD);
        text("ON THE GRASS", 160, 42, 0.58f, PAL_HUD);
    }

    auto feet = [&](const gs::Mipped& m, float wx, float worldH, int pal, int fog = 0) {
        float sx, sy;
        project(wx, 0.f, ax, sx, sy);
        float ht = std::max(4.f, worldH * scale);
        spr(m, sx, sy - ht * 0.5f, ht, pal, false, fog);
    };

    if (craft) {
        const Ship& ship = art_.ship[wingFrame(catt)];
        float sx, sy;
        project(cx, ch, ax, sx, sy);
        float dest = float(ship.img.h) / ship.ppm * scale;
        sprAnchor(ship, sx, sy, std::max(8.f, dest), PAL_SHIP);
    }
    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        float sx, sy;
        project(p.x, 0.15f, ax, sx, sy);
        spr(art_.dust, sx, sy - 4.f, 8.f + (1.f - p.life) * 10.f, PAL_DUST, false, int((1.f - p.life) * 8));
    }
    if (craft && ch < 16.f) {
        float sx, sy;
        project(cx, 0.f, ax, sx, sy);
        float sh = clamp((1.6f + ch * 0.12f) * scale * 0.55f, 4.f, 28.f);
        spr(art_.shade, sx, sy - sh * 0.25f, sh, PAL_DUST, false, int(std::min(10.f, ch)));
    }

    float rx, rh;
    rivalAt(rx, rh);
    {
        const Ship& ship = art_.ship[3];
        float sx, sy;
        project(rx, rh, ax, sx, sy);
        float dest = float(ship.img.h) / ship.ppm * scale;
        sprAnchor(ship, sx, sy, std::max(8.f, dest), PAL_RIVAL);
    }

    // Props are emitted before the ground fill so they stand on top of it.
    for (float tx = 12.f; tx < kTree1 - 2.f; tx += 16.f) {
        int n = int(tx) / 16;
        float th = 9.2f + float(n % 4) * 0.7f;
        const gs::Mipped& img = (n & 1) ? art_.oak : art_.pine;
        float dist = std::fabs(tx - camX_);
        int fog = dist > 70.f ? int(clamp((dist - 70.f) / 14.f, 0.f, 7.f)) : 0;
        feet(img, tx, th, PAL_TREE, fog);
    }
    feet(art_.sign, kGrass0 - 14.f, 3.4f, PAL_PROP, 0);
    int sock = int(t_ * 5.f) % 3;
    if (sock < 0) sock = 0;
    feet(art_.sock[sock], kGrass0 + 6.f, 4.6f, PAL_PROP, 0);
    for (float bx = kGrass0 + 18.f; bx < kGrass1 - 8.f; bx += 28.f) feet(art_.tuft, bx, 1.15f, PAL_TREE, 0);
    feet(art_.fence, kGrass1, 2.4f, PAL_PROP, 0);
    feet(art_.barn, kGrass1 + 18.f, 7.2f, PAL_PROP, 1);
    for (int i = 0; i < 5; i++) feet(art_.reed, kGrass1 + 8.f + float(i) * 6.f, 3.1f + float(i % 2) * 0.4f, PAL_TREE, 1);

    auto board = [&](float wx, float ww, float hh) {
        float sx0, sy0, sx1, sy1;
        project(wx, 0.f, ax, sx0, sy0);
        project(wx + ww, hh, ax, sx1, sy1);
        float top = std::min(sy0, sy1);
        float w = std::max(2.f, std::fabs(sx1 - sx0));
        float h = std::max(2.f, std::fabs(sy1 - sy0));
        sprBox(art_.white, (sx0 + sx1) * 0.5f, top, w, h, PAL_FIELD);
    };
    board(kGrass0, 1.3f, 0.85f);
    board(kGrass0 + 8.f, 1.1f, 0.7f);
    board(kGrass0 + 16.f, 1.1f, 0.7f);

    float left = camX_ - (ax + 24.f) / scale;
    float right = camX_ + (gs::SCREEN_W - ax + 24.f) / scale;
    float step = clamp(36.f / scale, 2.8f, 12.f);
    float start = std::floor(left / step) * step;
    const float tileH = 26.f;
    for (float wx = start; wx < right; wx += step) {
        float sx, sy;
        project(wx + step * 0.5f, 0.f, ax, sx, sy);
        float sw = step * scale + 2.f;
        const gs::Mipped* img = &art_.woods;
        float mid = wx + step * 0.5f;
        if (mid >= kGrass1 + 8.f) img = &art_.creek;
        else if (mid >= kGrass1) img = &art_.bank;
        else if (mid >= kGrass0) img = &art_.grass[(int(std::floor(mid / 12.f)) & 1)];
        else if (mid >= kTree1) img = &art_.gravel;
        float y = sy;
        if (y < -tileH) {
            int n = int((-y) / tileH);
            y += float(n) * tileH;
        }
        int rows = 0;
        for (; y < gs::SCREEN_H + 2.f && rows < 10; y += tileH - 1.f, rows++)
            sprBox(*img, sx, y, sw, tileH, PAL_FIELD);
    }

    int flap = int(t_ * 7.f) & 1;
    for (int i = 0; i < 4; i++) {
        float bx = 60.f + float(i) * 80.f + std::fmod(t_ * (8.f + float(i)), 36.f);
        float by = 26.f + float(i % 3) * 3.5f;
        float sx, sy;
        project(bx, by, ax, sx, sy);
        spr(art_.bird[flap], sx, sy, 7.f, PAL_BIRD, false, 1);
    }
    for (int i = 0; i < 4; i++) {
        float sx = std::fmod(30.f + float(i) * 120.f - camX_ * scale * 0.05f + t_ * 7.f, 480.f);
        if (sx < -50.f) sx += 480.f;
        spr(art_.cloud, sx, 28.f + float(i % 3) * 16.f, 16.f + float(i % 2) * 6.f, PAL_SKY, i & 1, 1);
    }
    for (int i = 0; i < 5; i++) {
        float sx = std::fmod(18.f + float(i) * 96.f - camX_ * scale * 0.16f, 520.f);
        if (sx < -90.f) sx += 520.f;
        spr(art_.hill, sx, 96.f, 34.f + float(i % 2) * 6.f, PAL_FAR, false, 7);
    }
    spr(art_.sun, 286.f, 26.f, 22.f, PAL_SKY, false, 0);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(7, "UP DOWN  PITCH", PAL_HUD);
        hudC(8, "Z X C SPACE  SPOILER BRAKE", PAL_HUD);
        hudC(10, "LAND ON THE GRASS", PAL_GOOD);
        hudC(11, "COME TO A FULL STOP", PAL_GOOD);
        hudC(13, "THE CLOCK IS THE OTHER CREW", PAL_AMBER);
        hudC(15, "CREW 0:38", PAL_AMBER);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_HUD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START FLIES", PAL_HUD);
        hudC(26, "ESC TITLE", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(23, why_, PAL_BAD);
        hudC(26, "START TRIES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "%.1f S", race_);
        hudC(23, buf, PAL_HUD);
        int left = int(std::ceil(crew_ - 1e-3f));
        if (left < 0) left = 0;
        std::snprintf(buf, sizeof buf, "CREW HAD %d S", left);
        hudC(25, buf, PAL_GOOD);
    } else {
        std::snprintf(buf, sizeof buf, "ALT %4.1f", std::max(0.f, h_));
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(12, 1, buf, (!ground_ && v_ < 12.f) ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "VS %+5.1f", vy_);
        hud(23, 1, buf, vy_ < -3.6f ? PAL_BAD : vy_ > 0.8f ? PAL_GOOD : PAL_HUD);
        int cs = int(std::ceil(crew_ - 1e-4f));
        if (cs < 0) cs = 0;
        std::snprintf(buf, sizeof buf, "CREW %d:%02d", cs / 60, cs % 60);
        int cpal = cs <= 10 ? PAL_BAD : PAL_AMBER;
        if (cs > 10 || (sys_->frame / 10) % 2 == 0) hud(28, 2, buf, cpal);
        if (spoil_ > 0.4f) hud(1, 2, ground_ ? "BRAKE" : "SPOILER", PAL_AMBER);

        const char* line = "FIND THE GRASS";
        int pal = PAL_HUD;
        if (ground_ && v_ <= kStop) {
            line = "HOLD THE FULL STOP";
            pal = PAL_GOOD;
        } else if (ground_) {
            line = "BRAKE TO A FULL STOP";
            pal = PAL_GOOD;
        } else if (x_ >= kGrass0 && x_ <= kGrass1) {
            line = "PUT IT ON THE GRASS";
            pal = PAL_AMBER;
        } else if (x_ < kGrass0) {
            std::snprintf(buf, sizeof buf, "GRASS %3.0f M", std::max(0.f, kGrass0 - x_));
            line = buf;
            pal = PAL_AMBER;
        } else {
            line = "TOO LONG";
            pal = PAL_BAD;
        }
        hudC(26, line, pal);
        if (!ground_ && v_ < 12.f) hudC(3, "STALL", PAL_BAD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(10, 13, 11));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.18f, 0.1f);
    sys.apu.setPatch(0, bellPatch());
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ != Mode::Pause) t_ += kDt;
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.4f);
    for (Puff& p : puffs_)
        if (p.life > 0.f) p.life = std::max(0.f, p.life - kDt * 1.3f);

    if (chime_ >= 0) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        chimeT_ += kDt;
        if (chimeT_ > 0.13f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.18f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0.f;
            if (chime_ > 7) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        aimCamera(true);
        float bob = std::sin(t_ * 1.15f);
        draw(164.f, 7.8f + bob * 0.28f, -0.12f + std::sin(t_ * 0.7f) * 0.04f, true);
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
        draw(x_, h_, att_, true);
        if (pad.pressed(gs::BTN_START)) {
            blip(620.f);
            mode_ = Mode::Fly;
        } else if (pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        aimCamera(false);
        draw(x_, h_, att_, true);
        sys.apu.noise(0.f, 700.f, false);
        sys.apu.tone(2, 0.f, 0.f);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    float nose = 0.f, spoil = 0.f;
    if (bot_) {
        pilot(nose, spoil);
    } else {
        if (pad.down(gs::BTN_UP)) nose += 1.f;
        if (pad.down(gs::BTN_DOWN)) nose -= 1.f;
        if (std::fabs(pad.axisY) > 0.18f) nose = pad.axisY;
        nose = clamp(nose, -1.f, 1.f);
        bool brake = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_X) ||
                     pad.down(gs::BTN_TURBO);
        if (brake) spoil = 1.f;
        if (pad.accel > 0.08f) spoil = std::max(spoil, pad.accel);
        if (pad.brake > 0.08f) spoil = std::max(spoil, pad.brake);
        if (ground_ && pad.down(gs::BTN_DOWN)) spoil = 1.f;
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(420.f);
            draw(x_, h_, att_, true);
            return;
        }
    }

    int secNow = int(std::floor(std::max(0.f, crew_)));
    if (lastSec_ >= 0 && secNow != lastSec_ && secNow <= 9) blip(secNow <= 3 ? 190.f : 340.f);
    lastSec_ = secNow;

    physics(nose, spoil);
    if (mode_ == Mode::Fly || mode_ == Mode::Win) {
        float path = std::atan2(vy_, std::max(10.f, v_));
        float want = ground_ ? 0.02f : path * 0.85f + nose_ * 0.08f;
        att_ += (want - att_) * 0.28f;
        att_ = clamp(att_, -0.5f, 0.5f);
    }
    if (spoil_ > 0.45f && !spoilWas_) blip(880.f);
    spoilWas_ = spoil_ > 0.45f;
    if (ground_ && v_ > 2.f && mode_ == Mode::Fly && (sys.frame % 4) == 0) {
        puffs_[puffN_ % 8] = {x_ - 1.5f, 0.4f};
        puffN_++;
    }

    if (mode_ == Mode::Fly) {
        float wind = clamp((v_ - 8.f) / 20.f, 0.f, 1.f) * (ground_ ? 0.05f : 0.035f);
        if (spoil_ > 0.4f && !ground_) wind += 0.03f;
        sys.apu.noise(wind, ground_ ? 420.f : 860.f + v_ * 18.f, false);
        if (!ground_ && v_ < 12.f) sys.apu.tone(2, 160.f + std::max(0.f, 12.f - v_) * 14.f, 0.04f);
        else if (!ground_ && vy_ > 0.6f) sys.apu.tone(2, 480.f + vy_ * 30.f, 0.025f);
        else sys.apu.tone(2, 0.f, 0.f);
        if (ground_ && x_ >= kGrass0 && x_ <= kGrass1) sys.setLight(40, 170, 70);
        else if (vy_ < -3.4f) sys.setLight(170, 40, 20);
        else if (crew_ < 10.f) sys.setLight(170, 90, 20);
        else sys.setLight(40, 90, 160);
    }

    aimCamera(false);
    draw(x_, h_, att_, true);
}

}  // namespace ggrass
