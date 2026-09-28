#include "game/mark.h"

#include <cstdio>
#include <cstring>

namespace mark {
namespace {

constexpr int CELL = 16;
constexpr int PITCH = 18;
constexpr int OX = 22;
constexpr int OY = 40;

const char* kInk[] = {"PLASTER", "CLAY", "GOLD", "INK", "BONE", "LEAF"};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    phase_ = Phase::Title;
    t_ = 0;
    cx_ = 3;
    cy_ = 3;
    ink_ = 1;
    stamps_ = 0;
    won_ = false;
    over_ = false;
    paused_ = false;
    for (int i = 0; i < CELLS; i++) board_[i] = 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (paused_) {
        if (sys.pad.pressed(gs::BTN_START)) paused_ = false;
        draw();
        return;
    }
    update();
    draw();
}

int Game::marker() const {
    if (phase_ == Phase::Title) return 0;
    if (phase_ == Phase::Lay) return 1;
    if (phase_ == Phase::Seal) return 2;
    return 3;
}

bool Game::done() const {
    for (int i = 0; i < CELLS; i++)
        if (board_[i] != art_.mark[i]) return false;
    return true;
}

bool Game::confirm() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
}

const char* Game::inkName() const { return kInk[ink_]; }

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.18f);
    beep_ = 5;
}

void Game::stampHere() {
    int i = cy_ * N + cx_;
    if (board_[i] == ink_) return;
    board_[i] = ink_;
    stamps_++;
    blip(180.f + float(ink_) * 70.f);
}

void Game::update() {
    const gs::Pad& p = sys_->pad;
    t_++;
    if (phase_ == Phase::Title) {
        if (bot_ ? t_ > 10 : confirm()) {
            phase_ = Phase::Lay;
            t_ = 0;
        }
        return;
    }
    if (phase_ == Phase::Seal) {
        if (t_ > (bot_ ? 16 : 50)) {
            phase_ = Phase::Victory;
            t_ = 0;
            won_ = true;
            blip(523.f);
        }
        return;
    }
    if (phase_ == Phase::Victory) {
        if (t_ > (bot_ ? 12 : 90)) over_ = true;
        return;
    }

    if (!bot_ && p.pressed(gs::BTN_START)) {
        paused_ = true;
        return;
    }

    if (bot_) {
        int goal = -1;
        for (int i = 0; i < CELLS; i++) {
            if (board_[i] != art_.mark[i]) {
                goal = i;
                break;
            }
        }
        if (goal < 0) {
            phase_ = Phase::Seal;
            t_ = 0;
            return;
        }
        int gx = goal % N, gy = goal / N;
        if (cx_ < gx) cx_++;
        else if (cx_ > gx) cx_--;
        else if (cy_ < gy) cy_++;
        else if (cy_ > gy) cy_--;
        else if (ink_ != art_.mark[goal]) {
            ink_ = art_.mark[goal];
        } else {
            stampHere();
            if (done()) {
                phase_ = Phase::Seal;
                t_ = 0;
            }
        }
        return;
    }

    if (p.pressed(gs::BTN_LEFT) && cx_ > 0) cx_--;
    if (p.pressed(gs::BTN_RIGHT) && cx_ < N - 1) cx_++;
    if (p.pressed(gs::BTN_UP) && cy_ > 0) cy_--;
    if (p.pressed(gs::BTN_DOWN) && cy_ < N - 1) cy_++;
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_X)) {
        ink_++;
        if (ink_ > INKS) ink_ = 1;
    }
    if (p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z)) {
        ink_--;
        if (ink_ < 1) ink_ = INKS;
    }
    if (p.pressed(gs::BTN_A)) stampHere();
    if (p.pressed(gs::BTN_B)) {
        int i = cy_ * N + cx_;
        if (board_[i] != 0) {
            board_[i] = 0;
            blip(120.f);
        }
    }
    if (done()) {
        phase_ = Phase::Seal;
        t_ = 0;
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Image& img, int x, int y, int w, int h) {
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.pal = PAL_TILE;
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = 3 + y / 40;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(4 + g / 4, 3, 2 + (y > 160 ? 1 : 0));
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    vdp.hudEnabled = true;
    backdrop();

    if (phase_ == Phase::Title) {
        hudC(3, "S3 MOSAIC MARK", PAL_GOLD);
        hudC(5, "LAY THE TESSERAE", PAL_CREAM);
        spr(art_.picture, 104, 62, 112, 112);
        hudC(23, "A FINISHED MARK ENDS IT", PAL_DIM);
        hudC(25, bot_ ? "LAYING" : "START", PAL_LEAF);
        return;
    }

    if (phase_ == Phase::Lay || phase_ == Phase::Seal) {
        spr(art_.ring, OX + cx_ * PITCH - 1, OY + cy_ * PITCH - 1, 18, 18);
    }
    for (int i = 0; i < CELLS; i++) {
        int x = OX + (i % N) * PITCH;
        int y = OY + (i / N) * PITCH;
        spr(art_.cell[board_[i]], x, y, CELL, CELL);
    }
    spr(art_.picture, 214, 78, 84, 84);

    hud(1, 1, "MOSAIC MARK", PAL_GOLD);
    hud(28, 1, "THE MARK", PAL_DIM);
    char line[40];
    std::snprintf(line, sizeof(line), "INK %s", inkName());
    hud(1, 25, line, PAL_CREAM);
    std::snprintf(line, sizeof(line), "STAMPS %d", stamps_);
    hud(22, 25, line, PAL_DIM);

    if (paused_) {
        hudC(14, "PAUSED", PAL_GOLD);
    } else if (phase_ == Phase::Seal) {
        hudC(27, "MARK SEALED", PAL_LEAF);
    } else if (phase_ == Phase::Victory) {
        hudC(12, "MARK FINISHED", PAL_GOLD);
        hudC(27, "THE PICTURE HOLDS", PAL_LEAF);
    } else {
        hudC(27, "ARROWS MOVE  A STAMP  C INK  B LIFT", PAL_DIM);
    }
}

}  // namespace mark
