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
    you_ = house_ = 0;
    next_ = 1;
    cursor_ = 0;
    wait_ = 0;
    life_ = 0;
    won_ = false;
    over_ = false;
    // Ascending run with house cards between. House must stay under seven.
    const Kind kinds[kCards] = {Kind::You,   Kind::House, Kind::You, Kind::You,   Kind::House,
                                Kind::You,   Kind::You,   Kind::House, Kind::You, Kind::You};
    const int ranks[kCards] = {1, 3, 2, 3, 5, 4, 5, 2, 6, 7};
    for (int i = 0; i < kCards; i++) {
        cards_[i].kind = kinds[i];
        cards_[i].rank = ranks[i];
        cards_[i].live = true;
    }
    mode_ = Mode::Play;
    sys_->apu.tone(0, 392.f, 0.06f);
}

int Game::nextYou() const {
    for (int i = 0; i < kCards; i++) {
        if (cards_[i].live && cards_[i].kind == Kind::You && cards_[i].rank == next_) return i;
    }
    return -1;
}

int Game::firstHouse() const {
    for (int i = 0; i < kCards; i++)
        if (cards_[i].live && cards_[i].kind == Kind::House) return i;
    return -1;
}

void Game::fault() {
    house_++;
    sys_->apu.tone(0, 140.f, 0.08f);
    sys_->apu.noiseBurst(0.12f, 700.f, 0.05f);
    if (house_ >= kSeven && you_ < kSeven) {
        won_ = false;
        mode_ = Mode::Lose;
        wait_ = 0;
    }
}

void Game::judge() {
    wait_ = 0;
    if (you_ >= kSeven && house_ < kSeven && next_ == kSeven + 1) {
        won_ = true;
        mode_ = Mode::Win;
        sys_->apu.tone(0, 523.f, 0.1f);
        if (!sys_->headless) sys_->rumble(0.3f, 0.55f, 140);
    } else if (house_ >= kSeven && you_ < kSeven) {
        won_ = false;
        mode_ = Mode::Lose;
        sys_->apu.tone(0, 160.f, 0.1f);
    }
}

void Game::fileYou(int i) {
    if (mode_ != Mode::Play || i < 0 || i >= kCards || !cards_[i].live) return;
    Card& c = cards_[i];
    if (c.kind == Kind::You && c.rank == next_) {
        you_++;
        next_++;
        c.live = false;
        sys_->apu.tone(0, 660.f, 0.07f);
        if (!sys_->headless) sys_->rumble(0.15f, 0.35f, 40);
        judge();
    } else {
        fault();
    }
}

void Game::fileHouse(int i) {
    if (mode_ != Mode::Play || i < 0 || i >= kCards || !cards_[i].live) return;
    Card& c = cards_[i];
    if (c.kind == Kind::House) {
        house_++;
        c.live = false;
        sys_->apu.tone(0, 196.f, 0.06f);
        judge();
        if (mode_ == Mode::Play && house_ >= kSeven && you_ < kSeven) {
            won_ = false;
            mode_ = Mode::Lose;
            wait_ = 0;
        }
    } else {
        fault();
    }
}

void Game::leave(int i) {
    if (mode_ != Mode::Play || i < 0 || i >= kCards || !cards_[i].live) return;
    if (cards_[i].kind != Kind::House) {
        fault();
        return;
    }
    cards_[i].live = false;
    sys_->apu.tone(1, 220.f, 0.04f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) deal();
    } else if (mode_ == Mode::Play) {
        life_++;
        if (bot_) {
            if (life_ % 8 == 0) {
                int g = nextYou();
                if (g >= 0) fileYou(g);
                else {
                    int h = firstHouse();
                    if (h >= 0) leave(h);
                }
            }
        } else {
            if (pad.pressed(gs::BTN_LEFT)) cursor_ = (cursor_ + kCards - 1) % kCards;
            if (pad.pressed(gs::BTN_RIGHT)) cursor_ = (cursor_ + 1) % kCards;
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_X)) fileYou(cursor_);
            else if (pad.pressed(gs::BTN_B)) leave(cursor_);
            else if (pad.pressed(gs::BTN_C)) fileHouse(cursor_);
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
        int shade = (y * 2) / gs::SCREEN_H;
        v.lineBackdrop[y] = gs::rgb4(0, shade + 1, shade);
    }

    blit(art_.felt, 160.f, 120.f, float(art_.felt.w), float(art_.felt.h), PAL_FELT);
    blit(art_.stack, 48.f, 48.f, 40.f, 22.f, PAL_YOU);
    blit(art_.stack, 272.f, 48.f, 40.f, 22.f, PAL_HOUSE);

    if (mode_ == Mode::Play || mode_ == Mode::Win || mode_ == Mode::Lose) {
        for (int i = 0; i < kCards; i++) {
            if (!cards_[i].live && mode_ == Mode::Play) continue;
            float x = 22.f + float(i) * 28.f;
            float y = cards_[i].live ? 132.f : (cards_[i].kind == Kind::You ? 46.f : 46.f);
            if (!cards_[i].live) x = cards_[i].kind == Kind::You ? 48.f : 272.f;
            int pal = cards_[i].kind == Kind::You ? PAL_YOU : PAL_HOUSE;
            bool hot = mode_ == Mode::Play && i == cursor_ && cards_[i].live && !bot_;
            float cy = hot ? y - 6.f : y;
            blit(art_.card, x, cy, 22.f, 32.f, pal);
            blit(cards_[i].kind == Kind::You ? art_.pip : art_.house, x, cy + 6.f, 10.f, 10.f, PAL_PIP);
            char rank[2] = {char('0' + cards_[i].rank), 0};
            int col = int(x) / 8;
            int row = int(cy - 14.f) / 8;
            if (cards_[i].live || mode_ != Mode::Play) lineAt(col, row, rank, pal == PAL_YOU ? PAL_TITLE : PAL_BAD);
        }
    }

    lineC(1, "S3 SOLITAIRE SEVEN", PAL_TITLE);
    if (mode_ == Mode::Title) {
        lineC(4, "FIRST TO SEVEN", PAL_INK);
        lineC(11, "BUILD YOUR RUN 1 TO 7", PAL_HINT);
        lineC(13, "A  FILE YOUR CARD", PAL_HINT);
        lineC(15, "B  LEAVE THE HOUSE", PAL_HINT);
        lineC(17, "C  FILES FOR THE HOUSE", PAL_BAD);
        lineC(21, "LEAVE WHEN YOU ARE FIRST", PAL_INK);
        lineC(25, "START", PAL_TITLE);
    } else if (mode_ == Mode::Play) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "YOU %d   HOUSE %d   NEXT %d", you_, house_, next_ > kSeven ? kSeven : next_);
        lineC(3, buf, PAL_INK);
        if (!bot_) lineC(26, "LEFT RIGHT   A YOURS   B LEAVE", PAL_HINT);
    } else if (mode_ == Mode::Win) {
        lineC(4, "FIRST TO SEVEN", PAL_WIN);
        char buf[40];
        std::snprintf(buf, sizeof buf, "YOU %d   HOUSE %d", you_, house_);
        lineC(6, buf, PAL_INK);
        lineC(25, "LEAVE", PAL_WIN);
    } else {
        lineC(4, "THE HOUSE WAS FIRST", PAL_BAD);
        lineC(25, "START", PAL_INK);
    }
}

}  // namespace solitaire
