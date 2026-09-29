#include "game/solitaire.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace solitaire {

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

void Game::deal() {
    score_ = bare_ = golds_ = cream_ = faults_ = 0;
    next_ = 1;
    cursor_ = 0;
    wait_ = 0;
    life_ = 0;
    goldOut_ = false;
    won_ = false;
    over_ = false;
    // Ascending golds with cream between them. Cream must be set aside.
    const Kind kinds[kCards] = {Kind::Gold, Kind::Cream, Kind::Gold, Kind::Gold, Kind::Cream, Kind::Gold};
    const int ranks[kCards] = {1, 5, 2, 3, 6, 4};
    for (int i = 0; i < kCards; i++) {
        cards_[i].kind = kinds[i];
        cards_[i].rank = ranks[i];
        cards_[i].live = true;
    }
    mode_ = Mode::Play;
    sys_->apu.tone(0, 392.f, 0.06f);
}

int Game::liveCount() const {
    int n = 0;
    for (int i = 0; i < kCards; i++)
        if (cards_[i].live) n++;
    return n;
}

int Game::nextGold() const {
    for (int i = 0; i < kCards; i++) {
        if (cards_[i].live && cards_[i].kind == Kind::Gold && cards_[i].rank == next_) return i;
    }
    return -1;
}

int Game::firstCream() const {
    for (int i = 0; i < kCards; i++)
        if (cards_[i].live && cards_[i].kind == Kind::Cream) return i;
    return -1;
}

void Game::fault() {
    faults_++;
    sys_->apu.tone(0, 140.f, 0.08f);
    sys_->apu.noiseBurst(0.12f, 700.f, 0.05f);
    if (faults_ >= 3) {
        won_ = false;
        mode_ = Mode::Lose;
        wait_ = 0;
    }
}

void Game::judge() {
    wait_ = 0;
    // Only the gold counts double. Cream face points cannot buy the line.
    if (goldOut_ && golds_ > 0 && cream_ == 0 && score_ >= kLine && bare_ < kLine && faults_ < 3 && liveCount() == 0) {
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

void Game::fileGold(int i) {
    if (i < 0 || i >= kCards || !cards_[i].live) return;
    Card& c = cards_[i];
    if (c.kind == Kind::Gold && c.rank == next_) {
        score_ += 2;
        bare_ += 1;
        golds_++;
        next_++;
        goldOut_ = true;
        c.live = false;
        sys_->apu.tone(0, 660.f, 0.07f);
        if (!sys_->headless) sys_->rumble(0.15f, 0.35f, 40);
        if (liveCount() == 0) judge();
    } else {
        fault();
    }
}

void Game::fileCream(int i) {
    if (i < 0 || i >= kCards || !cards_[i].live) return;
    Card& c = cards_[i];
    if (c.kind == Kind::Cream) {
        score_ += 1;
        bare_ += 1;
        cream_++;
        c.live = false;
        sys_->apu.tone(0, 330.f, 0.05f);
        if (liveCount() == 0) judge();
    } else {
        fault();
    }
}

void Game::leave(int i) {
    if (i < 0 || i >= kCards || !cards_[i].live) return;
    cards_[i].live = false;
    sys_->apu.tone(1, 220.f, 0.04f);
    if (liveCount() == 0) judge();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) deal();
    } else if (mode_ == Mode::Play) {
        life_++;
        if (bot_) {
            if (life_ % 10 == 0) {
                int g = nextGold();
                if (g >= 0) fileGold(g);
                else {
                    int c = firstCream();
                    if (c >= 0) leave(c);
                    else if (liveCount() == 0) judge();
                    else leave(0);
                }
            }
        } else {
            if (pad.pressed(gs::BTN_LEFT)) cursor_ = (cursor_ + kCards - 1) % kCards;
            if (pad.pressed(gs::BTN_RIGHT)) cursor_ = (cursor_ + 1) % kCards;
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_X)) fileGold(cursor_);
            else if (pad.pressed(gs::BTN_B)) leave(cursor_);
            else if (pad.pressed(gs::BTN_C)) fileCream(cursor_);
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
        int shade = 0 + (y * 2) / gs::SCREEN_H;
        v.lineBackdrop[y] = gs::rgb4(0, shade + 1, shade);
    }

    blit(art_.felt, 160.f, 124.f, float(art_.felt.w), float(art_.felt.h), PAL_FELT);

    if (mode_ == Mode::Play || mode_ == Mode::Win || mode_ == Mode::Lose) {
        blit(art_.stack, 268.f, 78.f, 36.f, 28.f, PAL_GOLD);
        blit(art_.stack, 268.f, 150.f, 36.f, 28.f, PAL_CREAM);
        for (int i = 0; i < kCards; i++) {
            if (!cards_[i].live && mode_ == Mode::Play) continue;
            float x = 28.f + float(i) * 36.f;
            float y = 128.f;
            if (!cards_[i].live) y = cards_[i].kind == Kind::Gold ? 70.f : 168.f;
            int pal = cards_[i].kind == Kind::Gold ? PAL_GOLD : PAL_CREAM;
            bool hot = mode_ == Mode::Play && i == cursor_ && cards_[i].live && !bot_;
            blit(art_.card, x, hot ? y - 6.f : y, 28.f, 40.f, pal);
            blit(cards_[i].kind == Kind::Gold ? art_.coin : art_.oval, x, (hot ? y - 6.f : y) + 8.f, 12.f, 12.f,
                 PAL_PIP);
            char rank[2] = {char('0' + cards_[i].rank), 0};
            int col = int(x) / 8 - 0;
            int row = int((hot ? y - 6.f : y) - 16.f) / 8;
            if (cards_[i].live || mode_ != Mode::Play) lineAt(col, row, rank, pal == PAL_GOLD ? PAL_TITLE : PAL_INK);
        }
    }

    lineC(1, "S3 SOLITAIRE GOLD", PAL_TITLE);
    if (mode_ == Mode::Title) {
        lineC(4, "ONLY THE GOLD COUNTS DOUBLE", PAL_INK);
        lineC(11, "BUILD GOLD 1 TO 4", PAL_HINT);
        lineC(13, "A  FILE THE GOLD", PAL_HINT);
        lineC(15, "B  SET THE CREAM ASIDE", PAL_HINT);
        lineC(17, "C  CREAM SCORES ONE", PAL_BAD);
        lineC(21, "CREAM CANNOT BUY THE LINE", PAL_INK);
        lineC(25, "START", PAL_TITLE);
    } else if (mode_ == Mode::Play) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d  LINE %d", score_, bare_, kLine);
        lineC(3, buf, PAL_INK);
        std::snprintf(buf, sizeof buf, "GOLD %d   CREAM %d   NEXT %d", golds_, cream_, next_);
        lineC(4, buf, PAL_HINT);
        if (!bot_) lineC(26, "LEFT RIGHT   A GOLD   B ASIDE", PAL_HINT);
    } else if (mode_ == Mode::Win) {
        lineC(4, "ONLY THE GOLD COUNTS DOUBLE", PAL_WIN);
        char buf[40];
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        lineC(6, buf, PAL_INK);
        lineC(25, "THE TABLE HELD", PAL_WIN);
    } else {
        lineC(4, "THE LINE DID NOT DOUBLE", PAL_BAD);
        lineC(25, "START", PAL_INK);
    }
}

}  // namespace solitaire
