#include "game/plat.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace gliderplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kDeck = 9.f;
constexpr float kNear = 128.f;
constexpr float kFar = 186.f;
constexpr float kMark = 170.f;
constexpr float kTouch = 152.f;
constexpr float kNose = 4.55f;
constexpr float kTail = 5.05f;
constexpr float kStop = 0.38f;
constexpr float kHoldNeed = 0.48f;
constexpr float kLevelTol = 1.85f;
constexpr float kAttMax = 0.24f;
constexpr float kHard = -3.6f;
constexpr float kLegLimit = 36.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float liftOf(float v) { return clampf((v - 7.f) / 14.f, 0.18f, 1.05f); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.18f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.18f, 0.6f, 0.2f};
    p.op[1] = {2.f, 0.32f, 0.02f, 0.22f, 0.35f, 0.18f};
    p.op[2] = {3.01f, 0.1f, 0.02f, 0.24f, 0.22f, 0.2f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f};
    p.vol = 0.22f;
    p.tone = 1600.f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.04f) return 3;
    if (onDeck_ || (x_ + kNose > kNear - 4.f && h_ < kDeck + 7.f && x_ > kNear - 28.f)) return 2;
    return 1;
}

int Game::wingFrame() const {
    int fi = int(std::lround((0.36f - att_) / 0.18f));
    return std::clamp(fi, 0, 4);
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    onDeck_ = false;
    hold_ = 0.f;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    snapCam_ = true;
    x_ = kNear - 16.f;
    h_ = kDeck + 6.5f;
    v_ = 15.f;
    vy_ = -1.1f;
    att_ = -0.08f;
    camX_ = 150.f;
    camH_ = kDeck + 5.2f;
    camS_ = 3.7f;
}

void Game::startRun() {
    x_ = 8.f;
    h_ = 22.f;
    v_ = 19.f;
    vy_ = -1.3f;
    att_ = 0.f;
    nose_ = 0.f;
    spoil_ = 0.f;
    onDeck_ = false;
    hold_ = 0.f;
    legT_ = 0.f;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    puffN_ = 0;
    shake_ = 0.f;
    for (Puff& p : puffs_) p = {};
    mode_ = Mode::Fly;
    snapCam_ = true;
    camX_ = x_ + 10.f;
    camH_ = h_;
    camS_ = 3.8f;
    blip(640.f);
}

void Game::pilot(float& nose, float& spoil) const {
    if (onDeck_) {
        float err = kMark - x_;
        nose = 0.f;
        if (err <= 0.08f) {
            spoil = 1.f;
        } else {
            float vStop = std::sqrt(std::max(0.f, 2.f * 9.f * err));
            spoil = v_ > vStop ? 1.f : (v_ > vStop * 0.9f ? 0.4f : 0.f);
        }
        return;
    }
    float dist = kTouch - x_;
    float slope = std::max(0.f, dist) * 0.105f;
    float hw = kDeck + std::max(0.f, slope - 1.15f);
    if (dist < 26.f) hw = kDeck - 0.08f;
    float altErr = hw - h_;
    float sw = clampf(altErr * 1.15f, -2.4f, 1.6f);
    if (dist < 26.f && h_ - kDeck < 0.7f) sw = std::max(sw, -1.15f);
    spoil = 0.f;
    float wantV = dist > 60.f ? 16.5f : (dist > 24.f ? 13.8f : 12.2f);
    if (v_ > wantV) spoil = std::max(spoil, clampf((v_ - wantV) / 4.f, 0.f, 0.8f));
    if (h_ > hw + 2.f) spoil = std::max(spoil, clampf((h_ - hw) / 7.f, 0.f, 0.65f));
    if (h_ < kDeck + 1.8f) spoil *= 0.15f;
    float lift = std::max(liftOf(v_), 0.2f);
    float stall = std::max(0.f, 11.5f - v_) * 0.55f;
    nose = (sw + spoil * 4.4f + stall + 1.2f * lift) / (5.8f * lift);
    nose = clampf(nose, -1.f, 1.f);
}

void Game::win() {
    if (mode_ != Mode::Fly) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "level";
    banner_ = "LEVEL";
    h_ = kDeck;
    vy_ = 0.f;
    v_ = 0.f;
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.28f, 0.1f, 160);
    sys_->setLight(40, 180, 80);
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
    sys_->rumble(0.55f, 0.28f, 180);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.42f, 420.f, 0.3f);
}

void Game::physics(float nose, float spoil) {
    nose_ = clampf(nose, -1.f, 1.f);
    spoil_ = clampf(spoil, 0.f, 1.f);
    legT_ += kDt;
    if (!onDeck_) {
        float lift = liftOf(v_);
        float stall = std::max(0.f, 11.5f - v_) * 0.55f;
        float vyCmd = (nose_ * 5.8f - 1.2f) * lift - spoil_ * 4.4f - stall;
        bool over = x_ >= kNear - 1.f && x_ <= kFar - 0.4f;
        float gap = h_ - kDeck;
        if (over && gap > 0.f && gap < 1.15f && vy_ < -1.7f) {
            float c = (1.15f - gap) / 1.15f;
            vy_ = -1.7f + (vy_ + 1.7f) * (1.f - 0.45f * c);
        }
        vy_ += (vyCmd - vy_) * std::min(1.f, 4.2f * kDt);
        v_ += (-0.42f - 0.10f * std::max(vy_, 0.f) + 0.04f * std::max(-vy_, 0.f) - spoil_ * 1.55f) * kDt;
        v_ = clampf(v_, 0.f, 28.f);
        x_ += std::max(0.f, v_ * 0.97f) * kDt;
        h_ += vy_ * kDt;
        if (!std::isfinite(x_) || !std::isfinite(h_) || !std::isfinite(v_)) {
            fail("lost the air", "LOST");
            return;
        }
        if (x_ + kNose >= kNear && h_ < kDeck - 0.15f && x_ < kNear + 2.f) {
            fail("missed the end", "MISSED");
            return;
        }
        if (h_ <= 0.08f && x_ + kNose < kNear) {
            fail("missed the end", "MISSED");
            return;
        }
        if (v_ < 7.f && h_ > kDeck + 3.f && x_ < kNear - 10.f) {
            fail("too slow", "TOO SLOW");
            return;
        }
        if (x_ - kTail > kFar) {
            fail("missed the end", "MISSED");
            return;
        }
        if (x_ > kNear + 2.f && h_ < kDeck - 0.2f && x_ < kFar) {
            fail("missed the end", "MISSED");
            return;
        }
        bool deckOver = (x_ - kTail) >= kNear - 0.2f && (x_ + kNose) <= kFar - 0.1f && x_ >= kNear + 0.5f;
        if (deckOver && h_ <= kDeck + 0.05f) {
            if (vy_ < kHard) {
                h_ = kDeck;
                fail("too hard", "TOO HARD");
                return;
            }
            if (std::fabs(att_) > kAttMax) {
                h_ = kDeck;
                fail("not level", "NOT LEVEL");
                return;
            }
            if (v_ < 6.5f) {
                h_ = kDeck;
                fail("too slow", "TOO SLOW");
                return;
            }
            onDeck_ = true;
            h_ = kDeck;
            vy_ = 0.f;
            sys_->rumble(0.22f, 0.08f, 70);
            sys_->apu.noiseBurst(0.18f, 220.f, 0.14f);
        }
    } else {
        float brake = 1.35f + spoil_ * 8.6f;
        v_ = std::max(0.f, v_ - brake * kDt);
        x_ += v_ * kDt;
        h_ = kDeck;
        vy_ = 0.f;
        if (x_ + kNose > kFar + 0.05f) {
            fail("missed the end", "MISSED");
            return;
        }
        bool aboard = (x_ - kTail) >= kNear - 0.05f && (x_ + kNose) <= kFar + 0.02f;
        bool atMark = std::fabs(x_ - kMark) <= kLevelTol;
        bool level = std::fabs(att_) <= kAttMax;
        bool stopped = v_ <= kStop;
        if (aboard && atMark && level && stopped) {
            hold_ += kDt;
            if (hold_ >= kHoldNeed) {
                win();
                return;
            }
        } else {
            hold_ = 0.f;
        }
        if (v_ <= 0.05f && !(aboard && atMark && level)) {
            if (!aboard || x_ < kMark - kLevelTol) fail("missed the end", "MISSED");
            else fail("not level", "NOT LEVEL");
            return;
        }
    }
    if (mode_ == Mode::Fly && legT_ > kLegLimit) fail("too late", "TOO LATE");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::audio() {
    if (mode_ != Mode::Fly) {
        sys_->apu.noise(0.f, 800.f, false);
        sys_->apu.tone(2, 0.f, 0.f);
        return;
    }
    float wind = clampf((v_ - 6.f) / 18.f, 0.f, 1.f) * (spoil_ > 0.45f && !onDeck_ ? 0.07f : 0.035f);
    sys_->apu.noise(wind, 860.f + v_ * 36.f, false);
    if (!onDeck_ && v_ < 12.f) sys_->apu.tone(2, 170.f + std::max(0.f, -vy_) * 18.f, 0.03f);
    else sys_->apu.tone(2, 0.f, 0.f);
    if (onDeck_ && std::fabs(x_ - kMark) <= kLevelTol) sys_->setLight(36, 170, 80);
    else if (vy_ < -3.2f) sys_->setLight(170, 40, 24);
    else sys_->setLight(40, 90, 160);
}

void Game::sky() {
    uint16_t zen = gs::rgb4(4, 7, 13);
    uint16_t mid = gs::rgb4(8, 12, 15);
    uint16_t hor = gs::rgb4(15, 13, 9);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 6, 5), 0.4f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(12, 15, 10), 0.28f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.55f ? lerpC(zen, mid, t / 0.55f) : lerpC(mid, hor, (t - 0.55f) / 0.45f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(9, 10, 12));
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

    const bool framing = mode_ == Mode::Win || mode_ == Mode::Fail || hold_ > 0.02f;
    float span = framing ? 44.f : clampf(40.f + std::max(0.f, h_ - kDeck) * 3.1f, 42.f, 130.f);
    if (onDeck_ && !framing) span = 52.f;
    float wantS = 270.f / span;
    float lead = (mode_ == Mode::Fly && !onDeck_ && !framing) ? clampf(v_ * 0.42f, 3.f, 14.f) : 0.f;
    float wantX = framing ? kMark : x_ + lead;
    float wantH = framing ? kDeck + 3.4f : std::max(kDeck * 0.45f, (h_ + kDeck) * 0.42f);
    if (mode_ == Mode::Title) {
        wantX = 152.f;
        wantH = kDeck + 4.6f;
        wantS = 3.55f;
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
    } else if (snapCam_) {
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
        snapCam_ = false;
    } else {
        camX_ += (wantX - camX_) * 0.16f;
        camH_ += (wantH - camH_) * 0.16f;
        camS_ += (wantS - camS_) * 0.16f;
    }
    if (shake_ > 0.f) {
        shx_ = std::sin(legT_ * 90.f) * shake_ * 4.f;
        shy_ = std::cos(legT_ * 70.f) * shake_ * 3.f;
        shake_ *= 0.9f;
        if (shake_ < 0.05f) shake_ = 0.f;
    } else {
        shx_ = shy_ = 0.f;
    }

    const float scale = camS_;
    const float ax = 160.f + shx_;
    const float ay = 108.f + shy_;
    auto project = [&](float wx, float wy, float& sx, float& sy) {
        sx = ax + (wx - camX_) * scale;
        sy = ay - (wy - camH_) * scale;
    };

    if (mode_ == Mode::Title) {
        text("GLIDER PLAT", 160, 22, 1.05f, PAL_HUD);
        text("STOP LEVEL", 160, 46, 0.62f, PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 26, 1.1f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(banner_, 160, 24, 1.05f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("LEVEL", 160, 22, 1.15f, PAL_GOOD);
    }

    const Wing& wing = art_.wing[wingFrame()];
    float gsx, gsy;
    project(x_, h_, gsx, gsy);
    float dest = float(wing.img.h) / wing.ppm * scale;
    sprAnchor(wing.img, wing.ax, wing.ay, gsx, gsy, std::max(10.f, dest), PAL_SHIP);

    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        float sx, sy;
        project(p.x, p.y, sx, sy);
        spr(art_.dust, sx, sy, 7.f + (1.f - p.life) * 10.f, PAL_DUST, false, int((1.f - p.life) * 10));
    }

    float deckH = std::max(5.f, 0.72f * scale);
    float band0, band1, bandY;
    project(kMark - kLevelTol, kDeck, band0, bandY);
    project(kMark + kLevelTol, kDeck, band1, bandY);
    sprBox(art_.stripe, (band0 + band1) * 0.5f, bandY, std::max(6.f, band1 - band0), deckH, PAL_END);

    float sx, sy;
    project(kMark, kDeck + 1.7f, sx, sy);
    spr(art_.sign, sx, sy, std::clamp(1.7f * scale, 14.f, 42.f), PAL_SIGN, false, 0);
    project(kFar - 1.2f, kDeck + 1.35f, sx, sy);
    spr(art_.lamp, sx, sy, std::clamp(1.5f * scale, 12.f, 36.f), PAL_END, false, 0);

    for (int i = 0; i < 4; i++) {
        float wx = kMark - 16.f + float(i) * 4.2f;
        if (wx >= kMark - kLevelTol) break;
        float cx, cy;
        project(wx, kDeck + 0.15f, cx, cy);
        spr(art_.chev, cx, cy, std::max(6.f, 0.55f * scale), PAL_END, false, 0);
    }

    int sock = int(anim_ * 5.f) % 3;
    if (sock < 0) sock = 0;
    project(kNear + 5.f, kDeck, sx, sy);
    float sockH = std::clamp(2.3f * scale, 12.f, 48.f);
    spr(art_.sock[sock], sx, sy - sockH * 0.35f, sockH, PAL_POST, false, 0);

    project(kNear + 14.f, kDeck, sx, sy);
    float hut = std::clamp(3.3f * scale, 16.f, 64.f);
    spr(art_.cabin, sx, sy - hut * 0.42f, hut, PAL_HOUSE, false, 0);

    float left = camX_ - (ax + 24.f) / scale;
    float right = camX_ + (gs::SCREEN_W - ax + 24.f) / scale;
    float step = 5.5f;
    float plank0 = std::max(kNear, std::floor(left / step) * step);
    for (float wx = plank0; wx < kFar && wx < right + step; wx += step) {
        float x0 = std::max(wx, kNear);
        float x1 = std::min(wx + step, kFar);
        float s0, y0, s1, y1;
        project(x0, kDeck, s0, y0);
        project(x1, kDeck, s1, y1);
        sprBox(art_.plank, (s0 + s1) * 0.5f, y0, std::max(2.f, s1 - s0 + 1.f), deckH, PAL_TIMBER);
    }

    for (float wx = kNear + 3.f; wx < kFar - 1.f; wx += 8.f) {
        float gx, gy, dx, dy;
        project(wx, 0.f, gx, gy);
        project(wx, kDeck, dx, dy);
        float ht = gy - dy;
        if (ht < 6.f) continue;
        spr(art_.trestle, dx, dy + ht * 0.5f, ht, PAL_TIMBER, false, 0);
    }

    float rock0 = std::max(kNear - 16.f, left);
    float rock1 = std::min(kNear + 18.f, right);
    for (float wx = std::floor(rock0 / 4.f) * 4.f; wx < rock1; wx += 4.f) {
        float s0, top, s1, bot;
        project(wx, kDeck, s0, top);
        project(wx + 4.f, 0.f, s1, bot);
        sprBox(art_.rock, (s0 + s1) * 0.5f, top, std::max(3.f, s1 - s0 + 1.f), std::max(4.f, bot - top), PAL_ROCK);
    }
    for (float wx = kNear - 22.f; wx < kNear + 6.f; wx += 6.f) {
        float crag = kDeck + 4.f + std::sin(wx * 0.17f) * 3.f;
        float s0, top, s1, bot;
        project(wx, crag, s0, top);
        project(wx + 6.f, kDeck - 0.2f, s1, bot);
        if (bot - top < 4.f) continue;
        sprBox(art_.rock, (s0 + s1) * 0.5f, top, std::max(3.f, s1 - s0), bot - top, PAL_ROCK);
    }

    const float trees[] = {18.f, 36.f, 54.f, 74.f, 96.f, 112.f, kFar + 8.f, kFar + 18.f, kFar + 30.f};
    for (float tx : trees) {
        float px, py;
        project(tx, 0.f, px, py);
        float ht = std::clamp(4.2f * scale, 14.f, 78.f);
        int fog = tx > kFar ? 6 : 1;
        spr(art_.pine, px, py - ht * 0.46f, ht, PAL_PINE, tx > kMark, fog);
    }

    float gStep = std::clamp(28.f / scale, 3.2f, 8.f);
    float g0 = std::floor(left / gStep) * gStep;
    for (float wx = g0; wx < right; wx += gStep) {
        float px, py;
        project(wx + gStep * 0.5f, 0.f, px, py);
        float sw = gStep * scale + 1.5f;
        float tile = 26.f;
        int rows = 0;
        for (float y = py; y < gs::SCREEN_H + 2.f && rows < 7; y += tile - 1.f, rows++)
            sprBox(art_.grass, px, y, sw, tile, PAL_GRASS);
    }

    float shY = (x_ >= kNear && x_ <= kFar) ? kDeck : 0.f;
    if (h_ - shY < 16.f) {
        float px, py;
        project(x_, shY, px, py);
        float sh = std::clamp((2.4f + (h_ - shY) * 0.12f) * scale * 0.42f, 3.f, 28.f);
        spr(art_.shade, px, py, sh, PAL_DUST, false, int(std::min(12.f, (h_ - shY) * 1.1f)));
    }

    for (int i = 0; i < 2; i++) {
        float bx = 40.f + float(i) * 36.f + std::sin(anim_ * 0.7f + float(i)) * 6.f;
        float by = kDeck + 8.f + float(i) * 2.4f;
        float px, py;
        project(bx, by, px, py);
        int fr = int(anim_ * 3.f + float(i)) & 1;
        spr(art_.gull[fr], px, py, std::max(6.f, 0.7f * scale), PAL_SKY, i & 1, 2);
    }

    for (int i = 0; i < 3; i++) {
        float sxr = std::fmod(20.f + float(i) * 150.f - camX_ * scale * 0.18f, 640.f);
        if (sxr < -80.f) sxr += 640.f;
        spr(art_.ridge, sxr, 132.f, 28.f + float(i % 2) * 8.f, PAL_FAR, false, 9);
    }
    for (int i = 0; i < 4; i++) {
        float sxc = std::fmod(16.f + float(i) * 110.f - camX_ * scale * 0.06f + anim_ * 6.f, 480.f);
        if (sxc < -40.f) sxc += 480.f;
        spr(art_.cloud, sxc, 28.f + float(i % 3) * 16.f, 14.f + float(i % 2) * 4.f, PAL_SKY, i & 1, 1);
    }
    spr(art_.sun, 36.f, 30.f, 20.f, PAL_SKY, false, 0);

    if (mode_ == Mode::Title) {
        hudC(16, "UP CLIMB    DOWN DIVE", PAL_HUD);
        hudC(18, "Z  C  SPACE  SPOILER", PAL_HUD);
        hudC(19, "ON THE TIMBER IT IS THE BRAKE", PAL_AMBER);
        hudC(21, "STOP WITH THE WHEEL LEVEL", PAL_HUD);
        hudC(22, "WITH THE END OF THE PLATFORM", PAL_GOOD);
        hudC(23, "MISSING THE END FAILS THE LEG", PAL_AMBER);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START FLIES   ESC TITLE", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(24, why_, PAL_BAD);
        hudC(26, "START TRIES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(23, "LEVEL WITH THE PLATFORM", PAL_GOOD);
        char buf[40];
        std::snprintf(buf, sizeof buf, "%.1f S", legT_);
        hudC(25, buf, PAL_HUD);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "LVL %+5.1f", h_ - kDeck);
        int lp = std::fabs(h_ - kDeck) < 0.35f ? PAL_GOOD : PAL_HUD;
        hud(1, 1, buf, lp);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(12, 1, buf, v_ < 8.f ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "VS %+5.1f", vy_);
        hud(23, 1, buf, vy_ < -3.2f ? PAL_BAD : PAL_HUD);
        if (spoil_ > 0.4f) hud(33, 1, onDeck_ ? "BRAKE" : "SPOILER", PAL_AMBER);

        const char* line = "TO THE PLATFORM";
        int pal = PAL_HUD;
        bool atMark = std::fabs(x_ - kMark) <= kLevelTol;
        if (onDeck_ && atMark && v_ <= kStop) {
            line = "LEVEL";
            pal = PAL_GOOD;
        } else if (onDeck_ && atMark) {
            line = "LEVEL  BRAKE";
            pal = PAL_GOOD;
        } else if (onDeck_ && x_ < kMark) {
            std::snprintf(buf, sizeof buf, "END %3.0f M", std::max(0.f, kMark - x_));
            line = buf;
            pal = PAL_AMBER;
        } else if (onDeck_) {
            line = "PAST THE END";
            pal = PAL_BAD;
        } else if (x_ + kNose > kNear) {
            line = std::fabs(att_) > kAttMax ? "NOT LEVEL" : "MEET THE DECK";
            pal = std::fabs(h_ - kDeck) < 0.8f ? PAL_GOOD : PAL_AMBER;
        } else {
            std::snprintf(buf, sizeof buf, "PLAT %3.0f M", std::max(0.f, kNear - (x_ + kNose)));
            line = buf;
        }
        hudC(2, line, pal);
        if (hold_ > 0.02f) {
            int n = std::clamp(int(hold_ / kHoldNeed * 5.f + 0.001f), 0, 5);
            std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
            hudC(3, buf, PAL_GOOD);
        }
        hudC(26, "UP DOWN FLY    Z BRAKE", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(9, 10, 12));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.14f, 0.22f, 0.12f);
    sys.apu.setPatch(0, bellPatch());
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
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        chimeT_ += kDt;
        if (chimeT_ > 0.14f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.2f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0.f;
            if (chime_ > 7) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        x_ = kNear - 16.f;
        h_ = kDeck + 6.2f + std::sin(anim_ * 1.3f) * 0.35f;
        att_ = -0.1f + std::sin(anim_ * 0.8f) * 0.04f;
        v_ = 15.f;
        vy_ = -1.f;
        onDeck_ = false;
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
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO) ||
            pad.down(gs::BTN_X) || pad.down(gs::BTN_Z))
            spoil = 1.f;
        if (pad.accel > 0.08f) spoil = std::max(spoil, pad.accel);
        if (pad.brake > 0.08f) spoil = std::max(spoil, pad.brake);
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(420.f);
            draw();
            return;
        }
    }

    float path = std::atan2(vy_, std::max(8.f, v_));
    float wantAtt = onDeck_ ? 0.f : path * 0.35f + nose * 0.05f;
    att_ += (wantAtt - att_) * 0.35f;

    bool wasDeck = onDeck_;
    physics(nose, spoil);
    if (!wasDeck && onDeck_ && mode_ == Mode::Fly) {
        puffs_[puffN_ % 8] = {x_ - 1.2f, kDeck + 0.25f, 0.45f};
        puffN_++;
    }
    if (onDeck_ && v_ > 1.4f && mode_ == Mode::Fly && (sys.frame % 5) == 0) {
        puffs_[puffN_ % 8] = {x_ - kTail * 0.4f, kDeck + 0.2f, 0.35f};
        puffN_++;
    }
    audio();
    draw();
}

}  // namespace gliderplat
