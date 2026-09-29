#include "game/memorychime.h"

#include <cstdio>
#include <cstring>

namespace memchime {
namespace {

constexpr int OX = 28;
constexpr int OY = 62;
constexpr int PW = 40;
constexpr int PH = 56;
constexpr int CW = 32;
constexpr int CH = 44;

// Bell, star, moon, key, then the mates. Bell sits at 0 and 6.
constexpr int kDeal[CARDS] = {0, 1, 2, 3, 1, 2, 0, 3};

int mateOf(const int* faces, int i) {
    for (int j = 0; j < CARDS; j++)
        if (j != i && faces[j] == faces[i]) return j;
    return i;
}

}  // namespace

int Game::secAt(int frames) const {
    if (frames < 0) frames = 0;
    return kStartSec + frames / kFpc;
}

int Game::clockSec() const { return secAt(playFrames_); }

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::split(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    h = t / 3600;
    m = (t / 60) % 60;
    s = t % 60;
}

int Game::hour() const {
    int h, m, s;
    split(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    split(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    split(h, m, s);
    return s;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.45f);
    phase_ = Phase::Title;
    t_ = 0;
    won_ = false;
    over_ = false;
    clockOn_ = false;
    paused_ = false;
    reason_ = "";
    pairs_ = 0;
    early_ = 0;
    for (int i = 0; i < CARDS; i++) {
        cards_[i].face = kDeal[i];
        cards_[i].up = false;
        cards_[i].held = false;
    }
}

void Game::begin() {
    for (int i = 0; i < CARDS; i++) {
        cards_[i].face = kDeal[i];
        cards_[i].up = true;
        cards_[i].held = false;
    }
    cursor_ = 0;
    nOpen_ = 0;
    pairs_ = 0;
    early_ = 0;
    playFrames_ = 0;
    t_ = 0;
    won_ = false;
    over_ = false;
    reason_ = "";
    strikes_ = 0;
    strikeWait_ = 0;
    clockOn_ = false;
    phase_ = Phase::Study;
}

bool Game::confirm() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
}

void Game::note(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = 5;
}

void Game::beginChime() {
    won_ = true;
    reason_ = "CHIME";
    clockOn_ = false;
    for (int k = 0; k < nOpen_; k++) cards_[open_[k]].held = true;
    phase_ = Phase::Chime;
    t_ = 0;
    strikes_ = 0;
    strikeWait_ = 0;
    note(523.f, 0.22f);
}

void Game::fail(const char* why) {
    reason_ = why;
    won_ = false;
    clockOn_ = false;
    phase_ = Phase::Fail;
    t_ = 0;
    note(90.f, 0.14f);
}

void Game::flipAt(int i) {
    if (i < 0 || i >= CARDS) return;
    if (cards_[i].held || cards_[i].up) return;
    if (nOpen_ >= 2) return;
    cards_[i].up = true;
    open_[nOpen_++] = i;
    note(180.f + float(cards_[i].face) * 50.f, 0.14f);
    if (nOpen_ < 2) return;
    int a = open_[0], b = open_[1];
    bool match = cards_[a].face == cards_[b].face;
    hourVerdict_ = false;
    lateVerdict_ = false;
    if (match && cards_[a].face == 0) {
        hourVerdict_ = onHour();
        lateVerdict_ = pastHour();
    }
    phase_ = Phase::Show;
    t_ = 0;
}

void Game::settle() {
    int a = open_[0], b = open_[1];
    bool match = cards_[a].face == cards_[b].face;
    nOpen_ = 0;
    if (!match) {
        cards_[a].up = false;
        cards_[b].up = false;
        note(110.f, 0.1f);
        phase_ = Phase::Play;
        return;
    }
    if (cards_[a].face == 0) {
        if (hourVerdict_) {
            beginChime();
            return;
        }
        cards_[a].up = false;
        cards_[b].up = false;
        if (lateVerdict_ || pastHour()) {
            fail("LATE");
            return;
        }
        early_++;
        reason_ = "EARLY";
        note(98.f, 0.16f);
        if (early_ >= 3) {
            fail("EARLY");
            return;
        }
        phase_ = Phase::Early;
        t_ = 0;
        return;
    }
    cards_[a].held = true;
    cards_[b].held = true;
    pairs_++;
    note(330.f, 0.16f);
    phase_ = Phase::Play;
}

void Game::botPlay() {
    int faces[CARDS];
    for (int i = 0; i < CARDS; i++) faces[i] = cards_[i].face;

    int target = -1;
    if (nOpen_ == 1) {
        int a = open_[0];
        if (cards_[a].face == 0 && !onHour()) return;
        target = mateOf(faces, a);
    } else {
        bool ordinary = false;
        for (int i = 0; i < CARDS; i++)
            if (!cards_[i].held && cards_[i].face != 0) ordinary = true;
        if (ordinary) {
            for (int i = 0; i < CARDS; i++) {
                if (!cards_[i].held && !cards_[i].up && cards_[i].face != 0) {
                    target = i;
                    break;
                }
            }
        } else if (onHour()) {
            for (int i = 0; i < CARDS; i++) {
                if (!cards_[i].held && !cards_[i].up && cards_[i].face == 0) {
                    target = i;
                    break;
                }
            }
        } else {
            return;
        }
    }
    if (target < 0) return;
    int tx = target % COLS, ty = target / COLS;
    int cx = cursor_ % COLS, cy = cursor_ / COLS;
    if (cx < tx) cursor_++;
    else if (cx > tx) cursor_--;
    else if (cy < ty) cursor_ += COLS;
    else if (cy > ty) cursor_ -= COLS;
    else flipAt(cursor_);
}

bool Game::faceUp(int i) const {
    if (phase_ == Phase::Title || phase_ == Phase::Study) return true;
    return cards_[i].up || cards_[i].held;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (paused_) {
        if (sys.pad.pressed(gs::BTN_START)) paused_ = false;
        draw();
        return;
    }

    const gs::Pad& p = sys.pad;
    t_++;

    if (phase_ == Phase::Title) {
        if (bot_ ? t_ > 12 : confirm()) begin();
        draw();
        return;
    }
    if (phase_ == Phase::Study) {
        if (t_ > (bot_ ? 8 : 90)) {
            for (int i = 0; i < CARDS; i++) cards_[i].up = false;
            phase_ = Phase::Cover;
            t_ = 0;
        }
        draw();
        return;
    }
    if (phase_ == Phase::Cover) {
        if (t_ > (bot_ ? 6 : 24)) {
            phase_ = Phase::Play;
            t_ = 0;
            clockOn_ = true;
        }
        draw();
        return;
    }
    if (phase_ == Phase::Show) {
        if (clockOn_) playFrames_++;
        if (t_ > (bot_ ? 4 : 28)) settle();
        draw();
        return;
    }
    if (phase_ == Phase::Early) {
        if (clockOn_) playFrames_++;
        if (t_ > (bot_ ? 6 : 36)) {
            phase_ = Phase::Play;
            t_ = 0;
            reason_ = "";
        }
        draw();
        return;
    }
    if (phase_ == Phase::Chime) {
        if (strikeWait_ > 0) strikeWait_--;
        else if (strikes_ < 12) {
            float f = (strikes_ % 2) ? 659.f : 523.f;
            note(f, 0.2f);
            strikes_++;
            strikeWait_ = bot_ ? 2 : 8;
        } else if (t_ > (bot_ ? 30 : 120)) {
            phase_ = Phase::Leave;
            t_ = 0;
        }
        draw();
        return;
    }
    if (phase_ == Phase::Leave) {
        if (t_ > (bot_ ? 8 : 40)) {
            phase_ = Phase::Over;
            over_ = true;
        }
        draw();
        return;
    }
    if (phase_ == Phase::Fail) {
        if (t_ > (bot_ ? 8 : 70)) {
            phase_ = Phase::Over;
            over_ = true;
        }
        draw();
        return;
    }
    if (phase_ == Phase::Over) {
        over_ = true;
        draw();
        return;
    }

    if (clockOn_) playFrames_++;
    if (pastHour()) {
        fail("LATE");
        draw();
        return;
    }

    if (!bot_ && p.pressed(gs::BTN_START)) {
        paused_ = true;
        draw();
        return;
    }

    if (bot_) {
        botPlay();
        draw();
        return;
    }

    int cx = cursor_ % COLS, cy = cursor_ / COLS;
    if (p.pressed(gs::BTN_LEFT) && cx > 0) cursor_--;
    if (p.pressed(gs::BTN_RIGHT) && cx < COLS - 1) cursor_++;
    if (p.pressed(gs::BTN_UP) && cy > 0) cursor_ -= COLS;
    if (p.pressed(gs::BTN_DOWN) && cy < ROWS - 1) cursor_ += COLS;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) flipAt(cursor_);
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

void Game::spr(const gs::Image& img, int x, int y, int w, int h) {
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.pal = PAL_CARD;
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int band = y < 48 ? 0 : (y - 48) / 28;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(1, 2 + band / 3, 3 + band / 4);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    vdp.hudEnabled = true;
    backdrop();

    if (phase_ == Phase::Title) {
        hudC(2, "S3 MEMORY CHIME", PAL_GOLD);
        hudC(4, "A SHORT MEMORY", PAL_CREAM);
        for (int i = 0; i < CARDS; i++) {
            int x = OX + (i % COLS) * PW;
            int y = 70 + (i / COLS) * PH;
            spr(art_.face[cards_[i].face], x, y, CW, CH);
        }
        spr(art_.tower, 250, 78, 28, 48);
        hudC(20, "TURN THE TABLE", PAL_DIM);
        hudC(21, "THE BELL WAITS FOR TWELVE", PAL_DIM);
        hudC(24, bot_ ? "DEALING" : "START", PAL_LEAF);
        return;
    }

    spr(art_.tower, 268, 28, 28, 48);
    if (phase_ == Phase::Play || phase_ == Phase::Show || phase_ == Phase::Early)
        spr(art_.cursor, OX + (cursor_ % COLS) * PW - 2, OY + (cursor_ / COLS) * PH - 2, 36, 48);
    for (int i = 0; i < CARDS; i++) {
        int x = OX + (i % COLS) * PW;
        int y = OY + (i / COLS) * PH;
        spr(faceUp(i) ? art_.face[cards_[i].face] : art_.back, x, y, CW, CH);
    }

    hud(1, 1, "MEMORY CHIME", PAL_GOLD);
    char line[48];
    int h, m, s;
    split(h, m, s);
    std::snprintf(line, sizeof(line), "%d:%02d:%02d", h, m, s);
    hud(28, 1, line, onHour() ? PAL_LEAF : PAL_CREAM);
    std::snprintf(line, sizeof(line), "PAIR %d", pairs_);
    hud(28, 2, line, PAL_DIM);

    if (paused_) {
        hudC(16, "PAUSED", PAL_GOLD);
    } else if (phase_ == Phase::Chime || phase_ == Phase::Leave || (phase_ == Phase::Over && won_)) {
        hudC(24, "THE HOUR CHIMES", PAL_GOLD);
        hudC(26, "LEAVE", PAL_LEAF);
    } else if (phase_ == Phase::Fail || (phase_ == Phase::Over && !won_)) {
        hudC(24, std::strcmp(reason_, "EARLY") == 0 ? "TOO EARLY" : "THE HOUR IS GONE", PAL_GOLD);
        hudC(26, "NO CHIME", PAL_DIM);
    } else if (phase_ == Phase::Early) {
        hudC(24, "THE BELL IS EARLY", PAL_GOLD);
        hudC(26, "IT FALLS SHUT", PAL_DIM);
    } else if (phase_ == Phase::Study) {
        hudC(24, "STUDY THE TABLE", PAL_CREAM);
    } else if (phase_ == Phase::Cover) {
        hudC(24, "CARDS DOWN", PAL_DIM);
    } else {
        if (onHour()) hudC(22, "THE HOUR HAS TO CHIME", PAL_LEAF);
        else hudC(22, "WAIT FOR TWELVE", PAL_DIM);
        hudC(26, "ARROWS MOVE   A TURNS A CARD", PAL_DIM);
    }
}

}  // namespace memchime
