#include "game/market.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace market {
namespace {

constexpr int DAY_N = 8;
constexpr int PAT = 12 * 60;
constexpr int SHIFT_N = 30;
constexpr int COIN_V[4] = {1, 2, 5, 10};

constexpr float COIN_X[4] = {36.f, 92.f, 148.f, 208.f};
constexpr float COIN_Y = 176.f;
constexpr float COIN_H[4] = {16.f, 18.f, 22.f, 26.f};

constexpr float SPOT_X[4] = {198.f, 236.f, 268.f, 298.f};
constexpr float SPOT_Y[4] = {140.f, 136.f, 132.f, 130.f};
constexpr float SPOT_H[4] = {62.f, 50.f, 42.f, 36.f};

constexpr float BOARD_X = 250.f;
constexpr float BOARD_Y = 58.f;
constexpr float DUE_X = 218.f;
constexpr float DISH_X = 282.f;
constexpr float NUM_Y = 60.f;

constexpr float AWN_X = 92.f;
constexpr float AWN_Y = 100.f;
constexpr float CLERK_X = 68.f;
constexpr float CLERK_Y = 138.f;
constexpr float COUNT_X = 96.f;
constexpr float COUNT_Y = 154.f;
constexpr float ITEM_X = 50.f;
constexpr float ITEM_Y = 126.f;
constexpr float BILL_X = 128.f;
constexpr float BILL_Y = 138.f;
constexpr float DISH_CX = 104.f;
constexpr float DISH_CY = 162.f;

struct Deal {
    const char* name;
    int price;
    int pay;
    int good;
};
constexpr Deal DAY[DAY_N] = {
    {"APPLE", 3, 10, 0}, {"LOAF", 8, 10, 1}, {"JAR", 4, 10, 3}, {"SOAP", 5, 10, 6},
    {"FISH", 12, 20, 2}, {"PEAR", 6, 20, 4}, {"EGGS", 9, 20, 5}, {"HONEY", 16, 20, 7},
};

float lerp(float a, float b, float t) { return a + (b - a) * t; }

float smooth(float t) {
    t = std::clamp(t, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    toTitle();
}

void Game::clearRound() {
    idx_ = 0;
    served_ = 0;
    faults_ = 0;
    score_ = 0;
    dish_ = 0;
    stackN_ = 0;
    cursor_ = 0;
    tries_ = 0;
    pat_ = PAT;
    wait_ = 0;
    repL_ = 0;
    repR_ = 0;
    axisN_ = 0;
    flashT_ = 0;
    flashK_ = 0;
    shiftN_ = 0;
    bellT_ = 0;
    ready_ = false;
    over_ = false;
    won_ = false;
    melStep_ = -1;
    melWait_ = 0;
    why_ = "";
}

void Game::toTitle() {
    clearRound();
    mode_ = Mode::Title;
}

void Game::startDay() {
    clearRound();
    mode_ = Mode::Play;
    wait_ = 12;
}

int Game::dueOf() const {
    if (idx_ < 0 || idx_ >= DAY_N) return 0;
    return DAY[idx_].pay - DAY[idx_].price;
}

void Game::blip(float freq, int frames) {
    beepF_ = freq;
    beepV_ = 0.09f;
    beepN_ = frames;
}

void Game::nudge(int dir) {
    cursor_ = (cursor_ + dir + 4) % 4;
    blip(480.f + float(cursor_) * 50.f, 3);
}

void Game::dropCoin() {
    if (mode_ != Mode::Play) return;
    if (stackN_ >= 16) return;
    int v = COIN_V[cursor_];
    if (dish_ + v > 40) return;
    stack_[stackN_++] = v;
    dish_ += v;
    blip(420.f + float(v) * 46.f, 5);
}

void Game::undoCoin() {
    if (mode_ != Mode::Play || stackN_ <= 0) return;
    dish_ -= stack_[--stackN_];
    blip(300.f, 4);
}

void Game::hand() {
    if (mode_ != Mode::Play) return;
    const int due = dueOf();
    if (dish_ == due && due > 0) {
        served_++;
        score_ += 100 + pat_ * 100 / PAT;
        if (tries_ == 0) score_ += 40;
        dish_ = 0;
        stackN_ = 0;
        ready_ = false;
        mode_ = Mode::Shift;
        shiftN_ = 0;
        flashK_ = 1;
        flashT_ = 26;
        bellT_ = 18;
        blip(880.f, 8);
        sys_->apu.tone(1, 1320.f, 0.05f);
        dingN_ = 12;
        return;
    }
    faults_++;
    tries_++;
    flashK_ = dish_ < due ? 2 : 3;
    flashT_ = 36;
    pat_ -= PAT / 5;
    if (pat_ < 0) pat_ = 0;
    dish_ = 0;
    stackN_ = 0;
    ready_ = false;
    blip(110.f, 10);
    sys_->apu.noiseBurst(0.12f, 520.f, 0.22f);
    sys_->rumble(0.45f, 0.25f, 80);
    if (faults_ >= 3) loseDay("wrong change stopped the line");
    else if (pat_ <= 0) loseDay("the line stalled");
}

void Game::endShift() {
    shiftN_ = 0;
    dish_ = 0;
    stackN_ = 0;
    tries_ = 0;
    ready_ = false;
    cursor_ = 0;
    if (served_ >= DAY_N) winDay();
    else {
        idx_ = served_;
        pat_ = PAT;
        mode_ = Mode::Play;
        wait_ = 12;
    }
}

void Game::winDay() {
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "the line moved";
    if (faults_ == 0) score_ += 200;
    melStep_ = 0;
    melWait_ = 0;
}

void Game::loseDay(const char* why) {
    if (mode_ == Mode::Lose || mode_ == Mode::Win) return;
    mode_ = Mode::Lose;
    over_ = true;
    won_ = false;
    why_ = why;
    sys_->rumble(0.7f, 0.4f, 160);
}

void Game::readInput() {
    gs::Pad& pad = sys_->pad;
    if (pad.pressed(gs::BTN_MODE)) {
        if (mode_ == Mode::Title) {
            if (!bot_) sys_->quit();
        } else if (mode_ == Mode::Pause) toTitle();
        else if (mode_ == Mode::Win || mode_ == Mode::Lose) toTitle();
        else {
            held_ = mode_;
            mode_ = Mode::Pause;
        }
        return;
    }
    if (pad.pressed(gs::BTN_START)) {
        if (mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Lose) startDay();
        else if (mode_ == Mode::Pause) mode_ = held_;
        else {
            held_ = mode_;
            mode_ = Mode::Pause;
        }
        return;
    }
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_X) ||
            pad.pressed(gs::BTN_Y) || pad.pressed(gs::BTN_Z) || pad.pressed(gs::BTN_TURBO))
            startDay();
        return;
    }
    if (mode_ != Mode::Play) return;

    if (pad.pressed(gs::BTN_LEFT)) {
        nudge(-1);
        repL_ = 10;
    } else if (pad.down(gs::BTN_LEFT)) {
        if (--repL_ <= 0) {
            nudge(-1);
            repL_ = 5;
        }
    }
    if (pad.pressed(gs::BTN_RIGHT)) {
        nudge(1);
        repR_ = 10;
    } else if (pad.down(gs::BTN_RIGHT)) {
        if (--repR_ <= 0) {
            nudge(1);
            repR_ = 5;
        }
    }
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

    const bool drop = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO) || pad.pressed(gs::BTN_X);
    const bool back = pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_Y);
    const bool give = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z);
    if (drop) dropCoin();
    else if (back) undoCoin();
    if (give) hand();
}

void Game::driveBot() {
    // Keys latch on the next frame, the same path a player uses.
    for (int i = 0; i < gs::BTN_COUNT; i++) sys_->pad.keys[i] = false;
    if (mode_ == Mode::Title) {
        if (age_ >= 36) sys_->pad.keys[gs::BTN_START] = true;
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
        wait_ = 1;
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
    if (dingN_ > 0 && --dingN_ == 0) sys_->apu.tone(1, 0, 0);
    if (mode_ == Mode::Shift) {
        if (shiftN_ >= SHIFT_N) endShift();
        else shiftN_++;
    } else if (mode_ == Mode::Play) {
        if (--pat_ <= 0) {
            pat_ = 0;
            loseDay("the line stalled");
        }
    } else if (mode_ == Mode::Win && melStep_ >= 0 && melStep_ < 6) {
        if (melWait_ > 0) melWait_--;
        else {
            static const float notes[6] = {523.f, 659.f, 784.f, 1046.f, 784.f, 1318.f};
            blip(notes[melStep_], 8);
            melStep_++;
            melWait_ = 7;
        }
    }
    if (mode_ == Mode::Play) {
        const int due = dueOf();
        const bool match = due > 0 && dish_ == due;
        if (match && !ready_) blip(1046.f, 5);
        ready_ = match;
    } else ready_ = false;
}

void Game::audio() {
    if (beepN_ <= 0) return;
    sys_->apu.tone(0, beepF_, beepV_);
    beepV_ *= 0.84f;
    if (--beepN_ == 0) sys_->apu.tone(0, 0, 0);
}

void Game::lights() {
    if (mode_ == Mode::Win) sys_->setLight(40, 200, 70);
    else if (mode_ == Mode::Lose) sys_->setLight(200, 30, 20);
    else if (mode_ == Mode::Play && pat_ * 100 / PAT < 30) sys_->setLight(200, 50, 20);
    else sys_->setLight(190, 120, 30);
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

void Game::shadeAt(float cx, float cy, float w) {
    if (w < 4.f) return;
    gs::Sprite s;
    s.img = art_.shade;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::max(4, int(std::lround(w * 0.28f))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::digits(int n, float cx, float cy, int pal) {
    n = std::clamp(n, 0, 99);
    if (n < 10) {
        sprI(art_.digit[n], cx, cy, float(art_.digit[n].h), pal);
        return;
    }
    const gs::Image& a = art_.digit[n / 10];
    const gs::Image& b = art_.digit[n % 10];
    const float gap = 1.f;
    const float w = float(a.w + b.w) + gap;
    const float x = cx - w * 0.5f;
    sprI(a, x + a.w * 0.5f, cy, float(a.h), pal);
    sprI(b, x + a.w + gap + b.w * 0.5f, cy, float(b.h), pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    const bool lose = mode_ == Mode::Lose;
    const bool won = mode_ == Mode::Win;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 18 || y >= 200) c = gs::rgb4(2, 2, 4);
        else if (y < 96) {
            int t = y - 18;
            int r = 7 + t * 6 / 78;
            int g = 8 + t * 3 / 78;
            int b = 13 - t * 5 / 78;
            if (lose) {
                r = std::min(15, r + 4);
                b = std::max(2, b - 4);
            } else if (won) {
                g = std::min(15, g + 2);
                b = std::min(15, b + 1);
            }
            c = gs::rgb4(r, g, b);
        } else c = gs::rgb4(7, 5, 3);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::drawPeople() {
    const bool moving = mode_ == Mode::Shift;
    const int base = moving ? served_ - 1 : idx_;
    const float e = moving ? smooth(float(shiftN_) / float(SHIFT_N)) : 0.f;
    auto body = [&](int n, float x, float y, float h, bool front) {
        if (n < 0 || n >= DAY_N) return;
        float bob = std::sin((age_ + n * 11) * (front && pat_ < PAT / 3 ? 0.28f : 0.08f)) * 1.1f;
        if (front && mode_ == Mode::Play && pat_ * 3 < PAT) x += 5.f;
        int pal = PAL_C0 + (n & 3);
        float head = y + h * ((18.f / 70.f) - 0.5f);
        shadeAt(x, y + h * 0.42f, h * 0.7f);
        spr(art_.person, x, y + bob, h, pal, true);
        if ((n & 1) == 0) spr(art_.hat, x, head + bob - h * 0.08f, h * 0.2f, pal, false);
        else spr(art_.bag, x - h * 0.28f, y + bob, h * 0.26f, PAL_GOODS, false);
    };
    if (!moving) {
        if (mode_ == Mode::Win) return;
        for (int k = 0; k < 4; k++) body(base + k, SPOT_X[k], SPOT_Y[k], SPOT_H[k], k == 0);
        return;
    }
    body(base, lerp(SPOT_X[0], -40.f, e), SPOT_Y[0], SPOT_H[0], true);
    for (int k = 1; k <= 3; k++) {
        int n = base + k;
        if (n >= DAY_N) break;
        body(n, lerp(SPOT_X[k], SPOT_X[k - 1], e), lerp(SPOT_Y[k], SPOT_Y[k - 1], e),
             lerp(SPOT_H[k], SPOT_H[k - 1], e), k == 1);
    }
    int neu = base + 4;
    if (neu < DAY_N)
        body(neu, lerp(340.f, SPOT_X[3], e), SPOT_Y[3], SPOT_H[3], false);
}

void Game::drawCoins() {
    for (int i = 0; i < 4; i++) {
        float y = COIN_Y - (i == cursor_ && (mode_ == Mode::Play || mode_ == Mode::Title) ? 5.f : 0.f);
        spr(art_.coin[i], COIN_X[i], y, COIN_H[i], PAL_COIN, false);
    }
    if (mode_ == Mode::Play || mode_ == Mode::Title) {
        float y = COIN_Y + COIN_H[cursor_] * 0.45f;
        sprI(art_.bracket, COIN_X[cursor_], y, float(art_.bracket.h), PAL_WARN);
    }
    if (mode_ != Mode::Play && mode_ != Mode::Pause) return;
    for (int i = 0; i < stackN_; i++) {
        int v = stack_[i];
        int kind = v >= 10 ? 3 : v >= 5 ? 2 : v >= 2 ? 1 : 0;
        float x = DISH_CX - 22.f + float(i % 8) * 8.f;
        float y = DISH_CY - 2.f + float(i / 8) * 6.f;
        spr(art_.coin[kind], x, y, 12.f, PAL_COIN, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    const bool showBoard = mode_ != Mode::Win;
    const int due = dueOf();
    const int who = idx_;

    if (mode_ == Mode::Win) {
        for (int i = 0; i < 18; i++) {
            float ph = float(age_) * (1.4f + float(i % 5) * 0.15f) + float(i) * 20.f;
            float x = std::fmod(ph * 3.f + float(i) * 18.f, 320.f);
            if (x < 0) x += 320.f;
            float y = std::fmod(ph * 1.6f, 180.f);
            if (y < 0) y += 180.f;
            int pal = (i % 3 == 0) ? PAL_OK : (i % 3 == 1) ? PAL_WARN : PAL_BAD;
            solid(x, y, (i & 1) ? 4.f : 3.f, (i & 1) ? 3.f : 5.f, pal);
        }
        sprI(art_.moved, 160.f, 96.f, float(art_.moved.h), PAL_OK);
    } else if (mode_ == Mode::Lose) {
        sprI(art_.stalled, 150.f, 92.f, float(art_.stalled.h), PAL_BAD);
    } else if (mode_ == Mode::Title) {
        sprI(art_.title, 92.f, 28.f, float(art_.title.h), PAL_HUD);
        sprI(art_.stall, 92.f, 48.f, float(art_.stall.h), PAL_WARN);
    }
    if (flashT_ > 0) {
        const gs::Image* w = nullptr;
        int pal = PAL_HUD;
        if (flashK_ == 1) {
            w = &art_.exact;
            pal = PAL_OK;
        } else if (flashK_ == 2) {
            w = &art_.brief;
            pal = PAL_BAD;
        } else if (flashK_ == 3) {
            w = &art_.over;
            pal = PAL_BAD;
        }
        if (w) sprI(*w, 118.f, 78.f, float(w->h), pal);
    }
    if (showBoard) {
        int dp = PAL_HUD;
        if (dish_ == 0) dp = PAL_HUD;
        else if (dish_ == due) dp = PAL_OK;
        else if (dish_ > due) dp = PAL_BAD;
        else dp = PAL_WARN;
        digits(due, DUE_X, NUM_Y, PAL_WARN);
        digits(dish_, DISH_X, NUM_Y, dp);
    }

    drawCoins();
    drawPeople();

    if (mode_ == Mode::Shift && served_ > 0) {
        float e = smooth(float(shiftN_) / float(SHIFT_N));
        int g = DAY[served_ - 1].good;
        spr(art_.good[g], lerp(ITEM_X, -24.f, e), lerp(ITEM_Y, SPOT_Y[0], e), 26.f, PAL_GOODS, false);
    } else if (mode_ != Mode::Win && who >= 0 && who < DAY_N) {
        spr(art_.good[DAY[who].good], ITEM_X, ITEM_Y, 28.f, PAL_GOODS, false);
    }
    if (mode_ == Mode::Play || mode_ == Mode::Title || mode_ == Mode::Pause || mode_ == Mode::Lose)
        spr(art_.bill, BILL_X, BILL_Y, 22.f, PAL_BILL, false);

    spr(art_.dish, DISH_CX, DISH_CY, 16.f, PAL_STALL, false);
    spr(art_.crate, 48.f, 140.f, 26.f, PAL_STALL, false);
    spr(art_.counter, COUNT_X, COUNT_Y, 36.f, PAL_STALL, false);
    spr(art_.clerk, CLERK_X, CLERK_Y + std::sin(age_ * 0.07f), 66.f, PAL_CLERK, false);
    float bx = 24.f + (bellT_ > 0 ? std::sin(float(bellT_) * 1.4f) * 4.f : 0.f);
    spr(art_.bell, bx, 112.f, 18.f, PAL_COIN, false);
    spr(art_.post, 16.f, 126.f, 78.f, PAL_STALL, false);
    spr(art_.post, 168.f, 126.f, 78.f, PAL_STALL, false);
    spr(art_.awning, AWN_X, AWN_Y, 40.f, PAL_STALL, false);
    if (showBoard) spr(art_.board, BOARD_X, BOARD_Y, 50.f, PAL_STALL, false);
    if (mode_ == Mode::Title) solid(6.f, 14.f, 168.f, 44.f, PAL_DIM);

    if (mode_ != Mode::Win) {
        int pct = std::clamp(pat_ * 100 / PAT, 0, 100);
        int barPal = pct > 55 ? PAL_OK : pct > 28 ? PAL_WARN : PAL_BAD;
        if (pct <= 28 && ((age_ / 8) & 1)) barPal = PAL_HUD;
        float barW = 112.f * float(pct) / 100.f;
        if (barW >= 1.f) solid(196.f, 90.f, barW, 6.f, barPal);
        solid(196.f, 90.f, 112.f, 6.f, PAL_DIM);
        sprI(art_.line, 168.f, 93.f, float(art_.line.h), PAL_HUD);
    }
    shadeAt(COUNT_X, COUNT_Y + 16.f, 150.f);

    char line[48];
    std::snprintf(line, sizeof line, "LINE %d/8  FLT %d  SC %d", served_, faults_, score_);
    hud(0, 0, "S3 MARKET");
    hud(11, 0, line);
    if (mode_ == Mode::Win) {
        hud(8, 1, faults_ == 0 ? "CHANGE WAS EXACT" : "THE LINE MOVED");
    } else if (mode_ == Mode::Lose) {
        hud(4, 1, faults_ >= 3 ? "WRONG CHANGE STOPPED THE LINE" : "TOO SLOW  THE LINE STALLED");
    } else if (who >= 0 && who < DAY_N) {
        std::snprintf(line, sizeof line, "%s  PRICE %d  PAID %d", DAY[who].name, DAY[who].price, DAY[who].pay);
        hud(0, 1, line);
    }
    if (showBoard) {
        hudAt(DUE_X, 3, "DUE");
        hudAt(DISH_X, 3, "DISH");
    }
    if (mode_ == Mode::Pause) hud(16, 4, "PAUSED");

    hudAt(COIN_X[0], 25, "1");
    hudAt(COIN_X[1], 25, "2");
    hudAt(COIN_X[2], 25, "5");
    hudAt(COIN_X[3], 25, "10");
    if (mode_ == Mode::Title) {
        hud(2, 26, "ARROWS COIN  Z DROP  X BACK  C HAND");
        if ((age_ / 24) % 2 == 0) hud(9, 27, "ENTER OPENS THE STALL");
    } else if (mode_ == Mode::Play && due > 0 && dish_ == due) {
        hud(8, 27, "RIGHT CHANGE   PRESS C");
    } else if (mode_ == Mode::Win) {
        hud(8, 27, "ENTER OPENS ANOTHER DAY");
    } else if (mode_ == Mode::Lose) {
        hud(11, 27, "ENTER TRIES THE DAY");
    } else {
        hud(2, 27, "ARROWS COIN  Z DROP  X BACK  C HAND");
    }
}

}  // namespace market
