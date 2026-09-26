#include "slip.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace gslip {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kStartX = 24.f;
constexpr float kStartH = 17.f;
constexpr float kStartV = 18.2f;
constexpr float kJetty0 = 104.f;
constexpr float kJetty1 = 132.f;
constexpr float kJettyH = 4.6f;
constexpr float kPileFoot = -1.7f;
constexpr float kRock0 = 132.f;
constexpr float kRock1 = 156.f;
constexpr float kRockH = 1.15f;
constexpr float kSlip0 = 156.f;
constexpr float kSlip1 = 300.f;
constexpr float kLip0 = 300.f;
constexpr float kLip1 = 314.f;
constexpr float kLipH = 0.9f;
constexpr float kQuay0 = 314.f;
constexpr float kQuay1 = 360.f;
constexpr float kQuayH = 8.5f;
constexpr float kAim = 220.f;
constexpr float kCrew0 = 36.f;
constexpr float kTideDrop = 1.35f;
constexpr float kMouthOut = 20.f;
constexpr float kMouthIn = 14.f;
constexpr float kEbb = 1.35f;
constexpr float kNose = 3.3f;
constexpr float kTail = 4.5f;
constexpr float kHard = -5.0f;
constexpr float kMinV = 7.0f;
constexpr float kStop = 0.18f;
constexpr float kHoldNeed = 0.42f;
constexpr float kPitch = 6.4f;
constexpr float kTrim = 1.75f;
constexpr float kSpoilSink = 4.6f;
constexpr float kXClear = 136.f;
constexpr float kHClear = 8.0f;

float clamp(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float liftOf(float v) { return clamp((v - 7.2f) / 12.5f, 0.28f, 1.05f); }

float stallOf(float v) { return std::max(0.f, 11.2f - v) * 0.72f; }

bool overlaps(float a0, float a1, float b0, float b1) { return a1 > b0 && a0 < b1; }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 6;
    p.fb = 0.12f;
    p.op[0] = {1.f, 1.f, 0.012f, 0.22f, 0.55f, 0.28f};
    p.op[1] = {2.01f, 0.22f, 0.02f, 0.26f, 0.3f, 0.22f};
    p.op[2] = {3.5f, 0.06f, 0.02f, 0.3f, 0.15f, 0.25f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f};
    p.vol = 0.18f;
    p.tone = 1400.f;
    p.echo = 0.18f;
    return p;
}

}  // namespace

float Game::tide() const { return clamp(crew_ / kCrew0, 0.f, 1.f); }

float Game::waterLine() const { return -(1.f - tide()) * kTideDrop; }

void Game::mouth(float& a, float& b) const {
    float u = 1.f - tide();
    a = kSlip0 + u * kMouthOut;
    b = kSlip1 - u * kMouthIn;
}

bool Game::onSlip() const {
    if (!afloat_) return false;
    float a, b;
    mouth(a, b);
    return x_ >= a && x_ <= b;
}

bool Game::fuselageHits(float a, float b, float top) const {
    return h_ < top && overlaps(x_ - kTail, x_ + kNose, a, b);
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (afloat_) return 3;
    if (x_ + kNose >= kSlip0 && x_ - kTail <= kSlip1) return 2;
    return 1;
}

int Game::wingFrame(float att) const {
    int fi = int(std::lround((0.34f - att) / 0.17f));
    return std::clamp(fi, 0, 4);
}

void Game::rivalAt(float& rx, float& rh) const {
    float u = 1.f - tide();
    float s = u * u * (3.f - 2.f * u);
    float far = kQuay0 + 18.f;
    float home = (kSlip0 + kSlip1) * 0.5f;
    rx = far + (home - far) * s;
    rh = waterLine();
}

void Game::begin() {
    x_ = kStartX;
    h_ = kStartH;
    v_ = kStartV;
    vy_ = -2.1f;
    att_ = -0.08f;
    nose_ = 0.f;
    spoil_ = 0.f;
    afloat_ = false;
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
}

void Game::startRun() {
    begin();
    mode_ = Mode::Fly;
    snap_ = true;
    blip(520.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    beep_ = 0.07f;
}

void Game::win() {
    if (mode_ != Mode::Fly) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    v_ = 0.f;
    vy_ = 0.f;
    h_ = waterLine();
    why_ = "berthed";
    banner_ = "BERTHED";
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.2f, 0.06f, 160);
    sys_->setLight(30, 160, 90);
}

void Game::fail(const char* why, const char* banner) {
    if (mode_ != Mode::Fly) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    banner_ = banner;
    if (h_ < waterLine()) h_ = waterLine();
    shake_ = 1.f;
    sys_->rumble(0.5f, 0.28f, 180);
    sys_->setLight(170, 36, 24);
    sys_->apu.noiseBurst(0.42f, 360.f, 0.28f);
}

void Game::pilot(float& nose, float& spoil) const {
    if (afloat_) {
        nose = 0.f;
        spoil = 1.f;
        return;
    }
    float wh = waterLine();
    float alt = h_ - wh;
    if (x_ + kNose > kJetty0 - 8.f && x_ - kTail < kJetty1 + 3.f && h_ < kJettyH + 1.7f) {
        nose = 1.f;
        spoil = 0.f;
        return;
    }
    float aimH = wh - 0.15f;
    auto wantAt = [&](float x) {
        if (x < 78.f) {
            float u = clamp((x - kStartX) / (78.f - kStartX), 0.f, 1.f);
            return kStartH + (11.2f - kStartH) * u;
        }
        if (x < kXClear) {
            float u = clamp((x - 78.f) / (kXClear - 78.f), 0.f, 1.f);
            return 11.2f + (kHClear - 11.2f) * u;
        }
        if (x < kAim) {
            float u = clamp((x - kXClear) / (kAim - kXClear), 0.f, 1.f);
            return kHClear + (aimH - kHClear) * u;
        }
        return aimH;
    };
    float w0 = wantAt(x_);
    float w1 = wantAt(x_ + 2.f);
    float pathVy = (w1 - w0) / 2.f * std::max(v_, 11.f);
    float err = w0 - h_;
    float sw = pathVy + clamp(err * 1.05f - (vy_ - pathVy) * 0.5f, -2.1f, 2.3f);
    sw = clamp(sw, -3.4f, 3.0f);
    if (alt < 2.6f) sw = std::max(sw, -1.3f);
    if (alt < 1.25f) sw = std::max(sw, -0.72f);
    if (alt < 0.55f) sw = std::max(sw, -0.36f);
    if (x_ > kAim - 6.f && alt > 0.85f) sw = std::min(sw, -0.65f);
    if (x_ > kAim + 24.f && alt > 0.45f) sw = std::min(sw, -1.05f);
    if (race_ > 16.f && alt > 1.4f) sw = std::min(sw, -1.25f);
    if (x_ > kSlip1 - 40.f && alt > 0.4f) sw = std::min(sw, -1.45f);
    float lift = liftOf(v_);
    float stall = stallOf(v_);
    spoil = (v_ > 26.f && alt > 6.f) ? 0.5f : 0.f;
    nose = (sw + kTrim * lift + spoil * kSpoilSink + stall) / (kPitch * std::max(lift, 0.22f));
    nose = clamp(nose, -1.f, 1.f);
}

void Game::physics(float nose, float spoil) {
    nose_ = clamp(nose, -1.f, 1.f);
    spoil_ = clamp(spoil, 0.f, 1.f);
    race_ += kDt;
    crew_ = std::max(0.f, crew_ - kDt);
    float wh = waterLine();

    if (!afloat_) {
        float lift = liftOf(v_);
        float stall = stallOf(v_);
        float vyCmd = (nose_ * kPitch - kTrim) * lift - spoil_ * kSpoilSink - stall;
        vy_ += (vyCmd - vy_) * std::min(1.f, 5.f * kDt);
        v_ += (-0.09f - 0.16f * std::max(vy_, 0.f) + 0.04f * std::max(-vy_, 0.f) - spoil_ * 1.05f) * kDt;
        v_ = clamp(v_, 0.f, 32.f);
        x_ += v_ * kDt;
        h_ += vy_ * kDt;
        if (!std::isfinite(x_) || !std::isfinite(h_) || !std::isfinite(v_)) {
            fail("lost the air", "LOST");
            return;
        }
        if (fuselageHits(kJetty0, kJetty1, kJettyH)) {
            fail("hit the jetty", "THE JETTY");
            return;
        }
        if (fuselageHits(kQuay0, kQuay1, kQuayH)) {
            fail("hit the quay", "THE QUAY");
            return;
        }
        if (fuselageHits(kRock0, kRock1, kRockH)) {
            fail("caught the rocks", "THE ROCKS");
            return;
        }
        if (fuselageHits(kLip0, kLip1, kLipH)) {
            fail("past the slip", "PAST THE SLIP");
            return;
        }
        if (h_ <= wh + 0.04f) {
            float hitVy = vy_;
            h_ = wh;
            float a, b;
            mouth(a, b);
            if (x_ < kJetty0) {
                fail("ditched in the bay", "THE BAY");
                return;
            }
            if (x_ < a) {
                fail(x_ < kSlip0 ? "caught the rocks" : "on the sill", x_ < kSlip0 ? "THE ROCKS" : "THE SILL");
                return;
            }
            if (x_ > b) {
                fail(x_ > kSlip1 ? "past the slip" : "on the sill", x_ > kSlip1 ? "PAST THE SLIP" : "THE SILL");
                return;
            }
            if (hitVy < kHard) {
                fail("hit too hard", "TOO HARD");
                return;
            }
            if (v_ < kMinV) {
                fail("too slow", "TOO SLOW");
                return;
            }
            afloat_ = true;
            vy_ = 0.f;
            shake_ = 0.45f;
            sys_->rumble(0.28f, 0.1f, 90);
            sys_->apu.noiseBurst(0.22f, 180.f, 0.18f);
            puffs_[puffN_ % 8] = {x_ - 1.4f, wh + 0.2f, 0.55f};
            puffN_++;
        }
    } else {
        h_ = wh;
        vy_ = 0.f;
        float decel = 1.7f + spoil_ * 8.8f;
        v_ = std::max(0.f, v_ - decel * kDt);
        float a, b;
        mouth(a, b);
        bool hook = spoil_ > 0.45f && v_ <= kStop && x_ >= a && x_ <= b;
        if (hook) {
            v_ = 0.f;
            hold_ += kDt;
            if (hold_ >= kHoldNeed) {
                win();
                return;
            }
        } else {
            hold_ = 0.f;
            x_ += v_ * kDt;
            x_ -= (1.f - tide()) * kEbb * kDt;
        }
        mouth(a, b);
        if (fuselageHits(kQuay0, kQuay1, kQuayH)) {
            fail("hit the quay", "THE QUAY");
            return;
        }
        if (fuselageHits(kLip0, kLip1, kLipH) || x_ > kSlip1) {
            fail("past the slip", "PAST THE SLIP");
            return;
        }
        if (x_ > b) {
            fail("the ebb took you", "THE EBB");
            return;
        }
        if (x_ < a || fuselageHits(kRock0, kRock1, kRockH)) {
            fail("on the sill", "THE SILL");
            return;
        }
    }
    if (mode_ == Mode::Fly && crew_ <= 0.f) fail("the tide turned", "TIDE TURNED");
}

void Game::aimCamera(bool scenic) {
    if (scenic) {
        camX_ = 176.f;
        camH_ = 2.6f;
        camS_ = 4.15f;
        anchorY_ = 158.f;
        return;
    }
    float above = std::max(0.f, h_ - waterLine());
    float wantS = clamp(4.85f + (4.f - above) * 0.05f, 4.55f, 5.7f);
    float lead = 0.f;
    if (mode_ == Mode::Fly && !afloat_) lead = clamp(v_ * 0.4f, 5.f, 13.f);
    else if (mode_ == Mode::Fly && afloat_) lead = 2.2f;
    float wantX = x_ + lead;
    float wantH = waterLine() + std::max(0.75f, above * 0.72f);
    float wantA = clamp(156.f - above * 2.1f, 84.f, 164.f);
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        wantS = 5.05f;
        wantX = x_;
        wantH = waterLine() + 1.6f;
        wantA = 142.f;
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
    uint16_t zen = gs::rgb4(3, 6, 12);
    uint16_t mid = gs::rgb4(6, 10, 14);
    uint16_t hor = gs::rgb4(14, 11, 8);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(13, 6, 4), 0.45f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(10, 14, 8), 0.4f);
    float fall = 1.f - tide();
    hor = lerpC(hor, gs::rgb4(10, 8, 7), fall * 0.35f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.58f ? lerpC(zen, mid, t / 0.58f) : lerpC(mid, hor, (t - 0.58f) / 0.42f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = true;
    sys_->vdp.setFogColor(gs::rgb4(10, 12, 12));
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip, int fog, int clip) {
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
    s.clipY = int16_t(std::clamp(clip, 0, gs::SCREEN_H));
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

void Game::sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal, int clip) {
    if (w < 1.2f || h < 1.2f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(top)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 4 || s.x + s.w < -4 || s.y > gs::SCREEN_H || s.y + s.h < 0) return;
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.clipY = int16_t(std::clamp(clip, 0, gs::SCREEN_H));
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
    if (shake_ > 0.f) shy = std::cos(t_ * 41.f) * 2.2f * shake_;
    sx = ax + (wx - camX_) * camS_;
    if (shake_ > 0.f) sx += std::sin(t_ * 47.f) * 2.8f * shake_;
    sy = anchorY_ + shy - (wy - camH_) * camS_;
}

void Game::worldBox(float x0, float y0, float x1, float y1, const gs::Mipped& m, int pal, float ax, int clip) {
    float sx0, sy0, sx1, sy1;
    project(x0, y0, ax, sx0, sy0);
    project(x1, y1, ax, sx1, sy1);
    float left = std::min(sx0, sx1);
    float top = std::min(sy0, sy1);
    float w = std::max(2.f, std::fabs(sx1 - sx0));
    float h = std::max(2.f, std::fabs(sy1 - sy0));
    sprBox(m, left + w * 0.5f, top, w, h, pal, clip);
}

void Game::draw(float cx, float ch, float catt, bool craft) {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();
    const float scale = camS_;
    const float ax = 146.f;
    const float wh = waterLine();
    float waterSx, waterSy;
    project(0.f, wh, ax, waterSx, waterSy);
    int clip = int(std::lround(waterSy));
    clip = std::clamp(clip, 0, gs::SCREEN_H);

    if (mode_ == Mode::Title) {
        text("S3 GLIDER SLIP", 160, 14, 0.68f, PAL_HUD);
        text("BERTH BEFORE THE TIDE", 160, 32, 0.42f, PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 22, 1.0f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(banner_, 160, 18, 0.9f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("BERTHED", 160, 16, 1.0f, PAL_GOOD);
        text("IN THE SLIP", 160, 40, 0.52f, PAL_HUD);
    }

    auto feet = [&](const gs::Mipped& m, float wx, float baseY, float worldH, int pal, int fog = 0, int cly = gs::SCREEN_H) {
        float sx, sy;
        project(wx, baseY, ax, sx, sy);
        float ht = std::max(3.f, worldH * scale);
        spr(m, sx, sy - ht * 0.5f, ht, pal, false, fog, cly);
    };

    if (craft) {
        const Ship& ship = art_.boat[wingFrame(catt)];
        float sx, sy;
        project(cx, ch, ax, sx, sy);
        float dest = float(ship.img.h) / ship.ppm * scale;
        sprAnchor(ship, sx, sy, std::max(10.f, dest), PAL_SHIP);
    }
    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        float sx, sy;
        project(p.x, p.y, ax, sx, sy);
        spr(art_.spray, sx, sy, 7.f + (1.f - p.life) * 12.f, PAL_SPRAY, false, int((1.f - p.life) * 8));
    }
    if (craft && ch - wh < 14.f && ch > wh + 0.15f) {
        float sx, sy;
        project(cx, wh, ax, sx, sy);
        float sh = clamp((1.8f + (ch - wh) * 0.15f) * scale * 0.45f, 4.f, 26.f);
        spr(art_.shade, sx, sy, sh, PAL_SPRAY, false, int(std::min(12.f, ch - wh)));
    }

    float rx, rh;
    if (mode_ == Mode::Title) {
        rx = 214.f;
        rh = wh;
    } else {
        rivalAt(rx, rh);
    }
    {
        float sx, sy;
        project(rx, rh, ax, sx, sy);
        float dest = 26.f * scale / 13.f;
        spr(art_.skiff, sx, sy - dest * 0.42f, dest, PAL_RIVAL, false, 0);
    }

    float left = camX_ - (ax + 40.f) / scale;
    float right = camX_ + (gs::SCREEN_W - ax + 40.f) / scale;
    int pennant = int(t_ * 4.f) % 3;
    if (pennant < 0) pennant = 0;
    if (kSlip0 > left - 6.f && kSlip0 < right + 6.f) {
        feet(art_.pennant[pennant], kSlip0 + 2.f, kPileFoot, 4.6f - kPileFoot, PAL_TOWN, 0, clip);
        feet(art_.sign, kSlip0 + 7.f, wh, 2.3f, PAL_TOWN, 0);
        feet(art_.staff, kSlip0 + 12.f, kPileFoot, 1.6f - kPileFoot, PAL_TOWN, 0, clip);
    }
    for (float px = kJetty0 + 3.f; px < kJetty1 - 1.f; px += 4.6f) {
        if (px < left - 4.f || px > right + 4.f) continue;
        float dist = std::fabs(px - camX_);
        int fog = dist > 55.f ? int(clamp((dist - 55.f) / 12.f, 0.f, 6.f)) : 0;
        feet(art_.pier, px, kPileFoot, kJettyH - kPileFoot, PAL_WOOD, fog, clip);
    }
    if (kJetty1 > left && kJetty0 < right) {
        worldBox(kJetty0, kJettyH - 0.38f, kJetty1, kJettyH, art_.plank, PAL_WOOD, ax, clip);
    }
    for (float dx = kSlip0 + 8.f; dx < kSlip1 - 6.f; dx += 22.f) {
        if (dx < left - 4.f || dx > right + 4.f) continue;
        feet(art_.dolphin, dx, kPileFoot, 2.5f - kPileFoot, PAL_WOOD, 0, clip);
    }
    if (kQuay0 < right + 8.f && kQuay0 > left - 16.f) {
        feet(art_.quay, kQuay0 + 8.f, kPileFoot, kQuayH - kPileFoot, PAL_TOWN, 1, clip);
    }
    if (8.f > left - 6.f && 8.f < right + 6.f) feet(art_.light, 6.f, -0.4f, 7.4f, PAL_TOWN, 2, clip);
    auto buoyAt = [&](float wx, float base) {
        if (wx < left - 3.f || wx > right + 3.f) return;
        float bob = std::sin(t_ * 2.4f + wx) * 0.08f;
        feet(art_.buoy, wx, base + bob, 1.7f, PAL_TOWN, 0);
    };
    buoyAt(88.f, wh);
    for (float bx = kSlip0 + 16.f; bx < kSlip1 - 8.f; bx += 18.f) {
        float a, b;
        mouth(a, b);
        float base = (bx >= a && bx <= b) ? wh : 0.f;
        buoyAt(bx, base);
    }

    float a, b;
    mouth(a, b);
    if (b - a > 3.f) worldBox(a, wh + 0.55f, b, wh + 0.08f, art_.plank, PAL_AMBER, ax, gs::SCREEN_H);

    if (a - kSlip0 > 0.8f) worldBox(kSlip0, 0.15f, a, wh - 0.2f, art_.mud, PAL_HARBOR, ax, gs::SCREEN_H);
    if (kSlip1 - b > 0.8f) worldBox(b, 0.15f, kSlip1, wh - 0.2f, art_.mud, PAL_HARBOR, ax, gs::SCREEN_H);
    if (kRock1 > left && kRock0 < right) worldBox(kRock0, kRockH, kRock1, wh - 1.5f, art_.rock, PAL_HARBOR, ax, gs::SCREEN_H);
    if (kLip1 > left && kLip0 < right) worldBox(kLip0, kLipH, kLip1, wh - 1.2f, art_.rock, PAL_HARBOR, ax, gs::SCREEN_H);
    if (12.f > left) worldBox(-8.f, 2.4f, 14.f, wh - 2.f, art_.bluff, PAL_HARBOR, ax, clip);

    int phase = int(t_ * 5.f) & 1;
    float step = clamp(34.f / scale, 3.2f, 11.f);
    float start = std::floor(left / step) * step;
    const float tileH = 24.f;
    for (float wx = start; wx < right; wx += step) {
        float mid = wx + step * 0.5f;
        float sx, sy;
        project(mid, wh, ax, sx, sy);
        float sw = step * scale + 2.f;
        const gs::Mipped* img = &art_.bay[phase];
        if (mid >= kSlip0 && mid <= kSlip1) img = &art_.slipW[phase];
        float y = sy;
        if (y < -tileH) {
            int n = int((-y) / tileH);
            y += float(n) * tileH;
        }
        int rows = 0;
        for (; y < gs::SCREEN_H + 2.f && rows < 8; y += tileH - 1.f, rows++) sprBox(*img, sx, y, sw, tileH, PAL_HARBOR);
    }

    int flap = int(t_ * 6.f) & 1;
    for (int i = 0; i < 4; i++) {
        float gx = 40.f + float(i) * 70.f + std::fmod(t_ * (9.f + float(i)), 40.f);
        float gy = 22.f + float(i % 3) * 2.8f;
        float sx, sy;
        project(gx, gy, ax, sx, sy);
        spr(art_.gull[flap], sx, sy, 8.f, PAL_GULL, i & 1, 1);
    }
    for (int i = 0; i < 4; i++) {
        float sx = std::fmod(20.f + float(i) * 130.f - camX_ * scale * 0.04f + t_ * 8.f, 500.f);
        if (sx < -60.f) sx += 500.f;
        spr(art_.cloud, sx, 22.f + float(i % 3) * 14.f, 16.f + float(i % 2) * 6.f, PAL_SKY, i & 1, 1);
    }
    spr(art_.sun, 274.f, 30.f, 22.f, PAL_SKY, false, 0);
    for (int i = 0; i < 4; i++) {
        float sx = std::fmod(10.f + float(i) * 150.f - camX_ * scale * 0.12f, 640.f);
        if (sx < -120.f) sx += 640.f;
        spr(art_.head, sx, 108.f, 36.f + float(i % 2) * 8.f, PAL_FAR, false, 8);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(8, "UP DOWN  PITCH", PAL_HUD);
        hudC(9, "Z X C SPACE  BRAKE", PAL_HUD);
        hudC(11, "BERTH IN THE SLIP", PAL_GOOD);
        hudC(12, "BEFORE THE TIDE TURNS", PAL_GOOD);
        hudC(14, "THE CLOCK IS THE OTHER CREW", PAL_AMBER);
        hudC(16, "TIDE 0:36", PAL_AMBER);
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
        int leftS = int(std::ceil(crew_ - 1e-3f));
        if (leftS < 0) leftS = 0;
        std::snprintf(buf, sizeof buf, "TIDE HAD %d S", leftS);
        hudC(25, buf, PAL_GOOD);
    } else {
        float above = std::max(0.f, h_ - wh);
        std::snprintf(buf, sizeof buf, "ALT %4.1f", above);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(12, 1, buf, (!afloat_ && v_ < 11.f) ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "VS %+5.1f", vy_);
        hud(23, 1, buf, vy_ < -3.8f ? PAL_BAD : vy_ > 0.8f ? PAL_GOOD : PAL_HUD);
        int cs = int(std::ceil(crew_ - 1e-4f));
        if (cs < 0) cs = 0;
        std::snprintf(buf, sizeof buf, "TIDE %d:%02d", cs / 60, cs % 60);
        int cpal = cs <= 10 ? PAL_BAD : PAL_AMBER;
        if (cs > 10 || (sys_->frame / 10) % 2 == 0) hud(28, 2, buf, cpal);
        if (spoil_ > 0.4f) hud(1, 2, afloat_ ? "BRAKE" : "SPOILER", PAL_AMBER);
        const char* line = "FIND THE SLIP";
        int pal = PAL_HUD;
        if (afloat_ && v_ <= kStop && spoil_ > 0.4f) {
            line = "HOLD THE BERTH";
            pal = PAL_GOOD;
        } else if (afloat_) {
            line = "BRAKE TO BERTH";
            pal = PAL_GOOD;
        } else if (x_ >= kSlip0 && x_ <= kSlip1) {
            line = "SET DOWN IN THE SLIP";
            pal = PAL_AMBER;
            if (kSlip1 - x_ < 36.f) {
                line = "SLIP ENDING";
                pal = PAL_BAD;
            }
        } else if (x_ < kSlip0) {
            std::snprintf(buf, sizeof buf, "SLIP %3.0f M", std::max(0.f, kSlip0 - x_));
            line = buf;
            pal = x_ + kNose > kJetty0 && x_ < kJetty1 ? PAL_AMBER : PAL_HUD;
            if (x_ + kNose > kJetty0 && h_ < kJettyH + 3.f) {
                line = "CLEAR THE JETTY";
                pal = PAL_AMBER;
            }
        } else {
            line = "TOO LONG";
            pal = PAL_BAD;
        }
        hudC(26, line, pal);
        if (!afloat_ && v_ < 11.f) hudC(3, "STALL", PAL_BAD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(10, 12, 12));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.14f, 0.2f, 0.12f);
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
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.5f);
    for (Puff& p : puffs_)
        if (p.life > 0.f) p.life = std::max(0.f, p.life - kDt * 1.25f);

    if (chime_ >= 0) {
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        chimeT_ += kDt;
        if (chimeT_ > 0.14f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.16f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0.f;
            if (chime_ > 7) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        aimCamera(true);
        float bob = std::sin(t_ * 1.2f);
        draw(168.f, 6.4f + bob * 0.22f, -0.06f + std::sin(t_ * 0.6f) * 0.04f, true);
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            blip(640.f);
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
            blip(480.f);
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
        if (afloat_ && pad.down(gs::BTN_DOWN)) spoil = 1.f;
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(360.f);
            draw(x_, h_, att_, true);
            return;
        }
    }

    int secNow = int(std::floor(std::max(0.f, crew_)));
    if (lastSec_ >= 0 && secNow != lastSec_ && secNow <= 9) blip(secNow <= 3 ? 180.f : 320.f);
    lastSec_ = secNow;

    physics(nose, spoil);
    if (mode_ == Mode::Fly || mode_ == Mode::Win) {
        float path = std::atan2(vy_, std::max(10.f, v_));
        float want = afloat_ ? 0.02f : path * 0.82f + nose_ * 0.07f;
        att_ += (want - att_) * 0.28f;
        att_ = clamp(att_, -0.48f, 0.48f);
    }
    if (spoil_ > 0.45f && !spoilWas_) blip(760.f);
    spoilWas_ = spoil_ > 0.45f;
    if (afloat_ && v_ > 2.4f && mode_ == Mode::Fly && (sys.frame % 4) == 0) {
        puffs_[puffN_ % 8] = {x_ - 1.6f, waterLine() + 0.15f, 0.4f};
        puffN_++;
    }

    if (mode_ == Mode::Fly) {
        float wind = clamp((v_ - 8.f) / 22.f, 0.f, 1.f) * (afloat_ ? 0.04f : 0.03f);
        if (spoil_ > 0.4f && !afloat_) wind += 0.028f;
        sys.apu.noise(wind, afloat_ ? 280.f : 820.f + v_ * 16.f, false);
        if (!afloat_ && v_ < 11.f) sys.apu.tone(2, 150.f + std::max(0.f, 11.f - v_) * 16.f, 0.04f);
        else if (!afloat_ && vy_ > 0.7f) sys.apu.tone(2, 440.f + vy_ * 24.f, 0.02f);
        else sys.apu.tone(2, 0.f, 0.f);
        if (afloat_ && onSlip()) sys.setLight(30, 150, 110);
        else if (vy_ < -3.6f) sys.setLight(170, 40, 28);
        else if (crew_ < 10.f) sys.setLight(170, 100, 30);
        else sys.setLight(30, 90, 150);
    }

    aimCamera(false);
    draw(x_, h_, att_, true);
}

}  // namespace gslip
