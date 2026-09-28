#include "game/tower.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace tower {
namespace {
constexpr int HOLD = 180 * 60;
constexpr int WARN = 48;
constexpr int LANES[3] = {118, 148, 178};
}  // namespace

void Game::tone(int ch, float freq, float vol) {
    sys_->apu.tone(ch, freq, vol);
    toneT_ = freq > 0 ? 6 : 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    play_ = 0;
    lane_ = 1;
    lean_ = 0;
    bracing_ = false;
    bar_ = 0.82f;
    flash_ = 0;
    shake_ = 0;
    climbIx_ = 0;
    won_ = false;
    over_ = false;
    climbs_.clear();
    uint32_t rng = 0x70E5u;
    int t = 70;
    int lane = 1;
    while (t < HOLD - 40) {
        rng = rng * 1664525u + 1013904223u;
        int step = int(rng % 3u);
        if (step == 0) lane = (lane + 2) % 3;
        else if (step == 1) lane = (lane + 1) % 3;
        climbs_.push_back({t, lane});
        t += 70 + int((rng >> 8) % 36u);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.42f);
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
}

int Game::secondsLeft() const {
    if (mode_ != Mode::Play) return mode_ == Mode::Won ? 0 : 180;
    int left = HOLD - play_;
    if (left < 0) left = 0;
    return (left + 59) / 60;
}

int Game::barPct() const { return std::clamp(int(bar_ * 100.f + 0.5f), 0, 100); }

void Game::act() {
    const gs::Pad& p = sys_->pad;
    if (bot_) {
        bracing_ = true;
        int want = lane_;
        if (climbIx_ < int(climbs_.size())) {
            const Climb& c = climbs_[climbIx_];
            if (play_ >= c.at - WARN - 12) want = c.lane;
        }
        if (lane_ < want) lane_++;
        else if (lane_ > want) lane_--;
        if (mode_ == Mode::Title && age_ > 20) begin();
        if ((mode_ == Mode::Won || mode_ == Mode::Lost) && age_ > 50) {
            over_ = true;
            won_ = mode_ == Mode::Won;
        }
        return;
    }
    bracing_ = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X) ||
               p.down(gs::BTN_TURBO);
    if (p.pressed(gs::BTN_LEFT)) lane_ = std::max(0, lane_ - 1);
    if (p.pressed(gs::BTN_RIGHT)) lane_ = std::min(2, lane_ + 1);
    if (mode_ == Mode::Title && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) begin();
    if ((mode_ == Mode::Won || mode_ == Mode::Lost) && p.pressed(gs::BTN_START)) {
        age_ = 0;
        mode_ = Mode::Title;
    }
}

void Game::playTick() {
    play_++;
    if (bracing_) bar_ = std::min(1.f, bar_ + 0.00035f);
    else bar_ = std::max(0.f, bar_ - 0.00055f);
    if (climbIx_ < int(climbs_.size()) && play_ == climbs_[climbIx_].at) {
        const Climb& c = climbs_[climbIx_];
        bool held = bracing_ && lane_ == c.lane;
        if (held) {
            bar_ = std::min(1.f, bar_ + 0.02f);
            shake_ = 3;
            flash_ = 5;
            tone(0, 140.f, 0.16f);
        } else {
            bar_ = std::max(0.f, bar_ - 0.16f);
            shake_ = 8;
            flash_ = 10;
            tone(0, 58.f, 0.24f);
        }
        climbIx_++;
    }
    if (bar_ <= 0.f) {
        mode_ = Mode::Lost;
        age_ = 0;
        tone(0, 40.f, 0.3f);
    } else if (play_ >= HOLD) {
        mode_ = Mode::Won;
        age_ = 0;
        tone(0, 440.f, 0.22f);
    }
}

void Game::spr(const gs::Image& img, int x, int y, int w, int h, int pal, bool flip) {
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hudText(int col, int row, const char* s, int pal) {
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        int x = col + i;
        if (x < 0 || x > 39 || row < 0 || row > 27) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::sky() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int n = y < 150 ? y : 150;
        int r = 1 + n / 40;
        int g = 2 + n / 28;
        int b = 5 + n / 18;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.clear();
    vdp.B.clear();
    sky();
    int sh = shake_ > 0 ? ((play_ & 1) ? shake_ : -shake_) : 0;
    spr(art_.moon, 24, 12, 28, 28, PAL_MOON);
    spr(art_.tower, 100 + sh, 12, 120, 200, PAL_STONE);
    float open = (1.f - bar_) * 22.f;
    int dy = 132 + sh;
    spr(art_.doorL, int(132 - open) + sh, dy, 28, 70, flash_ > 4 ? PAL_GOLD : PAL_WOOD);
    spr(art_.doorR, int(160 + open) + sh, dy, 28, 70, flash_ > 4 ? PAL_GOLD : PAL_WOOD);
    if (bracing_ && mode_ == Mode::Play) spr(art_.bar, LANES[lane_] - 16, 156, 40, 8, PAL_IRON);
    if (mode_ == Mode::Play) {
        for (int i = climbIx_; i < int(climbs_.size()); i++) {
            int lead = climbs_[i].at - play_;
            if (lead > 90) break;
            if (lead < 0) continue;
            int y = 200 - (90 - lead) * 2;
            bool hot = lead <= WARN;
            spr(art_.climber, LANES[climbs_[i].lane] - 6, y, 16, 24, hot ? PAL_GOLD : PAL_FOE, (play_ / 8) & 1);
        }
        spr(art_.guard, LANES[lane_] - 10, bracing_ ? 118 : 114, 22, 34, PAL_GUARD, lean_ < 0);
    } else {
        spr(art_.guard, 138, 114, 22, 34, PAL_GUARD);
    }
    int wave = (age_ / 20) % 3;
    spr(art_.flag, 152, 8 + wave, 22, 16, PAL_MOON);

    char buf[40];
    int sec = secondsLeft();
    std::snprintf(buf, sizeof buf, "RELIEF %d:%02d", sec / 60, sec % 60);
    hudText(1, 1, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "BAR %d%%", barPct());
    hudText(28, 1, buf, PAL_GOLD);
    if (mode_ == Mode::Title) {
        hudText(12, 8, "TOWER DOOR", PAL_HUD);
        hudText(6, 12, "HOLD THE DOOR 3:00", PAL_GOLD);
        hudText(5, 20, "LEFT RIGHT  HOLD A", PAL_HUD);
        hudText(10, 22, "START TO BAR", PAL_HUD);
    } else if (mode_ == Mode::Won) {
        hudText(11, 10, "THE HORN", PAL_GOLD);
        hudText(8, 12, "DOOR HELD", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudText(10, 10, "DOOR BROKE", PAL_GOLD);
        hudText(9, 12, "START RETRY", PAL_HUD);
    } else if (climbIx_ < int(climbs_.size())) {
        int lead = climbs_[climbIx_].at - play_;
        if (lead > 0 && lead <= WARN) {
            const char* side = climbs_[climbIx_].lane == 0 ? "LEFT JAMB" : climbs_[climbIx_].lane == 2 ? "RIGHT JAMB" : "CENTER";
            hudText(14, 25, side, PAL_GOLD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    if (toneT_ > 0 && --toneT_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
    if (shake_ > 0) shake_--;
    if (flash_ > 0) flash_--;
    act();
    if (mode_ == Mode::Play) playTick();
    draw();
}

}  // namespace tower
