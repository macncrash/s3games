#include "dawn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace rdawn {
namespace {
constexpr int DAWN = 60 * 28;
constexpr float SPEED = 3.1f;
constexpr float REACH = 16.f;
constexpr float DRAIN = 0.00115f;

int mix4(int a, int b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return int(std::lround(a + (b - a) * t));
}

int lowest(const float fuel[kFlares]) {
    int w = 0;
    for (int i = 1; i < kFlares; i++)
        if (fuel[i] < fuel[w]) w = i;
    return w;
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
    acting_ = false;
    px_ = 48.f;
    face_ = 1;
    rain_ = 0;
    rainIx_ = 1;
    focus_ = 1;
    oil_ = 0.85f;
    fuel_[0] = 0.8f;
    fuel_[1] = 0.55f;
    fuel_[2] = 0.72f;
}

void Game::readPad(float& dir, bool& act) const {
    dir = 0;
    act = false;
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT)) dir -= 1.f;
    if (p.down(gs::BTN_RIGHT)) dir += 1.f;
    if (std::fabs(p.axisX) > 0.25f) dir = p.axisX;
    act = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.accel > 0.4f;
}

void Game::think(float& dir, bool& act) {
    int need = lowest(fuel_);
    if (rain_ > 12.f && fuel_[rainIx_] < fuel_[need] + 0.02f) need = rainIx_;
    if (focus_ < 0 || focus_ >= kFlares) focus_ = need;
    if (fuel_[focus_] >= 0.9f || fuel_[need] + 0.14f < fuel_[focus_]) focus_ = need;
    bool hungry = fuel_[focus_] < 0.9f;
    bool barrel = false;
    if (oil_ < 0.12f && fuel_[focus_] > 0.22f) barrel = true;
    if (!hungry && oil_ < 0.75f) barrel = true;
    float goal = barrel ? BARREL_X : FLARE_X[focus_];
    float dx = goal - px_;
    if (std::fabs(dx) > 3.5f) {
        dir = dx > 0 ? 1.f : -1.f;
        act = false;
    } else if (barrel) {
        dir = 0;
        act = oil_ < 0.98f;
    } else {
        dir = 0;
        act = hungry && oil_ > 0.02f;
    }
}

void Game::tickWatch() {
    float dir = 0;
    bool act = false;
    if (bot_) think(dir, act);
    else readPad(dir, act);

    if (dir < -0.1f) face_ = -1;
    if (dir > 0.1f) face_ = 1;
    px_ += dir * SPEED;
    px_ = std::clamp(px_, 28.f, 292.f);
    if (std::fabs(dir) > 0.1f) step_ = (watch_ / 7) & 1;

    if (rain_ <= 0.f && (watch_ % 210) == 50) {
        rain_ = 64.f;
        rainIx_ = (watch_ / 210) % kFlares;
    }
    if (rain_ > 0.f) rain_ -= 1.f;

    bool poured = false;
    bool filled = false;
    if (act && std::fabs(px_ - BARREL_X) <= REACH && oil_ < 1.f) {
        bool onFlare = false;
        for (int i = 0; i < kFlares; i++)
            if (std::fabs(px_ - FLARE_X[i]) <= REACH && fuel_[i] < 0.9f) onFlare = true;
        if (!onFlare || oil_ < 0.12f) {
            oil_ = std::min(1.f, oil_ + 0.05f);
            filled = true;
        }
    }

    for (int i = 0; i < kFlares; i++) {
        float d = DRAIN;
        if (rain_ > 0.f && i == rainIx_) d *= 2.5f;
        fuel_[i] -= d;
        if (act && oil_ > 0.02f && std::fabs(px_ - FLARE_X[i]) <= REACH && fuel_[i] < 1.f) {
            float pour = std::min(0.07f, 1.f - fuel_[i]);
            pour = std::min(pour, oil_);
            fuel_[i] += pour;
            oil_ -= pour * 0.38f;
            poured = true;
        }
        if (fuel_[i] <= 0.f) {
            fuel_[i] = 0.f;
            dead_ = i;
            mode_ = Mode::Lost;
            won_ = false;
            over_ = true;
            sys_->apu.tone(0, 0, 0);
            sys_->apu.noiseBurst(0.35f, 380.f, 0.45f);
            return;
        }
    }
    oil_ = std::clamp(oil_, 0.f, 1.f);
    if (poured && !acting_) fed_++;
    acting_ = poured || filled;
    if (poured) sys_->apu.tone(0, 140.f + oil_ * 60.f, 0.07f);
    else if (filled) sys_->apu.tone(0, 90.f, 0.05f);
    else sys_->apu.tone(0, 0, 0);
    if (rain_ > 30.f) sys_->apu.noise(0.045f, 1400.f, false);
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
        int nr = mix4(1, 12, dawn * (1.f - k * 0.4f));
        int ng = mix4(1, 5, dawn);
        int nb = mix4(5, 3, dawn);
        if (k > 0.62f) {
            float g = (k - 0.62f) / 0.38f;
            nr = mix4(nr, mix4(3, 8, dawn), g);
            ng = mix4(ng, mix4(3, 5, dawn), g);
            nb = mix4(nb, mix4(2, 3, dawn), g);
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
    if (mode_ == Mode::Title) dawn = 0.05f;
    if (dawn < 0.9f) sprite(art_.moon, 246.f - dawn * 30.f, 34.f + dawn * 24.f, 24.f, PAL_MOON);
    if (dawn > 0.7f) sprite(art_.sun, 58.f, 96.f - (dawn - 0.7f) * 160.f, 26.f, PAL_FIRE);
    static const float sx[5] = {28, 86, 140, 200, 290};
    static const float sy[5] = {16, 34, 18, 42, 22};
    for (int i = 0; i < 5; i++) sprite(art_.star, sx[i], sy[i], 6.f, PAL_STAR, false, int(dawn * 14));

    sprite(art_.ditch, 160.f, 198.f, 16.f, PAL_EARTH);
    sprite(art_.redoubt, 160.f, 150.f, 86.f, PAL_WALL);
    float flap = ((watch_ / 10) & 1) ? 1.f : -1.f;
    sprite(art_.flag, 118.f + flap, 96.f, 22.f, PAL_FLAG, flap < 0);
    sprite(art_.barrel, BARREL_X, WALK_Y - 6.f, 22.f, PAL_OIL);

    for (int i = 0; i < kFlares; i++) {
        float life = fuel_[i];
        if (life > 0.04f) {
            int fr = ((watch_ / 6) + i) & 1;
            float h = 14.f + life * 20.f;
            sprite(art_.flame[fr], FLARE_X[i], FLARE_Y - 16.f - life * 6.f, h, PAL_FIRE);
        }
        sprite(art_.brazier, FLARE_X[i], FLARE_Y, 18.f, PAL_IRON);
    }
    if (rain_ > 4.f && mode_ == Mode::Watch) {
        float gx = FLARE_X[rainIx_];
        for (int d = 0; d < 5; d++) {
            float yy = 40.f + float((watch_ * 7 + d * 28) % 90);
            sprite(art_.drop, gx - 10.f + d * 5.f, yy, 8.f, PAL_RAIN);
        }
    }
    if (mode_ != Mode::Title) sprite(art_.watch[step_], px_, WALK_Y - 8.f, 42.f, PAL_YOU, face_ < 0);

    if (mode_ == Mode::Title) {
        text("S3 REDOUBT DAWN", 160, 34, 0.62f, PAL_GOLD);
        text("KEEP THE FLARES LIT", 160, 58, 0.44f, PAL_HUD);
        text("UNTIL DAWN", 160, 78, 0.44f, PAL_HUD);
        hudC(23, "LEFT RIGHT WALK THE WALL", PAL_HUD);
        hudC(24, "A FEEDS A BRAZIER OR THE BARREL", PAL_HUD);
        hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        text("HOLD", 160, 36, 0.8f, PAL_GOLD);
    } else if (mode_ == Mode::Won) {
        text("DAWN", 160, 26, 0.9f, PAL_GOLD);
        text("THE FLARES HELD", 160, 50, 0.52f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("A FLARE WENT OUT", 160, 26, 0.52f, PAL_ALERT);
        text("THE REDOUBT IS DARK", 160, 50, 0.42f, PAL_HUD);
    }

    if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        int left = std::max(0, (DAWN - watch_ + 59) / 60);
        char buf[40];
        std::snprintf(buf, sizeof buf, "DAWN %d:%02d", left / 60, left % 60);
        hud(1, 1, buf, PAL_HUD);
        int bars = int(oil_ * 8.f + 0.5f);
        char pip[12];
        for (int k = 0; k < 8; k++) pip[k] = k < bars ? '#' : '.';
        pip[8] = 0;
        hud(16, 1, "OIL", PAL_OIL);
        hud(20, 1, pip, oil_ < 0.25f ? PAL_ALERT : PAL_OIL);
        for (int i = 0; i < kFlares; i++) {
            int fb = int(fuel_[i] * 8.f + 0.5f);
            for (int k = 0; k < 8; k++) pip[k] = k < fb ? '#' : '.';
            pip[8] = 0;
            char name[8];
            std::snprintf(name, sizeof name, "F%d", i + 1);
            hud(1, 3 + i, name, PAL_HUD);
            hud(4, 3 + i, pip, fuel_[i] < 0.26f ? PAL_ALERT : PAL_OK);
        }
        if (rain_ > 0.f) hud(30, 1, "RAIN", PAL_RAIN);
    }
}

}  // namespace rdawn
