#include "dawn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cdawn {
namespace {
constexpr int DAWN = 60 * 28;
constexpr float TURN = 0.034f;
constexpr float REACH = 0.28f;
constexpr float DRAIN = 0.00145f;
constexpr float PI = 3.14159265f;

int mix4(int a, int b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return int(std::lround(a + (b - a) * t));
}

float wrapPi(float a) {
    while (a > PI) a -= 2.f * PI;
    while (a < -PI) a += 2.f * PI;
    return a;
}

float flareAng(int i) { return -PI * 0.5f + i * (2.f * PI / kFlares); }
}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) return 1;
    if (mode_ == Mode::Won) return 2;
    return 3;
}

int Game::lit() const {
    int n = 0;
    for (float f : fuel_)
        if (f > 0.02f) n++;
    return n;
}

void Game::place(float ang, float& x, float& y) const {
    x = CX + std::cos(ang) * RX;
    y = CY + std::sin(ang) * RY;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    watch_ = 0;
    fed_ = 0;
    dead_ = -1;
    over_ = false;
    won_ = false;
    feeding_ = false;
    ang_ = flareAng(0);
    gust_ = 0;
    gustIx_ = 0;
    focus_ = 0;
    fuel_[0] = 0.82f;
    fuel_[1] = 0.70f;
    fuel_[2] = 0.88f;
    fuel_[3] = 0.64f;
    fuel_[4] = 0.76f;
}

void Game::readPad(float& dir, bool& feed) const {
    dir = 0;
    feed = false;
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT)) dir -= 1.f;
    if (p.down(gs::BTN_RIGHT)) dir += 1.f;
    if (std::fabs(p.axisX) > 0.25f) dir = p.axisX;
    feed = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.accel > 0.4f;
}

void Game::think(float& dir, bool& feed) {
    int worst = 0;
    for (int i = 1; i < kFlares; i++)
        if (fuel_[i] < fuel_[worst]) worst = i;
    if (focus_ < 0 || focus_ >= kFlares) focus_ = worst;
    float here = std::fabs(wrapPi(flareAng(focus_) - ang_));
    if (here > REACH) {
        if (fuel_[worst] + 0.12f < fuel_[focus_]) focus_ = worst;
    } else if (fuel_[focus_] >= 0.78f) {
        focus_ = worst;
    }
    float d = wrapPi(flareAng(focus_) - ang_);
    if (std::fabs(d) > REACH * 0.55f) {
        dir = d > 0 ? 1.f : -1.f;
        feed = false;
    } else {
        dir = 0;
        feed = fuel_[focus_] < 0.78f;
    }
}

void Game::tickWatch() {
    float dir = 0;
    bool feed = false;
    if (bot_) think(dir, feed);
    else readPad(dir, feed);

    ang_ += dir * TURN;
    if (std::fabs(dir) > 0.1f) step_ = (watch_ / 8) & 1;

    if ((watch_ % 190) == 40) {
        gust_ = 70;
        gustIx_ = (watch_ / 190) % kFlares;
    }
    if (gust_ > 0) gust_ -= 1.f;

    bool touched = false;
    int near = -1;
    for (int i = 0; i < kFlares; i++) {
        float d = DRAIN;
        if (gust_ > 0 && i == gustIx_) d *= 2.5f;
        fuel_[i] -= d;
        if (feed && std::fabs(wrapPi(flareAng(i) - ang_)) <= REACH) {
            fuel_[i] = std::min(1.f, fuel_[i] + 0.048f);
            touched = true;
            near = i;
        }
        if (fuel_[i] <= 0.f) {
            fuel_[i] = 0.f;
            dead_ = i;
            mode_ = Mode::Lost;
            won_ = false;
            over_ = true;
            sys_->apu.tone(0, 0, 0);
            sys_->apu.noiseBurst(0.35f, 380.f, 0.4f);
            return;
        }
    }
    (void)near;
    if (touched && !feeding_) fed_++;
    feeding_ = touched;
    if (touched) sys_->apu.tone(0, 160.f + fuel_[0] * 50.f, 0.07f);
    else sys_->apu.tone(0, 0, 0);
    if (gust_ > 30) sys_->apu.noise(0.045f, 1100.f, false);
    else sys_->apu.noise(0, 0, false);

    watch_++;
    if (watch_ >= DAWN) {
        mode_ = Mode::Won;
        won_ = true;
        over_ = true;
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 494.f, 0.12f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    if (mode_ == Mode::Title) {
        if (bot_ && age_ > 12) beginWatch();
        else if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) beginWatch();
    } else if (mode_ == Mode::Watch) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else tickWatch();
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) mode_ = Mode::Watch;
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
        mode_ = Mode::Title;
        age_ = 0;
        over_ = false;
    }
    draw();
}

void Game::sprite(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int x = col + i;
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    int n = int(std::strlen(s));
    float adv = 18.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        sprite(g, left + float(i) * adv + adv * 0.5f, y, g.h * scale, pal, false, 0);
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    float dawn = 0;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) dawn = float(watch_) / float(DAWN);
    if (mode_ == Mode::Won) dawn = 1.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float k = y / float(gs::SCREEN_H);
        int nr = mix4(1, 11, dawn * (1.f - k * 0.3f));
        int ng = mix4(1, 7, dawn);
        int nb = mix4(5, 3, dawn);
        if (k > 0.62f) {
            float g = (k - 0.62f) / 0.38f;
            nr = mix4(nr, mix4(2, 8, dawn), g);
            ng = mix4(ng, mix4(3, 7, dawn), g);
            nb = mix4(nb, mix4(2, 4, dawn), g);
        }
        v.lineBackdrop[y] = gs::rgb4(nr, ng, nb);
        v.lineFog[y] = uint8_t(k > 0.78f ? int((k - 0.78f) * 18) : 0);
        v.road[y].on = false;
    }
    v.A.enabled = false;
    v.B.enabled = false;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.hudEnabled = true;
    sky();

    float dawn = (mode_ == Mode::Won) ? 1.f : float(watch_) / float(DAWN);
    if (mode_ == Mode::Title) dawn = 0;
    if (dawn < 0.9f) sprite(art_.moon, 246.f - dawn * 30.f, 34.f + dawn * 24.f, 24.f, PAL_MOON);
    if (dawn > 0.7f) sprite(art_.sun, 78.f, 96.f - (dawn - 0.7f) * 160.f, 30.f, PAL_FIRE);
    static const float sx[7] = {24, 58, 104, 188, 228, 290, 140};
    static const float sy[7] = {16, 38, 20, 26, 44, 18, 12};
    for (int i = 0; i < 7; i++) sprite(art_.star, sx[i], sy[i], 6.f, PAL_STAR, false, int(dawn * 14));

    sprite(art_.cistern, CX, CY + 6.f, 86.f, PAL_STONE);

    struct Slot {
        int i;
        float y;
    };
    Slot order[kFlares];
    for (int i = 0; i < kFlares; i++) {
        float x, y;
        place(flareAng(i), x, y);
        order[i] = {i, y};
    }
    std::sort(order, order + kFlares, [](const Slot& a, const Slot& b) { return a.y < b.y; });

    float px, py;
    place(ang_, px, py);
    bool keeperDrawn = mode_ == Mode::Title;
    for (int n = 0; n < kFlares; n++) {
        int i = order[n].i;
        float x, y;
        place(flareAng(i), x, y);
        if (!keeperDrawn && py < y) {
            sprite(art_.keeper[step_], px, py - 16.f, 40.f, PAL_YOU, std::cos(ang_) < 0);
            keeperDrawn = true;
        }
        float life = (mode_ == Mode::Title) ? 0.7f : fuel_[i];
        if (life > 0.04f) {
            int fr = ((watch_ / 6) + i) & 1;
            float h = 14.f + life * 18.f;
            sprite(art_.flame[fr], x, y - 22.f - life * 6.f, h, PAL_FIRE);
        }
        sprite(art_.post, x, y - 8.f, 22.f, PAL_POST);
        if (gust_ > 18 && i == gustIx_ && mode_ == Mode::Watch) {
            float dx = 10.f + (int(gust_) % 7);
            sprite(art_.drop, x + dx, y - 30.f, 10.f, PAL_RAIN);
        }
    }
    if (!keeperDrawn && mode_ != Mode::Title) sprite(art_.keeper[step_], px, py - 16.f, 40.f, PAL_YOU, std::cos(ang_) < 0);

    if (mode_ == Mode::Title) {
        text("S3 CISTERN DAWN", 160, 22, 0.55f, PAL_GOLD);
        text("KEEP THE FLARES LIT", 160, 42, 0.42f, PAL_HUD);
        hudC(25, "LEFT RIGHT AROUND THE RIM   A FEEDS", PAL_HUD);
        hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        text("HOLD", 160, 24, 0.7f, PAL_GOLD);
    } else if (mode_ == Mode::Won) {
        text("DAWN", 160, 20, 0.85f, PAL_GOLD);
        text("THE FLARES HELD", 160, 42, 0.48f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("A FLARE DIED", 160, 20, 0.55f, PAL_ALERT);
        text("THE WATCH IS OVER", 160, 42, 0.42f, PAL_HUD);
    }

    if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        int left = std::max(0, (DAWN - watch_ + 59) / 60);
        char buf[40];
        std::snprintf(buf, sizeof buf, "DAWN %d:%02d", left / 60, left % 60);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "LIT %d", lit());
        hud(14, 1, buf, PAL_OK);
        for (int i = 0; i < kFlares; i++) {
            int bars = int(fuel_[i] * 6.f + 0.5f);
            char pip[8];
            for (int k = 0; k < 6; k++) pip[k] = k < bars ? '#' : '.';
            pip[6] = 0;
            hud(24, 1 + i, pip, fuel_[i] < 0.26f ? PAL_ALERT : PAL_OK);
        }
        if (gust_ > 0) hud(1, 2, "RAIN", PAL_RAIN);
    }
}

}  // namespace cdawn
