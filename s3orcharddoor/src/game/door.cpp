#include "game/door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace orcharddoor {
namespace {
constexpr int HOLD = 180 * 60;
constexpr float DT = 1.f / 60.f;
constexpr float OPEN = 1.f;
}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.18f, 0.25f, 0.12f);
    bootTitle();
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    early_ = false;
    titleAge_ = 0;
    age_ = 0;
    braces_ = 0;
    fruit_ = 0;
    missed_ = 0;
    seam_ = 0;
    beatCount_ = 0;
    lean_ = 0;
    shake_ = 0;
    sys_->apu.silence();
}

void Game::layBeats() {
    beatCount_ = 0;
    int t = 150;
    const int pattern[] = {SHOVE_L, LATCH, SHOVE_R, FRUIT, SHOVE_L, SHOVE_R, LATCH, FRUIT};
    while (t < HOLD - 160 && beatCount_ < 48) {
        Beat b;
        b.start = t;
        b.kind = pattern[beatCount_ % 8];
        b.len = 42 + (beatCount_ % 4) * 8;
        b.met = false;
        beats_[beatCount_++] = b;
        int gap = 150 - beatCount_ * 2;
        if (gap < 70) gap = 70;
        t += b.len + gap;
    }
}

void Game::begin() {
    mode_ = Mode::Play;
    age_ = 0;
    seam_ = 0;
    braces_ = 0;
    fruit_ = 0;
    missed_ = 0;
    early_ = false;
    won_ = false;
    over_ = false;
    lean_ = 0;
    layBeats();
    for (auto& m : motes_) m.life = 0;
    sys_->apu.silence();
}

void Game::finish(bool held) {
    mode_ = held ? Mode::Won : Mode::Lost;
    won_ = held;
    over_ = true;
    endAge_ = 0;
    sys_->apu.silence();
    if (held) {
        sys_->apu.tone(0, 523.f, 0.12f);
        sys_->apu.tone(1, 659.f, 0.08f);
    } else {
        sys_->apu.noiseBurst(0.4f, 90.f, 0.25f);
    }
}

int Game::liveBeat() const {
    for (int i = 0; i < beatCount_; i++) {
        if (age_ >= beats_[i].start && age_ < beats_[i].start + beats_[i].len) return i;
    }
    return -1;
}

int Game::marker() const {
    if (mode_ == Mode::Title || mode_ == Mode::Pause) return mode_ == Mode::Title ? 0 : 1;
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    return 1;
}

void Game::applyBot() {
    if (!bot_) return;
    for (int i = 0; i < gs::BTN_COUNT; i++) sys_->pad.cur[i] = false;
    if (mode_ == Mode::Title) {
        if (titleAge_ > 30) sys_->pad.cur[gs::BTN_A] = true;
        return;
    }
    if (mode_ != Mode::Play) return;
    int i = liveBeat();
    if (i < 0) return;
    switch (beats_[i].kind) {
    case SHOVE_L: sys_->pad.cur[gs::BTN_LEFT] = true; break;
    case SHOVE_R: sys_->pad.cur[gs::BTN_RIGHT] = true; break;
    case LATCH: sys_->pad.cur[gs::BTN_A] = true; break;
    case FRUIT: sys_->pad.cur[gs::BTN_B] = true; break;
    }
}

void Game::updatePlay() {
    auto& pad = sys_->pad;
    if (pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        return;
    }
    if (pad.pressed(gs::BTN_C)) {
        early_ = true;
        seam_ = OPEN;
        finish(false);
        return;
    }
    int dir = 0;
    if (pad.down(gs::BTN_LEFT)) dir -= 1;
    if (pad.down(gs::BTN_RIGHT)) dir += 1;
    lean_ = dir;
    bool bar = pad.down(gs::BTN_A);
    bool kick = pad.down(gs::BTN_B);

    int i = liveBeat();
    float push = 0;
    if (i >= 0) {
        Beat& b = beats_[i];
        bool ok = false;
        if (b.kind == SHOVE_L) ok = dir < 0;
        else if (b.kind == SHOVE_R) ok = dir > 0;
        else if (b.kind == LATCH) ok = bar;
        else ok = kick;
        if (ok) {
            push = -0.012f;
            if (!b.met && age_ > b.start + 4) {
                b.met = true;
                if (b.kind == FRUIT) fruit_++;
                else braces_++;
                sys_->apu.tone(2, b.kind == FRUIT ? 440.f : 220.f, 0.06f);
            }
        } else {
            push = (b.kind == FRUIT) ? 0.018f : 0.011f;
            if (dir != 0 && (b.kind == SHOVE_L || b.kind == SHOVE_R)) push = 0.016f;
        }
        if (age_ == b.start + b.len - 1 && !b.met) missed_++;
    } else {
        push = -0.006f;
    }
    seam_ += push;
    if (seam_ < 0) seam_ = 0;
    shake_ = std::max(0.f, shake_ * 0.9f);
    if (push > 0) shake_ = std::min(6.f, shake_ + push * 40.f);
    if (seam_ >= OPEN) {
        seam_ = OPEN;
        finish(false);
        return;
    }
    age_++;
    if (age_ >= HOLD) finish(true);

    if ((age_ % 11) == 0) {
        for (auto& m : motes_) {
            if (m.life <= 0) {
                m.x = 40.f + float((age_ * 17) % 240);
                m.y = 20.f + float((age_ * 3) % 40);
                m.vx = -0.3f - float(age_ % 5) * 0.05f;
                m.vy = 0.35f;
                m.life = 1.f;
                break;
            }
        }
    }
    for (auto& m : motes_) {
        if (m.life <= 0) continue;
        m.x += m.vx;
        m.y += m.vy;
        m.life -= DT * 0.35f;
    }
}

void Game::serviceAudio() {
    if (mode_ != Mode::Play) return;
    float hz = 70.f + seam_ * 140.f;
    sys_->apu.tone(0, hz, 0.015f + seam_ * 0.05f);
    int sec = age_ / 60;
    if (age_ % 60 == 0 && sec > 0) sys_->apu.tone(1, (sec % 2) ? 330.f : 392.f, 0.04f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    applyBot();
    if (mode_ == Mode::Title) {
        titleAge_++;
        if (sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_START)) begin();
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
        if (sys.pad.pressed(gs::BTN_B)) bootTitle();
    } else if (mode_ == Mode::Play) {
        updatePlay();
        serviceAudio();
    } else {
        endAge_++;
        if (!bot_ && (sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_START))) bootTitle();
    }
    sky();
    draw();
    int r = 40 + int(seam_ * 180);
    int g = 80 - int(seam_ * 50);
    sys.setLight(r, g, 30);
}

void Game::sky() {
    float dusk = mode_ == Mode::Play ? std::min(1.f, float(age_) / float(HOLD)) : (mode_ == Mode::Won ? 1.f : 0.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = float(y) / float(gs::SCREEN_H);
        int r = int(4 + (1.f - u) * 8 + dusk * 4);
        int g = int(5 + (1.f - u) * 4 - dusk * 2);
        int b = int(10 - u * 4 - dusk * 3);
        if (y > 168) {
            r = 2 + int((y - 168) * 0.15f);
            g = 5 + int((y - 168) * 0.2f);
            b = 1;
        }
        sys_->vdp.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        sys_->vdp.lineFog[y] = uint8_t(y < 40 ? 2 : 0);
        sys_->vdp.road[y].on = false;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(std::max(1.f, w)));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

const char* Game::kindName(int k) const {
    switch (k) {
    case SHOVE_L: return "LEAN LEFT";
    case SHOVE_R: return "LEAN RIGHT";
    case LATCH: return "HOLD THE BAR";
    case FRUIT: return "KICK THE SILL";
    default: return "HOLD";
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s && s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.clear();
    vdp.B.clear();
    float sh = (shake_ > 0.2f) ? std::sin(float(age_) * 0.9f) * shake_ : 0.f;
    float doorX = 168.f + seam_ * 36.f + sh;
    int step = ((age_ / 8) & 1);

    for (int i = 0; i < 16; i++) {
        if (motes_[i].life > 0)
            spr(art_.leaf, motes_[i].x, motes_[i].y, 8.f, PAL_LEAF, false, 4);
    }
    int live = (mode_ == Mode::Play) ? liveBeat() : -1;
    if (live >= 0) {
        int k = beats_[live].kind;
        if (k == SHOVE_L) spr(art_.hand, 118.f, 150.f, 22.f, PAL_ALERT, false);
        if (k == SHOVE_R) spr(art_.hand, 230.f, 150.f, 22.f, PAL_ALERT, true);
        if (k == FRUIT) spr(art_.apple, doorX - 8.f, 176.f, 14.f, PAL_APPLE);
    }
    spr(art_.man[step], 150.f + lean_ * 4.f + sh, 188.f, 52.f, PAL_MAN, lean_ < 0, 0, true);
    spr(art_.bar, doorX, 132.f - (live >= 0 && beats_[live].kind == LATCH && !beats_[live].met ? 6.f : 0.f), 10.f,
        PAL_IRON);
    spr(art_.door, doorX, 188.f, 100.f, seam_ > 0.65f ? PAL_ALERT : PAL_WOOD, false, 0, true);
    spr(art_.post, 128.f, 192.f, 118.f, PAL_WOOD, false, 0, true);
    spr(art_.post, 214.f + seam_ * 10.f, 192.f, 118.f, PAL_WOOD, true, 0, true);
    spr(art_.tree, 36.f, 196.f, 120.f, PAL_LEAF, false, 1, true);
    spr(art_.tree, 78.f, 200.f, 100.f, PAL_LEAF, true, 2, true);
    spr(art_.tree, 286.f, 198.f, 130.f, PAL_LEAF, false, 1, true);
    spr(art_.apple, 24.f, 120.f, 10.f, PAL_APPLE);
    spr(art_.apple, 48.f, 108.f, 10.f, PAL_APPLE);
    spr(art_.apple, 300.f, 112.f, 10.f, PAL_APPLE);

    if (mode_ == Mode::Title) {
        hudC(6, "S3 ORCHARD DOOR", PAL_HUD);
        hudC(9, "HOLD THE DOOR", PAL_HUD);
        hudC(11, "THREE MINUTES", PAL_HUD);
        hudC(16, "LEFT RIGHT  LEAN", PAL_GRASS);
        hudC(17, "A  THE BAR", PAL_GRASS);
        hudC(18, "B  THE SILL", PAL_GRASS);
        hudC(19, "C  OPENS IT EARLY", PAL_ALERT);
        hudC(23, "A  START THE WATCH", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_HUD);
        hudC(15, "A  BACK TO THE DOOR", PAL_GRASS);
        hudC(16, "B  TITLE", PAL_GRASS);
    } else {
        int left = (mode_ == Mode::Play) ? std::max(0, HOLD - age_) : 0;
        if (mode_ == Mode::Won) left = 0;
        int sec = left / 60;
        char clock[16];
        std::snprintf(clock, sizeof(clock), "%d:%02d", sec / 60, sec % 60);
        hud(1, 1, "ORCHARD", PAL_HUD);
        hud(32, 1, clock, seam_ > 0.7f ? PAL_ALERT : PAL_HUD);
        char seam[20];
        int bars = int(std::lround((1.f - seam_) * 16.f));
        if (bars < 0) bars = 0;
        if (bars > 16) bars = 16;
        for (int i = 0; i < 16; i++) seam[i] = (i < bars) ? '#' : '.';
        seam[16] = 0;
        hud(12, 26, seam, seam_ > 0.55f ? PAL_ALERT : PAL_GRASS);
        if (mode_ == Mode::Play && live >= 0) hudC(24, kindName(beats_[live].kind), PAL_ALERT);
        else if (mode_ == Mode::Play) hudC(24, "HOLD", PAL_HUD);
        if (mode_ == Mode::Won) {
            hudC(10, "THE DOOR HELD", PAL_HUD);
            hudC(12, "THREE MINUTES", PAL_HUD);
            hudC(16, "IT IS DONE", PAL_GRASS);
        } else if (mode_ == Mode::Lost) {
            hudC(10, early_ ? "OPENED EARLY" : "THE DOOR OPENED", PAL_ALERT);
            hudC(12, "THE WATCH IS OVER", PAL_HUD);
        }
    }
}

}  // namespace orcharddoor
