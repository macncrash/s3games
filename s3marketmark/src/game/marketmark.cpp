#include "game/marketmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace marketmark {
namespace {

constexpr int LINE_N = 3;
constexpr int MARK_AT = 2;
constexpr int PAT = 18 * 60;
constexpr int STEP_N = 28;
constexpr int COIN_V[4] = {1, 2, 5, 10};

constexpr float COIN_X[4] = {48.f, 104.f, 160.f, 220.f};
constexpr float COIN_Y = 188.f;
constexpr float COIN_H[4] = {14.f, 16.f, 20.f, 24.f};

constexpr float SPOT_X[3] = {214.f, 258.f, 296.f};
constexpr float SPOT_Y[3] = {132.f, 126.f, 122.f};
constexpr float SPOT_H[3] = {58.f, 46.f, 38.f};

constexpr float DUE_X = 236.f;
constexpr float DISH_X = 292.f;
constexpr float NUM_Y = 52.f;
constexpr float CLERK_X = 72.f;
constexpr float CLERK_Y = 136.f;
constexpr float COUNT_X = 108.f;
constexpr float COUNT_Y = 156.f;
constexpr float DISH_CX = 118.f;
constexpr float DISH_CY = 164.f;
constexpr float NOTE_X = 150.f;
constexpr float NOTE_Y = 128.f;

struct Deal {
    const char* name;
    int price;
    int pay;
    int good;  // 0 pear, 1 loaf, 2 jar
};

constexpr Deal LINE[LINE_N] = {
    {"PEAR", 4, 10, 0},
    {"LOAF", 7, 10, 1},
    {"JAR", 12, 20, 2},
};

float lerp(float a, float b, float t) { return a + (b - a) * t; }

float smooth(float t) {
    t = std::clamp(t, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

const gs::Mipped* goodOf(const Art& a, int g) {
    if (g == 1) return &a.loaf;
    if (g == 2) return &a.jar;
    return &a.pear;
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
    pat_ = PAT;
    wait_ = 0;
    repL_ = 0;
    repR_ = 0;
    axisN_ = 0;
    flashT_ = 0;
    flashK_ = 0;
    stepN_ = 0;
    ready_ = false;
    over_ = false;
    won_ = false;
    finished_ = false;
    onMark_ = false;
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
    mode_ = Mode::Play;
    wait_ = 10;
    onMark_ = false;
}

int Game::dueOf() const {
    if (idx_ < 0 || idx_ >= LINE_N) return 0;
    return LINE[idx_].pay - LINE[idx_].price;
}

bool Game::isMark() const { return idx_ == MARK_AT; }

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
    if (mode_ != Mode::Play) return;
    if (stackN_ >= 12) return;
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

void Game::finishMark() {
    finished_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "gold note exact";
    score_ += 250;
    if (faults_ == 0) score_ += 80;
    melStep_ = 0;
    melWait_ = 0;
    flashK_ = 1;
    flashT_ = 40;
    blip(880.f, 10);
    sys_->apu.tone(1, 1320.f, 0.06f);
}

void Game::fail(const char* why) {
    if (mode_ == Mode::Lose || mode_ == Mode::Win) return;
    mode_ = Mode::Lose;
    over_ = true;
    won_ = false;
    finished_ = false;
    why_ = why;
    sys_->rumble(0.65f, 0.35f, 140);
}

void Game::hand() {
    if (mode_ != Mode::Play) return;
    const int due = dueOf();
    if (dish_ == due && due > 0) {
        served_++;
        score_ += 40 + (isMark() ? 60 : 0);
        dish_ = 0;
        stackN_ = 0;
        ready_ = false;
        flashK_ = 1;
        flashT_ = 22;
        blip(820.f, 7);
        if (isMark()) {
            onMark_ = true;
            finishMark();
        } else {
            mode_ = Mode::Step;
            stepN_ = 0;
        }
        return;
    }
    faults_++;
    flashK_ = dish_ < due ? 2 : 3;
    flashT_ = 30;
    pat_ -= PAT / 6;
    if (pat_ < 0) pat_ = 0;
    dish_ = 0;
    stackN_ = 0;
    ready_ = false;
    blip(110.f, 10);
    sys_->apu.noiseBurst(0.12f, 480.f, 0.2f);
    sys_->rumble(0.4f, 0.2f, 70);
    if (isMark()) fail("the mark walked");
    else if (faults_ >= 2) fail("wrong change closed the stall");
    else if (pat_ <= 0) fail("the line stalled");
}

void Game::advance() {
    stepN_ = 0;
    dish_ = 0;
    stackN_ = 0;
    cursor_ = 0;
    ready_ = false;
    idx_ = served_;
    pat_ = PAT;
    onMark_ = isMark();
    mode_ = Mode::Play;
    wait_ = 8;
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
        if (mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Lose) openStall();
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
            openStall();
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
    const float ax = pad.axisX;
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
    for (int i = 0; i < gs::BTN_COUNT; i++) sys_->pad.keys[i] = false;
    if (mode_ == Mode::Title) {
        if (age_ >= 30) sys_->pad.keys[gs::BTN_START] = true;
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
    if (mode_ == Mode::Step) {
        if (stepN_ >= STEP_N) advance();
        else stepN_++;
    } else if (mode_ == Mode::Play) {
        if (--pat_ <= 0) {
            pat_ = 0;
            fail(isMark() ? "the mark walked" : "the line stalled");
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
        onMark_ = isMark();
    }
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
    else if (onMark_ || (mode_ == Mode::Play && isMark())) sys_->setLight(220, 170, 30);
    else sys_->setLight(180, 110, 40);
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
        if (y < 16) c = gs::rgb4(2, 2, 4);
        else if (y < 100) {
            int t = y - 16;
            int r = 8 + t * 4 / 84;
            int g = 7 + t * 2 / 84;
            int b = 12 - t * 4 / 84;
            if (lose) {
                r = std::min(15, r + 3);
                b = std::max(2, b - 3);
            } else if (won) {
                g = std::min(15, g + 3);
            } else if (isMark() && mode_ == Mode::Play) {
                r = std::min(15, r + 2);
                g = std::min(15, g + 1);
            }
            c = gs::rgb4(r, g, b);
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

    const bool showNums = mode_ != Mode::Win;
    const int due = dueOf();
    const bool moving = mode_ == Mode::Step;
    const float e = moving ? smooth(float(stepN_) / float(STEP_N)) : 0.f;

    if (mode_ == Mode::Win) {
        for (int i = 0; i < 12; i++) {
            float ph = float(age_) * 1.6f + float(i) * 26.f;
            float x = std::fmod(ph * 2.4f, 320.f);
            float y = 30.f + std::fmod(ph * 1.1f, 140.f);
            solid(x, y, 4.f, 3.f, (i & 1) ? PAL_WARN : PAL_OK);
        }
        sprI(art_.done, 160.f, 78.f, float(art_.done.h), PAL_OK);
        spr(art_.star, 160.f, 118.f, 28.f, PAL_NOTE, false);
    } else if (mode_ == Mode::Lose) {
        sprI(art_.walked, 160.f, 70.f, float(art_.walked.h), PAL_BAD);
    } else if (mode_ == Mode::Title) {
        sprI(art_.title, 160.f, 22.f, float(art_.title.h), PAL_HUD);
        sprI(art_.sub, 160.f, 44.f, float(art_.sub.h), PAL_WARN);
    }

    if (flashT_ > 0 && mode_ != Mode::Win) {
        const gs::Image* w = nullptr;
        int pal = PAL_HUD;
        if (flashK_ == 1) {
            w = &art_.exact;
            pal = PAL_OK;
        } else if (flashK_ == 2) {
            w = &art_.brief;
            pal = PAL_BAD;
        } else if (flashK_ == 3) {
            w = &art_.heavy;
            pal = PAL_BAD;
        }
        if (w) sprI(*w, 120.f, 78.f, float(w->h), pal);
    }

    for (int i = 0; i < 4; i++) {
        float y = COIN_Y - (i == cursor_ && (mode_ == Mode::Play || mode_ == Mode::Title) ? 5.f : 0.f);
        spr(art_.coin[i], COIN_X[i], y, COIN_H[i], PAL_COIN, false);
    }
    if (mode_ == Mode::Play || mode_ == Mode::Title)
        sprI(art_.bracket, COIN_X[cursor_], COIN_Y + 14.f, float(art_.bracket.h), PAL_WARN);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        for (int i = 0; i < stackN_; i++) {
            int val = stack_[i];
            int kind = val >= 10 ? 3 : val >= 5 ? 2 : val >= 2 ? 1 : 0;
            float x = DISH_CX - 18.f + float(i % 6) * 8.f;
            float y = DISH_CY - 4.f - float(i / 6) * 5.f;
            spr(art_.coin[kind], x, y, 11.f, PAL_COIN, false);
        }
    }

    if (showNums && mode_ != Mode::Title && mode_ != Mode::Lose) {
        int dp = PAL_HUD;
        if (dish_ == due && due > 0) dp = PAL_OK;
        else if (dish_ > due) dp = PAL_BAD;
        else if (dish_ > 0) dp = PAL_WARN;
        digits(due, DUE_X, NUM_Y, isMark() ? PAL_WARN : PAL_HUD);
        digits(dish_, DISH_X, NUM_Y, dp);
    }

    auto body = [&](int n, float x, float y, float h) {
        if (n < 0 || n >= LINE_N) return;
        float bob = std::sin((age_ + n * 9) * 0.08f) * 1.f;
        bool gold = n == MARK_AT;
        spr(art_.buyer, x, y + bob, h, gold ? PAL_MARK : PAL_BUYER, true);
        spr(art_.hat, x, y - h * 0.38f + bob, h * 0.18f, gold ? PAL_NOTE : PAL_BUYER, false);
        if (gold) spr(art_.star, x + h * 0.22f, y - h * 0.05f + bob, h * 0.22f, PAL_NOTE, false);
        shadeAt(x, y + h * 0.42f, h * 0.62f);
    };

    if (mode_ != Mode::Win) {
        if (!moving) {
            for (int k = 2; k >= 0; k--) body(idx_ + k, SPOT_X[k], SPOT_Y[k], SPOT_H[k]);
        } else {
            body(idx_, lerp(SPOT_X[0], -36.f, e), SPOT_Y[0], SPOT_H[0]);
            for (int k = 1; k < 3; k++) {
                int n = idx_ + k;
                if (n >= LINE_N) continue;
                body(n, lerp(SPOT_X[k], SPOT_X[k - 1], e), lerp(SPOT_Y[k], SPOT_Y[k - 1], e),
                     lerp(SPOT_H[k], SPOT_H[k - 1], e));
            }
        }
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Title) {
        if (idx_ >= 0 && idx_ < LINE_N && mode_ != Mode::Title)
            spr(*goodOf(art_, LINE[idx_].good), 52.f, 124.f, 26.f, PAL_GOODS, false);
        int notePal = (mode_ != Mode::Title && isMark()) ? PAL_NOTE : PAL_COIN;
        spr(art_.note, NOTE_X, NOTE_Y, 20.f, notePal, false);
        if (mode_ != Mode::Title && isMark()) spr(art_.star, NOTE_X, NOTE_Y - 2.f, 12.f, PAL_NOTE, false);
    }

    spr(art_.dish, DISH_CX, DISH_CY, 14.f, PAL_STALL, false);
    spr(art_.counter, COUNT_X, COUNT_Y, 32.f, PAL_STALL, false);
    spr(art_.clerk, CLERK_X, CLERK_Y + std::sin(age_ * 0.07f), 62.f, PAL_CLERK, false);
    spr(art_.post, 18.f, 124.f, 70.f, PAL_STALL, false);
    spr(art_.post, 176.f, 124.f, 70.f, PAL_STALL, false);
    spr(art_.awning, 96.f, 96.f, 36.f, PAL_STALL, false);
    shadeAt(COUNT_X, COUNT_Y + 14.f, 130.f);

    if (mode_ == Mode::Play || mode_ == Mode::Step) {
        int pct = std::clamp(pat_ * 100 / PAT, 0, 100);
        int barPal = pct > 50 ? PAL_OK : pct > 25 ? PAL_WARN : PAL_BAD;
        float barW = 90.f * float(pct) / 100.f;
        if (barW >= 1.f) solid(200.f, 78.f, barW, 5.f, barPal);
        solid(200.f, 78.f, 90.f, 5.f, PAL_DIM);
    }

    char line[48];
    std::snprintf(line, sizeof line, "LINE %d/%d  FLT %d", std::min(served_ + (mode_ == Mode::Win ? 0 : 1), LINE_N),
                  LINE_N, faults_);
    hud(0, 0, "S3 MARKETMARK");
    hud(18, 0, line);
    if (mode_ == Mode::Win) {
        hud(6, 1, "FINISHED MARK  GOLD NOTE EXACT");
    } else if (mode_ == Mode::Lose) {
        hud(8, 1, why_ && why_[0] ? why_ : "STALL CLOSED");
    } else if (mode_ != Mode::Title && idx_ >= 0 && idx_ < LINE_N) {
        std::snprintf(line, sizeof line, "%s  PRICE %d  PAID %d%s", LINE[idx_].name, LINE[idx_].price, LINE[idx_].pay,
                      isMark() ? "  MARK" : "");
        hud(0, 1, line);
    } else if (mode_ == Mode::Title) {
        hud(4, 1, "EXACT CHANGE ON THE GOLD NOTE");
    }
    if (showNums && mode_ != Mode::Title && mode_ != Mode::Lose) {
        hudAt(DUE_X, 2, isMark() ? "MARK" : "DUE");
        hudAt(DISH_X, 2, "DISH");
    }
    if (mode_ == Mode::Pause) hud(16, 4, "PAUSED");
    hudAt(COIN_X[0], 25, "1");
    hudAt(COIN_X[1], 25, "2");
    hudAt(COIN_X[2], 25, "5");
    hudAt(COIN_X[3], 25, "10");
    if (mode_ == Mode::Title) {
        hud(1, 26, "ARROWS COIN   Z DROP   X BACK   C HAND");
        if ((age_ / 24) % 2 == 0) hud(8, 27, "ENTER OPENS THE STALL");
    } else if (mode_ == Mode::Play && due > 0 && dish_ == due) {
        hud(isMark() ? 6 : 8, 27, isMark() ? "HAND THE MARK   PRESS C" : "RIGHT CHANGE   PRESS C");
    } else if (mode_ == Mode::Win) {
        hud(7, 27, "THE FINISHED MARK ENDS IT");
    } else if (mode_ == Mode::Lose) {
        hud(10, 27, "ENTER TRIES AGAIN");
    } else {
        hud(1, 27, "ARROWS COIN   Z DROP   X BACK   C HAND");
    }
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

}  // namespace marketmark
