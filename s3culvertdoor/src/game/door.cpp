#include "door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace culvertdoor {
namespace {
constexpr int HOLD = 180 * 60;
constexpr int WARN = 40;
constexpr float MOUTH_X = 160.f;
constexpr float MOUTH_Y = 112.f;
}  // namespace

void Game::tone(int ch, float freq, float vol) {
    sys_->apu.tone(ch, freq, vol);
    toneT_ = freq > 0 ? 8 : 0;
}

void Game::bootPictures() {
    buildArt(sys_->vdp, art_);
    sys_->apu.setMaster(0.45f);
}

void Game::buildSurges() {
    surges_.clear();
    uint32_t rng = 0xC011E7u;
    int t = 90;
    int dir = -1;
    while (t < HOLD - 40) {
        rng = rng * 1664525u + 1013904223u;
        int gap = 100 + int(rng % 70u);
        dir = ((rng >> 3) & 1u) ? 1 : -1;
        surges_.push_back({t, dir});
        t += gap;
    }
}

void Game::begin() {
    mode_ = Mode::Play;
    play_ = 0;
    seal_ = 0.10f;
    water_ = 0.18f;
    lean_ = 0;
    bracing_ = false;
    surgeIx_ = 0;
    flash_ = 0;
    shake_ = 0;
    won_ = false;
    over_ = false;
    buildSurges();
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
        if (surgeIx_ < int(surges_.size())) {
            const Surge& h = surges_[surgeIx_];
            if (play_ >= h.at - WARN && play_ <= h.at + 2) lean_ = h.dir;
        }
        if (mode_ == Mode::Title && age_ > 20) begin();
        if ((mode_ == Mode::Won || mode_ == Mode::Lost) && age_ > 50) {
            won_ = mode_ == Mode::Won;
            over_ = true;
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
    if (bracing_) {
        seal_ -= 0.0011f;
        water_ -= 0.00055f;
    } else {
        seal_ += 0.0048f;
        water_ += 0.0026f;
    }
    if (surgeIx_ < int(surges_.size())) {
        const Surge& h = surges_[surgeIx_];
        int lead = h.at - play_;
        if (lead <= WARN && lead > 0) {
            if (lean_ == h.dir && bracing_) {
                seal_ -= 0.0004f;
            } else {
                seal_ += 0.0008f;
                water_ += 0.0003f;
            }
            if ((play_ % 8) == 0) tone(1, lean_ == h.dir ? 520.f : 140.f, 0.07f);
        }
        if (play_ == h.at) {
            if (lean_ == h.dir && bracing_) {
                seal_ -= 0.09f;
                water_ -= 0.03f;
                shake_ = 4;
                flash_ = 5;
                tone(0, 110.f, 0.16f);
            } else {
                seal_ += 0.20f;
                water_ += 0.10f;
                shake_ = 9;
                flash_ = 12;
                tone(0, 48.f, 0.26f);
            }
            surgeIx_++;
        }
    }
    seal_ = std::clamp(seal_, 0.f, 1.15f);
    water_ = std::clamp(water_, 0.f, 1.15f);
    if (shake_ > 0) shake_--;
    if (flash_ > 0) flash_--;
    if (bracing_ && (play_ % 22) == 0) tone(2, 70.f + water_ * 30.f, 0.04f);

    secondsLeft_ = std::max(0, (HOLD - play_ + 59) / 60);
    sealPct_ = int(std::min(100.f, seal_ * 100.f));
    waterPct_ = int(std::min(100.f, water_ * 100.f));
    if (seal_ >= 1.f || water_ >= 1.f) {
        mode_ = Mode::Lost;
        age_ = 0;
        tone(0, 36.f, 0.3f);
        return;
    }
    if (play_ >= HOLD) {
        mode_ = Mode::Won;
        age_ = 0;
        won_ = true;
        seal_ = std::min(seal_, 0.4f);
        tone(0, 392.f, 0.18f);
    }
}

void Game::endTick() {
    age_++;
    if (mode_ == Mode::Won && age_ == 10) tone(0, 494.f, 0.14f);
    if (mode_ == Mode::Won && age_ == 22) tone(0, 587.f, 0.14f);
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
    float leafX = MOUTH_X + open * 70.f + sx;
    int figs = int(open * 4.f);
    for (int i = 0; i < figs; i++) {
        float fy = 150.f - i * 8.f;
        float fx = MOUTH_X - 18.f + i * 12.f;
        sprite(art_.figure, fx, fy, 40.f - i * 3.f, PAL_FIG, i & 1);
    }
    sprite(art_.leaf, leafX, MOUTH_Y + sx * 0.2f, 132.f, PAL_DOOR, false);

    float wy = 176.f - std::clamp(water_, 0.f, 1.f) * 70.f;
    sprite(art_.sheet, 160.f + sx, wy, 18.f + water_ * 10.f, PAL_WATER, false);

    float armY = 168.f + (bracing_ ? -6.f : 4.f);
    sprite(art_.shoulder, leafX - 36.f, armY, bracing_ ? 28.f : 22.f, PAL_BODY, false);
    sprite(art_.shoulder, leafX + 28.f, armY + 8.f, bracing_ ? 24.f : 18.f, PAL_BODY, true);

    if (mode_ == Mode::Play && surgeIx_ < int(surges_.size())) {
        const Surge& h = surges_[surgeIx_];
        int lead = h.at - play_;
        if (lead <= WARN && lead > 0 && (lead < 10 || (play_ / 5) % 2 == 0)) {
            float cx = h.dir < 0 ? 48.f : 272.f;
            sprite(art_.chev, cx, 100.f, 20.f, PAL_WARN, h.dir > 0);
        }
    }

    char buf[48];
    int sec = mode_ == Mode::Title ? 180 : secondsLeft_;
    std::snprintf(buf, sizeof buf, "WATCH %d:%02d", sec / 60, sec % 60);
    hud(1, 0, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "LEAF %d", mode_ == Mode::Title ? 10 : sealPct_);
    hud(16, 0, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "WATER %d", mode_ == Mode::Title ? 18 : waterPct_);
    hud(28, 0, buf, flash_ > 6 ? PAL_WARN : PAL_HUD);

    if (mode_ == Mode::Title) {
        hudC(8, "CULVERT DOOR", PAL_HUD);
        hudC(10, "HOLD IT THREE MINUTES", PAL_HUD);
        hudC(12, "BRACE  Z X C OR SPACE", PAL_HUD);
        hudC(13, "LEAN INTO THE SURGE", PAL_HUD);
        hudC(16, "THE LEAF OR THE WATER ENDS IT", PAL_HUD);
        if ((age_ / 30) % 2 == 0) hudC(20, "ENTER TO TAKE THE DOOR", PAL_HUD);
    } else if (mode_ == Mode::Play) {
        hud(1, 26, bracing_ ? "BRACED ON THE LEAF" : "THE DOOR IS WALKING", PAL_HUD);
        if (lean_ < 0) hud(24, 26, "LEAN LEFT", PAL_HUD);
        else if (lean_ > 0) hud(23, 26, "LEAN RIGHT", PAL_HUD);
        else hud(22, 26, "SET A SHOULDER", PAL_HUD);
    } else if (mode_ == Mode::Won) {
        hudC(10, "DOOR HELD", PAL_HUD);
        hudC(12, "THREE MINUTES", PAL_HUD);
        hudC(14, "THE CULVERT IS DONE", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudC(10, seal_ >= 1.f ? "THE LEAF GAVE" : "THE WATER TOOK THE LOCK", PAL_HUD);
        hudC(12, "THE WATCH IS OVER", PAL_HUD);
        hudC(16, "ENTER TO TAKE THE DOOR AGAIN", PAL_HUD);
    }
}

}  // namespace culvertdoor
