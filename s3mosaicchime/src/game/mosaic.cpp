#include "game/mosaic.h"

#include <cstdio>
#include <cstring>

namespace mosaicchime {
namespace {

constexpr int CELL = 12;
constexpr int PITCH = 14;
constexpr int OX = 28;
constexpr int OY = 48;

const char* kInk[] = {"PLASTER", "BRONZE", "BONE", "SOOT", "GOLD"};

int countSet(const int* board, const int* mark) {
    int n = 0;
    for (int i = 0; i < CELLS; i++)
        if (board[i] == mark[i] && mark[i] != 0) n++;
    return n;
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
    why_ = "";
    set_ = 0;
    early_ = 0;
    playFrames_ = 0;
    for (int i = 0; i < CELLS; i++) board_[i] = 0;
}

void Game::begin() {
    for (int i = 0; i < CELLS; i++) board_[i] = 0;
    cx_ = 0;
    cy_ = 0;
    ink_ = 1;
    set_ = 0;
    early_ = 0;
    playFrames_ = 0;
    t_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    strikes_ = 0;
    strikeWait_ = 0;
    clockOn_ = true;
    phase_ = Phase::Lay;
}

bool Game::filled() const {
    for (int i = 0; i < CELLS; i++)
        if (board_[i] != art_.mark[i]) return false;
    return true;
}

bool Game::confirm() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
}

const char* Game::inkName() const { return kInk[ink_]; }

void Game::note(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = 5;
}

void Game::wipeEarly() {
    early_++;
    why_ = "EARLY";
    note(110.f, 0.16f);
    for (int i = 0; i < CELLS; i++) board_[i] = 0;
    set_ = 0;
    if (early_ >= 3) {
        fail("EARLY");
        return;
    }
    phase_ = Phase::Early;
    t_ = 0;
}

void Game::beginChime() {
    won_ = true;
    why_ = "CHIME";
    clockOn_ = false;
    set_ = countSet(board_, art_.mark);
    phase_ = Phase::Chime;
    t_ = 0;
    strikes_ = 0;
    strikeWait_ = 0;
    note(523.f, 0.22f);
}

void Game::fail(const char* why) {
    why_ = why;
    won_ = false;
    clockOn_ = false;
    phase_ = Phase::Fail;
    t_ = 0;
    note(90.f, 0.14f);
}

void Game::stampHere() {
    int i = cy_ * COLS + cx_;
    int want = art_.mark[i];
    if (want == 0) return;
    if (board_[i] == ink_) return;
    if (ink_ != want) {
        note(140.f, 0.12f);
        return;
    }
    board_[i] = ink_;
    set_ = countSet(board_, art_.mark);
    note(220.f + float(ink_) * 40.f, 0.16f);
    if (!filled()) return;
    if (onHour()) beginChime();
    else if (!pastHour()) wipeEarly();
    else fail("LATE");
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
        if (bot_ ? t_ > 10 : confirm()) begin();
        draw();
        return;
    }
    if (phase_ == Phase::Early) {
        if (t_ > (bot_ ? 4 : 40)) {
            phase_ = Phase::Lay;
            t_ = 0;
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
            strikeWait_ = bot_ ? 2 : 7;
        } else if (t_ > (bot_ ? 28 : 100)) {
            phase_ = Phase::Leave;
            t_ = 0;
        }
        draw();
        return;
    }
    if (phase_ == Phase::Leave) {
        if (t_ > (bot_ ? 6 : 36)) {
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
    if (pastHour() && !filled()) {
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
        int goal = -1;
        int open = 0;
        for (int i = 0; i < CELLS; i++) {
            if (board_[i] != art_.mark[i] && art_.mark[i] != 0) {
                if (goal < 0) goal = i;
                open++;
            }
        }
        if (goal >= 0) {
            bool holdLast = open == 1 && !onHour();
            if (!holdLast) {
                int gx = goal % COLS, gy = goal / COLS;
                if (cx_ < gx) cx_++;
                else if (cx_ > gx) cx_--;
                else if (cy_ < gy) cy_++;
                else if (cy_ > gy) cy_--;
                else if (ink_ != art_.mark[goal]) ink_ = art_.mark[goal];
                else stampHere();
            }
        }
        draw();
        return;
    }

    if (p.pressed(gs::BTN_LEFT) && cx_ > 0) cx_--;
    if (p.pressed(gs::BTN_RIGHT) && cx_ < COLS - 1) cx_++;
    if (p.pressed(gs::BTN_UP) && cy_ > 0) cy_--;
    if (p.pressed(gs::BTN_DOWN) && cy_ < ROWS - 1) cy_++;
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_X)) {
        ink_++;
        if (ink_ > INKS) ink_ = 1;
    }
    if (p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z)) {
        ink_--;
        if (ink_ < 1) ink_ = INKS;
    }
    if (p.pressed(gs::BTN_B)) {
        int i = cy_ * COLS + cx_;
        if (board_[i] != 0) {
            board_[i] = 0;
            set_ = countSet(board_, art_.mark);
            note(160.f, 0.1f);
        }
    }
    if (p.pressed(gs::BTN_A)) stampHere();
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
    s.pal = PAL_TILE;
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int dusk = y / 32;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(1 + dusk / 4, 1, 4 + dusk / 3);
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
        hudC(3, "S3 MOSAIC CHIME", PAL_GOLD);
        hudC(5, "A SHORT MOSAIC", PAL_CREAM);
        spr(art_.picture, 124, 62, COLS * 12, ROWS * 12);
        spr(art_.tower, 250, 58, 22, 40);
        hudC(20, "LAY THE CLOCK", PAL_DIM);
        hudC(21, "LEAVE WHEN THE HOUR CHIMES", PAL_DIM);
        hudC(24, bot_ ? "LAYING" : "START", PAL_LEAF);
        return;
    }

    spr(art_.tower, 276, 36, 22, 40);
    if (phase_ == Phase::Lay || phase_ == Phase::Early) spr(art_.cursor, OX + cx_ * PITCH - 1, OY + cy_ * PITCH - 1, 14, 14);
    for (int i = 0; i < CELLS; i++) {
        int x = OX + (i % COLS) * PITCH;
        int y = OY + (i / COLS) * PITCH;
        int ink = board_[i];
        if (ink < 0 || ink > INKS) ink = 0;
        spr(art_.cell[ink], x, y, CELL, CELL);
    }
    spr(art_.picture, 196, 78, COLS * 10, ROWS * 10);

    hud(1, 1, "MOSAIC CHIME", PAL_GOLD);
    char line[48];
    int h, m, s;
    split(h, m, s);
    std::snprintf(line, sizeof(line), "%d:%02d:%02d", h, m, s);
    hud(28, 1, line, onHour() ? PAL_LEAF : PAL_CREAM);
    std::snprintf(line, sizeof(line), "TILE %d", set_);
    hud(28, 2, line, PAL_DIM);

    if (paused_) {
        hudC(16, "PAUSED", PAL_GOLD);
    } else if (phase_ == Phase::Chime || phase_ == Phase::Leave || (phase_ == Phase::Over && won_)) {
        hudC(24, "THE HOUR CHIMES", PAL_GOLD);
        hudC(26, "LEAVE", PAL_LEAF);
    } else if (phase_ == Phase::Fail || (phase_ == Phase::Over && !won_)) {
        hudC(24, std::strcmp(why_, "EARLY") == 0 ? "TOO EARLY" : "THE HOUR IS GONE", PAL_GOLD);
        hudC(26, "NO CHIME", PAL_DIM);
    } else if (phase_ == Phase::Early) {
        hudC(24, "NOT YET THE HOUR", PAL_GOLD);
        hudC(26, "THE BOARD WIPES", PAL_DIM);
    } else {
        std::snprintf(line, sizeof(line), "INK %s", inkName());
        hud(1, 24, line, PAL_CREAM);
        if (onHour()) hudC(22, "THE HOUR HAS TO CHIME", PAL_LEAF);
        else hudC(22, "WAIT FOR TWELVE", PAL_DIM);
        hudC(26, "ARROWS  A STAMP  C INK  B LIFT", PAL_DIM);
    }
}

}  // namespace mosaicchime
