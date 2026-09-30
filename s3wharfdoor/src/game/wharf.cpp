#include "wharf.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace wharf {
namespace {
constexpr int HOLD = 180 * 60;
constexpr int TIDE_WARN = 36;
constexpr int HOIST_WARN = 28;
constexpr float DOOR_X = 112.f;
constexpr float DOOR_Y = 62.f;
}  // namespace

void Game::tone(int ch, float freq, float vol) {
    sys_->apu.tone(ch, freq, vol);
    toneT_ = freq > 0 ? 8 : 0;
}

void Game::schedule() {
    tides_.clear();
    hoists_.clear();
    uint32_t rng = 0x57A7F001u;
    int t = 140;
    int dir = -1;
    while (t < HOLD - 40) {
        rng = rng * 1664525u + 1013904223u;
        dir = ((rng >> 5) & 1u) ? 1 : -1;
        tides_.push_back({t, dir});
        t += 100 + int(rng % 40u);
    }
    t = 220;
    while (t < HOLD - 50) {
        rng = rng * 1664525u + 1013904223u;
        hoists_.push_back({t});
        t += 210 + int(rng % 50u);
    }
}

void Game::begin() {
    mode_ = Mode::Play;
    play_ = 0;
    gap_ = 0.1f;
    lean_ = 0;
    pin_ = 0;
    bracing_ = false;
    tideIx_ = 0;
    hoistIx_ = 0;
    flash_ = 0;
    shake_ = 0;
    crateY_ = -40.f;
    won_ = false;
    over_ = false;
    schedule();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.45f);
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
    secondsLeft_ = 180;
    gapPct_ = 10;
}

void Game::act() {
    const gs::Pad& p = sys_->pad;
    if (bot_) {
        bracing_ = true;
        lean_ = 0;
        pin_ = 0;
        if (tideIx_ < int(tides_.size())) {
            const Tide& h = tides_[tideIx_];
            if (play_ >= h.at - TIDE_WARN && play_ <= h.at + 2) lean_ = h.dir;
        }
        if (hoistIx_ < int(hoists_.size())) {
            const Hoist& h = hoists_[hoistIx_];
            if (play_ >= h.at - HOIST_WARN && play_ <= h.at + 2) pin_ = 1;
        }
        if (mode_ == Mode::Title && age_ > 20) begin();
        if ((mode_ == Mode::Won || mode_ == Mode::Lost) && age_ > 70) {
            over_ = true;
            won_ = mode_ == Mode::Won;
        }
        return;
    }
    bracing_ = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) ||
               p.down(gs::BTN_X) || p.down(gs::BTN_Z);
    lean_ = 0;
    if (p.down(gs::BTN_LEFT) || p.axisX < -0.35f) lean_ = -1;
    if (p.down(gs::BTN_RIGHT) || p.axisX > 0.35f) lean_ = 1;
    pin_ = (p.down(gs::BTN_UP) || p.axisY > 0.45f) ? 1 : 0;
    if (mode_ == Mode::Title && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) begin();
    if ((mode_ == Mode::Won || mode_ == Mode::Lost) && p.pressed(gs::BTN_START)) {
        age_ = 0;
        mode_ = Mode::Title;
    }
}

void Game::playTick() {
    play_++;
    float walk = bracing_ ? -0.00045f : 0.0038f;
    if (tideIx_ < int(tides_.size())) {
        const Tide& h = tides_[tideIx_];
        int lead = h.at - play_;
        if (lead <= TIDE_WARN && lead > 0) {
            walk += 0.0007f;
            if (bracing_ && lean_ == h.dir) walk -= 0.0011f;
            if ((play_ % 12) == 0) tone(1, lean_ == h.dir ? 520.f : 140.f, 0.07f);
        }
        if (play_ == h.at) {
            if (bracing_ && lean_ == h.dir) {
                gap_ -= 0.07f;
                shake_ = 4;
                tone(0, 98.f, 0.16f);
                flash_ = 5;
            } else {
                gap_ += bracing_ ? 0.2f : 0.32f;
                shake_ = 12;
                tone(0, 48.f, 0.28f);
                flash_ = 16;
            }
            tideIx_++;
        }
    }
    if (hoistIx_ < int(hoists_.size())) {
        const Hoist& h = hoists_[hoistIx_];
        int lead = h.at - play_;
        if (lead <= HOIST_WARN && lead > 0) {
            float u = 1.f - float(lead) / float(HOIST_WARN);
            crateY_ = -20.f + u * 90.f;
            if ((play_ % 14) == 0) tone(2, 880.f, 0.06f);
        }
        if (play_ == h.at) {
            if (pin_) {
                gap_ -= 0.04f;
                crateY_ = 62.f;
                tone(0, 240.f, 0.14f);
                flash_ = 4;
            } else {
                gap_ += 0.22f;
                crateY_ = 100.f;
                shake_ = 14;
                tone(0, 60.f, 0.26f);
                flash_ = 14;
            }
            hoistIx_++;
        }
    } else if (crateY_ > -30.f) {
        crateY_ -= 1.4f;
    }
    if (hoistIx_ < int(hoists_.size()) && hoists_[hoistIx_].at - play_ > HOIST_WARN && crateY_ > -30.f) crateY_ -= 1.4f;

    gap_ += walk;
    gap_ = std::clamp(gap_, 0.f, 1.15f);
    if (shake_ > 0) shake_--;
    if (flash_ > 0) flash_--;
    if (bracing_ && (play_ % 22) == 0) tone(2, 42.f + gap_ * 30.f, 0.04f);

    secondsLeft_ = std::max(0, (HOLD - play_ + 59) / 60);
    gapPct_ = int(std::min(100.f, gap_ * 100.f));
    if (gap_ >= 1.f) {
        mode_ = Mode::Lost;
        age_ = 0;
        tone(0, 36.f, 0.3f);
        return;
    }
    if (play_ >= HOLD) {
        mode_ = Mode::Won;
        age_ = 0;
        won_ = true;
        gap_ = std::min(gap_, 0.99f);
        tone(0, 392.f, 0.18f);
    }
}

void Game::endTick() {
    age_++;
    if (mode_ == Mode::Won && age_ == 10) tone(0, 494.f, 0.15f);
    if (mode_ == Mode::Won && age_ == 22) tone(0, 587.f, 0.15f);
    if (mode_ == Mode::Won && age_ == 34) tone(0, 784.f, 0.14f);
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
    if (h < 1.f || m.h < 1) return;
    float s = h / float(m.h);
    float w = float(m.w) * s;
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.x = int16_t(x);
    sp.y = int16_t(y);
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

    const uint16_t sky = gs::rgb4(1, 2, 6);
    const uint16_t haze = gs::rgb4(3, 5, 9);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.road[y].on = false;
        vdp.lineBackdrop[y] = y < 90 ? sky : haze;
        vdp.lineFog[y] = 0;
    }
    int clock = mode_ == Mode::Play ? play_ : age_;
    for (int y = 168; y < gs::SCREEN_H; y++) {
        gs::RoadLine& r = vdp.road[y];
        r.on = true;
        r.style = 2;
        r.cx = 160.f + std::sin(clock * 0.01f) * 6.f;
        r.hw = 520.f;
        r.v = float(y * 4 + clock);
        r.pal = PAL_WATER;
        r.band = ((y / 4 + clock / 10) & 1) ? 1 : 0;
        r.left = gs::GROUND_WATER;
        r.right = gs::GROUND_WATER;
    }
    vdp.roadTime = clock;

    int sx = 0;
    if (shake_ > 0) sx = (shake_ & 1) ? shake_ / 2 : -shake_ / 2;

    float open = std::clamp(gap_, 0.f, 1.f);
    float doorX = DOOR_X + open * 86.f + sx;

    // Earlier sprites sit on top. Crew, then the drop, then the door, then piles in the water.
    float arm = bracing_ ? 0.f : 4.f;
    float leanX = lean_ * 4.f;
    sprite(art_.crew, 78.f + leanX + sx, 108.f + arm, 62.f, PAL_CREW, lean_ > 0);

    if (crateY_ > -18.f) sprite(art_.crate, 168.f + sx, crateY_, 26.f, PAL_CRATE, false);

    float sway = std::sin(clock * 0.05f) * (bracing_ ? 1.5f : 4.f);
    sprite(art_.lamp, 250.f + sway, 48.f, 70.f, PAL_LAMP, false);
    sprite(art_.gull, 20.f + (clock % 400) * 0.7f, 28.f + std::sin(clock * 0.08f) * 4.f, 12.f, PAL_BIRD, false);
    sprite(art_.gull, 300.f - (clock % 520) * 0.4f, 18.f, 10.f, PAL_BIRD, true);

    if (mode_ == Mode::Play && tideIx_ < int(tides_.size())) {
        const Tide& h = tides_[tideIx_];
        int lead = h.at - play_;
        if (lead <= TIDE_WARN && lead > 0 && (lead < 10 || (play_ / 6) % 2 == 0)) {
            float cx = h.dir < 0 ? 16.f : 292.f;
            sprite(art_.chev, cx, 96.f, 18.f, PAL_WARN, h.dir > 0);
        }
    }
    if (mode_ == Mode::Play && hoistIx_ < int(hoists_.size())) {
        int lead = hoists_[hoistIx_].at - play_;
        if (lead <= HOIST_WARN && lead > 0 && (play_ / 5) % 2 == 0) sprite(art_.chev, 176.f, 40.f, 14.f, PAL_WARN, false);
    }

    sprite(art_.door, doorX, DOOR_Y, 128.f, PAL_DOOR, false);
    sprite(art_.pile, 18.f, 150.f, 70.f, PAL_CRATE, false);
    sprite(art_.pile, 292.f, 154.f, 66.f, PAL_CRATE, false);
    sprite(art_.pile, 54.f, 158.f, 58.f, PAL_CRATE, false);

    char buf[48];
    int sec = mode_ == Mode::Title ? 180 : secondsLeft_;
    std::snprintf(buf, sizeof buf, "WATCH %d:%02d", sec / 60, sec % 60);
    hud(1, 0, buf, flash_ > 8 ? PAL_WARN : PAL_HUD);
    std::snprintf(buf, sizeof buf, "GAP %d%%", mode_ == Mode::Title ? 10 : gapPct_);
    hud(30, 0, buf, PAL_HUD);

    if (mode_ == Mode::Title) {
        hudC(7, "WHARF DOOR", PAL_HUD);
        hudC(9, "HOLD IT FOR THREE MINUTES", PAL_HUD);
        hudC(11, "BRACE  Z X C OR SPACE", PAL_HUD);
        hudC(12, "LEAN WITH THE TIDE", PAL_HUD);
        hudC(13, "UP PINS THE HOIST", PAL_HUD);
        hudC(16, "MISS IT AND THE WATCH IS OVER", PAL_HUD);
        if ((age_ / 30) % 2 == 0) hudC(20, "ENTER TO TAKE THE WATCH", PAL_HUD);
    } else if (mode_ == Mode::Play) {
        hud(1, 26, bracing_ ? "ON THE DOOR" : "DOOR WALKING", bracing_ ? PAL_HUD : PAL_WARN);
        if (pin_) hud(16, 26, "PIN", PAL_HUD);
        else if (lean_ < 0) hud(16, 26, "LEAN LEFT", PAL_HUD);
        else if (lean_ > 0) hud(16, 26, "LEAN RIGHT", PAL_HUD);
        else hud(16, 26, "SET YOUR WEIGHT", PAL_HUD);
        int cells = std::clamp(gapPct_ / 10, 0, 10);
        for (int i = 0; i < 10; i++) sys_->vdp.HUD.set(29 + i, 1, gs::entry(art_.font[i < cells ? '-' - 32 : '.' - 32], PAL_HUD));
    } else if (mode_ == Mode::Won) {
        hudC(9, "DOOR HELD", PAL_HUD);
        hudC(11, "THREE MINUTES ON THE WHARF", PAL_HUD);
        hudC(13, "THE WATCH IS YOURS", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudC(9, "THE DOOR WALKED", PAL_WARN);
        hudC(11, "THE WATCH IS OVER", PAL_HUD);
        hudC(15, "ENTER TO TAKE IT AGAIN", PAL_HUD);
    }
}

}  // namespace wharf
