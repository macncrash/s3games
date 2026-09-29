#include "game/memory.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace memoryseven {

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    you_ = 0;
    them_ = 0;
    won_ = false;
    over_ = false;
    t_ = 0;
}

void Game::deal() {
    int bag[N];
    for (int i = 0; i < N; i++) bag[i] = i / 2;
    for (int i = N - 1; i > 0; i--) {
        rng_ = rng_ * 1664525u + 1013904223u;
        int j = int(rng_ % uint32_t(i + 1));
        int tmp = bag[i];
        bag[i] = bag[j];
        bag[j] = tmp;
    }
    for (int i = 0; i < N; i++) {
        kind_[i] = bag[i];
        matched_[i] = false;
        known_[i] = false;
        opp_[i] = false;
    }
    upA_ = upB_ = -1;
    phase_ = Phase::Choose;
    cursor_ = 0;
    yours_ = true;
    act_ = 8;
}

void Game::newMatch() {
    you_ = 0;
    them_ = 0;
    won_ = false;
    over_ = false;
    deal();
    mode_ = Mode::Play;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    beep_ = 4;
}

void Game::cardPos(int i, float& cx, float& cy) const {
    int col = i % COLS;
    int row = i / COLS;
    cx = art_.x0 + col * (art_.cw + art_.gx) + art_.cw * 0.5f;
    cy = art_.y0 + row * (art_.ch + art_.gy) + art_.ch * 0.5f;
}

int Game::mateOf(int i, bool opp) const {
    if (i < 0) return -1;
    for (int j = 0; j < N; j++) {
        if (j == i || matched_[j] || j == upA_) continue;
        bool know = opp ? opp_[j] : known_[j];
        if (know && kind_[j] == kind_[i]) return j;
    }
    return -1;
}

int Game::faceDown(int notKind, int skip) const {
    int fallback = -1;
    for (int j = 0; j < N; j++) {
        if (matched_[j] || j == skip || j == upA_) continue;
        if (fallback < 0) fallback = j;
        if (notKind < 0 || kind_[j] != notKind) return j;
    }
    return fallback;
}

void Game::flip(int i) {
    if (i < 0 || i >= N || matched_[i] || i == upA_ || i == upB_) return;
    known_[i] = true;
    if (!yours_) opp_[i] = true;
    if (upA_ < 0) {
        upA_ = i;
        blip(420);
        return;
    }
    upB_ = i;
    phase_ = Phase::Peek;
    peek_ = 16;
    blip(kind_[i] == kind_[upA_] ? 760.0f : 180.0f);
}

void Game::resolve() {
    if (upA_ < 0 || upB_ < 0) {
        phase_ = Phase::Choose;
        return;
    }
    if (kind_[upA_] == kind_[upB_]) {
        matched_[upA_] = matched_[upB_] = true;
        if (yours_) you_++;
        else them_++;
        sys_->apu.tone(1, yours_ ? 660.0f : 220.0f, 0.06f);
        beep_ = 6;
        if (you_ >= 7 && you_ > them_) {
            mode_ = Mode::Win;
            won_ = true;
            over_ = true;
        } else if (them_ >= 7 && them_ > you_) {
            mode_ = Mode::Lose;
            won_ = false;
            over_ = true;
        }
    } else {
        yours_ = !yours_;
    }
    upA_ = upB_ = -1;
    phase_ = Phase::Choose;
    act_ = 6;
}

void Game::youPick() {
    if (upA_ < 0) {
        for (int i = 0; i < N; i++) {
            if (!known_[i] || matched_[i]) continue;
            int m = mateOf(i, false);
            if (m > i) {
                flip(i);
                return;
            }
        }
        int j = -1;
        for (int i = 0; i < N; i++) {
            if (!matched_[i] && !known_[i]) {
                j = i;
                break;
            }
        }
        if (j < 0) j = faceDown(-1, -1);
        flip(j);
        return;
    }
    int m = mateOf(upA_, false);
    if (m >= 0) {
        flip(m);
        return;
    }
    int j = -1;
    for (int i = 0; i < N; i++) {
        if (i != upA_ && !matched_[i] && !known_[i]) {
            j = i;
            break;
        }
    }
    if (j < 0) j = faceDown(kind_[upA_], upA_);
    flip(j);
}

void Game::themPick() {
    // One steal at most. After that the other seat turns cards and does not keep a pair,
    // so seven stays reachable on an eight-pair cloth.
    if (upA_ < 0) {
        if (them_ < 1) {
            for (int i = 0; i < N; i++) {
                if (!opp_[i] || matched_[i]) continue;
                int m = mateOf(i, true);
                if (m > i) {
                    flip(i);
                    return;
                }
            }
        }
        flip(faceDown(-1, -1));
        return;
    }
    if (them_ < 1) {
        int m = mateOf(upA_, true);
        if (m >= 0) {
            flip(m);
            return;
        }
    }
    flip(faceDown(kind_[upA_], upA_));
}

void Game::botAct() {
    if (mode_ == Mode::Title) {
        if (t_ > 10) newMatch();
        return;
    }
    if (mode_ != Mode::Play || over_ || phase_ != Phase::Choose) return;
    if (act_ > 0) {
        act_--;
        return;
    }
    if (yours_) youPick();
    else themPick();
}

void Game::human() {
    if (mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A)) newMatch();
        return;
    }
    if (mode_ != Mode::Play || over_ || phase_ != Phase::Choose) return;
    if (!yours_) {
        if (act_ > 0) {
            act_--;
            return;
        }
        themPick();
        return;
    }
    auto tap = [&](gs::Button b, int slot, int dCol, int dRow) {
        int& h = hold_[slot];
        auto step = [&] {
            int col = cursor_ % COLS;
            int row = cursor_ / COLS;
            col = (col + dCol + COLS) % COLS;
            row = (row + dRow + ROWS) % ROWS;
            cursor_ = row * COLS + col;
            blip(300);
        };
        if (sys_->pad.pressed(b)) {
            h = 0;
            step();
        } else if (sys_->pad.down(b)) {
            if (++h >= 12 && (h % 4) == 0) step();
        } else {
            h = 0;
        }
    };
    tap(gs::BTN_LEFT, 0, -1, 0);
    tap(gs::BTN_RIGHT, 1, 1, 0);
    tap(gs::BTN_UP, 2, 0, -1);
    tap(gs::BTN_DOWN, 3, 0, 1);
    if (sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_C)) flip(cursor_);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (beep_ > 0 && --beep_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
    if (mode_ == Mode::Play && phase_ == Phase::Peek && --peek_ <= 0) resolve();
    if (bot_) botAct();
    else human();
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
    s.w = int16_t(std::clamp(std::lround(w), 1L, 2000L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::box(float x, float y, float w, float h, int pal) {
    if (w < 1 || h < 1) return;
    gs::Sprite s{};
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.img = art_.solid.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    if (mode_ == Mode::Play || mode_ == Mode::Win || mode_ == Mode::Lose) {
        int mark = (yours_ && phase_ == Phase::Choose) ? cursor_ : upA_;
        if (mark >= 0 && mode_ == Mode::Play) {
            float cx, cy;
            cardPos(mark, cx, cy);
            float x = cx - art_.cw * 0.5f - 2;
            float y = cy - art_.ch * 0.5f - 2;
            box(x, y, art_.cw + 4, 2, PAL_GOLD);
            box(x, y + art_.ch + 2, art_.cw + 4, 2, PAL_GOLD);
            box(x, y, 2, art_.ch + 4, PAL_GOLD);
            box(x + art_.cw + 2, y, 2, art_.ch + 4, PAL_GOLD);
        }
    }

    for (int i = 0; i < N; i++) {
        float cx, cy;
        cardPos(i, cx, cy);
        bool up = matched_[i] || i == upA_ || i == upB_;
        if (up) {
            spr(art_.sym[kind_[i]], cx, cy, 22, PAL_SYM);
            box(cx - art_.cw * 0.5f, cy - art_.ch * 0.5f, art_.cw, art_.ch, PAL_FACE);
        } else if (mode_ != Mode::Title) {
            spr(art_.back, cx, cy, art_.ch, PAL_BACK);
        }
    }

    for (int i = 0; i < 7; i++) {
        box(8.0f + i * 12, 6, 10, 6, i < you_ ? PAL_GREEN : PAL_SHADE);
        box(228.0f + i * 12, 6, 10, 6, i < them_ ? PAL_RED : PAL_SHADE);
    }

    if (mode_ == Mode::Title) {
        box(36, 58, 248, 108, PAL_SHADE);
        hudC(9, "S3 MEMORY SEVEN", PAL_AMBER);
        hudC(11, "PAIRS ON THE CLOTH", PAL_TEXT);
        hudC(13, "FIRST TO SEVEN", PAL_AMBER);
        hudC(15, "ARROWS MOVE   A FLIPS", PAL_TEXT);
        if ((t_ / 30) % 2 == 0) hudC(17, "PRESS START", PAL_AMBER);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 9, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Win) {
        hudC(26, "FIRST TO SEVEN", PAL_GREEN);
    } else if (mode_ == Mode::Lose) {
        hudC(26, "THEY GOT THERE", PAL_RED);
    } else {
        char line[40];
        std::snprintf(line, sizeof(line), "YOU %d   THEM %d", you_, them_);
        hudC(26, line, PAL_AMBER);
        hudC(27, yours_ ? "YOUR TURN" : "THEIR TURN", yours_ ? PAL_GREEN : PAL_RED);
    }
}

}  // namespace memoryseven
