#include "game/sally.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace sally {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float DAWN = 52.f;
constexpr float REACH = 26.f;
constexpr int N = 4;
constexpr float XS[N] = {46.f, 118.f, 202.f, 276.f};
constexpr float RATES[N] = {0.046f, 0.052f, 0.044f, 0.050f};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    titleT_ = 0;
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    px_ = 160;
    face_ = 1;
    fuels_[0] = 0.62f;
    fuels_[1] = 0.48f;
    fuels_[2] = 0.70f;
    fuels_[3] = 0.55f;
    cds_ = 0;
    left_ = DAWN;
    clock_ = 0;
    gust_ = 0;
    gustT_ = 5;
    flicker_ = 0;
    fed_ = -1;
}

void Game::blip(float freq) { sys_->apu.tone(0, freq, 0.08f); }

int Game::nearest() const {
    int b = 0;
    float bd = 1e9f;
    for (int i = 0; i < N; i++) {
        float d = std::fabs(px_ - XS[i]);
        if (d < bd) {
            bd = d;
            b = i;
        }
    }
    return b;
}

void Game::stoke() {
    int i = nearest();
    if (std::fabs(px_ - XS[i]) > REACH || cds_ > 0) return;
    fuels_[i] = std::min(1.f, fuels_[i] + 0.50f);
    cds_ = 0.20f;
    fed_ = i;
    blip(180.f + fuels_[i] * 220.f);
    sys_->apu.noiseBurst(0.12f, 2400.f, 18.f);
}

void Game::update(float dt) {
    cds_ = std::max(0.f, cds_ - dt);
    flicker_ += dt;
    if (mode_ == Mode::Title) {
        titleT_ += dt;
        if (bot_ && titleT_ > 0.4f) beginWatch();
        if (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_B) ||
            sys_->pad.pressed(gs::BTN_C))
            beginWatch();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (sys_->pad.pressed(gs::BTN_START)) mode_ = Mode::Watch;
        return;
    }
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) {
        if (!bot_ && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A))) beginWatch();
        return;
    }

    if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        sys_->apu.tone(0, 0, 0);
        return;
    }

    float axis = sys_->pad.axisX;
    float dir = 0;
    if (bot_) {
        int urgent = 0;
        for (int i = 1; i < N; i++)
            if (fuels_[i] < fuels_[urgent]) urgent = i;
        float dx = XS[urgent] - px_;
        if (std::fabs(dx) > 8.f) dir = dx > 0 ? 1.f : -1.f;
        else stoke();
    } else {
        if (sys_->pad.down(gs::BTN_LEFT) || axis < -0.3f) dir = -1.f;
        if (sys_->pad.down(gs::BTN_RIGHT) || axis > 0.3f) dir = 1.f;
        if (sys_->pad.down(gs::BTN_A) || sys_->pad.down(gs::BTN_B) || sys_->pad.down(gs::BTN_C) ||
            sys_->pad.down(gs::BTN_TURBO))
            stoke();
    }
    if (dir != 0) face_ = dir;
    px_ = clampf(px_ + dir * 120.f * dt, 18.f, 302.f);

    gustT_ -= dt;
    if (gust_ > 0) gust_ -= dt;
    if (gustT_ <= 0) {
        gust_ = 1.7f;
        gustT_ = 7.5f;
    }
    float wind = gust_ > 0 ? 1.55f : 1.f;
    clock_ += dt;
    left_ -= dt;
    bool dead = false;
    for (int i = 0; i < N; i++) {
        fuels_[i] -= RATES[i] * wind * dt;
        if (fuels_[i] <= 0.f) {
            fuels_[i] = 0.f;
            dead = true;
        }
    }
    sys_->apu.tone(1, gust_ > 0 ? 70.f : 55.f, gust_ > 0 ? 0.03f : 0.012f);
    if (fed_ >= 0 && cds_ < 0.05f) fed_ = -1;

    if (dead) {
        mode_ = Mode::Fail;
        over_ = true;
        won_ = false;
        sys_->apu.tone(0, 90.f, 0.12f);
        sys_->apu.tone(1, 60.f, 0.1f);
        return;
    }
    if (left_ <= 0.f) {
        left_ = 0;
        mode_ = Mode::Victory;
        over_ = true;
        won_ = true;
        sys_->apu.tone(0, 330.f, 0.1f);
        sys_->apu.tone(1, 440.f, 0.08f);
        sys_->apu.tone(2, 554.f, 0.07f);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y + g.h * scale, g.h * scale, pal, false);
    }
}

void Game::sky() {
    float d = 0;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) d = clampf(1.f - left_ / DAWN, 0.f, 1.f);
    if (mode_ == Mode::Victory) d = 1.f;
    d = d * d;
    auto mix = [&](int r0, int g0, int b0, int r1, int g1, int b1) {
        auto m = [&](int a, int b) { return a + int(std::lround((b - a) * d)); };
        return gs::rgb4(m(r0, r1), m(g0, g1), m(b0, b1));
    };
    uint16_t top = mix(1, 1, 4, 7, 8, 13);
    uint16_t mid = mix(1, 1, 3, 12, 8, 6);
    uint16_t hor = mix(3, 2, 4, 15, 10, 5);
    uint16_t yard = mix(1, 1, 2, 4, 4, 3);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys_->vdp.road[y].on = false;
        sys_->vdp.lineFog[y] = 0;
        if (y >= 158) sys_->vdp.lineBackdrop[y] = yard;
        else if (y < 70) sys_->vdp.lineBackdrop[y] = top;
        else if (y < 120) sys_->vdp.lineBackdrop[y] = mid;
        else sys_->vdp.lineBackdrop[y] = hor;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    sky();

    float dawn = 0;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) dawn = clampf(1.f - left_ / DAWN, 0.f, 1.f);
    if (mode_ == Mode::Victory) dawn = 1.f;

    if (dawn < 0.65f) {
        spr(art_.moon, 268, 46, 26, PAL_HUD, false);
        static const float sx[8] = {24, 70, 110, 150, 196, 230, 300, 40};
        static const float sy[8] = {18, 30, 14, 36, 22, 12, 28, 48};
        for (int i = 0; i < 8; i++) spr(art_.star, sx[i], sy[i], 5, PAL_HUD, false);
    }
    spr(art_.wall, 160, 188, 96, PAL_STONE, false);

    int frame = int(flicker_ * 8.f) % 3;
    for (int i = 0; i < N; i++) {
        float f = fuels_[i];
        bool dark = mode_ == Mode::Fail && f <= 0.f;
        if (!dark && f > 0.02f && mode_ != Mode::Title) {
            float h = 16.f + f * 30.f;
            int fi = (frame + i) % 3;
            spr(art_.flame[fi], XS[i], 168, h, f < 0.22f ? PAL_RED : PAL_FLAME, false);
        } else if (mode_ == Mode::Title) {
            spr(art_.flame[(frame + i) % 3], XS[i], 168, 28, PAL_FLAME, false);
        }
        spr(art_.pot, XS[i], 186, 22, PAL_WOOD, false);
        if (mode_ == Mode::Watch || mode_ == Mode::Pause || mode_ == Mode::Victory || mode_ == Mode::Fail) {
            int pal = f < 0.22f ? PAL_RED : f < 0.45f ? PAL_AMBER : PAL_HUD;
            spr(art_.bar, XS[i] - 16 + std::max(2.f, f) * 16.f, 196, 5, pal, false);
        }
    }

    if (mode_ != Mode::Title) {
        int side = face_ < 0 ? 0 : 1;
        spr(art_.sentry[side], px_, 186, 52, PAL_MAN, false);
    } else {
        spr(art_.sentry[1], 160, 186, 52, PAL_MAN, false);
    }

    if (mode_ == Mode::Title) {
        text("SALLY", 160, 28, 1.4f, PAL_AMBER, 0);
        text("KEEP THE FLARES LIT", 160, 58, 0.55f, PAL_HUD, 0);
        text("UNTIL DAWN", 160, 74, 0.55f, PAL_HUD, 0);
        if (int(titleT_ * 2.f) % 2 == 0) text("PRESS START", 160, 208, 0.5f, PAL_AMBER, 0);
    } else if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        int sec = int(std::ceil(left_));
        char buf[32];
        std::snprintf(buf, sizeof(buf), "DAWN %d", sec);
        text(buf, 8, 8, 0.5f, PAL_HUD, -1);
        if (gust_ > 0) text("WIND", 312, 8, 0.45f, PAL_AMBER, 1);
        if (mode_ == Mode::Pause) text("HELD", 160, 96, 1.0f, PAL_AMBER, 0);
        int n = nearest();
        if (!bot_ && std::fabs(px_ - XS[n]) <= REACH) text("STOKE", XS[n], 128, 0.4f, PAL_AMBER, 0);
    } else if (mode_ == Mode::Victory) {
        text("DAWN", 160, 36, 1.3f, PAL_AMBER, 0);
        text("THE WATCH HELD", 160, 68, 0.55f, PAL_HUD, 0);
    } else if (mode_ == Mode::Fail) {
        text("DARK", 160, 36, 1.3f, PAL_RED, 0);
        text("THE WATCH IS OVER", 160, 68, 0.55f, PAL_HUD, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    update(DT);
    draw();
}

}  // namespace sally
