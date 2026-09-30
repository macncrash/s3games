#include "game/pawn.h"

#include <cstdio>
#include <cstring>

namespace pawnseven {

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    if (bot_) open();
}

void Game::open() {
    you_ = 0;
    them_ = 0;
    over_ = false;
    won_ = false;
    t_ = 0;
    cool_ = 0;
    foe_ = 0;
    mode_ = Mode::Play;
}

void Game::settle() {
    if (you_ >= 7 && you_ > them_) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        sys_->apu.tone(1, 660.f, 0.2f);
        return;
    }
    if (them_ >= 7 && them_ > you_) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        sys_->apu.tone(1, 110.f, 0.2f);
    }
}

void Game::stepYou() {
    if (mode_ != Mode::Play || you_ >= 7) return;
    you_++;
    cool_ = 8;
    sys_->apu.tone(0, 440.f + you_ * 40.f, 0.16f);
    settle();
}

void Game::stepThem() {
    if (mode_ != Mode::Play || them_ >= 7) return;
    them_++;
    foe_ = 0;
    sys_->apu.tone(0, 180.f, 0.1f);
    settle();
}

void Game::botAct() {
    if (cool_ > 0) cool_--;
    else stepYou();
    foe_++;
    if (foe_ >= 22) stepThem();
}

void Game::human() {
    if (cool_ > 0) cool_--;
    const gs::Pad& p = sys_->pad;
    if (cool_ == 0 && (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_UP) || p.pressed(gs::BTN_C))) stepYou();
    foe_++;
    if (foe_ >= 28) stepThem();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) open();
        draw();
        return;
    }
    if (mode_ == Mode::Play) {
        t_++;
        if (bot_) botAct();
        else human();
    }
    draw();
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s{};
    s.w = int16_t(w < 1 ? 1 : w);
    s.h = int16_t(h < 1 ? 1 : h);
    s.x = int16_t(cx - s.w * 0.5f);
    s.y = int16_t(cy - s.h * 0.5f);
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

float Game::rankY(int rank) const {
    // Rank 0 sits on the near rank; rank 7 is the far gold line.
    return 186.f - rank * 20.f;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    if (mode_ == Mode::Title) {
        hudC(2, "S3 PAWN SEVEN", PAL_AMBER);
        hudC(5, "FIRST TO SEVEN", PAL_GREEN);
        hudC(8, "PUSH YOUR PAWN", PAL_TEXT);
        hudC(24, "A OR UP STEPS", PAL_TEXT);
        spr(art_.white, 108, 140, 48, PAL_WHITE);
        spr(art_.black, 212, 140, 48, PAL_BLACK);
        spr(art_.flag, 160, 100, 22, PAL_GOLD);
        return;
    }

    char buf[32];
    std::snprintf(buf, sizeof buf, "YOU %d", you_);
    hud(2, 1, buf, PAL_GREEN);
    std::snprintf(buf, sizeof buf, "THEM %d", them_);
    hud(30, 1, buf, PAL_RED);
    hudC(1, "FIRST TO SEVEN", PAL_AMBER);
    hud(4, 26, "RANK", PAL_TEXT);
    hud(30, 26, "7 WINS", PAL_GOLD);

    spr(art_.flag, 95, rankY(7) - 16, 16, PAL_GOLD);
    spr(art_.flag, 225, rankY(7) - 16, 16, PAL_GOLD);
    spr(art_.white, 95, rankY(you_), 28, PAL_WHITE);
    spr(art_.black, 225, rankY(them_), 28, PAL_BLACK);

    if (mode_ == Mode::Win) hudC(12, "PAWN TO SEVEN", PAL_GREEN);
    else if (mode_ == Mode::Lose) hudC(12, "THEY GOT THERE", PAL_RED);
}

}  // namespace pawnseven
