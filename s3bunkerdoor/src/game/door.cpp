#include "door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace door {
namespace {
constexpr int HOLD = 180 * 60;
constexpr int WARN = 42;
constexpr float DOOR_X = 86.f;
constexpr float DOOR_Y = 34.f;
}  // namespace

void Game::tone(int ch, float freq, float vol) {
    sys_->apu.tone(ch, freq, vol);
    toneT_ = freq > 0 ? 8 : 0;
}

void Game::bootPictures() {
    buildArt(sys_->vdp, art_);
    sys_->apu.setMaster(0.45f);
}

void Game::buildHeaves() {
    heaves_.clear();
    uint32_t rng = 0xB00D001u;
    int t = 100;
    int dir = -1;
    while (t < HOLD - 30) {
        rng = rng * 1664525u + 1013904223u;
        int gap = 118 + int(rng % 50u);
        dir = -dir;
        if ((rng >> 8) & 3u) dir = ((rng >> 4) & 1u) ? 1 : -1;
        heaves_.push_back({t, dir});
        t += gap;
    }
}

void Game::begin() {
    mode_ = Mode::Play;
    play_ = 0;
    seal_ = 0.08f;
    lean_ = 0;
    bracing_ = false;
    heaveIx_ = 0;
    flash_ = 0;
    shake_ = 0;
    won_ = false;
    over_ = false;
    buildHeaves();
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
        if (heaveIx_ < int(heaves_.size())) {
            const Heave& h = heaves_[heaveIx_];
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
    if (bracing_) seal_ -= 0.00085f;
    bool impacted = false;
    if (heaveIx_ < int(heaves_.size())) {
        const Heave& h = heaves_[heaveIx_];
        int lead = h.at - play_;
        if (lead <= WARN && lead > 0) {
            push += 0.0006f;
            if (lean_ == h.dir && bracing_) push -= 0.0009f;
            if ((play_ % 10) == 0) tone(1, lean_ == h.dir ? 660.f : 180.f, 0.08f);
        }
        if (play_ == h.at) {
            impacted = true;
            if (lean_ == h.dir && bracing_) {
                seal_ -= 0.11f;
                shake_ = 4;
                tone(0, 90.f, 0.2f);
                flash_ = 6;
            } else {
                seal_ += 0.24f;
                shake_ = 10;
                tone(0, 55.f, 0.28f);
                flash_ = 14;
            }
            heaveIx_++;
        }
    }
    if (!impacted) seal_ += push;
    seal_ = std::clamp(seal_, 0.f, 1.2f);
    if (shake_ > 0) shake_--;
    if (flash_ > 0) flash_--;
    if (bracing_ && (play_ % 18) == 0) tone(2, 48.f + seal_ * 40.f, 0.05f);

    secondsLeft_ = std::max(0, (HOLD - play_ + 59) / 60);
    sealPct_ = int(std::min(100.f, seal_ * 100.f));
    if (seal_ >= 1.f) {
        mode_ = Mode::Lost;
        age_ = 0;
        tone(0, 40.f, 0.3f);
        return;
    }
    if (play_ >= HOLD) {
        mode_ = Mode::Won;
        age_ = 0;
        seal_ = std::min(seal_, 0.99f);
        tone(0, 392.f, 0.18f);
    }
}

void Game::endTick() {
    age_++;
    if (mode_ == Mode::Won && age_ == 12) tone(0, 523.f, 0.16f);
    if (mode_ == Mode::Won && age_ == 24) tone(0, 659.f, 0.16f);
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

    int sx = 0;
    if (shake_ > 0) sx = (shake_ % 2 == 0) ? shake_ / 2 : -shake_ / 2;

    float open = std::clamp(seal_, 0.f, 1.f);
    float slabX = DOOR_X + 70.f + open * 78.f + sx;
    sprite(art_.moon, DOOR_X + 28.f, 58.f, 16.f, PAL_NIGHT, false);
    int figs = 1 + int(open * 3.f);
    for (int i = 0; i < figs; i++) {
        float fy = 150.f - i * 6.f;
        float fx = DOOR_X + 18.f + i * 14.f + std::sin((play_ + i * 20) * 0.08f) * 2.f;
        sprite(art_.figure, fx, fy, 52.f - i * 4.f, PAL_FIG, i & 1);
    }
    sprite(art_.slab, slabX, DOOR_Y + 75.f, 150.f, PAL_DOOR, false);

    float armY = 128.f + (bracing_ ? 0.f : 6.f);
    float armX = slabX - 78.f;
    sprite(art_.arm, armX, armY, bracing_ ? 30.f : 24.f, PAL_ARM, false);
    sprite(art_.arm, armX + 34.f, armY + 16.f, bracing_ ? 26.f : 20.f, PAL_ARM, true);

    if (mode_ == Mode::Play && heaveIx_ < int(heaves_.size())) {
        const Heave& h = heaves_[heaveIx_];
        int lead = h.at - play_;
        if (lead <= WARN && lead > 0 && (lead < 8 || (play_ / 6) % 2 == 0)) {
            float cx = h.dir < 0 ? 36.f : 284.f;
            sprite(art_.chev, cx, 110.f, 22.f, PAL_WARN, h.dir > 0);
        }
    }

    char buf[48];
    int sec = mode_ == Mode::Title ? 180 : secondsLeft_;
    std::snprintf(buf, sizeof buf, "WATCH %d:%02d", sec / 60, sec % 60);
    hud(1, 0, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "SEAL %d%%", mode_ == Mode::Title ? 0 : sealPct_);
    hud(28, 0, buf, flash_ > 8 ? PAL_HUD : PAL_HUD);

    if (mode_ == Mode::Title) {
        hudC(8, "BUNKER DOOR", PAL_HUD);
        hudC(10, "HOLD IT THREE MINUTES", PAL_HUD);
        hudC(12, "BRACE  Z X C OR SPACE", PAL_HUD);
        hudC(13, "LEAN INTO THE HEAVE", PAL_HUD);
        hudC(16, "MISS IT AND THE WATCH IS OVER", PAL_HUD);
        if ((age_ / 30) % 2 == 0) hudC(20, "ENTER TO TAKE THE WATCH", PAL_HUD);
    } else if (mode_ == Mode::Play) {
        hud(1, 26, bracing_ ? "BRACED" : "DOOR WALKING", bracing_ ? PAL_HUD : PAL_HUD);
        if (lean_ < 0) hud(16, 26, "LEAN LEFT", PAL_HUD);
        else if (lean_ > 0) hud(16, 26, "LEAN RIGHT", PAL_HUD);
        else hud(16, 26, "SET YOUR SHOULDER", PAL_HUD);
        int cells = std::clamp(sealPct_ / 5, 0, 20);
        hud(28, 26, "OPEN", PAL_HUD);
        for (int i = 0; i < 10; i++) {
            // bar sits in the seal number's shadow on row 1
            sys_->vdp.HUD.set(28 + i, 1, gs::entry(art_.font[i < cells / 2 ? 8 : 7], PAL_HUD));
        }
        (void)cells;
    } else if (mode_ == Mode::Won) {
        hudC(10, "DOOR HELD", PAL_HUD);
        hudC(12, "THREE MINUTES", PAL_HUD);
        hudC(14, "THE WATCH IS YOURS", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudC(10, "THE DOOR GAVE", PAL_HUD);
        hudC(12, "THE WATCH IS OVER", PAL_HUD);
        hudC(16, "ENTER TO TRY THE DOOR AGAIN", PAL_HUD);
    }
}

}  // namespace door
