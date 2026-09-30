#include "game/bell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace bellbell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kLife = 5.5f;

}  // namespace

float Game::bellX(int i) const { return 70.f + float(i) * 90.f; }

int Game::irnd(int n) {
    rng_ = rng_ * 1664525u + 1013904223u;
    return int(rng_ % uint32_t(n));
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    sys.apu.setMaster(0.36f);
    rng_ = bot_ ? 0xBE11u : (0xBE11u ^ uint32_t(sys.frame) * 0x9E3779B9u);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    hold_ = 0;
    sel_ = 1;
    live_ = 1;
    think_ = 0;
    why_ = "";
    life_ = 0;
    bellAmp_ = 0.08f;
}

void Game::begin() {
    dead_ = 0;
    tryNo_ = 0;
    won_ = false;
    rung_ = false;
    over_ = false;
    why_ = "";
    armTry();
}

void Game::armTry() {
    hold_ = 0;
    think_ = 0;
    sel_ = 1;
    live_ = irnd(BELLS);
    lifeMax_ = kLife;
    life_ = lifeMax_;
    tryNo_++;
    mode_ = Mode::Strike;
    why_ = "";
    bellAmp_ = 0.08f;
}

void Game::strike() {
    if (mode_ != Mode::Strike) return;
    if (sel_ != live_) {
        dieTry("COLD");
        return;
    }
    ring();
}

void Game::ring() {
    rung_ = true;
    won_ = true;
    why_ = "RUNG";
    mode_ = Mode::Ring;
    hold_ = 0;
    bellAmp_ = 1.f;
    if (sys_) {
        sys_->apu.tone(0, 392.f, 0.16f);
        sys_->apu.tone(1, 588.f, 0.08f);
        if (!sys_->headless) sys_->rumble(0.3f, 0.5f, 160);
    }
}

void Game::dieTry(const char* why) {
    why_ = why;
    dead_++;
    mode_ = Mode::Dead;
    hold_ = 0;
    bellAmp_ = 0.04f;
    if (sys_) sys_->apu.tone(0, 110.f, 0.08f);
}

void Game::steer() {
    const gs::Pad& pad = sys_->pad;
    if (pad.pressed(gs::BTN_LEFT)) sel_ = std::max(0, sel_ - 1);
    if (pad.pressed(gs::BTN_RIGHT)) sel_ = std::min(BELLS - 1, sel_ + 1);
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B)) strike();
}

void Game::botAct() {
    sel_ = live_;
    think_++;
    if (think_ == 24) strike();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    bellPh_ += kDt * (rung_ ? 8.f : 1.6f);
    bellAmp_ *= rung_ ? 0.994f : 0.99f;
    if (bellAmp_ < 0.06f) bellAmp_ = rung_ ? 0.4f : 0.06f;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Strike) {
        if (bot_) botAct();
        else steer();
        if (mode_ == Mode::Strike) {
            life_ -= kDt;
            if (life_ <= 0) dieTry("LATE");
        }
    } else if (mode_ == Mode::Dead) {
        hold_++;
        if (hold_ > (bot_ ? 10 : 48)) {
            if (dead_ >= 3) {
                won_ = false;
                over_ = true;
                mode_ = Mode::Over;
                hold_ = 0;
            } else {
                armTry();
            }
        }
    } else if (mode_ == Mode::Ring) {
        hold_++;
        if (hold_ == 12) sys.apu.tone(0, 494.f, 0.12f);
        if (hold_ == 26) sys.apu.tone(0, 659.f, 0.12f);
        if (hold_ > 40) {
            mode_ = Mode::Leave;
            hold_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        hold_++;
        if (hold_ > (bot_ ? 8 : 36)) {
            over_ = true;
            mode_ = Mode::Over;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Over) {
        if (!won_ && !bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) toTitle();
    }
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int dusk = y < 90 ? 1 : 2;
        v.lineBackdrop[y] = gs::rgb4(dusk, dusk, dusk + 3);
    }

    spr(art_.stone, 160.f, 206.f, float(art_.stone.w), float(art_.stone.h), PAL_TOWER);
    spr(art_.beam, 160.f, 48.f, float(art_.beam.w), float(art_.beam.h), PAL_TOWER);

    float pulse = 0.75f + 0.25f * std::sin(clock_ * 9.f);
    for (int i = 0; i < BELLS; i++) {
        float swing = std::sin(bellPh_ + float(i) * 0.7f) * bellAmp_ * (i == live_ && rung_ ? 10.f : 4.f);
        float x = bellX(i) + swing;
        spr(art_.bell, x, 96.f, float(art_.bell.w), float(art_.bell.h), PAL_BELL, false);
        if (mode_ == Mode::Strike && i == live_)
            spr(art_.mark, x, 58.f, 12.f * pulse, 16.f * pulse, PAL_MARK);
    }
    if (mode_ == Mode::Strike || mode_ == Mode::Title)
        spr(art_.mallet, bellX(mode_ == Mode::Title ? 1 : sel_), 150.f, 28.f, 14.f, PAL_MALLET);

    if (mode_ == Mode::Title) {
        hudC(2, "BELL BELL", PAL_TITLE);
        hudC(4, "STRIKE THE LIT BELL", PAL_INK);
        hudC(18, "THE BELL RINGS", PAL_INK);
        hudC(19, "BEFORE THE THIRD TRY DIES", PAL_HINT);
        hudC(22, "START", PAL_TITLE);
    } else if (mode_ == Mode::Strike) {
        char buf[40];
        std::snprintf(buf, sizeof(buf), "TRY %d   DEAD %d", tryNo_, dead_);
        hudC(1, buf, PAL_INK);
        int bars = int(std::max(0.f, life_ / lifeMax_) * 10.f + 0.5f);
        if (bars > 10) bars = 10;
        char meter[24];
        std::snprintf(meter, sizeof(meter), "TRY %.*s%.*s", bars, "##########", 10 - bars, "..........");
        hudC(26, meter, life_ < 1.6f ? PAL_DEAD : PAL_TITLE);
        hudC(24, "LEFT RIGHT   A STRIKES", PAL_HINT);
    } else if (mode_ == Mode::Dead) {
        hudC(1, why_, PAL_DEAD);
        hudC(25, dead_ >= 3 ? "THE THIRD TRY DIED" : "THAT TRY DIED", PAL_DEAD);
    } else if (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        hudC(2, "BELL BELL", PAL_WIN);
        hudC(24, "THE BELL RINGS", PAL_WIN);
        hudC(25, "BEFORE THE THIRD TRY DIES", PAL_HINT);
    } else if (mode_ == Mode::Over) {
        hudC(24, "END", PAL_DEAD);
        hudC(25, "START", PAL_HINT);
    }
}

}  // namespace bellbell
