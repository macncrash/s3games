#include "game/glider.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace gbox {
namespace {

constexpr double DT = 1.0 / 60.0;
constexpr double kBox0 = 108.0;
constexpr double kBox1 = 162.0;
constexpr double kNose = 4.2;
constexpr double kTail = 4.6;
constexpr double kGear = 1.22;
constexpr double kHard = -4.8;
constexpr double kHoldNeed = 0.42;
constexpr double kStop = 0.42;
constexpr double kTd = 128.0;

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

double liftOf(double v) { return clampd((v - 6.0) / 12.0, 0.22, 1.08); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.2f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.16f, 0.65f, 0.18f};
    p.op[1] = {2.f, 0.35f, 0.02f, 0.2f, 0.4f, 0.16f};
    p.op[2] = {3.f, 0.12f, 0.02f, 0.22f, 0.25f, 0.2f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f};
    p.vol = 0.22f;
    p.tone = 1800.f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05) return 3;
    if (x_ + kNose > kBox0 && x_ - kTail < kBox1) return 2;
    return 1;
}

bool Game::hullInside() const {
    return (x_ - kTail) >= kBox0 - 0.04 && (x_ + kNose) <= kBox1 + 0.04;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    ground_ = false;
    hold_ = 0;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    camX_ = 128;
    camH_ = 5.5;
    camS_ = 4.15;
}

void Game::startRun() {
    x_ = 0;
    h_ = 16;
    v_ = 18;
    vy_ = -1.1;
    att_ = -0.08;
    nose_ = 0;
    spoil_ = 0;
    ground_ = false;
    touched_ = false;
    hold_ = 0;
    time_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    puffN_ = 0;
    for (Puff& p : puffs_) p = {};
    mode_ = Mode::Fly;
    camX_ = x_ + 12;
    camH_ = h_;
    camS_ = 2.6;
    blip(620.f);
}

void Game::pilot(double& nose, double& spoil) const {
    if (ground_) {
        nose = 0;
        spoil = 1;
        return;
    }
    double dist = kTd - x_;
    double hw = x_ < kTd - 20.0 ? std::max(1.6, dist * 0.15) : std::max(0.2, std::max(dist, 0.2) * 0.05);
    double sw = clampd(-(h_ - hw) * 0.9, -4.0, 1.8);
    if (h_ < 2.3) sw = std::max(sw, -1.05);
    if (h_ < 1.05) sw = std::max(sw, -0.48);
    spoil = 0;
    if (h_ > hw + 1.0 && h_ > 2.6) spoil = clampd((h_ - hw) / 5.5, 0.0, 0.8);
    if (v_ > 20.5 && h_ > 3.0) spoil = std::max(spoil, 0.25);
    if (h_ < 2.5) spoil *= 0.15;
    double lift = liftOf(v_);
    double stall = std::max(0.0, 12.2 - v_) * 0.62;
    nose = (sw + 1.45 * lift + spoil * 5.0 + stall) / (6.4 * lift);
    nose = clampd(nose, -1.0, 1.0);
}

void Game::win() {
    if (mode_ != Mode::Fly) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "stopped inside the box";
    banner_ = "STOPPED";
    chime_ = 0;
    chimeT_ = 0;
    h_ = 0;
    vy_ = 0;
    sys_->rumble(0.2f, 0.08f, 120);
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
    sys_->rumble(0.55f, 0.3f, 160);
    sys_->setLight(180, 30, 20);
    sys_->apu.noiseBurst(0.45f, 520.f, 0.28f);
}

void Game::physics(double nose, double spoil) {
    nose_ = clampd(nose, -1.0, 1.0);
    spoil_ = clampd(spoil, 0.0, 1.0);
    time_ += DT;
    if (!ground_) {
        double lift = liftOf(v_);
        double stall = std::max(0.0, 12.2 - v_) * 0.62;
        double vyCmd = (nose_ * 6.4 - 1.45) * lift - spoil_ * 5.0 - stall;
        vy_ += (vyCmd - vy_) * std::min(1.0, 5.0 * DT);
        v_ += (-0.48 - 0.12 * std::max(vy_, 0.0) + 0.05 * std::max(-vy_, 0.0) - spoil_ * 1.7) * DT;
        v_ = clampd(v_, 0.0, 26.0);
        double vx = std::max(0.0, v_ * 0.96);
        if (h_ < 3.0 && vy_ < -2.0) {
            double c = (3.0 - h_) / 3.0;
            vy_ = -2.0 + (vy_ + 2.0) * (1.0 - 0.7 * c);
        }
        x_ += vx * DT;
        h_ += vy_ * DT;
        if (!std::isfinite(x_) || !std::isfinite(h_) || !std::isfinite(v_)) {
            fail("lost the air", "LOST");
            return;
        }
        if (h_ <= 0.06) {
            if (vy_ < kHard || v_ < 7.5) {
                h_ = 0;
                fail(v_ < 7.5 ? "too slow" : "hit too hard", v_ < 7.5 ? "TOO SLOW" : "TOO HARD");
                return;
            }
            ground_ = true;
            h_ = 0;
            touched_ = true;
            sys_->rumble(0.28f, 0.1f, 70);
            sys_->apu.noiseBurst(0.22f, 240.f, 0.16f);
        } else if ((x_ - kTail) > kBox1 + 6.0) {
            fail("missed the box", "MISSED");
            return;
        }
    } else {
        v_ = std::max(0.0, v_ - (1.25 + spoil_ * 9.0) * DT);
        x_ += v_ * DT;
        h_ = 0;
        vy_ = 0;
        bool inside = hullInside();
        if (v_ <= kStop && inside) hold_ += DT;
        else hold_ = 0;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
        if (x_ + kNose > kBox1 + 0.25) {
            fail("rolled out of the box", "ROLLED OUT");
            return;
        }
        if (v_ <= 0.05 && !inside) {
            fail("stopped outside the box", "OUTSIDE");
            return;
        }
    }
    if (time_ > 45.0) fail("too late", "TOO LATE");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::sky() {
    uint16_t zen = gs::rgb4(4, 7, 13);
    uint16_t mid = gs::rgb4(8, 12, 15);
    uint16_t hor = gs::rgb4(15, 12, 8);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 7, 6), 0.35f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(12, 15, 10), 0.25f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.58f ? lerpC(zen, mid, t / 0.58f) : lerpC(mid, hor, (t - 0.58f) / 0.42f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
}

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

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 18.f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::draw(double x, double h, double att, double vy, double spd, bool craft) {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();

    // Earlier sprites sit on top, so banners and the glider are emitted first.
    if (mode_ == Mode::Title) {
        text("GLIDER BOX", 160, 24, 1.15f, PAL_HUD);
        text("STOP INSIDE", 160, 48, 0.72f, PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 28, 1.15f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(banner_, 160, 26, 1.05f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("STOPPED", 160, 24, 1.15f, PAL_GOOD);
    }

    const bool framing = mode_ == Mode::Win || hold_ > 0.02;
    double span = framing ? 50.0 : clampd(26.0 + h * 3.4, 28.0, 150.0);
    double wantS = 230.0 / span;
    double lead = framing ? 0.0 : clampd(spd * 0.55, 2.0, 16.0);
    if (ground_ && !framing) lead = std::min(lead, 5.0);
    double wantX = framing ? (kBox0 + kBox1) * 0.5 : x + lead;
    double wantH = framing ? 3.2 : std::max(2.4, h * 0.72 + 1.6);
    if (mode_ == Mode::Title) {
        wantX = 130;
        wantH = 3.4;
        wantS = 3.85;
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
    } else {
        double k = 0.18;
        camX_ += (wantX - camX_) * k;
        camH_ += (wantH - camH_) * k;
        camS_ += (wantS - camS_) * k;
    }
    const float scale = float(camS_);
    const float ax = 160.f;
    const float ay = 86.f;
    auto project = [&](double wx, double wy, float& sx, float& sy) {
        sx = ax + float(wx - camX_) * scale;
        sy = ay - float(wy - camH_) * scale;
    };

    if (craft) {
        int fi = int(std::lround((0.40 - att) / 0.20));
        fi = std::clamp(fi, 0, 4);
        const Ship& ship = art_.ship[fi];
        float sx, sy;
        project(x, h + kGear, sx, sy);
        float dest = float(ship.img.h) / ship.ppm * scale;
        sprAnchor(ship.img, ship.ax, ship.ay, sx, sy, std::max(8.f, dest), PAL_SHIP);
    }
    for (const Puff& p : puffs_) {
        if (p.life <= 0) continue;
        float sx, sy;
        project(p.x, 0.2, sx, sy);
        spr(art_.dust, sx, sy - 4.f, 8.f + float(1.0 - p.life) * 10.f, PAL_DUST, false, int((1.0 - p.life) * 10));
    }
    if (craft && h < 14.0) {
        float sx, sy;
        project(x, 0, sx, sy);
        float sh = std::clamp((2.2f + float(h) * 0.15f) * scale * 0.55f, 3.f, 36.f);
        spr(art_.shade, sx, sy, sh, PAL_DUST, false, int(std::min(12.0, h * 1.2)));
    }

    auto postAt = [&](double wx) {
        float sx, sy;
        project(wx, 0, sx, sy);
        int fr = int(time_ * 4.0) & 1;
        float ht = std::clamp(4.6f * scale, 14.f, 78.f);
        spr(art_.post[fr], sx, sy - ht * 0.5f, ht, PAL_POST, wx > (kBox0 + kBox1) * 0.5, 0);
    };
    postAt(kBox0);
    postAt(kBox1);

    float sockX, sockY;
    project(kBox0 - 8.0, 0, sockX, sockY);
    int sock = int(time_ * 5.0) % 3;
    if (sock < 0) sock = 0;
    float sockH = std::clamp(3.4f * scale, 12.f, 52.f);
    spr(art_.sock[sock], sockX, sockY - sockH * 0.5f, sockH, PAL_POST, false, 0);

    float nearL, nearY, farR, farY;
    project(kBox0, 0, nearL, nearY);
    project(kBox1, 0, farR, farY);
    float depth = std::clamp(16.f + scale * 2.4f, 18.f, 46.f);
    float boxTop = nearY - depth;
    float boxW = std::max(8.f, farR - nearL);
    float boxCx = (nearL + farR) * 0.5f;
    // Paint, border, and the word sit under the posts (posts were emitted first).
    float signX, signY;
    project(kBox0 + 14.0, 0, signX, signY);
    spr(art_.sign, signX, boxTop + depth * 0.46f, std::clamp(depth * 0.55f, 12.f, 30.f), PAL_SIGN, false, 0);
    for (int i = 0; i < 4; i++) {
        float sx = nearL + boxW * (0.2f + 0.2f * float(i));
        bool inward = sx < boxCx;
        spr(art_.chev, sx, nearY - 8.f, std::max(7.f, depth * 0.28f), PAL_POST, !inward, 0);
    }
    sprBox(art_.edge, boxCx, boxTop, boxW, 4.f, PAL_POST);
    sprBox(art_.edge, boxCx, nearY - 4.f, boxW, 5.f, PAL_POST);
    sprBox(art_.edge, nearL, boxTop, 5.f, depth, PAL_POST);
    sprBox(art_.edge, farR, boxTop, 5.f, depth, PAL_POST);
    sprBox(art_.pad, boxCx, boxTop + 3.f, std::max(4.f, boxW - 8.f), std::max(4.f, depth - 7.f), PAL_PAD);

    const double bushes[] = {18, 42, 68, 92, kBox1 + 6, kBox1 + 14, kBox1 + 22};
    for (double bxw : bushes) {
        float sx, sy;
        project(bxw, 0, sx, sy);
        float ht = 1.6f * scale;
        spr(art_.bush, sx, sy - ht * 0.45f, std::max(6.f, ht), PAL_FAR, false, 3);
    }

    float left = float(camX_) - (ax + 20.f) / scale;
    float right = float(camX_) + (gs::SCREEN_W - ax + 30.f) / scale;
    float step = std::clamp(32.f / scale, 2.6f, 9.f);
    float start = std::floor(left / step) * step;
    for (float wx = start; wx < right; wx += step) {
        float sx, sy;
        project(wx + step * 0.5, 0, sx, sy);
        float sw = step * scale + 1.5f;
        float tile = 28.f;
        int rows = 0;
        for (float y = sy; y < gs::SCREEN_H + 2.f && rows < 8; y += tile - 1.f, rows++)
            sprBox(art_.salt, sx, y, sw, tile, PAL_SALT);
    }

    for (int i = 0; i < 4; i++) {
        float sx = std::fmod(30.f + float(i) * 180.f - float(camX_) * scale * 0.2f, 760.f);
        if (sx < -100.f) sx += 760.f;
        spr(art_.mesa, sx, 120.f, 34.f + float(i % 2) * 6.f, PAL_FAR, false, 8);
    }
    for (int i = 0; i < 4; i++) {
        float sx = std::fmod(40.f + float(i) * 120.f - float(camX_) * scale * 0.08f + float(time_) * 8.f, 520.f);
        if (sx < -40.f) sx += 520.f;
        spr(art_.cloud, sx, 32.f + float(i % 3) * 14.f, 16.f + float(i % 2) * 5.f, PAL_SKY, i & 1, 2);
    }
    spr(art_.sun, 28.f, 30.f, 22.f, PAL_SKY, false, 0);

    if (mode_ == Mode::Title) {
        hudC(16, "UP CLIMB    DOWN DIVE", PAL_HUD);
        hudC(18, "Z  C  SPACE  SPOILER", PAL_HUD);
        hudC(19, "SAME BUTTON BRAKES THE WHEEL", PAL_AMBER);
        hudC(21, "THE WHOLE GLIDER HAS TO STOP", PAL_HUD);
        hudC(22, "INSIDE THE PAINTED BOX", PAL_GOOD);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START FLIES   ESC TITLE", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(24, why_, PAL_BAD);
        hudC(26, "START TRIES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(23, "INSIDE THE BOX", PAL_GOOD);
        char buf[40];
        std::snprintf(buf, sizeof buf, "%.1f S", time_);
        hudC(25, buf, PAL_HUD);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "ALT %4.1f", std::max(0.0, h));
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", spd);
        hud(12, 1, buf, spd < 10.0 ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "VS %+5.1f", vy);
        hud(23, 1, buf, vy < -3.2 ? PAL_BAD : vy > 0.4 ? PAL_GOOD : PAL_HUD);
        if (spoil_ > 0.4) hud(33, 1, ground_ ? "BRAKE" : "SPOILER", PAL_AMBER);

        const char* line = "TO THE BOX";
        int pal = PAL_HUD;
        if (ground_ && hullInside() && v_ > kStop) {
            line = "INSIDE  BRAKE";
            pal = PAL_GOOD;
        } else if (hullInside()) {
            line = "IN THE BOX";
            pal = PAL_GOOD;
        } else if (ground_ && (x_ + kNose) > kBox1) {
            line = "TOO LONG";
            pal = PAL_BAD;
        } else if (ground_ && (x_ - kTail) < kBox0) {
            line = "TOO SHORT";
            pal = PAL_BAD;
        } else if (x_ + kNose > kBox0) {
            line = "GET IN AND STOP";
            pal = PAL_AMBER;
        } else {
            std::snprintf(buf, sizeof buf, "BOX %3.0f M", std::max(0.0, kBox0 - (x_ + kNose)));
            line = buf;
        }
        hudC(2, line, pal);
        if (hold_ > 0.02) {
            int n = int(hold_ / kHoldNeed * 5.0 + 0.001);
            n = std::clamp(n, 0, 5);
            std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
            hudC(3, buf, PAL_GOOD);
        }
        hudC(26, "UP DOWN FLY    Z BRAKE", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(10, 9, 8));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.2f, 0.12f);
    sys.apu.setPatch(0, bellPatch());
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ != Mode::Pause) {
        // Title clock only waves the sock. Flight time lives in physics.
        if (mode_ == Mode::Title) time_ += DT;
    }
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
        double bob = std::sin(time_ * 1.3) * 0.25;
        draw(96.0, 5.6 + bob * 0.4, -0.18 + std::sin(time_ * 0.8) * 0.05, -1.7, 16.0, true);
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
        draw(x_, h_, att_, vy_, v_, true);
        if (pad.pressed(gs::BTN_START)) {
            blip(620.f);
            mode_ = Mode::Fly;
        } else if (pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        draw(x_, h_, att_, 0, v_, true);
        sys.apu.noise(0.f, 800.f, false);
        sys.apu.tone(2, 0.f, 0.f);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Win && pad.pressed(gs::BTN_MODE)) showTitle();
            else if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    double nose = 0, spoil = 0;
    if (bot_) {
        pilot(nose, spoil);
    } else {
        if (pad.down(gs::BTN_UP)) nose += 1;
        if (pad.down(gs::BTN_DOWN)) nose -= 1;
        if (std::fabs(pad.axisY) > 0.18f) nose = pad.axisY;
        nose = clampd(nose, -1.0, 1.0);
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO) ||
            pad.down(gs::BTN_X))
            spoil = 1;
        if (pad.accel > 0.08f) spoil = std::max(spoil, double(pad.accel));
        if (pad.brake > 0.08f) spoil = std::max(spoil, double(pad.brake));
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(420.f);
            draw(x_, h_, att_, vy_, v_, true);
            return;
        }
    }

    bool wasGround = ground_;
    physics(nose, spoil);
    if (mode_ == Mode::Fly || mode_ == Mode::Win) {
        double path = std::atan2(vy_, std::max(8.0, v_));
        double want = ground_ ? 0.02 : path + nose_ * 0.08;
        att_ += (want - att_) * 0.35;
    }
    if (!wasGround && ground_ && mode_ == Mode::Fly) {
        // Wheel chirp already fired. Leave a puff at the tail.
        puffs_[puffN_ % 8] = {x_ - kTail, 0.45};
        puffN_++;
    }
    if (ground_ && v_ > 1.2 && mode_ == Mode::Fly) {
        if ((sys.frame % 5) == 0) {
            puffs_[puffN_ % 8] = {x_ - kTail * 0.6, 0.35};
            puffN_++;
        }
    }

    if (mode_ == Mode::Fly) {
        float wind = float(std::clamp((v_ - 6.0) / 18.0, 0.0, 1.0)) * (spoil_ > 0.4 && !ground_ ? 0.07f : 0.04f);
        sys.apu.noise(wind, 900.f + float(v_) * 40.f, false);
        if (!ground_ && v_ < 12.0) sys.apu.tone(2, 180.f, 0.035f);
        else sys.apu.tone(2, 0.f, 0.f);
        if (hullInside() && ground_) sys.setLight(40, 170, 80);
        else if (vy_ < -3.4) sys.setLight(170, 40, 20);
        else sys.setLight(40, 80, 160);
    }
    draw(x_, h_, att_, vy_, v_, true);
}

}  // namespace gbox
