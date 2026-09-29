#include "game/boardbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace boardbell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kLife = 6.5f;
constexpr int kX0 = 20;
constexpr int kX1 = 300;
constexpr int kY0 = 52;

const char* kLine[JACKS] = {"PIER", "INN", "FIRE", "CAB", "HALL", "WIRE"};

int jackY(int i) { return kY0 + i * 22; }

}  // namespace

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
    rng_ = bot_ ? 0xB04Du : (0xB04Du ^ uint32_t(sys.frame) * 0x9E3779B9u);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    rung_ = false;
    held_ = false;
    dead_ = 0;
    tryNo_ = 0;
    hold_ = 0;
    bank_ = 0;
    row_ = 0;
    why_ = "";
    life_ = 0;
    bellAmp_ = 0.12f;
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
    held_ = false;
    bank_ = 0;
    row_ = 0;
    hold_ = 0;
    trunk_ = irnd(JACKS);
    line_ = irnd(JACKS);
    if (line_ == trunk_) line_ = (line_ + 1 + irnd(JACKS - 1)) % JACKS;
    lifeMax_ = kLife;
    life_ = lifeMax_;
    tryNo_++;
    mode_ = Mode::Patch;
    why_ = "";
}

void Game::ring() {
    rung_ = true;
    won_ = true;
    why_ = "RUNG";
    mode_ = Mode::Ring;
    hold_ = 0;
    bellAmp_ = 1.f;
    held_ = false;
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.14f);
        sys_->apu.tone(1, 784.f, 0.08f);
        if (!sys_->headless) sys_->rumble(0.25f, 0.45f, 140);
    }
}

void Game::dieTry(const char* why) {
    why_ = why;
    dead_++;
    held_ = false;
    mode_ = Mode::Dead;
    hold_ = 0;
    bellAmp_ = 0.05f;
    if (sys_) sys_->apu.tone(0, 146.f, 0.08f);
}

void Game::seat() {
    if (mode_ != Mode::Patch) return;
    if (bank_ == 0) {
        if (row_ != trunk_) {
            dieTry("COLD");
            return;
        }
        held_ = !held_;
        if (sys_) sys_->apu.tone(2, held_ ? 440.f : 330.f, 0.06f);
        return;
    }
    if (!held_) {
        dieTry("EMPTY");
        return;
    }
    if (row_ != line_) {
        dieTry("WRONG");
        return;
    }
    ring();
}

void Game::steer() {
    const gs::Pad& pad = sys_->pad;
    if (pad.pressed(gs::BTN_UP)) row_ = std::max(0, row_ - 1);
    if (pad.pressed(gs::BTN_DOWN)) row_ = std::min(JACKS - 1, row_ + 1);
    if (pad.pressed(gs::BTN_LEFT)) bank_ = 0;
    if (pad.pressed(gs::BTN_RIGHT)) bank_ = 1;
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO)) seat();
    if (pad.pressed(gs::BTN_B)) held_ = false;
}

void Game::botAct() {
    if (!held_) {
        if (bank_ != 0) {
            bank_ = 0;
            return;
        }
        if (row_ != trunk_) {
            row_ += trunk_ > row_ ? 1 : -1;
            return;
        }
        seat();
        return;
    }
    if (bank_ != 1) {
        bank_ = 1;
        return;
    }
    if (row_ != line_) {
        row_ += line_ > row_ ? 1 : -1;
        return;
    }
    seat();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    bellPh_ += kDt * (rung_ ? 9.f : 2.2f);
    bellAmp_ *= rung_ ? 0.992f : 0.985f;
    if (bellAmp_ < 0.08f) bellAmp_ = rung_ ? 0.35f : 0.08f;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Patch) {
        if (bot_) botAct();
        else steer();
        if (mode_ == Mode::Patch) {
            life_ -= kDt;
            if (life_ <= 0) dieTry("LAMP");
        }
    } else if (mode_ == Mode::Dead) {
        hold_++;
        if (hold_ > (bot_ ? 12 : 50)) {
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
        if (hold_ == 10) sys.apu.tone(0, 659.f, 0.1f);
        if (hold_ == 22) sys.apu.tone(0, 784.f, 0.12f);
        if (hold_ > 36) {
            mode_ = Mode::Leave;
            hold_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        hold_++;
        if (hold_ > (bot_ ? 8 : 40)) {
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
        int sky = 1 + (y < 40 ? 1 : 0);
        v.lineBackdrop[y] = gs::rgb4(sky, sky, sky + 2);
    }

    spr(art_.desk, 160.f, 124.f, float(art_.desk.w), float(art_.desk.h), PAL_DESK);

    float pulse = 0.65f + 0.35f * std::sin(clock_ * 10.f);
    for (int i = 0; i < JACKS; i++) {
        float y = float(jackY(i));
        if (mode_ == Mode::Patch && i == trunk_)
            spr(art_.lamp, float(kX0 + 36), y, 16.f * pulse, 16.f * pulse, PAL_LAMP);
        else spr(art_.lamp, float(kX0 + 36), y, 10.f, 10.f, PAL_DESK);
        if (i == line_) {
            float swing = std::sin(bellPh_) * bellAmp_ * 5.f;
            spr(art_.bell, float(kX1 - 18) + swing, y - 2.f, 18.f, 16.f, PAL_BELL);
        } else {
            spr(art_.badge, float(kX1 - 18), y, 10.f, 10.f, PAL_HINT);
        }
        if (held_ && i == trunk_) spr(art_.plug, float(kX0 + 58), y, 10.f, 16.f, PAL_PLUG);
    }

    if (mode_ == Mode::Patch) {
        float cy = float(jackY(row_));
        float cx = bank_ == 0 ? float(kX0 + 18) : float(kX1 - 40);
        spr(art_.plug, cx, cy, 12.f, 18.f, PAL_CORD);
    }

    if (mode_ == Mode::Title) {
        hudC(2, "BOARD BELL", PAL_TITLE);
        hudC(4, "PATCH THE LIVE LAMP", PAL_INK);
        hudC(5, "INTO THE BELL", PAL_INK);
        hudC(18, "THE BELL RINGS", PAL_INK);
        hudC(19, "BEFORE THE THIRD TRY DIES", PAL_HINT);
        hudC(22, "START", PAL_TITLE);
    } else if (mode_ == Mode::Patch) {
        char buf[40];
        std::snprintf(buf, sizeof(buf), "TRY %d   DEAD %d", tryNo_, dead_);
        hudC(1, buf, PAL_INK);
        int bars = int(std::max(0.f, life_ / lifeMax_) * 10.f + 0.5f);
        if (bars > 10) bars = 10;
        char meter[24];
        std::snprintf(meter, sizeof(meter), "LAMP %.*s%.*s", bars, "##########", 10 - bars, "..........");
        hudC(26, meter, life_ < 2.f ? PAL_DEAD : PAL_TITLE);
        hud(1, 4 + row_, bank_ == 0 ? ">" : " ", PAL_HINT);
        hud(28, 4 + row_, bank_ == 1 ? "<" : " ", PAL_HINT);
        for (int i = 0; i < JACKS; i++) hud(31, 4 + i, kLine[i], i == line_ ? PAL_TITLE : PAL_INK);
        hudC(24, held_ ? "SEAT THE BELL" : "LIFT THE LAMP", PAL_HINT);
    } else if (mode_ == Mode::Dead) {
        hudC(1, why_, PAL_DEAD);
        hudC(25, dead_ >= 3 ? "THE THIRD TRY DIED" : "THAT TRY DIED", PAL_DEAD);
    } else if (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        hudC(2, "BOARD BELL", PAL_WIN);
        hudC(24, "THE BELL RINGS", PAL_WIN);
        hudC(25, "BEFORE THE THIRD TRY DIES", PAL_HINT);
    } else if (mode_ == Mode::Over) {
        hudC(24, "END", PAL_DEAD);
        hudC(25, "START", PAL_HINT);
    }
}

}  // namespace boardbell
