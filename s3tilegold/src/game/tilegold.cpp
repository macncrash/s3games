#include "game/tilegold.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace tilegold {

int Game::marker() const {
    if (mode_ == Mode::Win) return 2;
    if (mode_ == Mode::Play) return 1;
    return 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.4f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
}

void Game::openNight() {
    score_ = bare_ = golds_ = cream_ = faults_ = index_ = 0;
    goldOut_ = false;
    won_ = false;
    over_ = false;
    hover_ = 0;
    wait_ = 0;
    life_ = 0;
    std::memcpy(stack_, "GGCGCGG", kStack + 1);
    std::memset(laid_, 0, sizeof laid_);
    hand_ = Kind::Gold;
    mode_ = Mode::Play;
    sys_->apu.tone(0, 392.f, 0.06f);
}

void Game::advance() {
    index_++;
    life_ = 0;
    hover_ = 0;
    if (index_ >= kStack) {
        judge();
        return;
    }
    hand_ = stack_[index_] == 'G' ? Kind::Gold : Kind::Cream;
}

void Game::setTile(Kind where) {
    const bool gold = hand_ == Kind::Gold;
    if (where == Kind::Gold && gold) {
        score_ += 2;
        bare_ += 1;
        golds_++;
        goldOut_ = true;
        laid_[index_] = 'G';
        sys_->apu.tone(0, 660.f, 0.07f);
        if (!sys_->headless) sys_->rumble(0.15f, 0.35f, 40);
    } else if (where == Kind::Cream && !gold) {
        score_ += 1;
        bare_ += 1;
        cream_++;
        laid_[index_] = 'C';
        sys_->apu.tone(0, 330.f, 0.05f);
    } else {
        faults_++;
        laid_[index_] = 'x';
        sys_->apu.tone(0, 140.f, 0.08f);
        sys_->apu.noiseBurst(0.12f, 700.f, 0.05f);
        if (faults_ >= 3) {
            won_ = false;
            mode_ = Mode::Lose;
            wait_ = 0;
            return;
        }
    }
    advance();
}

void Game::leave() {
    // Cream left off the floor does not score. Gold left does not double.
    laid_[index_] = '.';
    sys_->apu.tone(1, 220.f, 0.04f);
    advance();
}

void Game::judge() {
    wait_ = 0;
    // Only the gold counts double. Cream face points cannot buy the line.
    if (goldOut_ && golds_ > 0 && cream_ == 0 && score_ >= kLine && bare_ < kLine && faults_ < 3) {
        won_ = true;
        mode_ = Mode::Win;
        sys_->apu.tone(0, 523.f, 0.1f);
        if (!sys_->headless) sys_->rumble(0.3f, 0.55f, 140);
    } else {
        won_ = false;
        mode_ = Mode::Lose;
        sys_->apu.tone(0, 160.f, 0.1f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) openNight();
    } else if (mode_ == Mode::Play) {
        life_++;
        if (bot_) {
            if (life_ == 10) {
                if (hand_ == Kind::Gold) setTile(Kind::Gold);
                else leave();
            }
        } else {
            if (pad.pressed(gs::BTN_UP)) hover_ = (hover_ + 2) % 3;
            if (pad.pressed(gs::BTN_DOWN)) hover_ = (hover_ + 1) % 3;
            if (pad.pressed(gs::BTN_A)) {
                if (hover_ == 0) setTile(Kind::Gold);
                else if (hover_ == 2) setTile(Kind::Cream);
                else leave();
            } else if (pad.pressed(gs::BTN_B)) {
                leave();
            } else if (pad.pressed(gs::BTN_C)) {
                setTile(Kind::Cream);
            } else if (pad.pressed(gs::BTN_X)) {
                setTile(Kind::Gold);
            } else if (life_ > 150) {
                leave();
            }
        }
    } else if (mode_ == Mode::Win) {
        wait_++;
        if (wait_ == 18) sys.apu.tone(0, 659.f, 0.08f);
        if (wait_ == 36) sys.apu.tone(0, 784.f, 0.12f);
        if (wait_ > 70) {
            over_ = true;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Lose) {
        if (bot_) {
            wait_++;
            if (wait_ > 24) over_ = true;
        } else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Title;
        }
    }
    paint();
}

void Game::blit(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::lineAt(int col, int row, const char* s, int pal) {
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

void Game::lineC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    lineAt(20 - n / 2, row, s, pal);
}

void Game::paint() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int shade = 1 + (y * 2) / gs::SCREEN_H;
        v.lineBackdrop[y] = gs::rgb4(shade, shade + 1, shade + 2);
    }

    blit(art_.floor, 160.f, 128.f, float(art_.floor.w), float(art_.floor.h), PAL_FLOOR);

    if (mode_ == Mode::Play || mode_ == Mode::Win || mode_ == Mode::Lose) {
        for (int i = 0; i < kStack; i++) {
            float x = 54.f + float(i) * 36.f;
            float y = 118.f;
            if (laid_[i] == 'G') blit(art_.tile, x, y, 22.f, 22.f, PAL_GOLD);
            else if (laid_[i] == 'C') blit(art_.tile, x, y, 22.f, 22.f, PAL_CREAM);
            else if (laid_[i] == 'x') blit(art_.chip, x, y, 10.f, 10.f, PAL_BAD);
            else if (i == index_ && mode_ == Mode::Play) blit(art_.chip, x, y, 8.f, 8.f, PAL_GROUT);
        }
        if (mode_ == Mode::Play) {
            int pal = hand_ == Kind::Gold ? PAL_GOLD : PAL_CREAM;
            float hx = 160.f;
            float hy = 78.f;
            if (!bot_ && hover_ == 0) {
                hx = 86.f;
                hy = 176.f;
            } else if (!bot_ && hover_ == 2) {
                hx = 234.f;
                hy = 176.f;
            } else if (bot_ && hand_ == Kind::Cream) {
                hx = 160.f;
                hy = 58.f;
            }
            blit(art_.tile, hx, hy, 26.f, 26.f, pal);
            blit(art_.trowel, hx + 22.f, hy + 10.f, 36.f, 16.f, PAL_GROUT);
        }
    }

    lineC(1, "S3 TILE GOLD", PAL_TITLE);
    if (mode_ == Mode::Title) {
        lineC(4, "ONLY THE GOLD COUNTS DOUBLE", PAL_INK);
        lineC(12, "A  SET THE GOLD", PAL_HINT);
        lineC(14, "B  LEAVE THE CREAM", PAL_HINT);
        lineC(16, "C  CREAM SCORES ONE", PAL_BAD);
        lineC(20, "CREAM CANNOT BUY THE LINE", PAL_INK);
        lineC(24, "START", PAL_TITLE);
    } else if (mode_ == Mode::Play) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d  LINE %d", score_, bare_, kLine);
        lineC(3, buf, PAL_INK);
        std::snprintf(buf, sizeof buf, "GOLD %d   CREAM %d   FAULT %d", golds_, cream_, faults_);
        lineC(4, buf, PAL_HINT);
        lineC(25, hand_ == Kind::Gold ? "TILE IN HAND IS GOLD" : "TILE IN HAND IS CREAM",
              hand_ == Kind::Gold ? PAL_TITLE : PAL_INK);
        if (!bot_) {
            lineAt(1, 26, hover_ == 0 ? "> GOLD" : "  GOLD", PAL_TITLE);
            lineAt(14, 26, hover_ == 1 ? "> LEAVE" : "  LEAVE", PAL_HINT);
            lineAt(28, 26, hover_ == 2 ? "> CREAM" : "  CREAM", PAL_BAD);
        }
    } else if (mode_ == Mode::Win) {
        lineC(4, "ONLY THE GOLD COUNTS DOUBLE", PAL_WIN);
        char buf[40];
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        lineC(6, buf, PAL_INK);
        lineC(24, "THE FLOOR HELD", PAL_WIN);
    } else {
        lineC(4, "THE LINE DID NOT DOUBLE", PAL_BAD);
        lineC(24, "START", PAL_INK);
    }
}

}  // namespace tilegold
