#include "trench.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace trench {
namespace {
constexpr int HOLD = 180 * 60;
constexpr int WARN = 42;
constexpr float GATE_X = 90.f;
constexpr float GATE_Y = 48.f;
}  // namespace

void Game::tone(int ch, float freq, float vol) {
    sys_->apu.tone(ch, freq, vol);
    toneT_ = freq > 0 ? 8 : 0;
}

void Game::bootPictures() {
    buildArt(sys_->vdp, art_);
    sys_->apu.setMaster(0.45f);
}

void Game::buildShoves() {
    shoves_.clear();
    uint32_t rng = 0x7EE11C01u;
    int t = 100;
    int dir = -1;
    while (t < HOLD - 30) {
        rng = rng * 1664525u + 1013904223u;
        int gap = 118 + int(rng % 50u);
        dir = -dir;
        if ((rng >> 8) & 3u) dir = ((rng >> 4) & 1u) ? 1 : -1;
        shoves_.push_back({t, dir});
        t += gap;
    }
}

void Game::begin() {
    mode_ = Mode::Play;
    play_ = 0;
    open_ = 0.08f;
    lean_ = 0;
    bracing_ = false;
    shoveIx_ = 0;
    flash_ = 0;
    shake_ = 0;
    won_ = false;
    over_ = false;
    buildShoves();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    bootPictures();
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
}

void Game::act() {
    const gs::Pad& p = sys_->pad;
    if (bot_) {
        bracing_ = true;
        lean_ = 0;
        if (shoveIx_ < int(shoves_.size())) {
            const Shove& h = shoves_[shoveIx_];
            if (play_ >= h.at - WARN && play_ <= h.at + 2) lean_ = h.dir;
        }
        if (mode_ == Mode::Title && age_ > 24) begin();
        if ((mode_ == Mode::Won || mode_ == Mode::Lost) && age_ > 90) {
            over_ = true;
            won_ = mode_ == Mode::Won;
        }
        return;
    }
    bracing_ = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) ||
               p.down(gs::BTN_X);
    lean_ = 0;
    if (p.down(gs::BTN_LEFT) || p.axisX < -0.35f) lean_ = -1;
    if (p.down(gs::BTN_RIGHT) || p.axisX > 0.35f) lean_ = 1;
    if (mode_ == Mode::Title && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) begin();
    if ((mode_ == Mode::Won || mode_ == Mode::Lost) && p.pressed(gs::BTN_START)) {
        age_ = 0;
        mode_ = Mode::Title;
    }
}

void Game::playTick() {
    play_++;
    float push = bracing_ ? 0.00004f : 0.0042f;
    if (bracing_) open_ -= 0.00085f;
    bool impacted = false;
    if (shoveIx_ < int(shoves_.size())) {
        const Shove& h = shoves_[shoveIx_];
        int lead = h.at - play_;
        if (lead <= WARN && lead > 0) {
            push += 0.0006f;
            if (lean_ == h.dir && bracing_) push -= 0.0009f;
            if ((play_ % 10) == 0) tone(1, lean_ == h.dir ? 520.f : 140.f, 0.07f);
        }
        if (play_ == h.at) {
            impacted = true;
            if (lean_ == h.dir && bracing_) {
                open_ -= 0.11f;
                shake_ = 4;
                tone(0, 80.f, 0.18f);
                flash_ = 6;
            } else {
                open_ += 0.24f;
                shake_ = 10;
                tone(0, 48.f, 0.26f);
                flash_ = 14;
            }
            shoveIx_++;
        }
    }
    if (!impacted) open_ += push;
    open_ = std::clamp(open_, 0.f, 1.2f);
    if (shake_ > 0) shake_--;
    if (flash_ > 0) flash_--;
    if (bracing_ && (play_ % 22) == 0) tone(2, 42.f + open_ * 30.f, 0.04f);

    secondsLeft_ = std::max(0, (HOLD - play_ + 59) / 60);
    gatePct_ = int(std::min(100.f, open_ * 100.f));
    if (open_ >= 1.f) {
        mode_ = Mode::Lost;
        age_ = 0;
        tone(0, 36.f, 0.3f);
        return;
    }
    if (play_ >= HOLD) {
        mode_ = Mode::Won;
        age_ = 0;
        open_ = std::min(open_, 0.99f);
        tone(0, 330.f, 0.16f);
    }
}

void Game::endTick() {
    age_++;
    if (mode_ == Mode::Won && age_ == 12) tone(0, 440.f, 0.14f);
    if (mode_ == Mode::Won && age_ == 24) tone(0, 554.f, 0.14f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    act();
    if (mode_ == Mode::Play) playTick();
    else if (mode_ == Mode::Won || mode_ == Mode::Lost) endTick();
    if (toneT_ > 0 && --toneT_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
        sys.apu.tone(2, 0, 0);
    }
    draw();
}

void Game::sprite(const gs::Mipped& m, float x, float y, float h, int pal, bool flip) {
    if (h < 1.f) return;
    float s = h / float(std::max(1, m.h));
    float w = float(m.w) * s;
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.x = int16_t(x - w * 0.5f);
    sp.y = int16_t(y - h * 0.5f);
    sp.w = int16_t(std::max(1.f, w));
    sp.h = int16_t(h);
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::hud(int col, int row, const char* s, int pal) {
    for (int i = 0; s[i]; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c < 32 || c > 127) c = ' ';
        int x = col + i;
        if (x < 0 || x >= 40 || row < 0 || row >= 28) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud((40 - n) / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = y < 90 ? gs::rgb4(1, 1, 3) : gs::rgb4(3, 2, 1);
        vdp.road[y].on = false;
    }

    int sx = 0;
    if (shake_ > 0) sx = (shake_ % 2 == 0) ? shake_ / 2 : -shake_ / 2;

    float gap = std::clamp(open_, 0.f, 1.f);
    float gateX = GATE_X + 74.f + gap * 70.f + sx;
    float flareX = 40.f + std::sin(age_ * 0.02f) * 18.f;
    float flareY = 28.f + std::cos(age_ * 0.015f) * 6.f;
    sprite(art_.flare, flareX, flareY, 16.f + (flash_ > 0 ? 6.f : 0.f), PAL_FLARE, false);
    sprite(art_.post, GATE_X - 8.f, 150.f, 96.f, PAL_GATE, false);
    sprite(art_.post, GATE_X + 156.f, 150.f, 96.f, PAL_GATE, false);

    int foes = 1 + int(gap * 3.f);
    for (int i = 0; i < foes; i++) {
        float fy = 148.f - i * 4.f;
        float fx = GATE_X + 24.f + i * 16.f + std::sin((play_ + i * 17) * 0.09f) * 3.f;
        sprite(art_.foe, fx, fy, 46.f - i * 3.f, PAL_FOE, i & 1);
    }
    sprite(art_.gate, gateX, GATE_Y + 64.f, 128.f, PAL_GATE, false);

    float armY = 150.f + (bracing_ ? 0.f : 5.f);
    float armX = gateX - 78.f;
    sprite(art_.shoulder, armX, armY, bracing_ ? 22.f : 16.f, PAL_SOLDIER, false);
    sprite(art_.shoulder, armX + 28.f, armY + 10.f, bracing_ ? 18.f : 14.f, PAL_SOLDIER, true);

    if (mode_ == Mode::Play && shoveIx_ < int(shoves_.size())) {
        const Shove& h = shoves_[shoveIx_];
        int lead = h.at - play_;
        if (lead <= WARN && lead > 0 && (lead < 8 || (play_ / 6) % 2 == 0)) {
            float cx = h.dir < 0 ? 28.f : 292.f;
            sprite(art_.chev, cx, 120.f, 18.f, PAL_WARN, h.dir > 0);
        }
    }

    char buf[48];
    int sec = mode_ == Mode::Title ? 180 : secondsLeft_;
    std::snprintf(buf, sizeof buf, "NIGHT %d:%02d", sec / 60, sec % 60);
    hud(1, 0, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "GATE %d%%", mode_ == Mode::Title ? 8 : gatePct_);
    hud(29, 0, buf, PAL_HUD);

    if (mode_ == Mode::Title) {
        hudC(8, "TRENCH DOOR", PAL_HUD);
        hudC(10, "ONE TRENCH. HOLD THE GATE.", PAL_HUD);
        hudC(12, "THREE MINUTES AND IT IS DONE", PAL_HUD);
        hudC(14, "BRACE  Z X C OR SPACE", PAL_HUD);
        hudC(15, "LEAN INTO THE SHOVE", PAL_HUD);
        if ((age_ / 30) % 2 == 0) hudC(20, "ENTER TO TAKE THE PARAPET", PAL_HUD);
    } else if (mode_ == Mode::Play) {
        hud(1, 26, bracing_ ? "SHOULDER IN" : "GATE WALKING", PAL_HUD);
        if (lean_ < 0) hud(16, 26, "LEAN LEFT", PAL_HUD);
        else if (lean_ > 0) hud(16, 26, "LEAN RIGHT", PAL_HUD);
        else hud(16, 26, "SET YOUR WEIGHT", PAL_HUD);
    } else if (mode_ == Mode::Won) {
        hudC(10, "GATE HELD", PAL_HUD);
        hudC(12, "THREE MINUTES", PAL_HUD);
        hudC(14, "THE TRENCH IS YOURS", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudC(10, "THE GATE GAVE", PAL_HUD);
        hudC(12, "THEY ARE IN THE TRENCH", PAL_HUD);
        hudC(16, "ENTER TO HOLD IT AGAIN", PAL_HUD);
    }
}

}  // namespace trench
