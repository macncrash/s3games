#include "game/cheftape.h"

#include <cstdio>
#include <cstring>

namespace cheftape {

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
        if (id >= 0 && id < kDishes) s += kDish[id].pay;
    }
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kDish[kTape[i]].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kDish[kTape[i]].pay;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Service) return 1;
    if (mode_ == Mode::Leave) return 2;
    return 3;
}

int Game::nextWant() const { return filled_ < kTapeN ? kTape[filled_] : -1; }

float Game::stallX(int i) const { return 48.f + i * 72.f; }

bool Game::goldZone() const {
    int spent = kLife - life_;
    return spent >= 34 && spent <= 68;
}

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
    reason_ = "SERVICE";
    mode_ = Mode::Service;
    arm();
}

void Game::arm() {
    life_ = kLife;
    act_ = bot_ ? 6 : 0;
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

void Game::plate() {
    if (mode_ != Mode::Service || offer_ >= kDeckN) return;
    int dish = kDeck[offer_];
    if (cursor_ != dish) {
        fail("WRONG PAN");
        return;
    }
    if (!goldZone()) {
        fail("NOT IN THE GOLD");
        return;
    }
    int want = nextWant();
    if (kDish[dish].decoy || dish != want) {
        fail("NOT ON THE TAPE");
        return;
    }
    drawer_[filled_] = dish;
    held_[filled_] = true;
    filled_++;
    flash_ = 10;
    if (sys_) sys_->apu.tone(0, kDish[dish].pitch, 0.16f);
    beep_ = 8;
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
    if (mode_ != Mode::Service || offer_ >= kDeckN) return;
    int dish = kDeck[offer_];
    int want = nextWant();
    if (dish == want) {
        fail("THE PLATE BURNED");
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
    won_ = matched() && faults_ == 0 && filled_ == kTapeN && drawerScore() == 12;
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
    if (mode_ != Mode::Service) return;
    if (act_ > 0) {
        act_--;
        return;
    }
    int dish = kDeck[offer_];
    int want = nextWant();
    if (dish != want) return;
    if (cursor_ < dish) cursor_++;
    else if (cursor_ > dish) cursor_--;
    else if (goldZone()) plate();
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
    if (p.pressed(gs::BTN_LEFT)) cursor_ = (cursor_ + kDishes - 1) % kDishes;
    if (p.pressed(gs::BTN_RIGHT)) cursor_ = (cursor_ + 1) % kDishes;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B)) plate();
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
    if (mode_ == Mode::Service && life_ > 0 && --life_ == 0) expire();
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

void Game::kitchen() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r = y < 120 ? 2 : 3;
        int g = y < 120 ? 1 : 2;
        v.lineBackdrop[y] = gs::rgb4(r, g, 1);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    kitchen();

    if (mode_ == Mode::Title) {
        spr(art_.word, 160, 42, 28, PAL_GOLD);
        spr(art_.chef, 160, 100, 64, PAL_TEXT);
        spr(art_.reel, 78, 96, 28, PAL_STEEL);
        spr(art_.reel, 242, 96, 28, PAL_STEEL);
        hudC(16, "MATCH THE TAPE", PAL_TEXT);
        hudC(18, "SOUP  STEAK  CAKE", PAL_GOLD);
        hudC(20, "GRAVY STAYS OUT", PAL_GRAVY);
        hudC(22, "PLATE WHILE THE BAR IS GOLD", PAL_FIRE);
        hudC(24, "A PLATES   ARROWS MOVE", PAL_TEXT);
        return;
    }

    spr(art_.chef, 292, 78, 52, PAL_TEXT);
    spr(art_.reel, 18, 18, 16, PAL_STEEL);
    hud(4, 1, "TAPE", PAL_GOLD);
    for (int i = 0; i < kTapeN; i++) {
        char buf[20];
        std::snprintf(buf, sizeof buf, "%s %d", tapeLabel(i), tapeScore(i));
        int pal = (i == filled_ && mode_ == Mode::Service) ? PAL_GOLD : PAL_TEXT;
        hud(4 + i * 10, 2, buf, pal);
    }
    char till[16];
    std::snprintf(till, sizeof till, "TILL %d", drawerScore());
    hud(32, 1, till, PAL_PAPER);

    int lit = (mode_ == Mode::Service && offer_ < kDeckN) ? kDeck[offer_] : -1;
    for (int i = 0; i < kDishes; i++) {
        float x = stallX(i);
        spr(art_.pan, x, 128, 18, PAL_STEEL);
        float h = (i == lit) ? (flash_ ? 36.f : 32.f) : 26.f;
        spr(art_.plate[i], x, 112, h, kDish[i].pal);
        if (i == cursor_) box(x - 4, 86, 8, 4, PAL_GOLD);
        hud(int(x / 8.f) - 2, 17, kDish[i].name, i == cursor_ ? PAL_GOLD : PAL_TEXT);
    }

    if (mode_ == Mode::Service && lit >= 0) {
        float heat = float(kLife - life_) / float(kLife);
        if (heat < 0) heat = 0;
        if (heat > 1) heat = 1;
        box(96, 154, 128, 6, PAL_KITCHEN);
        int pal = goldZone() ? PAL_GOLD : PAL_FIRE;
        box(96, 154, 128.f * heat, 6, pal);
        box(96 + 128.f * 34.f / kLife, 152, 2, 10, PAL_GOLD);
        box(96 + 128.f * 68.f / kLife, 152, 2, 10, PAL_GOLD);
    }

    box(28, 176, 264, 24, PAL_STEEL);
    hud(1, 22, "DRAWER", PAL_PAPER);
    for (int i = 0; i < kTapeN; i++) {
        float x = 96.f + i * 72.f;
        if (held_[i]) {
            spr(art_.ticket, x, 188, 14, PAL_PAPER);
            hud(int(x / 8.f) - 2, 23, kDish[drawer_[i]].name, kDish[drawer_[i]].pal);
        } else {
            box(x - 14, 182, 28, 12, PAL_KITCHEN);
        }
    }

    if (mode_ == Mode::Leave) hudC(26, "DRAWER FULL  A LEAVES", PAL_GOLD);
    else if (mode_ == Mode::Lose) hudC(26, reason_, PAL_ALERT);
    else if (mode_ == Mode::Over) hudC(26, reason_, won_ ? PAL_GOLD : PAL_ALERT);
    else if (lit >= 0 && kDish[lit].decoy) hudC(26, "LET THE GRAVY BURN", PAL_GRAVY);
    else if (lit >= 0 && goldZone()) hudC(26, "GOLD  PLATE IT", PAL_GOLD);
    else hudC(26, "WAIT FOR THE GOLD", PAL_TEXT);
}

}  // namespace cheftape
