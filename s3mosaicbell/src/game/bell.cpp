#include "game/bell.h"

#include <cstdio>
#include <cstring>

namespace mosaicbell {
namespace {

constexpr int CELL = 14;
constexpr int PITCH = 16;
constexpr int OX = 36;
constexpr int OY = 36;
constexpr int FUSE = 60 * 50;

const char* kInk[] = {"PLASTER", "BRONZE", "GOLD", "SOOT", "BONE"};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = true;
    for (int i = 0; i < CELLS; i++) {
        if (art_.mark[i] < 0 || art_.mark[i] > INKS) rules_ = false;
    }
    sys.apu.setMaster(0.7f);
    phase_ = Phase::Title;
    t_ = 0;
    tryNo_ = 1;
    dead_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    paused_ = false;
    reason_ = "bell silent";
    for (int i = 0; i < CELLS; i++) board_[i] = 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ == 0 && chimeLeft_ <= 0) sys.apu.tone(0, 0, 0);
    if (chimeLeft_ > 0) chime();
    if (paused_) {
        if (sys.pad.pressed(gs::BTN_START)) paused_ = false;
        draw();
        return;
    }
    update();
    draw();
}

bool Game::filled() const {
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
    sys_->apu.tone(0, freq, 0.2f);
    beep_ = 6;
}

void Game::chime() {
    static const float kNotes[] = {392.f, 523.f, 659.f, 784.f};
    chimeLeft_--;
    int step = (24 - chimeLeft_) / 6;
    if (step < 0) step = 0;
    if (step > 3) step = 3;
    if ((chimeLeft_ % 6) == 0) sys_->apu.tone(0, kNotes[step], 0.28f);
    if (chimeLeft_ == 0) sys_->apu.tone(0, 0, 0);
}

void Game::armTry() {
    for (int i = 0; i < CELLS; i++) board_[i] = 0;
    cx_ = 2;
    cy_ = 0;
    ink_ = 1;
    fuse_ = FUSE;
    t_ = 0;
}

void Game::killTry(const char* why) {
    dead_++;
    blip(90.f);
    if (dead_ >= 3) {
        phase_ = Phase::Over;
        won_ = false;
        rung_ = false;
        reason_ = why;
        t_ = 0;
        return;
    }
    tryNo_++;
    armTry();
}

void Game::stampHere() {
    int i = cy_ * COLS + cx_;
    int want = art_.mark[i];
    if (board_[i] == ink_) return;
    if (ink_ != want) {
        board_[i] = ink_;
        killTry("wrong tessera");
        return;
    }
    board_[i] = ink_;
    blip(200.f + float(ink_) * 60.f);
    if (filled()) {
        phase_ = Phase::Ring;
        rung_ = true;
        won_ = true;
        t_ = 0;
        chimeLeft_ = 24;
        reason_ = "bell";
    }
}

void Game::update() {
    const gs::Pad& p = sys_->pad;
    t_++;
    if (phase_ == Phase::Title) {
        if (bot_ ? t_ > 8 : confirm()) {
            phase_ = Phase::Lay;
            armTry();
        }
        return;
    }
    if (phase_ == Phase::Ring) {
        if (t_ > (bot_ ? 20 : 70)) {
            phase_ = Phase::Leave;
            t_ = 0;
        }
        return;
    }
    if (phase_ == Phase::Leave) {
        if (t_ > (bot_ ? 8 : 40)) over_ = true;
        return;
    }
    if (phase_ == Phase::Over) {
        if (t_ > (bot_ ? 8 : 90)) over_ = true;
        return;
    }

    if (!bot_ && p.pressed(gs::BTN_START)) {
        paused_ = true;
        return;
    }

    if (--fuse_ <= 0) {
        killTry("fuse died");
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
        if (goal < 0) return;
        int gx = goal % COLS, gy = goal / COLS;
        if (cx_ < gx) cx_++;
        else if (cx_ > gx) cx_--;
        else if (cy_ < gy) cy_++;
        else if (cy_ > gy) cy_--;
        else if (ink_ != art_.mark[goal]) ink_ = art_.mark[goal];
        else stampHere();
        return;
    }

    if (p.pressed(gs::BTN_LEFT) && cx_ > 0) cx_--;
    if (p.pressed(gs::BTN_RIGHT) && cx_ < COLS - 1) cx_++;
    if (p.pressed(gs::BTN_UP) && cy_ > 0) cy_--;
    if (p.pressed(gs::BTN_DOWN) && cy_ < ROWS - 1) cy_++;
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_X)) {
        ink_++;
        if (ink_ > INKS) ink_ = 1;
    }
    if (p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z)) {
        ink_--;
        if (ink_ < 1) ink_ = INKS;
    }
    if (p.pressed(gs::BTN_B)) {
        int i = cy_ * COLS + cx_;
        if (board_[i] != 0) {
            board_[i] = 0;
            blip(130.f);
        }
    }
    if (p.pressed(gs::BTN_A)) stampHere();
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
        int dusk = y / 28;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(2 + dusk / 3, 2, 4 + dusk / 2);
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
        hudC(3, "S3 MOSAIC BELL", PAL_GOLD);
        hudC(5, "A SHORT MOSAIC", PAL_CREAM);
        spr(art_.picture, 125, 64, COLS * 14, ROWS * 14);
        hudC(22, "RING THE BELL BEFORE", PAL_DIM);
        hudC(23, "THE THIRD TRY DIES", PAL_DIM);
        hudC(25, bot_ ? "LAYING" : "START", PAL_LEAF);
        return;
    }

    int swing = 0;
    if (phase_ == Phase::Ring || phase_ == Phase::Leave || (phase_ == Phase::Over && won_)) {
        int wob = (t_ / 4) % 4;
        swing = (wob == 1) ? 6 : (wob == 3) ? -6 : 0;
    }
    spr(art_.bell, 250 + swing, 28, 28, 36);

    if (phase_ == Phase::Lay) spr(art_.ring, OX + cx_ * PITCH - 1, OY + cy_ * PITCH - 1, 16, 16);
    for (int i = 0; i < CELLS; i++) {
        int x = OX + (i % COLS) * PITCH;
        int y = OY + (i / COLS) * PITCH;
        spr(art_.cell[board_[i] < 0 ? 0 : board_[i]], x, y, CELL, CELL);
    }
    spr(art_.picture, 200, 88, COLS * 10, ROWS * 10);

    hud(1, 1, "MOSAIC BELL", PAL_GOLD);
    char line[40];
    std::snprintf(line, sizeof(line), "TRY %d", tryNo_);
    hud(28, 1, line, PAL_CREAM);
    std::snprintf(line, sizeof(line), "DEAD %d", dead_);
    hud(28, 2, line, dead_ ? PAL_GOLD : PAL_DIM);

    if (paused_) {
        hudC(14, "PAUSED", PAL_GOLD);
    } else if (phase_ == Phase::Ring || phase_ == Phase::Leave) {
        hudC(24, "THE BELL", PAL_GOLD);
        hudC(26, "BEFORE THE THIRD TRY DIED", PAL_LEAF);
    } else if (phase_ == Phase::Over) {
        hudC(24, "THIRD TRY DIED", PAL_GOLD);
        hudC(26, "BELL SILENT", PAL_DIM);
    } else {
        std::snprintf(line, sizeof(line), "INK %s", inkName());
        hud(1, 25, line, PAL_CREAM);
        std::snprintf(line, sizeof(line), "FUSE %d", fuse_ / 60);
        hud(16, 25, line, fuse_ < 60 * 8 ? PAL_GOLD : PAL_DIM);
        hudC(27, "ARROWS  A STAMP  C INK  B LIFT", PAL_DIM);
    }
}

}  // namespace mosaicbell
