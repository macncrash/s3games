#include "game/tape.h"

#include <cstdio>
#include <cstring>

namespace markettape {
namespace {

const Good kGoods[kGoodN] = {
    {"APPLE", 12, 0}, {"PLUM", 12, -1}, {"LOAF", 9, 1}, {"BUN", 9, -1}, {"PEAR", 7, 2}, {"FIG", 7, -1},
};

const gs::Mipped* picOf(const Art& a, int g) {
    switch (g) {
    case 0: return &a.apple;
    case 1: return &a.plum;
    case 2: return &a.loaf;
    case 3: return &a.bun;
    case 4: return &a.pear;
    default: return &a.fig;
    }
}

int palOf(int g) {
    switch (g) {
    case 0: return PAL_APPLE;
    case 1: return PAL_PLUM;
    case 2: return PAL_LOAF;
    case 3: return PAL_BUN;
    case 4: return PAL_PEAR;
    default: return PAL_FIG;
    }
}

}  // namespace

const Good* goods() { return kGoods; }

const char* tapeName(int i) {
    if (i < 0 || i >= kTapeN) return "";
    for (int g = 0; g < kGoodN; g++)
        if (kGoods[g].line == i) return kGoods[g].name;
    return "";
}

int tapePay(int i) {
    if (i < 0 || i >= kTapeN) return 0;
    for (int g = 0; g < kGoodN; g++)
        if (kGoods[g].line == i) return kGoods[g].pay;
    return 0;
}

int tapeSum() {
    int s = 0;
    for (int i = 0; i < kTapeN; i++) s += tapePay(i);
    return s;
}

const char* Game::tapeLabel(int i) const { return tapeName(i); }
int Game::tapeScore(int i) const { return tapePay(i); }

int Game::drawerCount() const {
    int n = 0;
    for (int i = 0; i < 3; i++)
        if (drawer_[i] >= 0) n++;
    return n;
}

bool Game::inDrawer(int g) const {
    for (int i = 0; i < 3; i++)
        if (drawer_[i] == g) return true;
    return false;
}

bool Game::heldLine(int line) const {
    for (int i = 0; i < 3; i++) {
        int g = drawer_[i];
        if (g >= 0 && kGoods[g].line == line) return true;
    }
    return false;
}

bool Game::matched() const {
    bool seen[kTapeN] = {};
    int n = 0;
    for (int i = 0; i < 3; i++) {
        int g = drawer_[i];
        if (g < 0) return false;
        int line = kGoods[g].line;
        if (line < 0 || line >= kTapeN || seen[line]) return false;
        seen[line] = true;
        n++;
    }
    return n == kTapeN;
}

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < 3; i++)
        if (drawer_[i] >= 0) s += kGoods[drawer_[i]].pay;
    return s;
}

bool Game::audit() {
    if (std::strcmp(tapeName(0), "APPLE") != 0 || std::strcmp(tapeName(1), "LOAF") != 0 ||
        std::strcmp(tapeName(2), "PEAR") != 0)
        return false;
    if (tapePay(0) != 12 || tapePay(1) != 9 || tapePay(2) != 7 || tapeSum() != 28) return false;
    int decoy = 0;
    int sameName = 0;
    for (int g = 0; g < kGoodN; g++) {
        if (kGoods[g].line < 0) {
            decoy += kGoods[g].pay;
            for (int i = 0; i < kTapeN; i++)
                if (std::strcmp(kGoods[g].name, tapeName(i)) == 0) sameName++;
        }
    }
    if (decoy != tapeSum() || sameName != 0) return false;
    std::snprintf(reason_, sizeof reason_, "rules hold");
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) {
        std::snprintf(reason_, sizeof reason_, "rules failed");
        std::fprintf(stderr, "s3markettape rules failed\n");
    }
    sys.apu.setMaster(0.7f);
    mode_ = Mode::Title;
    age_ = 0;
}

void Game::begin() {
    for (int i = 0; i < 3; i++) drawer_[i] = -1;
    cursor_ = 0;
    faults_ = 0;
    won_ = false;
    over_ = false;
    left_ = false;
    shake_ = 0;
    mode_ = Mode::Play;
    std::snprintf(reason_, sizeof reason_, "open");
}

void Game::blip(float freq) {
    beepF_ = freq;
    beepN_ = 5;
}

void Game::toggle() {
    if (mode_ != Mode::Play) return;
    if (inDrawer(cursor_)) {
        for (int i = 0; i < 3; i++)
            if (drawer_[i] == cursor_) drawer_[i] = -1;
        blip(320.f);
        return;
    }
    if (drawerCount() >= 3) return;
    for (int i = 0; i < 3; i++) {
        if (drawer_[i] < 0) {
            drawer_[i] = cursor_;
            blip(520.f + float(kGoods[cursor_].pay) * 8.f);
            return;
        }
    }
}

void Game::tryLeave() {
    if (mode_ != Mode::Play) return;
    if (matched()) {
        left_ = true;
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        std::snprintf(reason_, sizeof reason_, "the drawer matches the tape");
        blip(880.f);
        sys_->apu.tone(1, 1320.f, 0.05f);
        return;
    }
    faults_++;
    shake_ = 16;
    blip(110.f);
    sys_->apu.noiseBurst(0.1f, 400.f, 0.2f);
    if (drawerScore() == tapeSum())
        std::snprintf(reason_, sizeof reason_, "same till, not the tape");
    else
        std::snprintf(reason_, sizeof reason_, "the drawer does not match");
    if (faults_ >= kTries) {
        mode_ = Mode::Lose;
        over_ = true;
        won_ = false;
        left_ = false;
    }
}

void Game::readInput() {
    gs::Pad& pad = sys_->pad;
    if (pad.pressed(gs::BTN_START) || (mode_ == Mode::Title && (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)))) {
        if (mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Lose) begin();
        return;
    }
    if (mode_ != Mode::Play) return;
    if (pad.pressed(gs::BTN_LEFT)) {
        cursor_ = (cursor_ + kGoodN - 1) % kGoodN;
        blip(400.f);
    }
    if (pad.pressed(gs::BTN_RIGHT)) {
        cursor_ = (cursor_ + 1) % kGoodN;
        blip(440.f);
    }
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B)) toggle();
    if (pad.pressed(gs::BTN_C)) tryLeave();
}

void Game::driveBot() {
    for (int i = 0; i < gs::BTN_COUNT; i++) sys_->pad.keys[i] = false;
    if (cool_ > 0) {
        cool_--;
        return;
    }
    if (mode_ == Mode::Title) {
        if (age_ > 24) {
            sys_->pad.keys[gs::BTN_START] = true;
            cool_ = 2;
        }
        return;
    }
    if (mode_ != Mode::Play) return;
    int want = -1;
    for (int g = 0; g < kGoodN; g++) {
        if (kGoods[g].line >= 0 && !inDrawer(g)) {
            want = g;
            break;
        }
    }
    if (want < 0) {
        sys_->pad.keys[gs::BTN_C] = true;
        cool_ = 2;
        return;
    }
    if (cursor_ < want) sys_->pad.keys[gs::BTN_RIGHT] = true;
    else if (cursor_ > want) sys_->pad.keys[gs::BTN_LEFT] = true;
    else sys_->pad.keys[gs::BTN_A] = true;
    cool_ = 1;
}

void Game::logic() {
    age_++;
    if (shake_ > 0) shake_--;
    if (beepN_ > 0 && --beepN_ == 0) sys_->apu.tone(0, 0, 0);
}

void Game::audio() {
    if (beepN_ > 0) sys_->apu.tone(0, beepF_, 0.06f);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || row < 0 || row > 27 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 2.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    int ih = int(h + 0.5f);
    int iw = int(w + 0.5f);
    if (ih < 1) ih = 1;
    if (iw < 1) iw = 1;
    if (ih > 400) ih = 400;
    if (iw > 400) iw = 400;
    s.h = int16_t(ih);
    s.w = int16_t(iw);
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

    float jx = (shake_ & 1) ? 2.f : 0.f;
    spr(art_.awning, 168, 36, 36, PAL_STALL);
    spr(art_.clerk, 28, 92, 56, PAL_CLERK);

    for (int g = 0; g < kGoodN; g++) {
        float x = 58.f + float(g) * 42.f;
        spr(art_.crate, x, 118, 26, PAL_WOOD);
        if (!inDrawer(g)) spr(*picOf(art_, g), x, 108, g == 4 ? 22.f : 18.f, palOf(g));
        if (g == cursor_ && mode_ == Mode::Play) spr(art_.arrow, x, 86, 12, PAL_INK);
    }
    spr(art_.drawer, 176 + jx, 176, 40, PAL_WOOD);

    int slot = 0;
    for (int i = 0; i < 3; i++) {
        int g = drawer_[i];
        if (g < 0) continue;
        float x = 132.f + float(slot) * 36.f + jx;
        spr(*picOf(art_, g), x, 172, 16, palOf(g));
        slot++;
    }

    hud(1, 1, "S3 MARKETTAPE", PAL_HUD);
    char tape[48];
    std::snprintf(tape, sizeof tape, "TAPE  %s %d   %s %d   %s %d", tapeName(0), tapePay(0), tapeName(1), tapePay(1),
                  tapeName(2), tapePay(2));
    hud(1, 3, tape, PAL_INK);

    for (int g = 0; g < kGoodN; g++) {
        int col = 5 + g * 5;
        hud(col, 16, kGoods[g].name, inDrawer(g) ? PAL_OK : PAL_HUD);
    }

    char till[40];
    std::snprintf(till, sizeof till, "DRAWER %d", drawerScore());
    hud(1, 22, till, matched() ? PAL_OK : PAL_HUD);
    if (mode_ == Mode::Title) hud(1, 24, "START  STOCK THE STALL", PAL_HUD);
    else if (mode_ == Mode::Play) {
        hud(1, 24, reason_, drawerScore() == tapeSum() && !matched() ? PAL_BAD : PAL_HUD);
        hud(1, 26, "A TAKE   C LEAVE", PAL_HUD);
    } else if (mode_ == Mode::Win) hud(1, 24, "DRAWER MATCHES THE TAPE", PAL_OK);
    else hud(1, 24, "STILL OPEN", PAL_BAD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (bot_) driveBot();
    readInput();
    logic();
    audio();
    draw();
}

}  // namespace markettape
