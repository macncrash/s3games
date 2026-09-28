#include "game/marketseven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace marketseven {
namespace {

constexpr int PTS[3] = {1, 2, 3};
constexpr int DUE[3] = {2, 1, 3};
constexpr int COIN_V[2] = {1, 2};
constexpr char NAME[3][8] = {"CREAM", "LOAF", "GOLD"};

constexpr float GOOD_X[3] = {78.f, 124.f, 170.f};
constexpr float GOOD_Y = 118.f;
constexpr float COIN_X[2] = {86.f, 148.f};
constexpr float COIN_Y = 188.f;

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Pick: return "pick";
    case Mode::Till: return "till";
    case Mode::Rival: return "rival";
    case Mode::Win: return "win";
    case Mode::Lose: return "lose";
    case Mode::Pause: return "pause";
    }
    return "?";
}

bool Game::rules() const {
    if (!won_ || you_ < 7 || them_ >= 7 || !shortSix_ || logN_ < 3) return false;
    int y = 0, t = 0;
    bool saw = false;
    for (int i = 0; i < logN_; i++) {
        if (log_[i].pts <= 0) return false;
        if (log_[i].yours) {
            if (y >= 7) return false;
            y += log_[i].pts;
            if (y == 6) saw = true;
            if (y >= 7 && t >= 7) return false;
        } else {
            if (t >= 7) return false;
            t += log_[i].pts;
            if (t >= 7 && y < 7) return false;
        }
    }
    return saw && y == you_ && t == them_ && y >= 7 && t < 7;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
    wait_ = 0;
    flashT_ = 0;
}

void Game::resetMatch() {
    you_ = 0;
    them_ = 0;
    faults_ = 0;
    good_ = 2;
    coin_ = 0;
    dish_ = 0;
    stackN_ = 0;
    logN_ = 0;
    shortSix_ = false;
    won_ = false;
    over_ = false;
    rivalT_ = 0;
    rivalPts_ = 0;
    why_ = "";
    mode_ = Mode::Pick;
}

void Game::blip(float freq, int frames) {
    beepF_ = freq;
    beepV_ = 0.22f;
    beepN_ = frames;
}

int Game::dueOf() const { return (good_ >= 0 && good_ < 3) ? DUE[good_] : 0; }

int Game::rivalWant() const {
    if (them_ + 3 < 7) return 3;
    if (them_ + 1 >= 7) return 1;
    if (them_ + 2 >= 7) return 2;
    return 3;
}

void Game::readInput() {
    act_ = {};
    if (bot_ || !sys_) return;
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) act_.dx = -1;
    else if (p.pressed(gs::BTN_RIGHT)) act_.dx = 1;
    if (p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_A)) act_.drop = true;
    if (p.pressed(gs::BTN_X) || p.pressed(gs::BTN_B)) act_.back = true;
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_Y)) act_.hand = true;
    if (p.pressed(gs::BTN_START)) act_.start = true;
}

void Game::driveBot() {
    if (wait_ > 0) {
        wait_--;
        return;
    }
    if (mode_ == Mode::Title) {
        act_.start = true;
        wait_ = 8;
        return;
    }
    if (mode_ == Mode::Pick) {
        const int want = you_ >= 6 ? 0 : 2;
        if (good_ < want) {
            act_.dx = 1;
            wait_ = 4;
        } else if (good_ > want) {
            act_.dx = -1;
            wait_ = 4;
        } else {
            act_.hand = true;
            wait_ = 6;
        }
        return;
    }
    if (mode_ == Mode::Till) {
        const int need = dueOf();
        if (dish_ == need) {
            act_.hand = true;
            wait_ = 8;
            return;
        }
        const int wantCoin = (need - dish_ >= 2) ? 1 : 0;
        if (coin_ < wantCoin) {
            act_.dx = 1;
            wait_ = 3;
        } else if (coin_ > wantCoin) {
            act_.dx = -1;
            wait_ = 3;
        } else {
            act_.drop = true;
            wait_ = 4;
        }
    }
}

void Game::bank(int pts) {
    if (logN_ < 24) {
        log_[logN_].yours = true;
        log_[logN_].pts = pts;
        logN_++;
    }
    you_ += pts;
    flashK_ = 1;
    flashT_ = 28;
    blip(880.f, 6);
    if (you_ == 6) shortSix_ = true;
    if (you_ >= 7) {
        winDay();
        return;
    }
    beginRival();
}

void Game::miss() {
    faults_++;
    flashK_ = 2;
    flashT_ = 28;
    blip(180.f, 8);
    dish_ = 0;
    stackN_ = 0;
    if (faults_ >= 3) {
        loseDay("wrong change");
        return;
    }
    beginRival();
}

void Game::beginRival() {
    rivalPts_ = rivalWant();
    rivalT_ = 26;
    mode_ = Mode::Rival;
}

void Game::finishRival() {
    if (logN_ < 24) {
        log_[logN_].yours = false;
        log_[logN_].pts = rivalPts_;
        logN_++;
    }
    them_ += rivalPts_;
    blip(440.f, 5);
    if (them_ >= 7 && you_ < 7) {
        loseDay("rival reached seven");
        return;
    }
    dish_ = 0;
    stackN_ = 0;
    coin_ = 0;
    mode_ = Mode::Pick;
}

void Game::winDay() {
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "first to seven";
    blip(660.f, 10);
}

void Game::loseDay(const char* why) {
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    why_ = why;
}

void Game::logic() {
    if (flashT_ > 0) flashT_--;
    if (mode_ == Mode::Title) {
        if (act_.start) resetMatch();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (act_.start) toTitle();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (act_.start) mode_ = held_;
        return;
    }
    if (act_.start && (mode_ == Mode::Pick || mode_ == Mode::Till)) {
        held_ = mode_;
        mode_ = Mode::Pause;
        return;
    }
    if (mode_ == Mode::Pick) {
        if (act_.dx) good_ = std::clamp(good_ + act_.dx, 0, 2);
        if (act_.hand || act_.drop) {
            dish_ = 0;
            stackN_ = 0;
            coin_ = 0;
            mode_ = Mode::Till;
            blip(520.f, 4);
        }
        return;
    }
    if (mode_ == Mode::Till) {
        if (act_.dx) coin_ = std::clamp(coin_ + act_.dx, 0, 1);
        if (act_.drop && stackN_ < 8 && dish_ < 12) {
            stack_[stackN_++] = COIN_V[coin_];
            dish_ += COIN_V[coin_];
            blip(700.f, 3);
        }
        if (act_.back && stackN_ > 0) {
            dish_ -= stack_[--stackN_];
            blip(300.f, 3);
        }
        if (act_.hand) {
            if (dish_ == dueOf() && dueOf() > 0) bank(PTS[good_]);
            else miss();
        }
        return;
    }
    if (mode_ == Mode::Rival) {
        if (rivalT_ > 0) rivalT_--;
        if (rivalT_ == 0) finishRival();
    }
}

void Game::audio() {
    if (!sys_) return;
    if (beepN_ <= 0) return;
    sys_->apu.tone(0, beepF_, beepV_);
    beepV_ *= 0.82f;
    if (--beepN_ == 0) sys_->apu.tone(0, 0, 0);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    readInput();
    if (bot_) driveBot();
    logic();
    audio();
    if (mode_ == Mode::Win) sys.setLight(40, 200, 70);
    else if (mode_ == Mode::Lose) sys.setLight(200, 30, 20);
    else if (you_ == 6) sys.setLight(200, 160, 30);
    else sys.setLight(180, 110, 40);
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

void Game::pips(int n, float x, float y, int pal) {
    n = std::clamp(n, 0, 7);
    for (int i = 0; i < 7; i++) {
        int p = i < n ? pal : PAL_DIM;
        solid(x + float(i) * 10.f, y, 7.f, 7.f, p);
    }
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    const bool lose = mode_ == Mode::Lose;
    const bool won = mode_ == Mode::Win;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 16) c = gs::rgb4(2, 2, 4);
        else if (y < 108) {
            int t = y - 16;
            int r = 8 + t * 4 / 92;
            int g = 9 + t * 2 / 92;
            int b = 14 - t * 6 / 92;
            if (lose) {
                r = std::min(15, r + 3);
                b = std::max(2, b - 3);
            } else if (won) {
                g = std::min(15, g + 3);
            } else if (you_ == 6 && y > 40) {
                r = std::min(15, r + 2);
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

    const float bob = std::sin(age_ * 0.08f);
    spr(art_.post, 28.f, 118.f, 70.f, PAL_STALL, false);
    spr(art_.post, 196.f, 118.f, 70.f, PAL_STALL, false);
    spr(art_.awning, 112.f, 78.f, 34.f, PAL_STALL, false);
    spr(art_.counter, 112.f, 150.f, 32.f, PAL_STALL, false);
    shadeAt(112.f, 166.f, 140.f);
    spr(art_.clerk, 48.f, 132.f + bob, 58.f, PAL_CLERK, false);
    spr(art_.bell, 22.f, 96.f, 16.f, PAL_COIN, false);

    const bool showBuyer = mode_ == Mode::Pick || mode_ == Mode::Till || mode_ == Mode::Pause || mode_ == Mode::Rival;
    if (showBuyer) {
        float bx = mode_ == Mode::Rival ? 250.f : 236.f;
        spr(art_.buyer, bx, 128.f, 52.f, mode_ == Mode::Rival ? PAL_RIVAL : PAL_CLERK, true);
    }
    if (mode_ == Mode::Rival) {
        int pal = rivalPts_ == 3 ? PAL_GOLD : rivalPts_ == 2 ? PAL_LOAF : PAL_CREAM;
        spr(art_.good[rivalPts_ == 1 ? 0 : rivalPts_ == 2 ? 1 : 2], 268.f, 108.f, 22.f, pal, false);
    }

    if (mode_ != Mode::Win) {
        for (int i = 0; i < 3; i++) {
            float lift = (i == good_ && (mode_ == Mode::Pick || mode_ == Mode::Title)) ? 6.f : 0.f;
            int pal = i == 0 ? PAL_CREAM : i == 1 ? PAL_LOAF : PAL_GOLD;
            spr(art_.good[i], GOOD_X[i], GOOD_Y - lift, 26.f, pal, false);
        }
    }

    spr(art_.dish, 118.f, 168.f, 14.f, PAL_STALL, false);
    for (int i = 0; i < stackN_; i++) {
        int kind = stack_[i] >= 2 ? 1 : 0;
        float x = 100.f + float(i % 6) * 8.f;
        float y = 164.f - float(i / 6) * 6.f;
        spr(art_.coin[kind], x, y, 11.f, PAL_COIN, false);
    }

    if (mode_ == Mode::Till || mode_ == Mode::Title || mode_ == Mode::Pause) {
        for (int i = 0; i < 2; i++) {
            float lift = (i == coin_ && mode_ == Mode::Till) ? 5.f : 0.f;
            spr(art_.coin[i], COIN_X[i], COIN_Y - lift, i == 0 ? 16.f : 20.f, PAL_COIN, false);
        }
        if (mode_ == Mode::Till) sprI(art_.bracket, COIN_X[coin_], COIN_Y + 16.f, float(art_.bracket.h), PAL_WARN);
    }

    if (mode_ == Mode::Title) {
        sprI(art_.title, 112.f, 28.f, float(art_.title.h), PAL_HUD);
        sprI(art_.seven, 160.f, 50.f, float(art_.seven.h) * 0.85f, PAL_WARN);
        solid(8.f, 12.f, 200.f, 48.f, PAL_DIM);
    } else if (mode_ == Mode::Win) {
        sprI(art_.winW, 160.f, 70.f, float(art_.winW.h), PAL_OK);
        for (int i = 0; i < 12; i++) {
            float x = std::fmod(float(age_ * 3 + i * 26), 320.f);
            solid(x, 20.f + float((i * 17) % 80), 4.f, 3.f, (i & 1) ? PAL_WARN : PAL_OK);
        }
    } else if (mode_ == Mode::Lose) {
        sprI(art_.loseW, 160.f, 64.f, float(art_.loseW.h), PAL_BAD);
    }

    if (flashT_ > 10) {
        if (flashK_ == 1) sprI(art_.exact, 150.f, 92.f, float(art_.exact.h), PAL_OK);
        else if (flashK_ == 2) sprI(art_.shortW, 150.f, 92.f, float(art_.shortW.h) * 0.8f, PAL_BAD);
    } else if (you_ == 6 && mode_ != Mode::Win && (age_ / 12) % 2 == 0) {
        sprI(art_.shortW, 160.f, 54.f, float(art_.shortW.h) * 0.7f, PAL_WARN);
    }

    digits(you_, 46.f, 40.f, you_ >= 7 ? PAL_OK : you_ == 6 ? PAL_WARN : PAL_HUD);
    digits(them_, 274.f, 40.f, them_ >= 7 ? PAL_BAD : PAL_HUD);
    pips(you_, 16.f, 54.f, PAL_OK);
    pips(std::min(them_, 7), 230.f, 54.f, PAL_BAD);

    if (mode_ == Mode::Till || mode_ == Mode::Pick) {
        int dp = dish_ == 0 ? PAL_HUD : dish_ == dueOf() ? PAL_OK : dish_ > dueOf() ? PAL_BAD : PAL_WARN;
        digits(dueOf(), 250.f, 96.f, PAL_WARN);
        digits(dish_, 292.f, 96.f, dp);
    }

    char line[48];
    std::snprintf(line, sizeof line, "YOU %d  RIVAL %d  FLT %d", you_, them_, faults_);
    hud(0, 0, "S3 MARKET SEVEN");
    hud(18, 0, line);
    if (mode_ == Mode::Win) hud(6, 1, "FIRST TO SEVEN");
    else if (mode_ == Mode::Lose) hud(4, 1, why_ ? why_ : "STALL LOST");
    else if (mode_ == Mode::Rival) {
        std::snprintf(line, sizeof line, "RIVAL SELLS %d", rivalPts_);
        hud(10, 1, line);
    } else if (mode_ == Mode::Pick || mode_ == Mode::Till || mode_ == Mode::Pause) {
        std::snprintf(line, sizeof line, "%s  BANKS %d  CHANGE %d", NAME[good_], PTS[good_], DUE[good_]);
        hud(4, 1, line);
    }
    if (mode_ == Mode::Till || mode_ == Mode::Pick) {
        hudAt(250.f, 9, "DUE");
        hudAt(292.f, 9, "DISH");
    }
    hudAt(GOOD_X[0], 16, "1");
    hudAt(GOOD_X[1], 16, "2");
    hudAt(GOOD_X[2], 16, "3");
    if (mode_ == Mode::Pause) hud(16, 4, "PAUSED");
    if (mode_ == Mode::Title) {
        hud(2, 26, "ARROWS GOOD  C SELL  Z COIN  X BACK");
        if ((age_ / 24) % 2 == 0) hud(8, 27, "ENTER OPENS THE STALL");
    } else if (mode_ == Mode::Pick) {
        hud(4, 27, "ARROWS GOOD   C SELLS IT");
    } else if (mode_ == Mode::Till && dish_ == dueOf()) {
        hud(6, 27, "EXACT CHANGE   PRESS C");
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        hud(8, 27, "ENTER BACK TO THE STALL");
    } else if (mode_ == Mode::Till) {
        hud(2, 27, "ARROWS COIN  Z DROP  X BACK  C HAND");
    }
    hudAt(40.f, 3, "YOU");
    hudAt(274.f, 3, "RIVAL");
}

}  // namespace marketseven
