#include "game/shelvetape.h"

#include <cstdio>
#include <cstring>

namespace shelvetape {

bool Game::matched() const {
    if (filled_ != kTapeN) return false;
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i] || drawer_[i] != kTape[i]) return false;
    return true;
}

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < filled_; i++) {
        int id = drawer_[i];
        if (id >= 0 && id < kRows) s += kSpine[id].pay;
    }
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kSpine[kTape[i]].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kSpine[kTape[i]].pay;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Shelve) return 1;
    if (mode_ == Mode::Leave) return 2;
    return 3;
}

int Game::nextWant() const { return filled_ < kTapeN ? kTape[filled_] : -1; }

float Game::rowY(int i) const { return 48.f + i * 32.f; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    reason_ = "OPEN";
}

void Game::begin() {
    filled_ = 0;
    faults_ = 0;
    returns_ = 0;
    cursor_ = 0;
    offer_ = 0;
    left_ = false;
    over_ = false;
    won_ = false;
    flash_ = 0;
    for (int i = 0; i < kTapeN; i++) {
        held_[i] = false;
        drawer_[i] = -1;
    }
    reason_ = "SHELVE";
    mode_ = Mode::Shelve;
    arm();
}

void Game::arm() {
    life_ = 80;
    act_ = bot_ ? 8 : 0;
    if (offer_ >= kDeckN) {
        fail("THE TAPE RAN OUT");
        return;
    }
}

void Game::fail(const char* why) {
    faults_++;
    reason_ = why;
    won_ = false;
    left_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    if (sys_) sys_->apu.tone(0, 110.f, 0.2f);
    beep_ = 10;
}

void Game::file() {
    if (mode_ != Mode::Shelve || offer_ >= kDeckN) return;
    int book = kDeck[offer_];
    if (cursor_ != book) {
        fail("WRONG ROW");
        return;
    }
    int want = nextWant();
    if (kSpine[book].decoy || book != want) {
        fail("NOT ON THE TAPE");
        return;
    }
    drawer_[filled_] = book;
    held_[filled_] = true;
    filled_++;
    flash_ = 8;
    if (sys_) sys_->apu.tone(0, 660.f, 0.16f);
    beep_ = 6;
    offer_++;
    if (filled_ == kTapeN && matched()) {
        reason_ = "DRAWER FULL";
        mode_ = Mode::Leave;
        act_ = bot_ ? 16 : 0;
        return;
    }
    arm();
}

void Game::expire() {
    if (mode_ != Mode::Shelve || offer_ >= kDeckN) return;
    int book = kDeck[offer_];
    int want = nextWant();
    if (book == want) {
        fail("THE BOOK CAME BACK");
        return;
    }
    returns_++;
    offer_++;
    if (sys_) sys_->apu.tone(1, 220.f, 0.08f);
    beep_ = 4;
    arm();
}

void Game::depart() {
    if (mode_ != Mode::Leave) return;
    left_ = true;
    won_ = matched() && faults_ == 0 && filled_ == kTapeN && returns_ == 2;
    over_ = true;
    reason_ = won_ ? "THE DRAWER MATCHES" : "THE DRAWER MISSED";
    mode_ = Mode::Over;
    if (sys_) sys_->apu.tone(1, won_ ? 880.f : 140.f, 0.18f);
    beep_ = 12;
}

void Game::botAct() {
    if (mode_ == Mode::Title) {
        if (t_ > 18) begin();
        return;
    }
    if (mode_ == Mode::Leave) {
        if (act_ > 0) {
            act_--;
            return;
        }
        depart();
        return;
    }
    if (mode_ != Mode::Shelve) return;
    if (act_ > 0) {
        act_--;
        return;
    }
    int book = kDeck[offer_];
    int want = nextWant();
    if (book != want) return;
    if (cursor_ < book) cursor_++;
    else if (cursor_ > book) cursor_--;
    else file();
}

void Game::human() {
    gs::Pad& p = sys_->pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_START)) begin();
        return;
    }
    if (mode_ == Mode::Lose || mode_ == Mode::Over) {
        if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            reason_ = "OPEN";
        }
        return;
    }
    if (mode_ == Mode::Leave) {
        if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_START)) depart();
        return;
    }
    if (p.pressed(gs::BTN_UP)) cursor_ = (cursor_ + kRows - 1) % kRows;
    if (p.pressed(gs::BTN_DOWN)) cursor_ = (cursor_ + 1) % kRows;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B)) file();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (beep_ > 0 && --beep_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
    if (bot_) botAct();
    else human();
    if (mode_ == Mode::Shelve && life_ > 0 && --life_ == 0) expire();
    if (flash_ > 0) flash_--;
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

void Game::box(float x, float y, float w, float h, int pal) {
    if (w < 1 || h < 1) return;
    gs::Sprite s{};
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.img = art_.solid.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    if (mode_ == Mode::Title) {
        hudC(3, "S3 SHELVETAPE", PAL_AMBER);
        hudC(5, "A SHORT SHELVE", PAL_TEXT);
        hudC(8, "MATCH THE DRAWER", PAL_GREEN);
        hudC(10, "FOLIO  ATLAS  PRIMER", PAL_INK);
        hudC(12, "LEDGER STAYS OUT", PAL_LEDGER);
        hudC(22, "A SHELVES   UP DOWN", PAL_TEXT);
        spr(art_.cart, 70, 150, 36, PAL_WOOD);
        spr(art_.book[0], 62, 132, 28, PAL_FOLIO);
        spr(art_.book[1], 78, 130, 30, PAL_ATLAS);
        return;
    }

    hud(1, 0, "TAPE", PAL_AMBER);
    for (int i = 0; i < kTapeN; i++) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "%s %d", tapeLabel(i), tapeScore(i));
        int pal = (i == filled_ && mode_ == Mode::Shelve) ? PAL_GREEN : PAL_INK;
        hud(7 + i * 11, 0, buf, pal);
    }
    char till[16];
    std::snprintf(till, sizeof till, "TILL %d", drawerScore());
    hud(30, 2, till, PAL_AMBER);

    spr(art_.cart, 56, 128, 40, PAL_WOOD);
    int book = (mode_ == Mode::Shelve && offer_ < kDeckN) ? kDeck[offer_] : -1;
    if (book >= 0) {
        float bh = flash_ ? 32.f : 28.f;
        spr(art_.book[book], 56, 108, bh, kSpine[book].pal);
    }

    for (int i = 0; i < kRows; i++) {
        float y = rowY(i);
        int pal = (i == cursor_) ? PAL_AMBER : PAL_TEXT;
        hud(22, int(y / 8.f) - 1, kSpine[i].name, pal);
        if (i == cursor_) spr(art_.hand, 168, y, 14, PAL_PAPER);
        if (i == book) box(200, y - 2, 90, 3, kSpine[i].pal);
    }

    for (int i = 0; i < kTapeN; i++) {
        float x = 34.f + i * 38.f;
        if (held_[i]) spr(art_.book[drawer_[i]], x, 186, 18, kSpine[drawer_[i]].pal);
    }

    if (mode_ == Mode::Leave) hudC(26, "DRAWER FULL  A LEAVES", PAL_GREEN);
    else if (mode_ == Mode::Lose) hudC(26, reason_, PAL_RED);
    else if (mode_ == Mode::Over) hudC(26, reason_, won_ ? PAL_GREEN : PAL_RED);
    else if (book >= 0 && kSpine[book].decoy) hudC(26, "LET LEDGER COME BACK", PAL_LEDGER);
    else hudC(26, "SHELVE THE LIT ROW", PAL_TEXT);
}

}  // namespace shelvetape
