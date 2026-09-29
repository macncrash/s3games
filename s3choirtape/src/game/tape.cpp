#include "game/tape.h"

#include <cstdio>
#include <cstring>

namespace choirtape {

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
        if (id >= 0 && id < kVoices) s += kPart[id].pay;
    }
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kPart[kTape[i]].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kPart[kTape[i]].pay;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Phrase) return 1;
    if (mode_ == Mode::Leave) return 2;
    return 3;
}

int Game::nextWant() const { return filled_ < kTapeN ? kTape[filled_] : -1; }

float Game::stallX(int i) const { return 48.f + i * 74.f; }

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
    reason_ = "LISTEN";
    mode_ = Mode::Phrase;
    arm();
}

void Game::arm() {
    life_ = 78;
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
    if (sys_) sys_->apu.tone(0, 98.f, 0.2f);
    beep_ = 12;
}

void Game::file() {
    if (mode_ != Mode::Phrase || offer_ >= kDeckN) return;
    int voice = kDeck[offer_];
    if (cursor_ != voice) {
        fail("WRONG STALL");
        return;
    }
    int want = nextWant();
    if (kPart[voice].decoy || voice != want) {
        fail("NOT ON THE TAPE");
        return;
    }
    drawer_[filled_] = voice;
    held_[filled_] = true;
    filled_++;
    flash_ = 10;
    if (sys_) sys_->apu.tone(0, kPart[voice].pitch, 0.16f);
    beep_ = 8;
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
    if (mode_ != Mode::Phrase || offer_ >= kDeckN) return;
    int voice = kDeck[offer_];
    int want = nextWant();
    if (voice == want) {
        fail("THE PART DROPPED");
        return;
    }
    offer_++;
    if (sys_) sys_->apu.tone(1, 164.f, 0.07f);
    beep_ = 5;
    arm();
}

void Game::depart() {
    if (mode_ != Mode::Leave) return;
    left_ = true;
    won_ = matched() && faults_ == 0 && filled_ == kTapeN;
    over_ = true;
    reason_ = won_ ? "THE DRAWER MATCHES" : "THE DRAWER MISSED";
    mode_ = Mode::Over;
    if (sys_) {
        sys_->apu.tone(0, won_ ? 523.25f : 130.f, 0.14f);
        sys_->apu.tone(1, won_ ? 659.25f : 98.f, 0.1f);
    }
    beep_ = 16;
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
    if (mode_ != Mode::Phrase) return;
    if (act_ > 0) {
        act_--;
        return;
    }
    int voice = kDeck[offer_];
    int want = nextWant();
    if (voice != want) return;
    if (cursor_ < voice) cursor_++;
    else if (cursor_ > voice) cursor_--;
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
    if (p.pressed(gs::BTN_LEFT)) cursor_ = (cursor_ + kVoices - 1) % kVoices;
    if (p.pressed(gs::BTN_RIGHT)) cursor_ = (cursor_ + 1) % kVoices;
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
    if (mode_ == Mode::Phrase && life_ > 0 && --life_ == 0) expire();
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

void Game::nave() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = 1 + (y < 140 ? 0 : (y - 140) / 28);
        if (g > 4) g = 4;
        v.lineBackdrop[y] = gs::rgb4(1, 1, 2 + (y < 90 ? 1 : 0));
        v.lineFog[y] = 0;
        v.road[y].on = false;
        (void)g;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    nave();

    if (mode_ == Mode::Title) {
        spr(art_.wordChoir, 160, 46, 28, PAL_GOLD);
        spr(art_.reel, 160, 96, 40, PAL_WOOD);
        spr(art_.note, 118, 78, 16, PAL_TREBLE);
        spr(art_.note, 202, 78, 16, PAL_ALTO);
        hudC(16, "MATCH THE TAPE", PAL_TEXT);
        hudC(18, "TREBLE  ALTO  BASS", PAL_GOLD);
        hudC(20, "HUM STAYS OUT", PAL_HUM);
        hudC(24, "A SINGS   ARROWS MOVE", PAL_TEXT);
        return;
    }

    spr(art_.reel, 22, 22, 18, PAL_WOOD);
    hud(5, 1, "TAPE", PAL_GOLD);
    for (int i = 0; i < kTapeN; i++) {
        char buf[20];
        std::snprintf(buf, sizeof buf, "%s %d", tapeLabel(i), tapeScore(i));
        int pal = (i == filled_ && mode_ == Mode::Phrase) ? PAL_GOLD : PAL_TEXT;
        hud(5 + i * 11, 2, buf, pal);
    }
    char till[16];
    std::snprintf(till, sizeof till, "TILL %d", drawerScore());
    hud(32, 1, till, PAL_PAPER);

    int lit = (mode_ == Mode::Phrase && offer_ < kDeckN) ? kDeck[offer_] : -1;
    for (int i = 0; i < kVoices; i++) {
        float x = stallX(i);
        float h = (i == lit) ? (flash_ ? 62.f : 56.f) : 48.f;
        spr(art_.singer[i], x, 118, h, kPart[i].pal);
        if (i == cursor_) spr(art_.note, x, 78, 14, PAL_GOLD);
        hud(int(x / 8.f) - 3, 16, kPart[i].name, i == cursor_ ? PAL_GOLD : PAL_TEXT);
    }

    box(36, 176, 248, 22, PAL_WOOD);
    hud(2, 22, "DRAWER", PAL_PAPER);
    for (int i = 0; i < kTapeN; i++) {
        float x = 88.f + i * 72.f;
        if (held_[i]) {
            spr(art_.slip, x, 186, 14, PAL_PAPER);
            hud(int(x / 8.f) - 3, 23, kPart[drawer_[i]].name, kPart[drawer_[i]].pal);
        } else {
            box(x - 12, 180, 24, 12, PAL_NAVE);
        }
    }

    if (mode_ == Mode::Leave) hudC(26, "DRAWER FULL  A LEAVES", PAL_GOLD);
    else if (mode_ == Mode::Lose) hudC(26, reason_, PAL_ALERT);
    else if (mode_ == Mode::Over) hudC(26, reason_, won_ ? PAL_GOLD : PAL_ALERT);
    else if (lit >= 0 && kPart[lit].decoy) hudC(26, "LET THE HUM DIE", PAL_HUM);
    else hudC(26, "FILE THE LIT VOICE", PAL_TEXT);
}

}  // namespace choirtape
