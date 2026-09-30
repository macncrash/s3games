#include "game/drum.h"

#include <cstdio>
#include <cstring>

namespace drumseven {

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
    lamp_ = 1;
    hold_ = 0;
    foe_ = 0;
    flash_ = 0;
    struck_ = -1;
    mode_ = Mode::Play;
}

void Game::settle() {
    if (you_ >= 7 && you_ > them_) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        sys_->apu.tone(1, 523.f, 0.22f);
        return;
    }
    if (them_ >= 7 && them_ > you_) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        sys_->apu.tone(1, 98.f, 0.2f);
    }
}

void Game::scoreYou() {
    if (mode_ != Mode::Play || you_ >= 7) return;
    you_++;
    struck_ = lamp_;
    flash_ = 10;
    sys_->apu.tone(0, 140.f + lamp_ * 30.f, 0.2f);
    sys_->apu.noiseBurst(0.35f, 0.4f, 0.08f);
    settle();
    if (mode_ == Mode::Play) advance();
}

void Game::scoreThem() {
    if (mode_ != Mode::Play || them_ >= 7) return;
    them_++;
    foe_ = 0;
    sys_->apu.tone(1, 90.f, 0.08f);
    settle();
}

void Game::advance() {
    lamp_ = (lamp_ + 1 + (t_ & 1)) % 3;
    hold_ = 0;
    struck_ = -1;
}

void Game::botAct() {
    if (flash_ > 0) flash_--;
    hold_++;
    if (hold_ == 8) scoreYou();
    foe_++;
    if (foe_ >= 46) scoreThem();
}

void Game::human() {
    if (flash_ > 0) flash_--;
    hold_++;
    const gs::Pad& p = sys_->pad;
    int pick = -1;
    if (p.pressed(gs::BTN_LEFT)) pick = 0;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_DOWN) || p.pressed(gs::BTN_C)) pick = 1;
    if (p.pressed(gs::BTN_RIGHT)) pick = 2;
    if (pick >= 0) {
        if (pick == lamp_) scoreYou();
        else {
            sys_->apu.tone(0, 70.f, 0.06f);
            advance();
        }
    } else if (hold_ > 40) {
        advance();
    }
    foe_++;
    if (foe_ >= 50) scoreThem();
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
    } else if (flash_ > 0) {
        flash_--;
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

int Game::drumPal(int i) const {
    if (i == 0) return PAL_DRUM;
    if (i == 1) return PAL_GOLD;
    return PAL_BLUE;
}

float Game::drumX(int i) const { return 64.f + i * 96.f; }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    if (mode_ == Mode::Title) {
        hudC(2, "S3 DRUM SEVEN", PAL_AMBER);
        hudC(5, "FIRST TO SEVEN", PAL_GREEN);
        hudC(8, "STRIKE THE LIT DRUM", PAL_TEXT);
        hudC(24, "LEFT  A  RIGHT", PAL_TEXT);
        for (int i = 0; i < 3; i++) spr(art_.drum, drumX(i), 150, 72, drumPal(i));
        spr(art_.glow, drumX(1), 96, 18, PAL_GLOW);
        spr(art_.mallet, drumX(1), 78, 36, PAL_MALLET);
        return;
    }

    char buf[32];
    std::snprintf(buf, sizeof buf, "YOU %d", you_);
    hud(2, 1, buf, PAL_GREEN);
    std::snprintf(buf, sizeof buf, "THEM %d", them_);
    hud(30, 1, buf, PAL_RED);
    hudC(1, "FIRST TO SEVEN", PAL_AMBER);
    hud(2, 26, "HIT THE LAMP", PAL_TEXT);
    hud(30, 26, "7 WINS", PAL_GOLD);

    for (int i = 0; i < 3; i++) {
        float y = 156.f;
        if (struck_ == i && flash_ > 0) y += 4.f;
        spr(art_.drum, drumX(i), y, 78, drumPal(i));
    }
    if (mode_ == Mode::Play || flash_ > 0) {
        float gx = drumX(lamp_);
        float gy = 92.f + (hold_ % 6);
        spr(art_.glow, gx, gy, flash_ > 0 ? 22.f : 16.f, PAL_GLOW);
        float mx = gx + (flash_ > 4 ? 0.f : 10.f);
        float my = flash_ > 0 ? 108.f : 78.f;
        spr(art_.mallet, mx, my, 40, PAL_MALLET);
    }

    if (mode_ == Mode::Win) hudC(12, "DRUM TO SEVEN", PAL_GREEN);
    else if (mode_ == Mode::Lose) hudC(12, "THEY GOT THERE", PAL_RED);
}

}  // namespace drumseven
