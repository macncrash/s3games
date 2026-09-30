#include "dawn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace bdawn {
namespace {
constexpr int DAWN = 60 * 32;
constexpr float SPEED = 3.4f;
constexpr float REACH = 20.f;
constexpr float DRAIN = 0.00115f;

int mix4(int a, int b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return int(std::lround(a + (b - a) * t));
}

bool underRain(float x, float rainX, float rainW) {
    return rainW > 1.f && x >= rainX && x <= rainX + rainW;
}
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

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(1, 1, 4));
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
    rainX_ = -80.f;
    rainW_ = 0;
    rainLeft_ = 90;
    focus_ = -1;
    fuel_[0] = 0.82f;
    fuel_[1] = 0.70f;
    fuel_[2] = 0.90f;
    fuel_[3] = 0.66f;
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
    float worstScore = 99.f;
    for (int i = 0; i < kFlares; i++) {
        float s = fuel_[i];
        if (underRain(FLARE_X[i], rainX_, rainW_)) s -= 0.22f;
        if (s < worstScore) {
            worstScore = s;
            worst = i;
        }
    }
    if (focus_ < 0 || focus_ >= kFlares) focus_ = worst;
    bool here = std::fabs(FLARE_X[focus_] - px_) <= REACH - 2.f;
    if (!here) {
        if (worstScore + 0.12f < fuel_[focus_]) focus_ = worst;
    } else if (fuel_[focus_] >= 0.78f && !underRain(FLARE_X[focus_], rainX_, rainW_)) {
        focus_ = worst;
    }
    float dx = FLARE_X[focus_] - px_;
    if (std::fabs(dx) > REACH - 2.f) {
        dir = dx > 0 ? 1.f : -1.f;
        feed = false;
    } else {
        dir = 0;
        feed = fuel_[focus_] < 0.78f || underRain(FLARE_X[focus_], rainX_, rainW_);
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
    px_ = std::clamp(px_, 18.f, 304.f);
    if (std::fabs(dir) > 0.1f) step_ = (watch_ / 7) & 1;

    if (rainLeft_ > 0) {
        rainLeft_--;
        if (rainLeft_ == 0) {
            rainW_ = 48.f;
            rainX_ = -48.f;
        }
    } else if (rainW_ > 1.f) {
        rainX_ += 1.6f;
        if (rainX_ > 340.f) {
            rainW_ = 0;
            rainLeft_ = 140;
        }
    }

    bool touched = false;
    for (int i = 0; i < kFlares; i++) {
        float d = DRAIN;
        bool wet = underRain(FLARE_X[i], rainX_, rainW_);
        bool shield = wet && std::fabs(px_ - FLARE_X[i]) <= REACH;
        if (wet && !shield) d *= 2.4f;
        fuel_[i] -= d;
        if (feed && std::fabs(px_ - FLARE_X[i]) <= REACH) {
            fuel_[i] = std::min(1.f, fuel_[i] + 0.05f);
            touched = true;
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
    if (touched && !feeding_) fed_++;
    feeding_ = touched;
    if (touched) sys_->apu.tone(0, 210.f + fuel_[2] * 30.f, 0.07f);
    else sys_->apu.tone(0, 0, 0);
    if (rainW_ > 1.f) sys_->apu.noise(0.04f, 1400.f, false);
    else sys_->apu.noise(0, 0, false);

    watch_++;
    if (watch_ >= DAWN) {
        mode_ = Mode::Won;
        won_ = true;
        over_ = true;
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 480.f, 0.12f);
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
        int nr = mix4(1, 11, dawn * (1.f - k * 0.3f));
        int ng = mix4(1, 5, dawn);
        int nb = mix4(5, 3, dawn);
        if (k > 0.62f) {
            float g = (k - 0.62f) / 0.38f;
            nr = mix4(nr, mix4(1, 8, dawn), g);
            ng = mix4(ng, mix4(2, 6, dawn), g);
            nb = mix4(nb, mix4(4, 4, dawn), g);
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
    if (dawn < 0.9f) sprite(art_.moon, 48.f + dawn * 20.f, 28.f + dawn * 24.f, 24.f, PAL_MOON);
    if (dawn > 0.7f) sprite(art_.sun, 250.f, 100.f - (dawn - 0.7f) * 160.f, 26.f, PAL_FIRE);
    static const float sx[7] = {20, 80, 120, 170, 210, 260, 300};
    static const float sy[7] = {16, 34, 18, 42, 22, 36, 14};
    for (int i = 0; i < 7; i++) sprite(art_.star, sx[i], sy[i], 5.f, PAL_STAR, false, int(dawn * 14));

    sprite(art_.tower, 160.f, 108.f, 110.f, PAL_TOWER);
    for (int i = 0; i < kFlares; i++) {
        float life = (mode_ == Mode::Title) ? 0.7f : fuel_[i];
        if (life > 0.04f) {
            int fr = ((watch_ / 6) + i) & 1;
            float h = 14.f + life * 20.f;
            sprite(art_.flame[fr], FLARE_X[i], FLARE_Y - 16.f - life * 7.f, h, PAL_FIRE);
        }
        sprite(art_.bowl, FLARE_X[i], FLARE_Y, 16.f, PAL_BOWL);
    }
    if (rainW_ > 1.f && (mode_ == Mode::Watch || mode_ == Mode::Pause)) {
        for (int i = 0; i < 8; i++) {
            float x = rainX_ + 4.f + i * 6.f;
            float y = 40.f + float((watch_ * 3 + i * 11) % 90);
            sprite(art_.drop, x, y, 7.f, PAL_RAIN);
        }
    }
    if (mode_ != Mode::Title) {
        sprite(art_.keeper[step_], px_, 146.f, 42.f, PAL_YOU, face_ < 0);
        sprite(art_.lamp, px_ + face_ * 12.f, 132.f, 12.f, PAL_FIRE);
    }

    if (mode_ == Mode::Title) {
        text("S3 BEACON DAWN", 160, 34, 0.62f, PAL_GOLD);
        text("KEEP THE FLARES LIT", 160, 58, 0.42f, PAL_HUD);
        text("UNTIL DAWN", 160, 76, 0.42f, PAL_HUD);
        hudC(24, "LEFT RIGHT MOVE   A FEEDS A FLARE", PAL_HUD);
        hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        text("HOLD", 160, 36, 0.8f, PAL_GOLD);
    } else if (mode_ == Mode::Won) {
        text("DAWN", 160, 26, 0.9f, PAL_GOLD);
        text("THE FLARES HELD", 160, 50, 0.5f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("A FLARE DIED", 160, 26, 0.58f, PAL_ALERT);
        text("THE WATCH IS OVER", 160, 50, 0.44f, PAL_HUD);
    }

    if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        int left = std::max(0, (DAWN - watch_ + 59) / 60);
        char buf[32];
        std::snprintf(buf, sizeof buf, "DAWN %d:%02d", left / 60, left % 60);
        hud(1, 1, buf, PAL_HUD);
        for (int i = 0; i < kFlares; i++) {
            int bars = int(fuel_[i] * 6.f + 0.5f);
            char pip[8];
            for (int k = 0; k < 6; k++) pip[k] = k < bars ? '#' : '.';
            pip[6] = 0;
            hud(28, 1 + i, pip, fuel_[i] < 0.25f ? PAL_ALERT : PAL_OK);
        }
        if (rainW_ > 1.f) hud(12, 1, "RAIN", PAL_RAIN);
    }
}

}  // namespace bdawn
