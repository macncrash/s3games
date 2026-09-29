#include "door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace quarry {
namespace {
constexpr int HOLD = 180 * 60;
constexpr int WARN = 48;
constexpr float GATE_X = 160.f;
constexpr float GATE_Y = 128.f;
}  // namespace

void Game::tone(int ch, float freq, float vol) {
    sys_->apu.tone(ch, freq, vol);
    toneT_ = freq > 0 ? 8 : 0;
}

void Game::buildHauls() {
    hauls_.clear();
    uint32_t rng = 0x0A11u;
    int t = 90;
    int dir = 1;
    while (t < HOLD - 40) {
        rng = rng * 1664525u + 1013904223u;
        int gap = 100 + int(rng % 46u);
        dir = ((rng >> 5) & 1u) ? 1 : -1;
        hauls_.push_back({t, dir});
        t += gap;
    }
}

void Game::begin() {
    mode_ = Mode::Play;
    play_ = 0;
    gap_ = 0.06f;
    lean_ = 0;
    planted_ = false;
    haulIx_ = 0;
    flash_ = 0;
    shake_ = 0;
    won_ = false;
    over_ = false;
    secondsLeft_ = 180;
    buildHauls();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.45f);
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
}

void Game::act() {
    const gs::Pad& p = sys_->pad;
    if (bot_) {
        planted_ = true;
        lean_ = 0;
        if (haulIx_ < int(hauls_.size())) {
            const Haul& h = hauls_[haulIx_];
            if (play_ >= h.at - WARN && play_ <= h.at + 2) lean_ = h.dir;
        }
        if (mode_ == Mode::Title && age_ > 24) begin();
        if ((mode_ == Mode::Won || mode_ == Mode::Lost) && age_ > 90) {
            over_ = true;
            won_ = mode_ == Mode::Won;
        }
        return;
    }
    planted_ = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) ||
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
    float creep = planted_ ? 0.00003f : 0.0036f;
    if (planted_) gap_ -= 0.0007f;
    bool hit = false;
    if (haulIx_ < int(hauls_.size())) {
        const Haul& h = hauls_[haulIx_];
        int lead = h.at - play_;
        if (lead <= WARN && lead > 0) {
            creep += 0.0005f;
            if (lean_ == h.dir && planted_) creep -= 0.0008f;
            if ((play_ % 12) == 0) tone(1, lean_ == h.dir ? 520.f : 140.f, 0.07f);
        }
        if (play_ == h.at) {
            hit = true;
            if (lean_ == h.dir && planted_) {
                gap_ -= 0.09f;
                shake_ = 5;
                tone(0, 70.f, 0.18f);
                flash_ = 6;
            } else {
                gap_ += 0.22f;
                shake_ = 12;
                tone(0, 42.f, 0.28f);
                flash_ = 16;
            }
            haulIx_++;
        }
    }
    if (!hit) gap_ += creep;
    gap_ = std::clamp(gap_, 0.f, 1.2f);
    if (shake_ > 0) shake_--;
    if (flash_ > 0) flash_--;
    if (planted_ && (play_ % 20) == 0) tone(2, 36.f + gap_ * 30.f, 0.04f);

    secondsLeft_ = std::max(0, (HOLD - play_ + 59) / 60);
    openPct_ = int(std::min(100.f, gap_ * 100.f));
    if (gap_ >= 1.f) {
        mode_ = Mode::Lost;
        age_ = 0;
        tone(0, 36.f, 0.3f);
        return;
    }
    if (play_ >= HOLD) {
        mode_ = Mode::Won;
        age_ = 0;
        gap_ = std::min(gap_, 0.99f);
        openPct_ = int(std::min(100.f, gap_ * 100.f));
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
    vdp.A.enabled = false;

    int sx = 0;
    if (shake_ > 0) sx = (shake_ % 2 == 0) ? shake_ / 2 : -shake_ / 2;

    float open = std::clamp(gap_, 0.f, 1.f);
    float slabX = GATE_X + open * 86.f + sx;
    int clock = mode_ == Mode::Play ? play_ : age_;

    if (mode_ == Mode::Play && haulIx_ < int(hauls_.size())) {
        const Haul& h = hauls_[haulIx_];
        int lead = h.at - play_;
        if (lead <= WARN + 20 && lead > -8) {
            float t = 1.f - std::clamp(lead / float(WARN + 20), 0.f, 1.f);
            float from = h.dir < 0 ? -20.f : 340.f;
            float to = h.dir < 0 ? 78.f : 242.f;
            float tx = from + (to - from) * t;
            sprite(art_.truck, tx, 176.f, 28.f, PAL_TRUCK, h.dir > 0);
            sprite(art_.dust, tx - h.dir * 22.f, 184.f, 12.f, PAL_DUST, false);
            if (lead <= WARN && lead > 0 && (lead < 10 || (play_ / 6) % 2 == 0)) {
                float cx = h.dir < 0 ? 28.f : 292.f;
                sprite(art_.chev, cx, 100.f, 20.f, PAL_WARN, h.dir > 0);
            }
        }
    }

    sprite(art_.gate, slabX, GATE_Y, 132.f, PAL_GATE, false);
    float crewX = slabX - 70.f + lean_ * 10.f;
    float crewY = 150.f + (planted_ ? 0.f : 4.f);
    sprite(art_.crew, crewX, crewY, planted_ ? 52.f : 48.f, PAL_CREW, lean_ > 0);
    float shoreX = crewX + (lean_ == 0 ? 16.f : lean_ * 22.f);
    sprite(art_.shore, shoreX, 132.f + std::sin(clock * 0.2f) * (planted_ ? 0.f : 2.f), planted_ ? 16.f : 12.f,
           PAL_GATE, lean_ < 0);

    char buf[48];
    int sec = mode_ == Mode::Title ? 180 : secondsLeft_;
    std::snprintf(buf, sizeof buf, "WATCH %d:%02d", sec / 60, sec % 60);
    hud(1, 0, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "OPEN %d%%", mode_ == Mode::Title ? 0 : openPct_);
    hud(29, 0, buf, flash_ > 8 ? PAL_HUD : PAL_HUD);

    if (mode_ == Mode::Title) {
        hudC(8, "QUARRY DOOR", PAL_HUD);
        hudC(10, "HOLD THE GATE THREE MINUTES", PAL_HUD);
        hudC(12, "PLANT THE SHORE  Z X C SPACE", PAL_HUD);
        hudC(13, "STEP INTO THE HAUL", PAL_HUD);
        hudC(16, "THE GATE OPENS AND YOU LOSE", PAL_HUD);
        if ((age_ / 30) % 2 == 0) hudC(20, "ENTER TO TAKE THE GATE", PAL_HUD);
    } else if (mode_ == Mode::Play) {
        hud(1, 26, planted_ ? "SHORE SET" : "GATE CREEPING", PAL_HUD);
        if (lean_ < 0) hud(18, 26, "LEFT JAMB", PAL_HUD);
        else if (lean_ > 0) hud(18, 26, "RIGHT JAMB", PAL_HUD);
        else hud(16, 26, "PICK A JAMB", PAL_HUD);
    } else if (mode_ == Mode::Won) {
        hudC(10, "GATE HELD", PAL_HUD);
        hudC(12, "THREE MINUTES", PAL_HUD);
        hudC(14, "THE QUARRY IS YOURS", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudC(10, "THE GATE OPENED", PAL_HUD);
        hudC(12, "THE QUARRY IS LOST", PAL_HUD);
        hudC(16, "ENTER TO TAKE THE GATE AGAIN", PAL_HUD);
    }
}

}  // namespace quarry
