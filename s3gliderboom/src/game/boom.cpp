#include "game/boom.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace gboom {
namespace {

constexpr double DT = 1.0 / 60.0;
constexpr double kG = 6.2;
constexpr double kKick = 0.40;
constexpr double kSnub = 0.105;
constexpr double kSnubMax = 1.85;
constexpr double kSmash = -16.0;
constexpr double kHold = 0.50;
constexpr double kSlingX = -0.50;
constexpr double kSlingY = -2.22;
constexpr double kVyNose = 4.8;
constexpr double kVyBias = 0.62;
constexpr double kWantH = 15.2;

enum class Hit { None, Bed, Edge, Carried, Stick, Smash, Water, Bank };

struct Body {
    double x = 0, y = 0, vx = 0, vy = 0;
};

struct Box {
    double x0, x1, y0, y1;
};

struct HitInfo {
    Hit hit = Hit::None;
    double land = 0;
    double impact = 0;
};

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

double minSettled() { return kBedX0 + kDriveHalf; }
double maxSettled() { return kBedX1 - kDriveHalf; }

Box stickL() { return {kBoomX0 - 0.05, kBoomX0 + kStickW, 0.30, kPostTop}; }
Box stickR() { return {kBoomX1 - kStickW, kBoomX1 + 0.05, 0.30, kPostTop}; }

bool overlapX(double a0, double a1, double b0, double b1) { return a1 > b0 + 0.06 && a0 < b1 - 0.06; }

bool inStick(double x, double y) {
    auto hit = [&](Box b) { return x > b.x0 - 0.12 && x < b.x1 + 0.12 && y > b.y0 - 0.12 && y < b.y1 + 0.12; };
    return hit(stickL()) || hit(stickR());
}

void fall(Body& d) {
    d.vy -= kG * DT;
    d.x += d.vx * DT;
    d.y += d.vy * DT;
}

HitInfo probe(const Body& d) {
    HitInfo info;
    info.impact = d.vy;
    info.land = d.x;
    const double bottom = d.y - kDriveThick * 0.5;
    const double top = d.y + kDriveThick * 0.5;
    const double left = d.x - kDriveHalf;
    const double right = d.x + kDriveHalf;
    const Box L = stickL();
    const Box R = stickR();
    const bool yIn = top > L.y0 + 0.04 && bottom < L.y1 - 0.04;
    if (yIn && (overlapX(left, right, L.x0, L.x1) || overlapX(left, right, R.x0, R.x1))) {
        info.hit = Hit::Stick;
        return info;
    }
    if (bottom <= kDeckY && overlapX(left, right, kBoomX0, kBoomX1)) {
        const double shift = clampd(std::max(d.vx, 0.0) * kSnub, 0.0, kSnubMax);
        const double settled = d.x + shift;
        info.land = settled;
        const bool body = left >= kBedX0 - 0.02 && right <= kBedX1 + 0.02;
        const bool rest = (settled - kDriveHalf) >= kBedX0 - 0.02 && (settled + kDriveHalf) <= kBedX1 + 0.02;
        if (!body) info.hit = Hit::Edge;
        else if (!rest) info.hit = Hit::Carried;
        else if (d.vy < kSmash) info.hit = Hit::Smash;
        else info.hit = Hit::Bed;
        return info;
    }
    if (bottom <= 0.0) {
        info.hit = (d.x > kShoreX && d.x < kFarShoreX) ? Hit::Water : Hit::Bank;
        info.land = d.x;
        return info;
    }
    return info;
}

// Same fall as the live drive. A Bed result is a delivery the integrator will repeat.
HitInfo forecast(Body b) {
    for (int i = 0; i < 480; i++) {
        fall(b);
        HitInfo info = probe(b);
        if (info.hit != Hit::None) return info;
        if (b.y < -30.0) break;
    }
    HitInfo miss;
    miss.land = b.x;
    miss.impact = b.vy;
    return miss;
}

Body sling(double x, double h, double v, double vy, double att) {
    const double ca = std::cos(att), sa = std::sin(att);
    Body b;
    b.x = x + kSlingX * ca - kSlingY * sa;
    b.y = h + kSlingX * sa + kSlingY * ca;
    b.vx = v;
    b.vy = vy - kKick;
    return b;
}

void sampleHull(double x, double h, double att, bool& ground, bool& stick) {
    static const double kP[][2] = {
        {3.15, 0.06}, {2.15, 0.20}, {0.35, -0.56}, {-0.2, 0.72}, {-3.40, 0.10}, {-3.10, 1.70},
    };
    const double ca = std::cos(att), sa = std::sin(att);
    ground = false;
    stick = false;
    for (const auto& p : kP) {
        const double sx = x + p[0] * ca - p[1] * sa;
        const double sy = h + p[0] * sa + p[1] * ca;
        if (sy <= 0.05) ground = true;
        if (inStick(sx, sy)) stick = true;
    }
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.18f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.16f, 0.62f, 0.18f};
    p.op[1] = {2.f, 0.32f, 0.02f, 0.2f, 0.4f, 0.16f};
    p.op[2] = {3.f, 0.1f, 0.02f, 0.22f, 0.22f, 0.2f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f};
    p.vol = 0.22f;
    p.tone = 1700.f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (phase_ == Phase::Settle) return 3;
    if (phase_ == Phase::Fall) return 2;
    return 1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    phase_ = Phase::Carry;
    won_ = false;
    over_ = false;
    gliderDown_ = false;
    wet_ = false;
    hold_ = 0;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    time_ = 0;
    x_ = 106;
    h_ = 13.2;
    v_ = 14.2;
    vy_ = -0.35;
    att_ = -0.06;
    nose_ = 0;
    spoil_ = 0;
    camX_ = 126;
    camH_ = 6.4;
    camS_ = 300.0 / 58.0;
}

void Game::startRun() {
    mode_ = Mode::Fly;
    phase_ = Phase::Carry;
    won_ = false;
    over_ = false;
    gliderDown_ = false;
    wet_ = false;
    hold_ = 0;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    puffN_ = 0;
    time_ = 0;
    x_ = 22;
    h_ = 16.4;
    v_ = 14.6;
    vy_ = -0.25;
    att_ = -0.04;
    nose_ = 0;
    spoil_ = 0;
    dx_ = dy_ = dvx_ = dvy_ = 0;
    for (Puff& p : puffs_) p = {};
    camX_ = x_ + 8;
    camH_ = 6.2;
    camS_ = 300.0 / 68.0;
    blip(640.f);
}

void Game::pilot(double& nose, double& spoil, bool& release) const {
    spoil = 0;
    release = false;
    if (phase_ != Phase::Carry || gliderDown_) {
        nose = 0.85;
        return;
    }
    nose = clampd(kVyBias / kVyNose + (kWantH - h_) * 0.24 + (0.0 - vy_) * 0.20, -1.0, 1.0);
    if (v_ > 17.0) spoil = 0.4;
    if (h_ < 8.0 || h_ > 21.0) return;
    const HitInfo info = forecast(sling(x_, h_, v_, vy_, att_));
    if (info.hit == Hit::Bed) release = true;
}

void Game::win() {
    if (mode_ != Mode::Fly) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "the drive is on the boom";
    banner_ = "DELIVERED";
    chime_ = 0;
    chimeT_ = 0;
    sys_->rumble(0.22f, 0.1f, 140);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why, const char* banner) {
    if (mode_ != Mode::Fly) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    why_ = why;
    banner_ = banner;
    if (h_ < 0) h_ = 0;
    sys_->rumble(0.55f, 0.3f, 170);
    sys_->setLight(180, 30, 20);
    sys_->apu.noiseBurst(0.48f, 420.f, 0.28f);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::physics(double nose, double spoil, bool release) {
    if (mode_ != Mode::Fly) return;
    time_ += DT;
    nose_ = clampd(nose, -1.0, 1.0);
    spoil_ = clampd(spoil, 0.0, 1.0);

    if (release && phase_ == Phase::Carry && !gliderDown_) {
        const Body b = sling(x_, h_, v_, vy_, att_);
        dx_ = b.x;
        dy_ = b.y;
        dvx_ = b.vx;
        dvy_ = b.vy;
        phase_ = Phase::Fall;
        puffs_[puffN_++ % 8] = {b.x, b.y, 0.45};
        blip(920.f);
        sys_->rumble(0.12f, 0.04f, 40);
    }

    if (!gliderDown_) {
        const double lift = clampd(v_ / 13.0, 0.55, 1.08);
        const double vyCmd = (nose_ * kVyNose - kVyBias) * lift - spoil_ * 3.1;
        vy_ += (vyCmd - vy_) * std::min(1.0, 4.2 * DT);
        double vTrim = 14.6 - std::max(vy_, 0.0) * 0.8 + std::max(-vy_, 0.0) * 0.28 - spoil_ * 5.0;
        vTrim = clampd(vTrim, 9.2, 20.0);
        v_ += (vTrim - v_) * (0.65 * DT);
        v_ = clampd(v_, 0.0, 24.0);
        if (v_ < 9.0) vy_ -= (9.0 - v_) * 1.4 * DT;
        if (h_ > 20.0) vy_ -= (h_ - 20.0) * 3.0 * DT;
        x_ += v_ * DT;
        h_ += vy_ * DT;
        const double wantAtt = std::atan2(vy_, std::max(v_, 9.0));
        att_ += (wantAtt - att_) * 0.22;
        att_ = clampd(att_, -0.45, 0.45);
        if (!std::isfinite(x_) || !std::isfinite(h_) || !std::isfinite(v_)) {
            fail("lost the air", "LOST");
            return;
        }
        bool ground = false, stick = false;
        sampleHull(x_, h_, att_, ground, stick);
        if (stick && (phase_ == Phase::Carry || phase_ == Phase::Fall)) {
            fail("flew into the boom", "HIT BOOM");
            return;
        }
        if (ground && phase_ == Phase::Carry) {
            h_ = std::max(0.0, h_);
            wet_ = x_ > kShoreX && x_ < kFarShoreX;
            fail("ditched with the drive", "DITCHED");
            return;
        }
        if (ground && phase_ == Phase::Fall) {
            gliderDown_ = true;
            h_ = 0.45;
            vy_ = 0;
            v_ = std::min(v_, 3.0);
            sys_->apu.noiseBurst(0.28f, 520.f, 0.18f);
        }
    } else {
        v_ = std::max(0.0, v_ - 6.0 * DT);
        x_ += v_ * DT;
        h_ = 0.45;
        vy_ = 0;
        att_ += (0.04 - att_) * 0.12;
    }

    if (phase_ == Phase::Fall) {
        Body b{dx_, dy_, dvx_, dvy_};
        fall(b);
        dx_ = b.x;
        dy_ = b.y;
        dvx_ = b.vx;
        dvy_ = b.vy;
        const HitInfo info = probe(b);
        if (info.hit == Hit::Bed) {
            dx_ = info.land;
            dy_ = kDeckY + kDriveThick * 0.5;
            dvx_ = dvy_ = 0;
            phase_ = Phase::Settle;
            hold_ = 0;
            puffs_[puffN_++ % 8] = {dx_, kDeckY + 0.4, 0.55};
            sys_->apu.noiseBurst(0.32f, 180.f, 0.16f);
            blip(240.f);
            sys_->rumble(0.3f, 0.12f, 90);
        } else if (info.hit == Hit::Smash) {
            dx_ = info.land;
            dy_ = kDeckY + kDriveThick * 0.5;
            fail("smashed the drive", "SMASHED");
            return;
        } else if (info.hit == Hit::Edge) {
            dy_ = kDeckY + kDriveThick * 0.5;
            fail("on the edge of the boom", "EDGE");
            return;
        } else if (info.hit == Hit::Carried) {
            dx_ = info.land;
            dy_ = kDriveThick * 0.35;
            wet_ = true;
            fail("carried off the boom", "CARRIED OFF");
            return;
        } else if (info.hit == Hit::Stick) {
            fail("broke the boom", "BROKE IT");
            return;
        } else if (info.hit == Hit::Water) {
            const bool beside = (dx_ + kDriveHalf) > kBoomX0 - 7.0 && (dx_ - kDriveHalf) < kBoomX1 + 7.0;
            wet_ = true;
            dy_ = kDriveThick * 0.28;
            fail(beside ? "beside the boom" : "in the water", beside ? "BESIDE" : "IN THE WATER");
            return;
        } else if (info.hit == Hit::Bank) {
            dy_ = kDriveThick * 0.35;
            fail(dx_ < kShoreX ? "dropped short" : "dropped long", dx_ < kShoreX ? "SHORT" : "LONG");
            return;
        }
    }

    if (phase_ == Phase::Settle) {
        hold_ += DT;
        if (hold_ >= kHold) {
            win();
            return;
        }
    }

    if (phase_ == Phase::Carry) {
        const Body s = sling(x_, h_, v_, vy_, att_);
        if (s.x > maxSettled() + 0.8) {
            dx_ = s.x;
            dy_ = s.y;
            fail("still carrying the drive", "STILL ABOARD");
            return;
        }
    }

    if (time_ > 36.0) fail("too late", "TOO LATE");
}

void Game::sky() {
    uint16_t zen = gs::rgb4(4, 7, 13);
    uint16_t mid = gs::rgb4(8, 12, 15);
    uint16_t hor = gs::rgb4(14, 12, 8);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 6, 5), 0.35f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(11, 14, 9), 0.28f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.55f ? lerpC(zen, mid, t / 0.55f) : lerpC(mid, hor, (t - 0.55f) / 0.45f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
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
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -8) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::sprAnchor(const gs::Mipped& m, float ax, float ay, float sx, float sy, float destH, int pal, bool flip,
                     int fog) {
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
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -8) return;
    s.img = m.pick(destH);
    s.pal = uint8_t(pal);
    s.hflip = flip;
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
    if (s.x > gs::SCREEN_W + 4 || s.x + s.w < -4 || s.y > gs::SCREEN_H || s.y + s.h < 0) return;
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::blit(const Spr& s, float wx, float wy, float scale, int pal, bool flip, int fog) {
    if (s.ppm < 0.05f || s.img.h < 1) return;
    float sx = 160.f + (wx - float(camX_)) * scale;
    float sy = 118.f - (wy - float(camH_)) * scale;
    float dest = float(s.img.h) / s.ppm * scale;
    sprAnchor(s.img, s.ax, s.ay, sx, sy, dest, pal, flip, fog);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !s[0]) return;
    const float adv = 18.f * scale;
    const int n = int(std::strlen(s));
    x -= float(n) * adv * 0.5f;
    for (int i = 0; i < n; i++) {
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

    const bool framing = phase_ != Phase::Carry || mode_ == Mode::Win || mode_ == Mode::Fail;
    double focusX = x_ + clampd(v_ * 0.42, 4.0, 11.0);
    double focusH = clampd(h_ * 0.34 + 3.5, 4.4, 8.2);
    double span = x_ < 80.0 ? 68.0 : 50.0;
    Body load = (phase_ == Phase::Carry) ? sling(x_, h_, v_, vy_, att_) : Body{dx_, dy_, dvx_, dvy_};
    if (framing && mode_ != Mode::Title) {
        focusX = load.x;
        focusH = 5.0;
        span = 36.0;
    }
    if (mode_ == Mode::Title) {
        camX_ = 126;
        camH_ = 6.4;
        camS_ = 300.0 / 58.0;
    } else {
        const double k = 0.14;
        camX_ += (focusX - camX_) * k;
        camH_ += (focusH - camH_) * k;
        camS_ += ((300.0 / span) - camS_) * k;
    }
    const float sc = float(camS_);
    const double bob = (phase_ == Phase::Settle || mode_ == Mode::Win) ? std::sin(sys_->frame * 0.09) * 0.06 : 0.0;

    if (mode_ == Mode::Title) {
        text("GLIDER BOOM", 160, 22, 1.05f, PAL_HUD);
        text("DELIVER THE DRIVE", 160, 46, 0.62f, PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 28, 1.1f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(banner_, 160, 24, 1.0f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("DELIVERED", 160, 22, 1.05f, PAL_GOOD);
    }

    if (wet_ && mode_ == Mode::Fail) blit(art_.splash, float(load.x), float(std::max(0.3, load.y)), sc, PAL_SPLASH);

    if (!(gliderDown_ && mode_ == Mode::Fail && phase_ == Phase::Carry)) {
        int fi = int(std::lround((att_ + 0.34) / 0.17));
        fi = std::clamp(fi, 0, 4);
        blit(art_.ship[fi], float(x_), float(h_), sc, PAL_SHIP);
    }
    blit(art_.drive, float(load.x), float(load.y + bob), sc, PAL_DRIVE);

    if (mode_ == Mode::Title) {
        const double mid = 0.5 * (minSettled() + maxSettled());
        blit(art_.chev, float(mid), float(kDeckY + 0.45), sc, PAL_GOOD);
    } else if (mode_ == Mode::Fly && phase_ == Phase::Carry) {
        const HitInfo info = forecast(sling(x_, h_, v_, vy_, att_));
        if (info.hit != Hit::None) {
            int pal = PAL_AMBER;
            double cy = 0.3;
            if (info.hit == Hit::Bed) {
                pal = PAL_GOOD;
                cy = kDeckY + 0.4;
            } else if (info.hit == Hit::Smash || info.hit == Hit::Stick) {
                pal = PAL_BAD;
                cy = kDeckY + 0.3;
            } else if (info.hit == Hit::Edge || info.hit == Hit::Carried) {
                cy = kDeckY + 0.3;
            }
            blit(art_.chev, float(info.land), float(cy), sc, pal);
        }
    }

    blit(art_.sign, float(kBoomX0 - 0.15), 3.6f, sc, PAL_SIGN);
    blit(art_.stick, float(kBoomX0 + kStickW * 0.5), 0.f, sc, PAL_BOOM);
    blit(art_.stick, float(kBoomX1 - kStickW * 0.5), 0.f, sc, PAL_BOOM, true);

    float bedL, bedY, bedR;
    {
        float sx = 160.f + (float(kBedX0) - float(camX_)) * sc;
        float sy = 118.f - (float(kDeckY + bob) - float(camH_)) * sc;
        bedL = sx;
        bedY = sy;
        bedR = 160.f + (float(kBedX1) - float(camX_)) * sc;
    }
    const float plankH = std::max(3.f, 0.26f * sc);
    sprBox(art_.plank, (bedL + bedR) * 0.5f, bedY, std::max(6.f, bedR - bedL), plankH, PAL_BOOM);

    for (double lx = kBoomX0 + 2.1; lx < kBoomX1 - 1.2; lx += 4.15)
        blit(art_.log, float(lx), float(kDeckY - 0.22 + bob), sc, PAL_BOOM);

    blit(art_.mill, float(kFarShoreX + 7.5), 0.f, sc, PAL_MILL);
    const double pines[] = {14, 30, 48, 156, 170};
    for (int i = 0; i < 5; i++) blit(art_.pine, float(pines[i]), 0.f, sc, PAL_TREE, i & 1);
    const double reeds[] = {kShoreX - 1.5, kShoreX + 1.2, kFarShoreX - 1.0, kFarShoreX + 1.4};
    for (double rx : reeds) blit(art_.reed, float(rx), 0.f, sc, PAL_TREE);

    for (const Puff& p : puffs_) {
        if (p.life <= 0) continue;
        blit(art_.splash, float(p.x), float(p.y), sc * (0.7f + float(1.0 - p.life)), PAL_SPLASH, false,
             int((1.0 - p.life) * 8));
    }

    if (!gliderDown_ && h_ < 17.0) {
        float sh = (2.4f + float(kDriveHalf) * 0.15f) * sc * float(std::max(0.25, 1.0 - h_ / 20.0));
        float sx = 160.f + (float(x_) - float(camX_)) * sc;
        float sy = 118.f - (0.f - float(camH_)) * sc;
        spr(art_.shade.img, sx, sy, std::max(3.f, sh), PAL_WATER);
    }

    const int tileFrame = (int(sys_->frame) / 12) & 1;
    const double step = 5.0;
    const double left = camX_ - 190.0 / camS_;
    const double right = camX_ + 190.0 / camS_;
    const double start = std::floor(left / step) * step;
    for (double wx = start; wx < right; wx += step) {
        const bool water = (wx + step) > kShoreX && wx < kFarShoreX;
        const float sx = 160.f + (float(wx + step * 0.5) - float(camX_)) * sc;
        const float sy = 118.f - (0.f - float(camH_)) * sc;
        const float sw = float(step) * sc + 1.6f;
        const float sh = 4.0f * sc + 1.f;
        const gs::Mipped& tile = water ? art_.water[tileFrame] : art_.grass[tileFrame];
        int rows = 0;
        for (float y = sy; y < gs::SCREEN_H + 2.f && rows < 6; y += sh - 1.f, ++rows)
            sprBox(tile, sx, y, sw, sh, water ? PAL_WATER : PAL_BANK);
    }

    for (int i = 0; i < 4; i++) {
        float sx = std::fmod(24.f + float(i) * 130.f - float(camX_) * sc * 0.06f + float(sys_->frame) * 0.15f, 560.f);
        if (sx < -40.f) sx += 560.f;
        spr(art_.cloud.img, sx, 30.f + float(i % 3) * 16.f, 16.f + float(i % 2) * 6.f, PAL_SKY, i & 1, 2);
    }
    for (int i = 0; i < 2; i++) {
        float flap = ((sys_->frame / 8) + i) & 1;
        float bx = std::fmod(30.f + float(i) * 170.f + float(sys_->frame) * 0.55f, 420.f) - 30.f;
        spr(art_.bird[int(flap)].img, bx, 48.f + float(i) * 18.f, 11.f, PAL_BIRD, false, 1);
    }
    for (int i = 0; i < 3; i++) {
        float sx = std::fmod(10.f + float(i) * 180.f - float(camX_) * sc * 0.12f, 640.f);
        if (sx < -80.f) sx += 640.f;
        spr(art_.hill.img, sx, 132.f, 32.f + float(i % 2) * 8.f, PAL_SKY, false, 8);
    }
    spr(art_.sun.img, 36.f, 26.f, 22.f, PAL_SKY);

    if (mode_ == Mode::Title) {
        hudC(16, "THE WHOLE DRIVE HAS TO REST", PAL_HUD);
        hudC(17, "ON THE BOOM. THE EDGE IS A MISS.", PAL_AMBER);
        hudC(19, "Z X W   RELEASE THE DRIVE", PAL_HUD);
        hudC(20, "UP CLIMB     DOWN DIVE", PAL_HUD);
        hudC(21, "C SPACE      SPOILER", PAL_HUD);
        if ((sys_->frame / 30) % 2 == 0) hudC(24, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START FLIES    ESC TITLE", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(23, why_, PAL_BAD);
        hudC(26, "START TRIES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(22, "THE DRIVE IS ON THE BOOM", PAL_GOOD);
        char buf[32];
        std::snprintf(buf, sizeof buf, "%.1f S", time_);
        hudC(24, buf, PAL_HUD);
        hudC(26, "START FOR THE TITLE", PAL_HUD);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "ALT %4.1f", std::max(0.0, h_));
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(14, 1, buf, PAL_HUD);
        if (spoil_ > 0.4) hud(28, 1, "SPOILER", PAL_AMBER);

        const char* line = "CARRYING THE DRIVE";
        int pal = PAL_HUD;
        char cue[32];
        if (phase_ == Phase::Fall) {
            line = "FALLING";
            pal = PAL_AMBER;
        } else if (phase_ == Phase::Settle) {
            line = "ON THE BOOM";
            pal = PAL_GOOD;
        } else {
            const HitInfo info = forecast(load);
            const double lo = minSettled();
            const double hi = maxSettled();
            if (info.hit == Hit::Bed) {
                line = "RELEASE";
                pal = PAL_GOOD;
            } else if (info.hit == Hit::Smash) {
                line = "TOO STEEP";
                pal = PAL_BAD;
            } else if (info.hit == Hit::Carried) {
                line = "TOO FAST";
                pal = PAL_AMBER;
            } else if (info.hit == Hit::Stick) {
                line = "STICK";
                pal = PAL_BAD;
            } else if (info.hit == Hit::Edge) {
                line = info.land < 0.5 * (lo + hi) ? "EDGE SHORT" : "EDGE LONG";
                pal = PAL_AMBER;
            } else if (info.land < lo) {
                std::snprintf(cue, sizeof cue, "SHORT %3.0f", std::max(0.0, lo - info.land));
                line = cue;
                pal = PAL_AMBER;
            } else {
                std::snprintf(cue, sizeof cue, "LONG %3.0f", std::max(0.0, info.land - hi));
                line = cue;
                pal = PAL_AMBER;
            }
        }
        hudC(2, line, pal);
        if (phase_ == Phase::Settle) {
            int n = int(hold_ / kHold * 5.0 + 0.001);
            n = std::clamp(n, 0, 5);
            std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
            hudC(3, buf, PAL_GOOD);
        } else if (phase_ == Phase::Carry) {
            std::snprintf(buf, sizeof buf, "BOOM %3.0f M", std::max(0.0, kBoomX0 - x_));
            hudC(3, buf, PAL_HUD);
        }
        hudC(26, "Z RELEASE   UP DOWN FLY   C SPOILER", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(7, 9, 12));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.18f, 0.12f);
    sys.apu.setPatch(0, bellPatch());
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (beep_ > 0.f) {
        beep_ -= float(DT);
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    for (Puff& p : puffs_)
        if (p.life > 0) p.life = std::max(0.0, p.life - DT);

    if (chime_ >= 0) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        chimeT_ += float(DT);
        if (chimeT_ > 0.13f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.2f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 7) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        x_ = 106.0;
        h_ = 13.2 + std::sin(sys.frame * 0.05) * 0.28;
        v_ = 14.2;
        vy_ = -0.35;
        att_ = -0.06 + std::sin(sys.frame * 0.04) * 0.05;
        phase_ = Phase::Carry;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_Y)) {
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
        draw();
        sys.apu.noise(0.f, 800.f, false);
        sys.apu.tone(2, 0.f, 0.f);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    double nose = 0, spoil = 0;
    bool release = false;
    if (bot_) {
        pilot(nose, spoil, release);
    } else {
        if (pad.down(gs::BTN_UP)) nose += 1;
        if (pad.down(gs::BTN_DOWN)) nose -= 1;
        if (std::fabs(pad.axisY) > 0.18f) nose = pad.axisY;
        nose = clampd(nose, -1.0, 1.0);
        if (pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO) || pad.down(gs::BTN_X)) spoil = 1;
        if (pad.accel > 0.12f) spoil = std::max(spoil, double(pad.accel));
        if (pad.brake > 0.12f) spoil = std::max(spoil, double(pad.brake));
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_Y)) release = true;
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(420.f);
            draw();
            return;
        }
    }

    physics(nose, spoil, release);
    if (mode_ == Mode::Fly) {
        float wind = float(clampd((v_ - 8.0) / 14.0, 0.0, 1.0)) * (spoil_ > 0.4 ? 0.07f : 0.045f);
        sys.apu.noise(wind, 680.f + float(v_) * 28.f, false);
        if (phase_ == Phase::Carry && v_ < 10.5) sys.apu.tone(2, 170.f, 0.03f);
        else sys.apu.tone(2, 0.f, 0.f);
        if (phase_ == Phase::Settle) sys.setLight(40, 170, 80);
        else if (vy_ < -4.0) sys.setLight(170, 50, 30);
        else sys.setLight(40, 90, 160);
    }
    draw();
}

}  // namespace gboom
