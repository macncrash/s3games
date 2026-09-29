#include "game/boardtape.h"

#include <cstdio>
#include <cstring>

namespace boardtape {

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
        if (id >= 0 && id < kJacks) s += kCall[id].pay;
    }
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kCall[kTape[i]].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kCall[kTape[i]].pay;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Patch) return 1;
    if (mode_ == Mode::Leave) return 2;
    return 3;
}

int Game::nextWant() const { return filled_ < kTapeN ? kTape[filled_] : -1; }

float Game::jackX(int i) const { return 50.f + i * 72.f; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    reason_ = "OPEN";
}

void Game::begin() {
    filled_ = 0;
    faults_ = 0;
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
    reason_ = "PATCH";
    mode_ = Mode::Patch;
    arm();
}

void Game::arm() {
    life_ = 72;
    act_ = bot_ ? 10 : 0;
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
    if (mode_ != Mode::Patch || offer_ >= kDeckN) return;
    int lamp = kDeck[offer_];
    if (cursor_ != lamp) {
        fail("WRONG JACK");
        return;
    }
    int want = nextWant();
    if (kCall[lamp].decoy || lamp != want) {
        fail("NOT ON THE TAPE");
        return;
    }
    drawer_[filled_] = lamp;
    held_[filled_] = true;
    filled_++;
    flash_ = 8;
    if (sys_) sys_->apu.tone(0, 740.f, 0.16f);
    beep_ = 6;
    offer_++;
    if (filled_ == kTapeN && matched()) {
        reason_ = "DRAWER FULL";
        mode_ = Mode::Leave;
        act_ = bot_ ? 14 : 0;
        return;
    }
    arm();
}

void Game::expire() {
    if (mode_ != Mode::Patch || offer_ >= kDeckN) return;
    int lamp = kDeck[offer_];
    int want = nextWant();
    if (lamp == want) {
        fail("THE CALL DROPPED");
        return;
    }
    offer_++;
    if (sys_) sys_->apu.tone(1, 220.f, 0.08f);
    beep_ = 4;
    arm();
}

void Game::depart() {
    if (mode_ != Mode::Leave) return;
    left_ = true;
    won_ = matched() && faults_ == 0 && filled_ == kTapeN;
    over_ = true;
    reason_ = won_ ? "THE DRAWER MATCHES" : "THE DRAWER MISSED";
    mode_ = Mode::Over;
    if (sys_) sys_->apu.tone(1, won_ ? 880.f : 140.f, 0.18f);
    beep_ = 12;
}

void Game::botAct() {
    if (mode_ == Mode::Title) {
        if (t_ > 20) begin();
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
    if (mode_ != Mode::Patch) return;
    if (act_ > 0) {
        act_--;
        return;
    }
    int lamp = kDeck[offer_];
    int want = nextWant();
    if (lamp != want) return;
    if (cursor_ < lamp) cursor_++;
    else if (cursor_ > lamp) cursor_--;
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
    if (p.pressed(gs::BTN_LEFT)) cursor_ = (cursor_ + kJacks - 1) % kJacks;
    if (p.pressed(gs::BTN_RIGHT)) cursor_ = (cursor_ + 1) % kJacks;
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
    if (mode_ == Mode::Patch && life_ > 0 && --life_ == 0) expire();
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
        hudC(3, "S3 BOARDTAPE", PAL_AMBER);
        hudC(5, "A SHORT BOARD", PAL_TEXT);
        hudC(8, "FILE THE TAPE", PAL_GREEN);
        hudC(10, "LOCAL  TRUNK  TOLL", PAL_INK);
        hudC(12, "NIGHT STAYS OUT", PAL_NIGHT);
        hudC(22, "A FILES   ARROWS MOVE", PAL_TEXT);
        spr(art_.slip, 160, 150, 22, PAL_PAPER);
        spr(art_.lamp, 160, 124, 20, PAL_LAMP);
        return;
    }

    hud(1, 1, "TAPE", PAL_AMBER);
    for (int i = 0; i < kTapeN; i++) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "%s %d", tapeLabel(i), tapeScore(i));
        int pal = (i == filled_ && mode_ == Mode::Patch) ? PAL_GREEN : PAL_INK;
        hud(8 + i * 10, 1, buf, pal);
    }
    hud(1, 3, "DRAWER", PAL_PAPER);
    char till[16];
    std::snprintf(till, sizeof till, "TILL %d", drawerScore());
    hud(30, 3, till, PAL_AMBER);

    int lamp = (mode_ == Mode::Patch && offer_ < kDeckN) ? kDeck[offer_] : -1;
    for (int i = 0; i < kJacks; i++) {
        float x = jackX(i);
        int pal = kCall[i].decoy ? PAL_NIGHT : PAL_BRASS;
        spr(art_.jack, x, 118, 28, pal);
        spr(art_.stamp[i], x, 118, 12, PAL_INK);
        hud(int(x / 8.f) - 2, 16, kCall[i].name, i == cursor_ ? PAL_AMBER : PAL_TEXT);
        if (i == lamp) spr(art_.lamp, x, 78, flash_ ? 26 : 22, PAL_LAMP);
        if (i == cursor_) spr(art_.plug, x, 100, 16, PAL_PAPER);
    }

    if (lamp >= 0) {
        float lx = jackX(lamp);
        float cx = jackX(cursor_);
        box(lx - 1.5f, 88, 3, 14, PAL_CORD);
        float x0 = lx < cx ? lx : cx;
        float x1 = lx < cx ? cx : lx;
        box(x0, 100, x1 - x0 + 3, 3, PAL_CORD);
    }

    for (int i = 0; i < kTapeN; i++) {
        float x = 72.f + i * 90.f;
        if (held_[i]) {
            spr(art_.slip, x, 188, 16, PAL_PAPER);
            spr(art_.stamp[drawer_[i]], x - 10, 188, 10, PAL_INK);
        }
    }

    if (mode_ == Mode::Leave) hudC(26, "DRAWER FULL  A LEAVES", PAL_GREEN);
    else if (mode_ == Mode::Lose) hudC(26, reason_, PAL_RED);
    else if (mode_ == Mode::Over) hudC(26, reason_, won_ ? PAL_GREEN : PAL_RED);
    else if (lamp >= 0 && kCall[lamp].decoy) hudC(26, "LET NIGHT DIE", PAL_NIGHT);
    else hudC(26, "PLUG THE LIT JACK", PAL_TEXT);
}

}  // namespace boardtape
