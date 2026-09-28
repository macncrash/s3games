#include "game/tower.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace tower {
namespace {

constexpr int NFL = 4;
constexpr float NIGHT = 36.0f;
constexpr float BURN = 8.5f;
constexpr float WALK = 168.0f;
constexpr float REACH = 18.0f;
constexpr float GRACE = 2.6f;
constexpr float STX[NFL] = {52.0f, 118.0f, 202.0f, 268.0f};
constexpr float BRAZIER_Y = 156.0f;

}  // namespace

int Game::flaresLit() const {
    int n = 0;
    for (int i = 0; i < NFL; i++)
        if (fuel_[i] > 0.02f) n++;
    return n;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Watch) return 1;
    return 2;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.0f / 16777216.0f);
}

int Game::nearest() const {
    int best = 0;
    float d = 1e9f;
    for (int i = 0; i < NFL; i++) {
        float a = std::fabs(px_ - STX[i]);
        if (a < d) {
            d = a;
            best = i;
        }
    }
    return best;
}

void Game::lightAt(int i) {
    if (i < 0 || i >= NFL) return;
    if (std::fabs(px_ - STX[i]) > REACH) return;
    if (lightCd_ > 0) return;
    if (fuel_[i] > 0.92f) return;
    fuel_[i] = 1.0f;
    dark_[i] = 0;
    lightCd_ = 0.18f;
    relights_++;
    sys_->apu.noiseBurst(0.28f, 900.0f, 0.08f);
    sys_->apu.tone(0, 520.0f, 0.12f);
    sys_->rumble(0.2f, 0.45f, 40);
}

void Game::finish(bool win) {
    mode_ = win ? Mode::Won : Mode::Lost;
    won_ = win;
    over_ = true;
    t_ = 0;
    sys_->apu.tone(0, 0, 0);
    if (win) {
        sys_->apu.tone(1, 660.0f, 0.18f);
        sys_->apu.tone(2, 880.0f, 0.14f);
    } else {
        sys_->apu.noiseBurst(0.45f, 240.0f, 0.35f);
    }
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    t_ = 0;
    clock_ = 0;
    relights_ = 0;
    px_ = 160;
    face_ = 1;
    lightCd_ = 0;
    gustT_ = 1.6f;
    gustLeft_ = 0;
    gust_ = -1;
    for (int i = 0; i < NFL; i++) {
        fuel_[i] = 0.82f + 0.04f * i;
        dark_[i] = 0;
    }
    sys_->apu.noise(0.04f, 900.0f, false);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    over_ = false;
    won_ = false;
    mode_ = Mode::Title;
    t_ = 0;
    frame_ = 0;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 2, 6));
    sys.apu.setMaster(0.45f);
    sys.apu.setEcho(0.18f, 0.25f, 0.12f);
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

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    x -= w * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sky() {
    float dawn = std::clamp(clock_ / NIGHT, 0.0f, 1.0f);
    if (mode_ == Mode::Title) dawn = 0;
    if (mode_ == Mode::Won) dawn = 1;
    int tr = 1 + int(6 * dawn), tg = 1 + int(4 * dawn), tb = 5 + int(4 * dawn);
    int hr = 3 + int(12 * dawn), hg = 2 + int(5 * dawn), hb = 6 - int(3 * dawn);
    if (hb < 2) hb = 2;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        float k = u * u;
        int r = int(tr + (hr - tr) * k);
        int g = int(tg + (hg - tg) * k);
        int b = int(tb + (hb - tb) * k);
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    float move = 0;
    bool light = false;
    int aim = nearest();

    if (bot_) {
        int worst = 0;
        for (int i = 1; i < NFL; i++)
            if (fuel_[i] < fuel_[worst]) worst = i;
        aim = worst;
        float dx = STX[aim] - px_;
        if (std::fabs(dx) > 6.0f) move = dx > 0 ? 1.0f : -1.0f;
        light = std::fabs(dx) <= REACH && fuel_[aim] < 0.78f;
    } else {
        move = std::fabs(pad.axisX) > 0.12f ? (pad.axisX > 0 ? 1.0f : -1.0f)
                                            : float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        light = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_B);
        aim = nearest();
    }

    if (move != 0) face_ = move;
    px_ += move * WALK * dt;
    px_ = std::clamp(px_, 28.0f, 292.0f);
    if (lightCd_ > 0) lightCd_ -= dt;
    if (light) lightAt(aim);

    gustT_ -= dt;
    if (gustT_ <= 0) {
        gust_ = int(rnd() * NFL);
        if (gust_ >= NFL) gust_ = NFL - 1;
        gustLeft_ = 1.5f;
        gustT_ = 3.4f + rnd() * 1.2f;
        sys_->apu.noiseBurst(0.12f, 400.0f, 0.2f);
    }
    if (gustLeft_ > 0) gustLeft_ -= dt;
    else gust_ = -1;

    for (int i = 0; i < NFL; i++) {
        float rate = 1.0f / BURN;
        if (gust_ == i && gustLeft_ > 0) rate *= 2.8f;
        fuel_[i] -= rate * dt;
        if (fuel_[i] < 0) fuel_[i] = 0;
        if (fuel_[i] <= 0) dark_[i] += dt;
        else dark_[i] = 0;
        if (dark_[i] > GRACE) {
            finish(false);
            return;
        }
    }

    clock_ += dt;
    if (clock_ >= NIGHT) {
        bool held = true;
        for (int i = 0; i < NFL; i++)
            if (fuel_[i] <= 0) held = false;
        finish(held);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();

    int flick = (frame_ / 7) % 3;
    for (int i = 0; i < NFL; i++) {
        if (fuel_[i] > 0.02f) {
            float h = 16.0f + fuel_[i] * 30.0f;
            int fr = (flick + i) % 3;
            spr(art_.flame[fr], STX[i], BRAZIER_Y - 22.0f, h, PAL_FLAME, fr == 2);
        }
    }
    spr(art_.keeper, px_, 138.0f, 58.0f, PAL_KEEPER, face_ < 0);
    for (int i = 0; i < NFL; i++) spr(art_.brazier, STX[i], BRAZIER_Y, 28.0f, PAL_IRON, false);
    spr(art_.tower, 160.0f, 108.0f, 150.0f, PAL_STONE, false);

    float dawn = std::clamp(clock_ / NIGHT, 0.0f, 1.0f);
    if (mode_ != Mode::Won && dawn < 0.85f) {
        spr(art_.moon, 46.0f + dawn * 20.0f, 36.0f, 28.0f, PAL_MOON, false);
        static const float sx[7] = {24, 70, 110, 180, 230, 270, 300};
        static const float sy[7] = {18, 28, 14, 22, 16, 30, 12};
        for (int i = 0; i < 7; i++) {
            if ((frame_ + i * 3) % 40 < 3) continue;
            spr(art_.star, sx[i], sy[i], 7.0f, PAL_MOON, false);
        }
    }

    if (mode_ == Mode::Title) {
        text("TOWER DAWN", 160, 48, 1.15f, PAL_AMBER);
        hudC(22, "KEEP THE FLARES LIT", PAL_HUD);
        hudC(24, "UNTIL DAWN", PAL_AMBER);
        hudC(26, "START", PAL_HUD);
    } else if (mode_ == Mode::Watch || mode_ == Mode::Won || mode_ == Mode::Lost) {
        int left = int(std::ceil(std::max(0.0f, NIGHT - clock_)));
        char line[40];
        std::snprintf(line, sizeof(line), "DAWN %02d", left);
        hud(1, 1, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "LIT %d", flaresLit());
        hud(30, 1, line, flaresLit() == NFL ? PAL_AMBER : PAL_RED);
        for (int i = 0; i < NFL; i++) {
            int bars = int(std::ceil(fuel_[i] * 6.0f));
            if (bars < 0) bars = 0;
            if (bars > 6) bars = 6;
            std::string m(size_t(bars), '#');
            int pal = fuel_[i] < 0.28f ? PAL_RED : PAL_AMBER;
            hud(2 + i * 10, 26, m, pal);
        }
        if (mode_ == Mode::Watch) hudC(3, "A LIGHTS THE FLARE", PAL_HUD);
        if (mode_ == Mode::Won) {
            text("DAWN", 160, 52, 1.4f, PAL_AMBER);
            hudC(23, "THE FLARES HELD", PAL_HUD);
        }
        if (mode_ == Mode::Lost) {
            text("DARK", 160, 52, 1.4f, PAL_RED);
            hudC(23, "THE SIGNAL FAILED", PAL_RED);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    frame_++;
    const float dt = 1.0f / 60.0f;
    t_ += dt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ && t_ > 0.35f) beginWatch();
        else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) beginWatch();
    } else if (mode_ == Mode::Watch) {
        update(dt);
    } else if (!bot_ && (mode_ == Mode::Lost || mode_ == Mode::Won)) {
        if (pad.pressed(gs::BTN_START)) {
            over_ = false;
            won_ = false;
            mode_ = Mode::Title;
            t_ = 0;
            clock_ = 0;
        }
    }

    // A human replay after the sim flag is irrelevant. Once the watch ends, stay ended.
    if (bot_ && (mode_ == Mode::Won || mode_ == Mode::Lost)) over_ = true;

    if (frame_ % 50 == 0 && mode_ == Mode::Watch) sys.apu.tone(0, 0, 0);

    draw();
}

}  // namespace tower
