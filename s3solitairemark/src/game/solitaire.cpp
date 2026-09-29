#include "game/solitaire.h"

#include <cstdio>

namespace solitaire {
namespace {

// Fixed deal. The mark is hearts, ace through king, in this order on the felt.
constexpr int kDeal[kCards] = {7, 2, 11, 4, 9, 1, 13, 6, 3, 10, 8, 12, 5};

struct Slot {
    int x, y;
};

Slot slotAt(int i) {
    int row = i < 5 ? 0 : (i < 9 ? 1 : 2);
    int col = i < 5 ? i : (i < 9 ? i - 5 : i - 9);
    int n = row == 0 ? 5 : 4;
    int span = n * 58;
    int x0 = (320 - span) / 2;
    int y0 = row == 0 ? 36 : (row == 1 ? 98 : 160);
    int xoff = row == 0 ? 0 : 29;
    return {x0 + xoff + col * 58, y0};
}

int rowOf(int i) { return i < 5 ? 0 : (i < 9 ? 1 : 2); }
int colOf(int i) { return i < 5 ? i : (i < 9 ? i - 5 : i - 9); }

int slotAtRC(int row, int col) {
    if (row < 0) row = 0;
    if (row > 2) row = 2;
    int n = row == 0 ? 5 : 4;
    if (col < 0) col = 0;
    if (col >= n) col = n - 1;
    if (row == 0) return col;
    if (row == 1) return 5 + col;
    return 9 + col;
}

const char* nextName(int rank) {
    static const char* names[] = {"", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", ""};
    if (rank < 1 || rank > 13) return "-";
    return names[rank];
}

}  // namespace

void Game::text(gs::VDP& vdp, const char* s, int x, int y, int pal) {
    int pen = x;
    for (const char* p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c < 32 || c > 126) c = 32;
        gs::Image g = art_.glyph[c - 32];
        gs::Sprite sp;
        sp.x = int16_t(pen);
        sp.y = int16_t(y);
        sp.w = g.w;
        sp.h = g.h;
        sp.img = g;
        sp.pal = uint8_t(pal);
        vdp.sprite(sp);
        pen += g.w + 1;
    }
}

void Game::start() {
    for (int i = 0; i < kCards; i++) {
        rank_[i] = kDeal[i];
        taken_[i] = false;
    }
    cursor_ = 5;
    need_ = 1;
    placed_ = 0;
    won_ = false;
    finished_ = false;
    mode_ = Mode::Play;
    hold_ = 0;
    flash_ = 0;
}

void Game::playAt(int slot) {
    if (slot < 0 || slot >= kCards || taken_[slot]) {
        flash_ = 12;
        return;
    }
    if (rank_[slot] != need_) {
        flash_ = 12;
        if (sys_) sys_->apu.tone(0, 110.f, 0.15f);
        return;
    }
    taken_[slot] = true;
    placed_++;
    need_++;
    flash_ = 0;
    if (sys_) sys_->apu.tone(0, 440.f + float(placed_) * 30.f, 0.2f);
    if (need_ > 13) {
        finished_ = true;
        won_ = true;
        mode_ = Mode::Win;
        if (sys_) sys_->apu.tone(1, 660.f, 0.25f);
    }
}

void Game::stepBot() {
    if (mode_ == Mode::Title) {
        if (sys_ && sys_->frame > 24) start();
        return;
    }
    if (mode_ != Mode::Play) return;
    if (botWait_ > 0) {
        botWait_--;
        return;
    }
    int target = -1;
    for (int i = 0; i < kCards; i++) {
        if (!taken_[i] && rank_[i] == need_) target = i;
    }
    if (target < 0) return;
    if (cursor_ != target) {
        int tr = rowOf(target);
        int tc = colOf(target);
        int r = rowOf(cursor_);
        int c = colOf(cursor_);
        if (r != tr) r += (tr > r) ? 1 : -1;
        else if (c != tc) c += (tc > c) ? 1 : -1;
        cursor_ = slotAtRC(r, c);
        botWait_ = 3;
        return;
    }
    playAt(cursor_);
    botWait_ = 4;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.vdp.setFogColor(gs::rgb4(0, 3, 1));
    sys.apu.setMaster(0.65f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = 5 + (y < 112 ? y / 40 : (223 - y) / 40);
        sys.vdp.lineBackdrop[y] = gs::rgb4(0, g, 2);
    }
    mode_ = Mode::Title;
    won_ = false;
    finished_ = false;
}

void Game::draw(gs::System& sys) {
    gs::VDP& vdp = sys.vdp;
    vdp.clearSprites();
    if (mode_ == Mode::Title) {
        text(vdp, "SOLITAIRE", 86, 70, PAL_TITLE);
        text(vdp, "A FINISHED MARK ENDS IT", 48, 96, PAL_INK);
        text(vdp, "HEARTS  ACE TO KING", 68, 118, PAL_GOLD);
        text(vdp, "START", 136, 156, (sys.frame / 20) % 2 ? PAL_GOLD : PAL_INK);
        return;
    }
    text(vdp, "MARK", 8, 6, PAL_GOLD);
    char line[32];
    std::snprintf(line, sizeof line, "NEXT %s", mode_ == Mode::Win ? "K" : nextName(need_));
    text(vdp, line, 48, 6, PAL_INK);
    std::snprintf(line, sizeof line, "%d/13", placed_);
    text(vdp, line, 250, 6, PAL_INK);
    if (mode_ == Mode::Win) text(vdp, "FINISHED MARK", 88, 100, PAL_WIN);

    if (mode_ == Mode::Play) {
        Slot s = slotAt(cursor_);
        gs::Sprite sp;
        sp.x = int16_t(s.x - 3);
        sp.y = int16_t(s.y - 3);
        sp.w = int16_t(art_.cursor.w);
        sp.h = int16_t(art_.cursor.h);
        sp.img = art_.cursor;
        sp.pal = flash_ ? PAL_WIN : PAL_GOLD;
        vdp.sprite(sp);
    }

    for (int i = 0; i < kCards; i++) {
        if (taken_[i]) continue;
        Slot s = slotAt(i);
        gs::Sprite sp;
        sp.x = int16_t(s.x);
        sp.y = int16_t(s.y);
        sp.w = kCardW;
        sp.h = kCardH;
        sp.img = art_.card[rank_[i] - 1];
        sp.pal = PAL_CARD;
        vdp.sprite(sp);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (flash_ > 0) flash_--;
    if (sys.frame % 8 == 0) sys.apu.tone(0, 0, 0);

    if (bot_) {
        stepBot();
    } else if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) start();
    } else if (mode_ == Mode::Play) {
        int r = rowOf(cursor_);
        int c = colOf(cursor_);
        int move = 0;
        if (sys.pad.pressed(gs::BTN_LEFT)) {
            c--;
            move = 1;
        } else if (sys.pad.pressed(gs::BTN_RIGHT)) {
            c++;
            move = 1;
        } else if (sys.pad.pressed(gs::BTN_UP)) {
            r--;
            move = 1;
        } else if (sys.pad.pressed(gs::BTN_DOWN)) {
            r++;
            move = 1;
        }
        if (move) {
            hold_ = 0;
            cursor_ = slotAtRC(r, c);
        }
        if (sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_B)) playAt(cursor_);
    } else if (mode_ == Mode::Win) {
        if (sys.pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            won_ = false;
            finished_ = false;
        }
    }
    draw(sys);
}

}  // namespace solitaire
