#include "game/boardgold.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace boardgold {

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
    std::memcpy(deck_, "GGCGCGG", kDeck + 1);
    lamp_ = Kind::Gold;
    mode_ = Mode::Play;
    sys_->apu.tone(0, 392.f, 0.06f);
}

void Game::advance() {
    index_++;
    life_ = 0;
    hover_ = 0;
    if (index_ >= kDeck) {
        judge();
        return;
    }
    lamp_ = deck_[index_] == 'G' ? Kind::Gold : Kind::Cream;
}

void Game::seat(Kind where) {
    const bool gold = lamp_ == Kind::Gold;
    if (where == Kind::Gold && gold) {
        score_ += 2;
        bare_ += 1;
        golds_++;
        goldOut_ = true;
        sys_->apu.tone(0, 660.f, 0.07f);
        if (!sys_->headless) sys_->rumble(0.15f, 0.35f, 40);
    } else if (where == Kind::Cream && !gold) {
        score_ += 1;
        bare_ += 1;
        cream_++;
        sys_->apu.tone(0, 330.f, 0.05f);
    } else {
        faults_++;
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
    // Cream left on the board does not score. Gold left does not double.
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
            if (life_ == 12) {
                if (lamp_ == Kind::Gold) seat(Kind::Gold);
                else leave();
            }
        } else {
            if (pad.pressed(gs::BTN_UP)) hover_ = (hover_ + 2) % 3;
            if (pad.pressed(gs::BTN_DOWN)) hover_ = (hover_ + 1) % 3;
            if (pad.pressed(gs::BTN_A)) {
                if (hover_ == 0) seat(Kind::Gold);
                else if (hover_ == 2) seat(Kind::Cream);
                else leave();
            } else if (pad.pressed(gs::BTN_B)) {
                leave();
            } else if (pad.pressed(gs::BTN_C)) {
                seat(Kind::Cream);
            } else if (pad.pressed(gs::BTN_X)) {
                seat(Kind::Gold);
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
        v.lineBackdrop[y] = gs::rgb4(shade, shade, shade + 2);
    }

    const float lampX = 69.f;
    const float lampY = 122.f;
    const float goldX = 251.f;
    const float goldY = 85.f;
    const float creamX = 251.f;
    const float creamY = 149.f;

    if (mode_ == Mode::Play || mode_ == Mode::Win || mode_ == Mode::Lose) {
        float tx = goldX;
        float ty = goldY;
        int cordPal = PAL_GOLD;
        if (mode_ == Mode::Play) {
            if (!bot_ && hover_ == 1) {
                tx = 160.f;
                ty = 40.f;
                cordPal = PAL_CORD;
            } else if ((!bot_ && hover_ == 2) || (bot_ && lamp_ == Kind::Cream)) {
                tx = creamX;
                ty = creamY;
                cordPal = PAL_CREAM;
            }
        }
        const int beads = 7;
        for (int i = beads; i >= 1; i--) {
            float u = float(i) / float(beads + 1);
            float x = lampX + (tx - lampX) * u;
            float y = lampY + (ty - lampY) * u - std::sin(u * 3.14159f) * 18.f;
            blit(art_.bead, x, y, 8.f, 8.f, cordPal);
        }
        blit(art_.plug, tx, ty + 8.f, 18.f, 22.f, cordPal == PAL_CREAM ? PAL_CREAM : PAL_GOLD);
        int lampPal = (mode_ != Mode::Play) ? PAL_GOLD : (lamp_ == Kind::Gold ? PAL_GOLD : PAL_CREAM);
        blit(art_.lamp, lampX, lampY, 28.f, 36.f, lampPal);
        blit(art_.plate, goldX, goldY - 22.f, 54.f, 16.f, PAL_GOLD);
        blit(art_.plate, creamX, creamY - 22.f, 54.f, 16.f, PAL_CREAM);
    }

    blit(art_.cabinet, 160.f, 124.f, float(art_.cabinet.w), float(art_.cabinet.h), PAL_CAB);

    lineC(1, "S3 BOARD GOLD", PAL_TITLE);
    if (mode_ == Mode::Title) {
        lineC(4, "ONLY THE GOLD COUNTS DOUBLE", PAL_INK);
        lineC(12, "A  SEAT THE GOLD", PAL_HINT);
        lineC(14, "B  LEAVE THE CREAM", PAL_HINT);
        lineC(16, "C  CREAM SCORES ONE", PAL_BAD);
        lineC(20, "CREAM CANNOT BUY THE LINE", PAL_INK);
        lineC(24, "START", PAL_TITLE);
    } else if (mode_ == Mode::Play) {
        char buf[40];
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d  LINE %d", score_, bare_, kLine);
        lineC(3, buf, PAL_INK);
        std::snprintf(buf, sizeof buf, "GOLD %d   CREAM %d   FAULT %d", golds_, cream_, faults_);
        lineC(4, buf, PAL_HINT);
        lineC(25, lamp_ == Kind::Gold ? "LAMP IS GOLD" : "LAMP IS CREAM",
              lamp_ == Kind::Gold ? PAL_TITLE : PAL_INK);
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
        lineC(24, "THE BOARD HELD", PAL_WIN);
    } else {
        lineC(4, "THE LINE DID NOT DOUBLE", PAL_BAD);
        lineC(24, "START", PAL_INK);
    }
}

}  // namespace boardgold
