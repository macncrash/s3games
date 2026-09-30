#include "game/pawn.h"

#include <cstdio>
#include <cstring>

namespace pawntape {

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
        if (id >= 0 && id < kKinds) s += kKind[id].pay;
    }
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kKind[kTape[i]].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kKind[kTape[i]].pay;
}

int Game::nextWant() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return kTape[i];
    return -1;
}

const gs::Mipped& Game::pawnOf(int id) const {
    if (id == 1) return art_.ebony;
    if (id == 2) return art_.crown;
    if (id == 3) return art_.ghost;
    return art_.ivory;
}

int Game::palOf(int id) const {
    if (id == 1) return PAL_EBONY;
    if (id == 2) return PAL_GOLD;
    if (id == 3) return PAL_GHOST;
    return PAL_IVORY;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    why_ = "OPEN";
}

void Game::begin() {
    filled_ = faults_ = offer_ = x_ = anim_ = 0;
    over_ = won_ = left_ = false;
    for (int i = 0; i < kTapeN; i++) {
        held_[i] = false;
        drawer_[i] = -1;
    }
    why_ = "OPEN";
    sys_->apu.silence();
    arm();
}

void Game::arm() {
    if (offer_ >= kDeckN) {
        fail("THE FILE RAN OUT");
        return;
    }
    x_ = -28;
    anim_ = 0;
    mode_ = Mode::March;
    why_ = "MARCH";
}

void Game::pocket() {
    if (mode_ != Mode::March || offer_ >= kDeckN) return;
    int id = kDeck[offer_];
    bool onSquare = x_ >= 132 && x_ <= 196;
    if (!onSquare) {
        faults_++;
        fail("OFF THE SQUARE");
        return;
    }
    int want = nextWant();
    if (kKind[id].decoy || id != want) {
        faults_++;
        fail(kKind[id].decoy ? "GHOST STAYS OUT" : "NOT ON THE TAPE");
        return;
    }
    drawer_[filled_] = id;
    held_[filled_] = true;
    filled_++;
    offer_++;
    anim_ = 10;
    mode_ = Mode::Pocket;
    why_ = "POCKETED";
    sys_->apu.tone(0, 520.f + filled_ * 40.f, 0.16f);
    if (filled_ == kTapeN && matched()) {
        why_ = "DRAWER FULL";
        mode_ = Mode::Leave;
        anim_ = bot_ ? 18 : 0;
    }
}

void Game::passOff() {
    if (mode_ != Mode::March || offer_ >= kDeckN) return;
    int id = kDeck[offer_];
    if (!kKind[id].decoy) {
        faults_++;
        fail("THE PAWN WALKED PAST");
        return;
    }
    offer_++;
    why_ = "GHOST WALKED";
    sys_->apu.tone(1, 180.f, 0.08f);
    arm();
}

void Game::fail(const char* why) {
    why_ = why;
    won_ = false;
    left_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    anim_ = 0;
    sys_->apu.tone(1, 110.f, 0.18f);
}

void Game::depart() {
    if (mode_ != Mode::Leave) return;
    left_ = true;
    won_ = matched() && faults_ == 0 && filled_ == kTapeN && drawerScore() == 15;
    over_ = true;
    why_ = won_ ? "the drawer matches the tape" : "THE DRAWER MISSED";
    mode_ = Mode::Over;
    sys_->apu.tone(1, won_ ? 660.f : 140.f, 0.2f);
}

void Game::botAct() {
    if (mode_ == Mode::Title) {
        if (t_ > 12) begin();
        return;
    }
    if (mode_ == Mode::Leave) {
        if (anim_ > 0) {
            anim_--;
            return;
        }
        depart();
        return;
    }
    if (mode_ != Mode::March) return;
    int id = kDeck[offer_];
    if (kKind[id].decoy) return;
    if (x_ >= 156 && x_ <= 176) pocket();
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
            why_ = "OPEN";
        }
        return;
    }
    if (mode_ == Mode::Leave) {
        if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_START)) depart();
        return;
    }
    if (mode_ == Mode::March && (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C))) pocket();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (bot_) botAct();
    else human();
    if (mode_ == Mode::March) {
        x_ += 3;
        if (x_ > 340) passOff();
    } else if (mode_ == Mode::Pocket) {
        if (anim_ > 0) anim_--;
        else arm();
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

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    if (mode_ == Mode::Title) {
        hudC(2, "S3 PAWNTAPE", PAL_AMBER);
        hudC(5, "MATCH THE TAPE", PAL_GREEN);
        hudC(8, "FILE   RANK   PASS", PAL_TEXT);
        hudC(10, "GHOST STAYS OUT", PAL_GHOST);
        hudC(22, "A POCKETS THE SQUARE", PAL_TEXT);
        spr(art_.ivory, 90, 140, 40, PAL_IVORY);
        spr(art_.ebony, 160, 140, 40, PAL_EBONY);
        spr(art_.crown, 230, 140, 40, PAL_GOLD);
        spr(art_.slip, 160, 96, 18, PAL_PAPER);
        return;
    }

    hud(1, 1, "TAPE", PAL_AMBER);
    for (int i = 0; i < kTapeN; i++) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "%s %d", tapeLabel(i), tapeScore(i));
        int pal = (i == filled_ && (mode_ == Mode::March || mode_ == Mode::Pocket)) ? PAL_GREEN : PAL_TEXT;
        hud(7 + i * 11, 1, buf, pal);
    }
    char till[16];
    std::snprintf(till, sizeof till, "TILL %d", drawerScore());
    hud(31, 3, till, PAL_AMBER);
    hud(1, 3, "DRAWER", PAL_PAPER);

    if (mode_ == Mode::March && offer_ < kDeckN) {
        int id = kDeck[offer_];
        spr(pawnOf(id), float(x_), 108.f, 36.f, palOf(id));
    }

    for (int i = 0; i < kTapeN; i++) {
        float x = 64.f + i * 96.f;
        if (held_[i]) {
            spr(art_.slip, x, 190.f, 14.f, PAL_PAPER);
            spr(pawnOf(drawer_[i]), x, 186.f, 22.f, palOf(drawer_[i]));
        }
    }

    if (mode_ == Mode::Leave) hudC(26, "DRAWER FULL  A LEAVES", PAL_GREEN);
    else if (mode_ == Mode::Lose || mode_ == Mode::Over) hudC(26, why_, won_ ? PAL_GREEN : PAL_RED);
    else if (mode_ == Mode::March && offer_ < kDeckN && kKind[kDeck[offer_]].decoy)
        hudC(26, "LET THE GHOST WALK", PAL_GHOST);
    else hudC(26, "POCKET THE LIT SQUARE", PAL_TEXT);
}

}  // namespace pawntape
