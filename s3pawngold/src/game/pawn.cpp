#include "game/pawn.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace pawngold {

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

void Game::openFile() {
    score_ = bare_ = golds_ = ivory_ = faults_ = index_ = 0;
    goldOut_ = false;
    won_ = false;
    over_ = false;
    hover_ = 0;
    wait_ = 0;
    life_ = 0;
    std::memcpy(file_, "GGPGGPG", kFile + 1);
    std::memset(taken_, 0, sizeof taken_);
    lamp_ = Kind::Gold;
    mode_ = Mode::Play;
    sys_->apu.tone(0, 392.f, 0.06f);
}

void Game::advance() {
    index_++;
    life_ = 0;
    hover_ = 0;
    if (index_ >= kFile) {
        judge();
        return;
    }
    lamp_ = file_[index_] == 'G' ? Kind::Gold : Kind::Ivory;
}

void Game::playPawn(Kind where) {
    const bool gold = lamp_ == Kind::Gold;
    if (where == Kind::Gold && gold) {
        score_ += 2;
        bare_ += 1;
        golds_++;
        goldOut_ = true;
        taken_[index_] = 'G';
        sys_->apu.tone(0, 660.f, 0.07f);
        if (!sys_->headless) sys_->rumble(0.15f, 0.35f, 40);
    } else if (where == Kind::Ivory && !gold) {
        score_ += 1;
        bare_ += 1;
        ivory_++;
        taken_[index_] = 'I';
        sys_->apu.tone(0, 330.f, 0.05f);
    } else {
        faults_++;
        taken_[index_] = 'X';
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
    // An ivory pawn left off the file does not score. Gold left does not double.
    if (index_ < kFile) taken_[index_] = 'L';
    sys_->apu.tone(1, 220.f, 0.04f);
    advance();
}

void Game::judge() {
    wait_ = 0;
    // Only the gold counts double. Ivory face points cannot buy the line.
    if (goldOut_ && golds_ > 0 && ivory_ == 0 && score_ >= kLine && bare_ < kLine && faults_ < 3) {
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
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) openFile();
    } else if (mode_ == Mode::Play) {
        life_++;
        if (bot_) {
            if (life_ == 12) {
                if (lamp_ == Kind::Gold) playPawn(Kind::Gold);
                else leave();
            }
        } else {
            if (pad.pressed(gs::BTN_UP)) hover_ = (hover_ + 2) % 3;
            if (pad.pressed(gs::BTN_DOWN)) hover_ = (hover_ + 1) % 3;
            if (pad.pressed(gs::BTN_A)) {
                if (hover_ == 0) playPawn(Kind::Gold);
                else if (hover_ == 2) playPawn(Kind::Ivory);
                else leave();
            } else if (pad.pressed(gs::BTN_B)) {
                leave();
            } else if (pad.pressed(gs::BTN_C)) {
                playPawn(Kind::Ivory);
            } else if (pad.pressed(gs::BTN_X)) {
                playPawn(Kind::Gold);
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
        int shade = 1 + (y * 3) / gs::SCREEN_H;
        v.lineBackdrop[y] = gs::rgb4(shade, shade + 1, shade);
    }

    blit(art_.board, 160.f, 132.f, float(art_.board.w), float(art_.board.h), PAL_BOARD);

    if (mode_ == Mode::Play || mode_ == Mode::Win || mode_ == Mode::Lose) {
        const float origin = 160.f - 3 * 30.f;
        for (int f = 0; f < kFile; f++) {
            float x = origin + f * 30.f;
            bool ahead = f > index_ || (mode_ == Mode::Play && f == index_);
            bool gold = file_[f] == 'G';
            if (!ahead && taken_[f] == 'L') continue;
            if (!ahead && taken_[f] == 'X') continue;
            int pal = gold ? PAL_GOLD : PAL_IVORY;
            float y = 118.f;
            if (mode_ == Mode::Play && f == index_) y = 100.f;
            if (!ahead && taken_[f] == 'G') y = 92.f;
            blit(art_.pawn, x, y, 20.f, 36.f, pal);
            if (!ahead && taken_[f] == 'G') blit(art_.crown, x, y - 22.f, 16.f, 10.f, PAL_MARK);
        }
    }

    lineC(1, "S3 PAWN GOLD", PAL_TITLE);
    if (mode_ == Mode::Title) {
        lineC(4, "ONLY THE GOLD COUNTS DOUBLE", PAL_INK);
        lineC(12, "A  PLAY THE GOLD PAWN", PAL_HINT);
        lineC(14, "B  LEAVE THE IVORY", PAL_HINT);
        lineC(16, "C  IVORY SCORES ONE", PAL_BAD);
        lineC(20, "IVORY CANNOT BUY THE LINE", PAL_INK);
        lineC(24, "START", PAL_TITLE);
    } else if (mode_ == Mode::Play) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d  LINE %d", score_, bare_, kLine);
        lineC(3, buf, PAL_INK);
        std::snprintf(buf, sizeof buf, "GOLD %d   IVORY %d   FAULT %d", golds_, ivory_, faults_);
        lineC(4, buf, PAL_HINT);
        lineC(25, lamp_ == Kind::Gold ? "PAWN IS GOLD" : "PAWN IS IVORY",
              lamp_ == Kind::Gold ? PAL_TITLE : PAL_INK);
        if (!bot_) {
            lineAt(1, 26, hover_ == 0 ? "> GOLD" : "  GOLD", PAL_TITLE);
            lineAt(14, 26, hover_ == 1 ? "> LEAVE" : "  LEAVE", PAL_HINT);
            lineAt(28, 26, hover_ == 2 ? "> IVORY" : "  IVORY", PAL_BAD);
        }
    } else if (mode_ == Mode::Win) {
        lineC(4, "ONLY THE GOLD COUNTS DOUBLE", PAL_WIN);
        char buf[40];
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        lineC(6, buf, PAL_INK);
        lineC(24, "THE FILE HELD", PAL_WIN);
    } else {
        lineC(4, "THE LINE DID NOT DOUBLE", PAL_BAD);
        lineC(24, "START", PAL_INK);
    }
}

}  // namespace pawngold
