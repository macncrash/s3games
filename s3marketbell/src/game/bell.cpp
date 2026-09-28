#include "game/bell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace marketbell {
namespace {

constexpr int PAT = 10 * 60;
constexpr int COIN_V[4] = {1, 2, 5, 10};
constexpr float COIN_X[4] = {48.f, 100.f, 156.f, 214.f};
constexpr float COIN_Y = 188.f;
constexpr float COIN_H[4] = {14.f, 16.f, 20.f, 22.f};

struct Sale {
    const char* name;
    int price;
    int pay;
};
constexpr Sale SALES[3] = {
    {"PEAR", 3, 10},
    {"LOAF", 6, 10},
    {"FISH", 8, 20},
};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    toTitle();
}

void Game::clearRound() {
    sale_ = 0;
    dead_ = 0;
    tryNo_ = 0;
    score_ = 0;
    dish_ = 0;
    stackN_ = 0;
    cursor_ = 0;
    pat_ = PAT;
    wait_ = 0;
    repL_ = 0;
    repR_ = 0;
    axisN_ = 0;
    flashT_ = 0;
    flashK_ = 0;
    bellT_ = 0;
    ringN_ = 0;
    ready_ = false;
    over_ = false;
    won_ = false;
    rung_ = false;
    melStep_ = -1;
    melWait_ = 0;
    why_ = "";
}

void Game::toTitle() {
    clearRound();
    mode_ = Mode::Title;
}

void Game::openStall() {
    clearRound();
    tryNo_ = 1;
    sale_ = 0;
    pat_ = PAT;
    mode_ = Mode::Play;
    wait_ = 8;
}

void Game::nextTry() {
    dish_ = 0;
    stackN_ = 0;
    cursor_ = 0;
    ready_ = false;
    pat_ = PAT;
    sale_ = dead_ < 3 ? dead_ : 2;
    tryNo_ = dead_ + 1;
    mode_ = Mode::Play;
    wait_ = 10;
}

int Game::dueOf() const {
    int i = std::clamp(sale_, 0, 2);
    return SALES[i].pay - SALES[i].price;
}

bool Game::rules() const {
    for (int i = 0; i < 3; i++) {
        int due = SALES[i].pay - SALES[i].price;
        if (due <= 0 || due > 30) return false;
    }
    return true;
}

void Game::blip(float freq, int frames) {
    beepF_ = freq;
    beepV_ = 0.09f;
    beepN_ = frames;
}

void Game::nudge(int dir) {
    cursor_ = (cursor_ + dir + 4) % 4;
    blip(460.f + float(cursor_) * 40.f, 3);
}

void Game::dropCoin() {
    if (mode_ != Mode::Play || stackN_ >= 8) return;
    int v = COIN_V[cursor_];
    if (dish_ + v > 30) return;
    stack_[stackN_++] = v;
    dish_ += v;
    blip(400.f + float(v) * 40.f, 4);
}

void Game::undoCoin() {
    if (mode_ != Mode::Play || stackN_ <= 0) return;
    dish_ -= stack_[--stackN_];
    blip(280.f, 4);
}

void Game::ringBell() {
    rung_ = true;
    won_ = true;
    score_ += 200 + (3 - dead_) * 40 + pat_;
    why_ = "the bell rang";
    mode_ = Mode::Ring;
    ringN_ = 0;
    bellT_ = 48;
    flashK_ = 1;
    flashT_ = 40;
    blip(880.f, 10);
    sys_->apu.tone(1, 1320.f, 0.07f);
    sys_->rumble(0.2f, 0.55f, 90);
}

void Game::killTry(const char* why) {
    dead_++;
    why_ = why;
    dish_ = 0;
    stackN_ = 0;
    ready_ = false;
    flashT_ = 28;
    blip(120.f, 10);
    sys_->apu.noiseBurst(0.12f, 480.f, 0.2f);
    sys_->rumble(0.4f, 0.2f, 70);
    if (dead_ >= 3) {
        mode_ = Mode::Lose;
        over_ = true;
        won_ = false;
        rung_ = false;
        why_ = "the third try died";
        return;
    }
    nextTry();
}

void Game::hand() {
    if (mode_ != Mode::Play) return;
    const int due = dueOf();
    if (dish_ == due && due > 0 && dead_ < 3) {
        ringBell();
        return;
    }
    flashK_ = dish_ < due ? 2 : 3;
    killTry(dish_ < due ? "short change" : "heavy change");
}

void Game::readInput() {
    const gs::Pad& pad = sys_->pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) openStall();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) toTitle();
        return;
    }
    if (pad.pressed(gs::BTN_MODE)) {
        if (mode_ == Mode::Pause) mode_ = held_;
        else if (mode_ == Mode::Play) {
            held_ = mode_;
            mode_ = Mode::Pause;
        }
        return;
    }
    if (mode_ != Mode::Play) return;
    if (pad.pressed(gs::BTN_LEFT)) {
        nudge(-1);
        repL_ = 12;
    } else if (pad.down(gs::BTN_LEFT)) {
        if (--repL_ <= 0) {
            nudge(-1);
            repL_ = 6;
        }
    } else repL_ = 0;
    if (pad.pressed(gs::BTN_RIGHT)) {
        nudge(1);
        repR_ = 12;
    } else if (pad.down(gs::BTN_RIGHT)) {
        if (--repR_ <= 0) {
            nudge(1);
            repR_ = 6;
        }
    } else repR_ = 0;
    const float ax = sys_->pad.axisX;
    if (ax > 0.55f) {
        if (axisN_ <= 0) {
            nudge(1);
            axisN_ = 12;
        } else axisN_--;
    } else if (ax < -0.55f) {
        if (axisN_ <= 0) {
            nudge(-1);
            axisN_ = 12;
        } else axisN_--;
    } else axisN_ = 0;
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO) || pad.pressed(gs::BTN_X)) dropCoin();
    else if (pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_Y)) undoCoin();
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z)) hand();
}

void Game::driveBot() {
    for (int i = 0; i < gs::BTN_COUNT; i++) sys_->pad.keys[i] = false;
    if (mode_ == Mode::Title) {
        if (age_ >= 20) sys_->pad.keys[gs::BTN_START] = true;
        return;
    }
    if (mode_ != Mode::Play) return;
    if (wait_ > 0) {
        wait_--;
        return;
    }
    const int due = dueOf();
    if (dish_ == due) {
        sys_->pad.keys[gs::BTN_C] = true;
        wait_ = 2;
        return;
    }
    const int need = due - dish_;
    int want = 0;
    if (need >= 10) want = 3;
    else if (need >= 5) want = 2;
    else if (need >= 2) want = 1;
    if (cursor_ < want) sys_->pad.keys[gs::BTN_RIGHT] = true;
    else if (cursor_ > want) sys_->pad.keys[gs::BTN_LEFT] = true;
    else sys_->pad.keys[gs::BTN_A] = true;
    wait_ = 1;
}

void Game::logic() {
    if (mode_ == Mode::Pause) return;
    if (flashT_ > 0) flashT_--;
    if (bellT_ > 0) bellT_--;
    if (mode_ == Mode::Play) {
        if (--pat_ <= 0) {
            pat_ = 0;
            flashK_ = 2;
            killTry("the try ran out");
        }
    } else if (mode_ == Mode::Ring) {
        ringN_++;
        if (ringN_ == 8) sys_->apu.tone(1, 0, 0);
        if (ringN_ > 36) {
            mode_ = Mode::Win;
            over_ = true;
            melStep_ = 0;
            melWait_ = 0;
        }
    } else if (mode_ == Mode::Win && melStep_ >= 0 && melStep_ < 5) {
        if (melWait_ > 0) melWait_--;
        else {
            static const float notes[5] = {523.f, 659.f, 784.f, 1046.f, 1318.f};
            blip(notes[melStep_], 7);
            melStep_++;
            melWait_ = 6;
        }
    }
    if (mode_ == Mode::Play) {
        const int due = dueOf();
        const bool match = due > 0 && dish_ == due;
        if (match && !ready_) blip(1040.f, 4);
        ready_ = match;
    } else ready_ = false;
}

void Game::audio() {
    if (beepN_ <= 0) return;
    sys_->apu.tone(0, beepF_, beepV_);
    beepV_ *= 0.82f;
    if (--beepN_ == 0) sys_->apu.tone(0, 0, 0);
}

void Game::lights() {
    if (mode_ == Mode::Win || mode_ == Mode::Ring) sys_->setLight(40, 190, 60);
    else if (mode_ == Mode::Lose) sys_->setLight(190, 30, 20);
    else if (mode_ == Mode::Play && pat_ < PAT / 4) sys_->setLight(190, 70, 20);
    else sys_->setLight(180, 130, 40);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    readInput();
    if (bot_) driveBot();
    logic();
    audio();
    lights();
    draw();
}

void Game::hud(int col, int row, const char* s) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], PAL_HUD));
    }
}

void Game::hudAt(float cx, int row, const char* s) {
    if (!s) return;
    int n = int(std::strlen(s));
    int col = int(std::lround(cx / 8.f)) - n / 2;
    hud(col, row, s);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 2.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sprI(const gs::Image& img, float cx, float cy, float h, int pal) {
    if (h < 1.f || img.h < 1 || img.w < 1) return;
    float w = h * float(img.w) / float(img.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::solid(float x, float y, float w, float h, int pal) {
    if (w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = art_.solid;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    const bool lose = mode_ == Mode::Lose;
    const bool won = mode_ == Mode::Win || mode_ == Mode::Ring;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 16) c = gs::rgb4(3, 3, 6);
        else if (y < 88) {
            int t = y - 16;
            int r = 8 + t / 18;
            int g = 9 + t / 28;
            int b = 14 - t / 16;
            if (lose) {
                r = std::min(15, r + 3);
                b = std::max(2, b - 5);
            } else if (won) {
                g = std::min(15, g + 2);
            }
            c = gs::rgb4(std::min(15, r), std::min(15, g), std::max(0, b));
        } else c = gs::rgb4(6, 4, 2);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    const int due = dueOf();
    const int who = std::clamp(sale_, 0, 2);
    const bool showNums = mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Title || mode_ == Mode::Ring;

    if (mode_ == Mode::Title) {
        sprI(art_.title, 160.f, 28.f, float(art_.title.h), PAL_HUD);
        sprI(art_.sub, 160.f, 48.f, float(art_.sub.h), PAL_WARN);
    } else if (mode_ == Mode::Win || mode_ == Mode::Ring) {
        sprI(art_.rung, 160.f, 36.f, float(art_.rung.h), PAL_OK);
    } else if (mode_ == Mode::Lose) {
        sprI(art_.dead, 160.f, 36.f, float(art_.dead.h), PAL_BAD);
    }
    if (flashT_ > 0) {
        const gs::Image* w = nullptr;
        int pal = PAL_HUD;
        if (flashK_ == 1) {
            w = &art_.exact;
            pal = PAL_OK;
        } else if (flashK_ == 2) {
            w = &art_.shortw;
            pal = PAL_BAD;
        } else if (flashK_ == 3) {
            w = &art_.overw;
            pal = PAL_BAD;
        }
        if (w) sprI(*w, 118.f, 78.f, float(w->h), pal);
    }

    if (showNums) {
        int dp = PAL_HUD;
        if (dish_ == due && due > 0) dp = PAL_OK;
        else if (dish_ > due) dp = PAL_BAD;
        else if (dish_ > 0) dp = PAL_WARN;
        auto dig = [&](int n, float cx, float cy, int pal) {
            n = std::clamp(n, 0, 99);
            if (n < 10) {
                sprI(art_.digit[n], cx, cy, float(art_.digit[n].h), pal);
                return;
            }
            sprI(art_.digit[n / 10], cx - 8.f, cy, float(art_.digit[0].h), pal);
            sprI(art_.digit[n % 10], cx + 8.f, cy, float(art_.digit[0].h), pal);
        };
        dig(due, 236.f, 62.f, PAL_WARN);
        dig(dish_, 286.f, 62.f, dp);
    }

    for (int i = 0; i < 4; i++) {
        float y = COIN_Y - (i == cursor_ && (mode_ == Mode::Play || mode_ == Mode::Title) ? 6.f : 0.f);
        spr(art_.coin[i], COIN_X[i], y, COIN_H[i], PAL_COIN, false);
    }
    if (mode_ == Mode::Play || mode_ == Mode::Title)
        sprI(art_.bracket, COIN_X[cursor_], COIN_Y + 16.f, float(art_.bracket.h), PAL_WARN);
    for (int i = 0; i < stackN_; i++) {
        int val = stack_[i];
        int kind = val >= 10 ? 3 : val >= 5 ? 2 : val >= 2 ? 1 : 0;
        spr(art_.coin[kind], 96.f - 18.f + float(i) * 9.f, 154.f, 11.f, PAL_COIN, false);
    }

    for (int i = 0; i < 3; i++) {
        int pal = i < dead_ ? PAL_DIM : (i == dead_ && mode_ == Mode::Play ? PAL_LAMP : PAL_OK);
        if (mode_ == Mode::Win || mode_ == Mode::Ring) pal = PAL_OK;
        if (mode_ == Mode::Lose) pal = PAL_BAD;
        spr(art_.lamp, 248.f + float(i) * 18.f, 96.f, 16.f, pal, false);
    }

    float swing = 0.f;
    if (bellT_ > 0) swing = std::sin(float(bellT_) * 0.7f) * (4.f + float(bellT_) * 0.15f);
    spr(art_.bell, 28.f + swing, 78.f, 34.f, PAL_BELL, false);

    if (mode_ != Mode::Win) {
        spr(art_.buyer, 250.f, 138.f, 62.f, PAL_CUST, true);
        spr(art_.basket, 52.f, 118.f, 28.f, PAL_GOODS, false);
    }
    spr(art_.dish, 104.f, 162.f, 14.f, PAL_BOARD, false);
    spr(art_.crate, 150.f, 128.f, 24.f, PAL_STALL, false);
    spr(art_.counter, 100.f, 150.f, 32.f, PAL_STALL, false);
    spr(art_.clerk, 78.f, 128.f + std::sin(age_ * 0.08f), 60.f, PAL_CLERK, false);
    spr(art_.post, 18.f, 120.f, 80.f, PAL_STALL, false);
    spr(art_.post, 176.f, 120.f, 80.f, PAL_STALL, false);
    spr(art_.awning, 96.f, 92.f, 36.f, PAL_STALL, false);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int pct = std::clamp(pat_ * 100 / PAT, 0, 100);
        int barPal = pct > 50 ? PAL_OK : pct > 25 ? PAL_WARN : PAL_BAD;
        float barW = 70.f * float(pct) / 100.f;
        solid(230.f, 108.f, 70.f, 5.f, PAL_DIM);
        if (barW >= 1.f) solid(230.f, 108.f, barW, 5.f, barPal);
    }

    char line[48];
    std::snprintf(line, sizeof line, "TRY %d  DEAD %d", tryNo_ == 0 ? 1 : tryNo_, dead_);
    hud(0, 0, "S3 MARKETBELL");
    hud(22, 0, line);
    if (mode_ == Mode::Win) hud(6, 1, "THE BELL RANG");
    else if (mode_ == Mode::Lose) hud(4, 1, "THE THIRD TRY DIED");
    else if (mode_ == Mode::Ring) hud(8, 1, "RING");
    else {
        std::snprintf(line, sizeof line, "%s  PRICE %d  PAID %d  DUE %d", SALES[who].name, SALES[who].price,
                      SALES[who].pay, due);
        hud(0, 1, line);
    }
    if (showNums) {
        hudAt(236.f, 5, "DUE");
        hudAt(286.f, 5, "DISH");
    }
    if (mode_ == Mode::Pause) hud(16, 4, "PAUSED");
    hudAt(COIN_X[0], 26, "1");
    hudAt(COIN_X[1], 26, "2");
    hudAt(COIN_X[2], 26, "5");
    hudAt(COIN_X[3], 26, "10");
    if (mode_ == Mode::Title) {
        hud(1, 25, "ARROWS COIN   Z DROP   X BACK   C HAND");
        if ((age_ / 24) % 2 == 0) hud(7, 27, "ENTER OPENS THE STALL");
    } else if (mode_ == Mode::Play && due > 0 && dish_ == due) {
        hud(8, 27, "EXACT  PRESS C");
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        hud(8, 27, "ENTER FOR THE STALL");
    }
}

}  // namespace marketbell
