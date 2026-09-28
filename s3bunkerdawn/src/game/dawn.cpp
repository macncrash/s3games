#include "dawn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace bdawn {
namespace {
constexpr int DAWN = 60 * 36;
constexpr float SPEED = 2.15f;
constexpr float REACH = 18.f;
constexpr float DRAIN = 0.0017f;

int mix4(int a, int b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return int(std::lround(a + (b - a) * t));
}
}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title || mode_ == Mode::Pause) return mode_ == Mode::Title ? 0 : 1;
    if (mode_ == Mode::Watch) return 1;
    if (mode_ == Mode::Won) return 2;
    return 3;
}

int Game::lit() const {
    int n = 0;
    for (float f : fuel_)
        if (f > 0.02f) n++;
    return n;
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
    px_ = 160.f;
    face_ = 1;
    gust_ = 0;
    gustIx_ = 0;
    focus_ = -1;
    fuel_[0] = 0.78f;
    fuel_[1] = 0.62f;
    fuel_[2] = 0.86f;
    fuel_[3] = 0.54f;
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
    bool here = std::fabs(FLARE_X[focus_] - px_) <= REACH - 2.f;
    if (!here) {
        if (fuel_[worst] + 0.15f < fuel_[focus_]) focus_ = worst;
    } else if (fuel_[focus_] >= 0.72f) {
        focus_ = worst;
    }
    float dx = FLARE_X[focus_] - px_;
    if (std::fabs(dx) > REACH - 2.f) {
        dir = dx > 0 ? 1.f : -1.f;
        feed = false;
    } else {
        dir = 0;
        feed = fuel_[focus_] < 0.72f;
    }
}

void Game::tickWatch() {
    float dir = 0;
    bool feed = false;
    if (bot_) think(dir, feed);
    else readPad(dir, feed);

    if (dir < -0.1f) face_ = -1;
    if (dir > 0.1f) face_ = 1;
    px_ += dir * SPEED;
    px_ = std::clamp(px_, 22.f, 300.f);
    if (std::fabs(dir) > 0.1f) step_ = (watch_ / 8) & 1;

    if ((watch_ % 220) == 0) {
        gust_ = 80;
        gustIx_ = (watch_ / 220) % kFlares;
    }
    if (gust_ > 0) gust_ -= 1.f;

    bool touched = false;
    for (int i = 0; i < kFlares; i++) {
        float d = DRAIN;
        if (gust_ > 0 && i == gustIx_) d *= 2.35f;
        fuel_[i] -= d;
        if (feed && std::fabs(px_ - FLARE_X[i]) <= REACH) {
            fuel_[i] = std::min(1.f, fuel_[i] + 0.042f);
            touched = true;
        }
        if (fuel_[i] <= 0.f) {
            fuel_[i] = 0.f;
            dead_ = i;
            mode_ = Mode::Lost;
            won_ = false;
            over_ = true;
            sys_->apu.tone(0, 0, 0);
            sys_->apu.noiseBurst(0.35f, 400.f, 0.4f);
            return;
        }
    }
    if (touched && !feeding_) fed_++;
    feeding_ = touched;
    if (touched) sys_->apu.tone(0, 180.f + fuel_[0] * 40.f, 0.08f);
    else sys_->apu.tone(0, 0, 0);
    if (gust_ > 40) sys_->apu.noise(0.05f, 900.f, false);
    else sys_->apu.noise(0, 0, false);

    watch_++;
    if (watch_ >= DAWN) {
        mode_ = Mode::Won;
        won_ = true;
        over_ = true;
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 520.f, 0.12f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    if (mode_ == Mode::Title) {
        if (bot_ && age_ > 16) beginWatch();
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
        int nr = mix4(1, 10, dawn * (1.f - k * 0.35f));
        int ng = mix4(1, 6, dawn);
        int nb = mix4(4, 3, dawn);
        if (k > 0.55f) {
            float g = (k - 0.55f) / 0.45f;
            nr = mix4(nr, mix4(2, 12, dawn), g);
            ng = mix4(ng, mix4(2, 7, dawn), g);
            nb = mix4(nb, mix4(3, 4, dawn), g);
        }
        v.lineBackdrop[y] = gs::rgb4(nr, ng, nb);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.hudEnabled = true;
    sky();

    float dawn = (mode_ == Mode::Won) ? 1.f : float(watch_) / float(DAWN);
    if (mode_ == Mode::Title) dawn = 0;
    if (dawn < 0.92f) sprite(art_.moon, 250.f - dawn * 40.f, 36.f + dawn * 30.f, 26.f, PAL_MOON);
    if (dawn > 0.72f) sprite(art_.sun, 70.f, 90.f - (dawn - 0.72f) * 140.f, 28.f, PAL_FIRE);
    static const float sx[6] = {30, 70, 110, 180, 220, 300};
    static const float sy[6] = {18, 40, 22, 28, 48, 16};
    for (int i = 0; i < 6; i++) {
        int fog = int(dawn * 14);
        sprite(art_.star, sx[i], sy[i], 6.f, PAL_STAR, false, fog);
    }

    sprite(art_.bunker, 160.f, 118.f, 92.f, PAL_BLOCK);
    for (int i = 0; i < kFlares; i++) {
        sprite(art_.bag, FLARE_X[i], GROUND_Y - 2.f, 16.f, PAL_BAG);
        float life = fuel_[i];
        if (life > 0.04f) {
            int fr = ((watch_ / 7) + i) & 1;
            float h = 16.f + life * 22.f;
            sprite(art_.flame[fr], FLARE_X[i], FLARE_Y - 18.f - life * 8.f, h, PAL_FIRE);
        }
        sprite(art_.pot, FLARE_X[i], FLARE_Y, 20.f, PAL_POT);
    }
    if (gust_ > 20 && mode_ == Mode::Watch) {
        float gx = FLARE_X[gustIx_] + 16.f;
        sprite(art_.spark, gx, FLARE_Y - 28.f, 7.f, PAL_WIND);
    }
    if (mode_ != Mode::Title) sprite(art_.man[step_], px_, 150.f, 46.f, PAL_YOU, face_ < 0);

    if (mode_ == Mode::Title) {
        text("S3 BUNKER DAWN", 160, 36, 0.72f, PAL_GOLD);
        text("KEEP THE FLARES LIT", 160, 62, 0.48f, PAL_HUD);
        text("UNTIL DAWN", 160, 82, 0.48f, PAL_HUD);
        hudC(24, "LEFT RIGHT MOVE   A FEEDS A FLARE", PAL_HUD);
        hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        text("HOLD", 160, 40, 0.8f, PAL_GOLD);
    } else if (mode_ == Mode::Won) {
        text("DAWN", 160, 28, 0.9f, PAL_GOLD);
        text("THE FLARES HELD", 160, 52, 0.55f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("A FLARE DIED", 160, 28, 0.62f, PAL_ALERT);
        text("THE WATCH IS OVER", 160, 52, 0.48f, PAL_HUD);
    }

    if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        int left = std::max(0, (DAWN - watch_ + 59) / 60);
        char buf[32];
        std::snprintf(buf, sizeof buf, "DAWN %d:%02d", left / 60, left % 60);
        hud(1, 1, buf, PAL_HUD);
        hud(1, 2, "FLARES", PAL_HUD);
        for (int i = 0; i < kFlares; i++) {
            int bars = int(fuel_[i] * 8.f + 0.5f);
            char pip[10];
            for (int k = 0; k < 8; k++) pip[k] = k < bars ? '#' : '.';
            pip[8] = 0;
            hud(8, 2 + i, pip, fuel_[i] < 0.28f ? PAL_ALERT : PAL_OK);
        }
        if (gust_ > 0) hud(28, 1, "WIND", PAL_WIND);
    }
}

}  // namespace bdawn
