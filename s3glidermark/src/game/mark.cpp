#include "game/mark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace gmark {
namespace {

constexpr double DT = 1.0 / 60.0;
constexpr double kMark = 176.0;
constexpr double kHalf = 7.0;
constexpr double kShore = 98.0;
constexpr double kSlope = 0.140;
constexpr double kFlare = 24.0;
constexpr double kFlareS = 0.055;
constexpr double kHard = -3.2;
constexpr double kMinV = 11.0;
constexpr double kSettle = 0.28;
constexpr double kKp = 0.8;
constexpr double kKd = 0.3;

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

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
    p.op[0] = {1.f, 1.f, 0.01f, 0.18f, 0.55f, 0.22f};
    p.op[1] = {2.01f, 0.32f, 0.02f, 0.22f, 0.35f, 0.2f};
    p.op[2] = {3.f, 0.1f, 0.02f, 0.24f, 0.2f, 0.22f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f};
    p.vol = 0.22f;
    p.tone = 1700.f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (ground_) return 3;
    double wx = x_ + kWheel;
    if (wx >= kMark - kHalf && wx <= kMark + kHalf) return 2;
    return 1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    ground_ = false;
    settle_ = 0;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    camX_ = 140;
    camH_ = 4.5;
    camS_ = 3.2;
}

void Game::startRun() {
    x_ = 6;
    h_ = 26;
    v_ = 18.2;
    vy_ = -1.6;
    att_ = -0.08;
    nose_ = 0;
    spoil_ = 0;
    ground_ = false;
    settle_ = 0;
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
    camH_ = 14;
    camS_ = 2.3;
    blip(540.f);
}

void Game::slopeAt(double dist, double speed, double& aim, double& desVy) const {
    double s;
    if (dist > kFlare) {
        s = kSlope;
        aim = dist * s;
    } else {
        double u = std::max(0.0, dist) / kFlare;
        s = kFlareS + (kSlope - kFlareS) * u;
        aim = std::max(0.0, dist) * s;
    }
    desVy = -s * std::max(8.0, speed) * 0.98;
    if (dist < 8.0) desVy = std::max(desVy, -1.15);
}

void Game::pilot(double& nose, double& spoil) const {
    double dist = kMark - (x_ + kWheel);
    double aim = 0, desVy = 0;
    slopeAt(dist, v_, aim, desVy);
    double cmd = (h_ - aim) * kKp + (vy_ - desVy) * kKd;
    nose = clampd(-cmd * 0.55, -1.0, 1.0);
    spoil = clampd(cmd * 0.7, 0.0, 1.0);
    if (v_ < 13.5) {
        spoil = 0;
        nose = std::min(nose, -0.15);
    }
    if (v_ > 22.0) spoil = std::max(spoil, 0.5);
}

void Game::win() {
    if (mode_ != Mode::Fly) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "set down on the mark";
    banner_ = "SET DOWN";
    chime_ = 0;
    chimeT_ = 0;
    h_ = 0;
    vy_ = 0;
    v_ = 0;
    sys_->rumble(0.18f, 0.06f, 140);
    sys_->setLight(30, 170, 80);
}

void Game::fail(const char* why, const char* banner) {
    if (mode_ != Mode::Fly) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    why_ = why;
    banner_ = banner;
    if (h_ < 0) h_ = 0;
    sys_->rumble(0.5f, 0.28f, 160);
    sys_->setLight(170, 36, 24);
    sys_->apu.noiseBurst(0.42f, 480.f, 0.26f);
}

void Game::physics(double nose, double spoil) {
    if (mode_ != Mode::Fly) return;
    nose_ = clampd(nose, -1.0, 1.0);
    spoil_ = clampd(spoil, 0.0, 1.0);
    time_ += DT;
    if (!ground_) {
        double mush = std::max(0.0, 12.6 - v_) * 0.62;
        double dive = std::max(0.0, -nose_);
        double hold = std::max(0.0, nose_);
        double vyCmd = -1.65 - dive * 5.0 + hold * 1.15 - spoil_ * 4.2 - mush;
        vyCmd += clampd((v_ - 17.5) * 0.03, -0.3, 0.4);
        vy_ += (vyCmd - vy_) * std::min(1.0, 6.0 * DT);
        v_ += (dive * 2.2 - hold * 1.35 - 0.2 - spoil_ * 2.7) * DT;
        v_ = clampd(v_, 0.0, 28.0);
        if (h_ < 0.9 && nose_ > 0.15 && vy_ < -1.05) {
            double c = clampd((0.9 - h_) / 0.9, 0.0, 1.0) * clampd((nose_ - 0.15) / 0.5, 0.0, 1.0);
            vy_ *= (1.0 - 0.5 * c);
        }
        x_ += v_ * 0.98 * DT;
        h_ += vy_ * DT;
        if (!std::isfinite(x_) || !std::isfinite(h_) || !std::isfinite(v_)) {
            fail("lost the air", "LOST");
            return;
        }
        double wx = x_ + kWheel;
        if (h_ <= 0.04) {
            h_ = 0;
            bool on = wx >= kMark - kHalf && wx <= kMark + kHalf;
            if (wx < kShore) {
                vy_ = 0;
                fail("in the lake", "LAKE");
                return;
            }
            if (!on) {
                vy_ = 0;
                fail(wx < kMark ? "short of the mark" : "long of the mark", wx < kMark ? "SHORT" : "LONG");
                return;
            }
            if (vy_ < kHard) {
                vy_ = 0;
                fail("too hard", "TOO HARD");
                return;
            }
            if (v_ < kMinV) {
                vy_ = 0;
                fail("too slow", "TOO SLOW");
                return;
            }
            ground_ = true;
            vy_ = 0;
            settle_ = 0;
            sys_->rumble(0.22f, 0.08f, 80);
            sys_->apu.noiseBurst(0.18f, 220.f, 0.14f);
        } else if (wx > kMark + kHalf) {
            fail("floated past the mark", "FLOAT");
            return;
        }
    } else {
        v_ = std::max(0.0, v_ - (1.15 + spoil_ * 7.4) * DT);
        x_ += v_ * DT;
        h_ = 0;
        vy_ = 0;
        settle_ += DT;
        double wx = x_ + kWheel;
        if (wx < kMark - kHalf || wx > kMark + kHalf) {
            fail("rolled off the mark", "ROLLED");
            return;
        }
        if (settle_ >= kSettle) {
            win();
            return;
        }
    }
    if (mode_ == Mode::Fly && time_ > 26.0) fail("too late", "TOO LATE");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    beep_ = 0.07f;
}

void Game::sky() {
    uint16_t zen = gs::rgb4(3, 5, 12);
    uint16_t mid = gs::rgb4(6, 11, 14);
    uint16_t hor = gs::rgb4(13, 12, 8);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 6, 5), 0.4f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(10, 14, 8), 0.35f);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.55f ? lerpC(zen, mid, t / 0.55f) : lerpC(mid, hor, (t - 0.55f) / 0.45f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
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
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float adv = 0;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) {
            adv += 14.f * scale;
            continue;
        }
        adv += (float(art_.glyph[c - 32].w) + 3.f) * scale;
    }
    x -= adv * 0.5f;
    float pen = x;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) {
            pen += 14.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float ht = float(g.h) * scale;
        spr(g, pen + float(g.w) * scale * 0.5f, y, ht, pal, false);
        pen += (float(g.w) + 3.f) * scale;
    }
}

void Game::draw(double x, double h, double att, double vy, double spd, bool craft) {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();

    if (mode_ == Mode::Title) {
        text("GLIDER MARK", 160, 22, 1.05f, PAL_HUD);
        text("SET DOWN ON IT", 160, 48, 0.62f, PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 26, 1.1f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(banner_, 160, 24, 1.05f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("SET DOWN", 160, 22, 1.15f, PAL_GOOD);
    }

    const bool framing = mode_ == Mode::Win || ground_;
    double span = framing ? 36.0 : clampd(32.0 + h * 2.4, 34.0, 130.0);
    double wantS = 210.0 / span;
    double dist = kMark - (x + kWheel);
    double lead = framing ? 0.0 : clampd(spd * 0.32, 3.0, 12.0);
    if (!framing && dist < 36.0 && dist > 0.0) lead = dist * 0.42;
    double wantX = framing ? kMark : x + lead;
    double wantH = framing ? 1.8 : std::max(2.2, h * 0.62 + 1.4);
    if (mode_ == Mode::Title) {
        wantX = 140;
        wantH = 4.5;
        wantS = 3.2;
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
    } else {
        double k = framing ? 0.22 : 0.16;
        camX_ += (wantX - camX_) * k;
        camH_ += (wantH - camH_) * k;
        camS_ += (wantS - camS_) * k;
    }
    const float scale = float(camS_);
    const float ax = 160.f;
    const float ay = 112.f;
    auto project = [&](double wx, double wy, float& sx, float& sy) {
        sx = ax + float(wx - camX_) * scale;
        sy = ay - float(wy - camH_) * scale;
    };

    if (craft) {
        int fi = int(std::lround((0.22 - att) / 0.14));
        fi = std::clamp(fi, 0, 4);
        const Ship& ship = art_.ship[fi];
        float sx, sy;
        project(x, h + kGear, sx, sy);
        float dest = float(ship.img.h) / ship.ppm * scale;
        sprAnchor(ship.img, ship.ax, ship.ay, sx, sy, std::max(10.f, dest), PAL_SHIP);
    }
    for (const Puff& p : puffs_) {
        if (p.life <= 0) continue;
        float sx, sy;
        project(p.x, 0.15, sx, sy);
        spr(art_.dust, sx, sy - 3.f, 7.f + float(1.0 - p.life) * 8.f, PAL_DUST, false, int((1.0 - p.life) * 8));
    }
    if (craft && h < 16.0) {
        float sx, sy;
        project(x, 0, sx, sy);
        float sh = std::clamp(2.4f * scale, 4.f, 28.f);
        int fog = int(std::min(14.0, h * 1.1));
        spr(art_.shade, sx, sy, sh, PAL_DUST, false, fog);
    }

    auto plant = [&](const gs::Mipped& m, double wx, float worldH, float minH, float maxH, int pal, bool flip, int fog) {
        float sx, sy;
        project(wx, 0, sx, sy);
        float ht = std::clamp(worldH * scale, minH, maxH);
        spr(m, sx, sy - ht * 0.5f, ht, pal, flip, fog);
    };
    int flagF = int(time_ * 3.0) & 1;
    plant(art_.flag[flagF], kMark, 3.6f, 16.f, 72.f, PAL_WOOD, false, 0);
    int sock = int(time_ * 6.0) % 3;
    if (sock < 0) sock = 0;
    plant(art_.sock[sock], kMark + 10.0, 2.6f, 14.f, 56.f, PAL_WOOD, false, 0);
    plant(art_.tower, kMark - 18.0, 4.4f, 18.f, 78.f, PAL_WOOD, false, 0);

    float mL, mY, mR, mY2;
    project(kMark - kHalf, 0, mL, mY);
    project(kMark + kHalf, 0, mR, mY2);
    float markW = std::max(10.f, mR - mL);
    float depth = std::clamp(8.f + scale * 1.6f, 10.f, 28.f);
    sprBox(art_.mark, (mL + mR) * 0.5f, mY - depth * 0.72f, markW, depth, PAL_MARK);
    for (int i = 0; i < 4; ++i) {
        double cx = kMark - 16.0 - i * 8.0;
        float sx, sy;
        project(cx, 0, sx, sy);
        spr(art_.chev, sx, sy - 6.f, std::max(6.f, scale * 0.7f), PAL_MARK, false, 1);
    }

    const double reeds[] = {kShore - 1.5, kShore + 2.0, kShore + 6.0, kShore + 11.0};
    for (double rx : reeds) plant(art_.reed, rx, 1.5f, 8.f, 26.f, PAL_TREE, false, 1);
    plant(art_.boat, 72.0, 1.15f, 8.f, 22.f, PAL_BOAT, false, 2);
    const double pines[] = {kShore + 8.0, 214.0, 228.0, 242.0, 258.0};
    for (double px : pines) plant(art_.pine, px, 3.8f, 12.f, 64.f, PAL_TREE, false, 2);

    float left = float(camX_) - (ax + 24.f) / scale;
    float right = float(camX_) + (gs::SCREEN_W - ax + 24.f) / scale;
    float step = std::clamp(34.f / scale, 2.8f, 9.f);
    float start = std::floor(left / step) * step;
    int tiles = 0;
    for (float wx = start; wx < right && tiles < 160; wx += step) {
        float sx, sy;
        project(wx + step * 0.5, 0, sx, sy);
        float sw = step * scale + 1.6f;
        float tile = 26.f;
        bool water = (wx + step * 0.5) < float(kShore);
        const gs::Mipped& tex = water ? art_.lake[(int(std::floor(wx)) & 1)] : art_.grass[(int(std::floor(wx / 3)) & 1)];
        int pal = water ? PAL_LAKE : PAL_GRASS;
        int rows = 0;
        for (float y = sy; y < gs::SCREEN_H + 2.f && rows < 7; y += tile - 1.f, ++rows) {
            sprBox(tex, sx, y, sw, tile, pal);
            if (++tiles > 160) break;
        }
    }
    {
        float sx, sy;
        project(kShore, 0, sx, sy);
        sprBox(art_.foam, sx, sy - 6.f, std::max(8.f, 2.2f * scale), std::max(6.f, 0.7f * scale), PAL_LAKE);
    }

    for (int i = 0; i < 4; ++i) {
        float sx = std::fmod(18.f + float(i) * 170.f - float(camX_) * scale * 0.18f, 720.f);
        if (sx < -80.f) sx += 720.f;
        spr(art_.hill, sx, 128.f, 28.f + float(i % 2) * 8.f, PAL_FAR, i & 1, 7);
    }
    for (int i = 0; i < 4; ++i) {
        float sx = std::fmod(24.f + float(i) * 130.f - float(camX_) * scale * 0.06f + float(time_) * 6.f, 560.f);
        if (sx < -40.f) sx += 560.f;
        spr(art_.cloud, sx, 28.f + float(i % 3) * 16.f, 14.f + float(i % 2) * 4.f, PAL_SKY, i & 1, 1);
    }
    spr(art_.sun, 286.f, 32.f, 22.f, PAL_SKY, false, 0);

    if (mode_ == Mode::Title) {
        hudC(16, "UP HOLDS THE NOSE", PAL_HUD);
        hudC(17, "DOWN STEEPENS", PAL_HUD);
        hudC(18, "Z  C  SPACE  SPOILER", PAL_AMBER);
        hudC(20, "THE WHEEL HAS TO MEET THE PAINT", PAL_HUD);
        hudC(21, "FLOATING PAST IT MISSES", PAL_GOOD);
        if ((sys_->frame / 30) % 2 == 0) hudC(24, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START FLIES   ESC TITLE", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(23, why_, PAL_BAD);
        hudC(25, "START TRIES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(22, "THE WHEEL IS ON THE MARK", PAL_GOOD);
        char buf[40];
        std::snprintf(buf, sizeof buf, "%.1f S", time_);
        hudC(24, buf, PAL_HUD);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "ALT %4.1f", std::max(0.0, h));
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", spd);
        hud(12, 1, buf, spd < kMinV + 1.0 ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "VS %+5.1f", vy);
        hud(23, 1, buf, vy < -2.6 ? PAL_BAD : vy < -2.0 ? PAL_AMBER : PAL_HUD);
        if (spoil_ > 0.45) hud(33, 1, ground_ ? "BRAKE" : "SPOILER", PAL_AMBER);

        double wx = x + kWheel;
        double aim = 0, des = 0;
        slopeAt(kMark - wx, spd, aim, des);
        double herr = h - aim;
        const char* line = "TO THE MARK";
        int pal = PAL_HUD;
        if (ground_) {
            line = "STAY ON THE PAINT";
            pal = PAL_GOOD;
        } else if (wx >= kMark - kHalf && wx <= kMark + kHalf) {
            line = "OVER THE MARK";
            pal = PAL_GOOD;
        } else if (wx > kMark + kHalf) {
            line = "TOO LONG";
            pal = PAL_BAD;
        } else {
            double near = (kMark - kHalf) - wx;
            if (near > 18.0) {
                if (herr > 1.6) {
                    line = "HIGH";
                    pal = PAL_AMBER;
                } else if (herr < -1.6) {
                    line = "LOW";
                    pal = PAL_BAD;
                } else {
                    line = "ON SLOPE";
                    pal = PAL_GOOD;
                }
                std::snprintf(buf, sizeof buf, "MARK %3.0f M", std::max(0.0, near));
                hudC(3, buf, PAL_HUD);
            } else {
                std::snprintf(buf, sizeof buf, "MARK %3.0f M", std::max(0.0, near));
                line = buf;
                pal = near < 8.0 ? PAL_AMBER : PAL_HUD;
            }
        }
        hudC(2, line, pal);
        if (ground_ && settle_ > 0.02) {
            int n = int(settle_ / kSettle * 5.0 + 0.001);
            n = std::clamp(n, 0, 5);
            std::snprintf(buf, sizeof buf, "SET %d/5", n);
            hudC(3, buf, PAL_GOOD);
        }
        hudC(26, "UP DOWN FLY    Z SPOILER", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(8, 10, 12));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.11f, 0.18f, 0.1f);
    sys.apu.setPatch(0, bellPatch());
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) time_ += DT;
    if (beep_ > 0.f) {
        beep_ -= float(DT);
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    for (Puff& p : puffs_)
        if (p.life > 0) p.life = std::max(0.0, p.life - DT);

    if (chime_ >= 0) {
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        chimeT_ += float(DT);
        if (chimeT_ > 0.14f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.2f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 7) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        double bob = std::sin(time_ * 1.15);
        draw(112.0 + bob * 1.4, 7.4 + bob * 0.25, -0.1 + bob * 0.03, -1.5, 16.0, true);
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            blip(660.f);
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
            blip(540.f);
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
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    double nose = 0, spoil = 0;
    if (bot_) {
        if (ground_) {
            nose = 0;
            spoil = 1;
        } else {
            pilot(nose, spoil);
        }
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
            blip(400.f);
            draw(x_, h_, att_, vy_, v_, true);
            return;
        }
    }

    bool wasGround = ground_;
    physics(nose, spoil);
    if (mode_ == Mode::Fly || mode_ == Mode::Win) {
        double path = std::atan2(vy_, std::max(8.0, v_));
        double want = ground_ ? 0.04 : path * 0.85 + nose_ * 0.1;
        att_ += (want - att_) * 0.3;
    }
    if (!wasGround && ground_ && mode_ == Mode::Fly) {
        puffs_[puffN_ % 8] = {x_ + kWheel, 0.4};
        puffN_++;
    }

    if (mode_ == Mode::Fly) {
        float wind = float(std::clamp((v_ - 8.0) / 16.0, 0.0, 1.0)) * (spoil_ > 0.4 && !ground_ ? 0.07f : 0.035f);
        sys.apu.noise(wind, 700.f + float(v_) * 30.f, false);
        if (!ground_ && v_ < 13.2) sys.apu.tone(2, 160.f, 0.04f);
        else sys.apu.tone(2, 0.f, 0.f);
        double wx = x_ + kWheel;
        bool over = wx >= kMark - kHalf && wx <= kMark + kHalf;
        if (ground_ || (over && h_ < 6.0)) sys.setLight(40, 150, 70);
        else if (vy_ < -2.8) sys.setLight(160, 40, 24);
        else sys.setLight(30, 70, 150);
    }
    draw(x_, h_, att_, vy_, v_, true);
}

}  // namespace gmark
