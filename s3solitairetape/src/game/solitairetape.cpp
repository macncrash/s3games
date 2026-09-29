#include "game/solitairetape.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace solitairetape {

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
        if (id >= 0 && id < kCards) s += kFace[id].pay;
    }
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kFace[kTape[i]].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kFace[kTape[i]].pay;
}

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Play: return "play";
    case Mode::Leave: return "leave";
    case Mode::Lose: return "lose";
    }
    return "?";
}

int Game::nextWant() const { return filled_ < kTapeN ? kTape[filled_] : -1; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.4f);
    rules_ = true;
    bool seen[kCards] = {};
    for (int i = 0; i < kTapeN; i++) {
        int id = kTape[i];
        if (id < 0 || id >= kCards || kFace[id].decoy || seen[id]) rules_ = false;
        seen[id] = true;
        if (i > 0 && std::strcmp(kFace[id].name, kFace[kTape[i - 1]].name) == 0) rules_ = false;
    }
    bool decoySame = false;
    for (int i = 0; i < kCards; i++) {
        if (!kFace[i].decoy) continue;
        if (kFace[i].pay == kFace[kTape[1]].pay) decoySame = true;
        for (int t = 0; t < kTapeN; t++)
            if (kTape[t] == i) rules_ = false;
    }
    if (!decoySame) rules_ = false;
    if (kFace[kTape[0]].pay + kFace[kTape[1]].pay + kFace[kTape[2]].pay != 15) rules_ = false;
    mode_ = Mode::Title;
    why_ = "OPEN";
}

void Game::begin() {
    filled_ = 0;
    faults_ = 0;
    cursor_ = 0;
    left_ = false;
    over_ = false;
    won_ = false;
    for (int i = 0; i < kCards; i++) live_[i] = true;
    for (int i = 0; i < kTapeN; i++) {
        held_[i] = false;
        drawer_[i] = -1;
    }
    why_ = "DEAL";
    mode_ = Mode::Play;
    act_ = bot_ ? 8 : 0;
    if (sys_) sys_->apu.tone(0, 392.f, 0.06f);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Play) return;
    faults_++;
    why_ = why;
    won_ = false;
    left_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    if (sys_) sys_->apu.tone(0, 120.f, 0.16f);
}

void Game::fileCard() {
    if (mode_ != Mode::Play || cursor_ < 0 || cursor_ >= kCards || !live_[cursor_]) return;
    int id = cursor_;
    int want = nextWant();
    if (kFace[id].decoy || id != want) {
        fail("NOT ON THE TAPE");
        return;
    }
    drawer_[filled_] = id;
    held_[filled_] = true;
    filled_++;
    live_[id] = false;
    if (sys_) sys_->apu.tone(0, 620.f + float(filled_) * 40.f, 0.08f);
    if (filled_ == kTapeN && matched()) {
        why_ = "DRAWER FULL";
        mode_ = Mode::Leave;
        act_ = bot_ ? 18 : 0;
        return;
    }
    act_ = bot_ ? 6 : 0;
}

void Game::passCard() {
    if (mode_ != Mode::Play || cursor_ < 0 || cursor_ >= kCards || !live_[cursor_]) return;
    if (cursor_ == nextWant()) {
        fail("THE CARD DROPPED");
        return;
    }
    live_[cursor_] = false;
    if (sys_) sys_->apu.tone(1, 220.f, 0.05f);
    act_ = bot_ ? 6 : 0;
}

void Game::depart() {
    if (mode_ != Mode::Leave) return;
    left_ = true;
    won_ = matched() && faults_ == 0 && filled_ == kTapeN;
    over_ = true;
    why_ = won_ ? "THE DRAWER MATCHES" : "THE DRAWER MISSED";
    if (sys_) sys_->apu.tone(0, won_ ? 784.f : 140.f, 0.14f);
    if (won_ && sys_ && !sys_->headless) {
        sys_->rumble(0.3f, 0.55f, 120);
        sys_->setLight(220, 190, 70);
        sys_->quit();
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    const gs::Pad& pad = sys.pad;

    if (bot_) {
        if (mode_ == Mode::Title) {
            if (t_ > 16) begin();
        } else if (mode_ == Mode::Leave) {
            if (act_ > 0) act_--;
            else depart();
        } else if (mode_ == Mode::Play) {
            if (act_ > 0) act_--;
            else {
                int want = nextWant();
                if (want < 0) fail("THE TAPE RAN OUT");
                else if (!live_[cursor_]) cursor_ = (cursor_ + 1) % kCards;
                else if (cursor_ == want) fileCard();
                else passCard();
            }
        } else if (mode_ == Mode::Lose && t_ > 8) {
            over_ = true;
        }
    } else if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Play) {
        if (pad.pressed(gs::BTN_LEFT)) cursor_ = (cursor_ + kCards - 1) % kCards;
        if (pad.pressed(gs::BTN_RIGHT)) cursor_ = (cursor_ + 1) % kCards;
        if (pad.pressed(gs::BTN_A)) fileCard();
        if (pad.pressed(gs::BTN_B)) passCard();
    } else if (mode_ == Mode::Leave) {
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START)) depart();
    } else if (mode_ == Mode::Lose) {
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            why_ = "OPEN";
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
    for (int y = 0; y < gs::SCREEN_H; y++) v.lineBackdrop[y] = gs::rgb4(0, 3, 2);

    blit(art_.felt, 160.f, 120.f, 312.f, 200.f, PAL_FELT);
    blit(art_.reel, 286.f, 28.f, 36.f, 36.f, PAL_REEL);

    lineC(1, "S3 SOLITAIRETAPE", PAL_TITLE);
    lineAt(1, 3, "TAPE", PAL_HINT);
    char tape[40];
    std::snprintf(tape, sizeof tape, "%s  %s  %s", tapeLabel(0), tapeLabel(1), tapeLabel(2));
    lineAt(7, 3, tape, PAL_INK);

    for (int i = 0; i < kTapeN; i++) {
        float x = 70.f + i * 70.f;
        blit(art_.slot, x, 58.f, 48.f, 22.f, PAL_SLOT);
        if (held_[i]) {
            blit(art_.card, x, 56.f, 28.f, 18.f, PAL_CARD);
            lineAt(6 + i * 9, 6, kFace[drawer_[i]].name, PAL_WIN);
        } else {
            lineAt(7 + i * 9, 6, "--", PAL_HINT);
        }
    }

    for (int i = 0; i < kCards; i++) {
        if (!live_[i]) continue;
        float x = 36.f + i * 48.f;
        float y = 148.f;
        blit(art_.card, x, y, 36.f, 52.f, PAL_CARD);
        int pips = kFace[i].pay > 4 ? 4 : kFace[i].pay;
        for (int p = 0; p < pips; p++) blit(art_.pip, x - 8.f + (p % 2) * 14.f, y - 6.f + (p / 2) * 12.f, 8.f, 8.f, PAL_PIP);
        if (i == cursor_ && mode_ == Mode::Play) blit(art_.pip, x, y + 32.f, 8.f, 8.f, PAL_PIP);
    }

    if (mode_ == Mode::Title) {
        lineC(20, "FILE THE TAPE INTO THE DRAWER", PAL_INK);
        lineC(22, "A FILES   B PASSES   JACK STAYS OUT", PAL_HINT);
        lineC(24, "PRESS START", PAL_TITLE);
    } else if (mode_ == Mode::Play) {
        lineC(24, "A FILE   B PASS", PAL_HINT);
        lineAt(1, 26, kFace[cursor_].name, live_[cursor_] ? PAL_INK : PAL_BAD);
    } else if (mode_ == Mode::Leave) {
        lineC(22, "THE DRAWER HAS TO MATCH THE TAPE", PAL_WIN);
        lineC(24, "A LEAVE", PAL_TITLE);
    } else if (mode_ == Mode::Lose) {
        lineC(22, why_, PAL_BAD);
        lineC(24, "THE DRAWER MISSED", PAL_BAD);
    }
    if (left_ && won_) lineC(26, "YOU LEAVE", PAL_WIN);
}

}  // namespace solitairetape
