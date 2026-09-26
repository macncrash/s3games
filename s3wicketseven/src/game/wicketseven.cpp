#include "game/wicketseven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace wicketseven {
namespace {

constexpr int kRun = 48;
constexpr int kPass = 64;
constexpr int kSweet0 = 34;
constexpr int kSweet1 = 48;
constexpr int kBotSwing = 40;

constexpr float kBatX = 98.f;
constexpr float kNearX = 62.f;
constexpr float kFarX = 226.f;
constexpr float kBowlerX = 204.f;
constexpr float kReleaseX = 186.f;
constexpr float kRopeX = 304.f;
constexpr float kKeepX = 40.f;
constexpr float kPi = 3.14159265f;

static_assert(kBotSwing >= kSweet0 && kBotSwing <= kSweet1, "bot swings in the window");

float clampf(float v, float a, float b) { return v < a ? a : (v > b ? v : b); }

float laneFeet(int line) { return 148.f + float(line) * 16.f; }

float depthOf(int line) { return 0.86f + 0.08f * float(line); }

const char* lengthName(int n) { return n == 0 ? "FULL" : n == 2 ? "SHORT" : "GOOD"; }

const char* shotName(int n) { return n == 0 ? "PUSH" : n == 1 ? "TWO" : n == 2 ? "FOUR" : "SIX"; }

const char* lineName(int n) { return n == 0 ? "OFF" : n == 2 ? "LEG" : "MIDDLE"; }

void pips(char* out, int n) {
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    for (int i = 0; i < 7; i++) out[i] = i < n ? '#' : '.';
    out[7] = 0;
}

}  // namespace

Game::Del Game::book() const {
    // Answered straight through: 4-0, 4-2, 6-2, 6-6, 7-6, and the next ball would make them seven.
    static constexpr Del kBook[6] = {
        {1, 0, 2, true},
        {0, 2, 1, false},
        {2, 1, 1, true},
        {1, 0, 2, false},
        {0, 1, 0, true},
        {1, 1, 0, false},
    };
    static_assert(kBook[0].yours && kBook[0].shot == 2 && kBook[0].length == 0, "you four");
    static_assert(!kBook[1].yours && kBook[1].shot == 1 && kBook[1].length == 2, "them two");
    static_assert(kBook[2].yours && kBook[2].shot == 1 && kBook[2].length == 1, "you two");
    static_assert(!kBook[3].yours && kBook[3].shot == 2 && kBook[3].length == 0, "them four");
    static_assert(kBook[4].yours && kBook[4].shot == 0 && kBook[4].length == 1, "you one");
    static_assert(!kBook[5].yours && kBook[5].shot == 0, "them threat");
    static_assert(4 + 2 + 1 == 7 && 2 + 4 == 6, "first to seven, them still short");
    if (ball_ >= 0 && ball_ < 6) return kBook[ball_];
    Del d;
    d.line = 1;
    d.length = 1;
    d.shot = 0;
    d.yours = (ball_ % 2) == 0;
    return d;
}

bool Game::fits(int length, int shot) const {
    if (length == 1) return true;
    if (length == 0) return shot == 0 || shot == 2;
    return shot == 1 || shot == 3;
}

int Game::shotRuns(int shot) const {
    if (shot == 1) return 2;
    if (shot == 2) return 4;
    if (shot == 3) return 6;
    return 1;
}

Game::Call Game::shotCall(int shot) const {
    if (shot == 1) return Call::Two;
    if (shot == 2) return Call::Four;
    if (shot == 3) return Call::Six;
    return Call::One;
}

void Game::tone(int ch, float freq, float vol, int hold) {
    if (!sys_ || ch < 0 || ch > 2) return;
    sys_->apu.tone(ch, freq, vol);
    toneUntil_[ch] = t_ + hold;
}

void Game::pumpAudio() {
    if (!sys_) return;
    for (int ch = 0; ch < 3; ch++) {
        if (toneUntil_[ch] > 0 && t_ >= toneUntil_[ch]) {
            sys_->apu.tone(ch, 0, 0);
            toneUntil_[ch] = 0;
        }
    }
}

bool Game::swingPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
}

bool Game::startPressed() const { return sys_->pad.pressed(gs::BTN_START); }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(8, 12, 15));
    sys.apu.setMaster(0.7f);
    you_ = 0;
    them_ = 0;
    ball_ = 0;
    faced_ = 0;
    line_ = 1;
    shot_ = 2;
    won_ = false;
    over_ = false;
    say_ = "FIRST TO SEVEN";
    mode_ = Mode::Title;
    if (bot_) beginMatch();
}

void Game::beginMatch() {
    you_ = 0;
    them_ = 0;
    ball_ = 0;
    faced_ = 0;
    won_ = false;
    over_ = false;
    hold_ = 0;
    fanStep_ = -1;
    line_ = 1;
    shot_ = 2;
    mode_ = Mode::Play;
    beginBall();
    tone(0, 330.f, 0.05f, 8);
}

void Game::beginBall() {
    phase_ = Phase::Run;
    phaseFrame_ = 0;
    resultT_ = 0;
    attempted_ = false;
    bounced_ = false;
    broke_ = false;
    call_ = Call::None;
    runsThis_ = 0;
    Del d = book();
    if (cpu(d)) {
        line_ = d.line;
        shot_ = d.shot;
    }
    say_ = d.yours ? "YOU BAT" : "THEM BAT";
}

void Game::steer(const Del& d) {
    if (cpu(d)) {
        line_ = d.line;
        shot_ = d.shot;
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_UP)) line_ = std::max(0, line_ - 1);
    if (p.pressed(gs::BTN_DOWN)) line_ = std::min(2, line_ + 1);
    if (p.pressed(gs::BTN_LEFT)) shot_ = std::max(0, shot_ - 1);
    if (p.pressed(gs::BTN_RIGHT)) shot_ = std::min(3, shot_ + 1);
}

void Game::flightPos(const Del& d, int frame, float& x, float& y) const {
    float u = clampf(frame / float(kPass), 0.f, 1.f);
    x = kReleaseX + (kBatX + 4.f - kReleaseX) * u;
    float feet = laneFeet(d.line);
    float yHand = feet - 36.f;
    float yBat = feet - (d.length == 2 ? 28.f : 14.f);
    float yBounce = feet - 3.f;
    float bu = d.length == 0 ? 0.74f : d.length == 2 ? 0.30f : 0.52f;
    if (u < bu) {
        y = yHand + (yBounce - yHand) * (u / bu);
    } else {
        float w = (u - bu) / (1.f - bu);
        float lift = d.length == 2 ? 16.f : d.length == 1 ? 8.f : 2.f;
        y = yBounce + (yBat - yBounce) * w - std::sin(w * kPi) * lift;
    }
}

void Game::resultPos(const Del& d, float u, float& x, float& y) const {
    float x0 = ballX_;
    float y0 = ballY_;
    u = clampf(u, 0.f, 1.f);
    float feet = laneFeet(d.line);
    if (call_ == Call::Six) {
        x = x0 + (kRopeX - x0) * u;
        y = y0 + (46.f - y0) * u - std::sin(u * kPi) * 26.f;
    } else if (call_ == Call::Four) {
        x = x0 + (kRopeX - 8.f - x0) * u;
        y = (feet - 6.f) + (y0 - (feet - 6.f)) * (1.f - u);
    } else if (call_ == Call::Two) {
        x = x0 + 72.f * u;
        y = (feet - 18.f) + (y0 - (feet - 18.f)) * (1.f - u) - std::sin(u * kPi) * 8.f;
    } else if (call_ == Call::One) {
        x = x0 + 38.f * u;
        y = (feet - 14.f) + (y0 - (feet - 14.f)) * (1.f - u) - std::sin(u * kPi) * 4.f;
    } else if (call_ == Call::Caught) {
        x = x0 + (268.f - x0) * u;
        y = y0 + ((feet - 34.f) - y0) * u - std::sin(u * kPi) * 28.f;
    } else if (broke_) {
        x = x0 + (kNearX - x0) * u;
        y = y0 + ((laneFeet(1) - 16.f) - y0) * u;
    } else {
        x = x0 - 18.f * u;
        y = y0 - std::sin(u * kPi) * 12.f;
    }
}

void Game::swing(const Del& d) {
    if (phase_ != Phase::Bowl || attempted_) return;
    attempted_ = true;
    flightPos(d, phaseFrame_, ballX_, ballY_);
    Call c;
    if (line_ != d.line) c = Call::Line;
    else if (phaseFrame_ < kSweet0) c = Call::Soon;
    else if (phaseFrame_ > kSweet1) c = Call::Late;
    else if (!fits(d.length, shot_)) c = shot_ == 3 ? Call::Caught : Call::Misread;
    else c = shotCall(shot_);
    enterResult(d, c);
}

void Game::enterResult(const Del& d, Call c) {
    call_ = c;
    phase_ = Phase::Result;
    resultT_ = 0;
    attempted_ = true;
    faced_ = ball_ + 1;
    int add = 0;
    const char* word = "BEATEN";
    if (c == Call::One) {
        add = 1;
        word = "ONE";
    } else if (c == Call::Two) {
        add = 2;
        word = "TWO";
    } else if (c == Call::Four) {
        add = 4;
        word = "FOUR";
    } else if (c == Call::Six) {
        add = 6;
        word = "SIX";
    } else if (c == Call::Soon) {
        word = "TOO SOON";
    } else if (c == Call::Late) {
        word = "TOO LATE";
    } else if (c == Call::Line) {
        word = "WRONG LINE";
    } else if (c == Call::Misread) {
        word = "MISREAD";
    } else if (c == Call::Caught) {
        word = "CAUGHT";
    }
    runsThis_ = add;
    if (d.yours) you_ += add;
    else them_ += add;
    say_ = word;
    broke_ = add == 0 && c != Call::Caught && d.line == 1;
    anim_ = add > 0 ? 46 : 32;
    total_ = anim_ + 14;
    if (add >= 4) {
        tone(0, 523.f, 0.06f, 8);
        tone(1, 784.f, 0.04f, 10);
        if (sys_) sys_->apu.noiseBurst(0.2f, 900.f, 0.07f);
    } else if (add > 0) {
        tone(0, 440.f, 0.05f, 7);
        if (sys_) sys_->apu.noiseBurst(0.12f, 640.f, 0.05f);
    } else {
        tone(0, 110.f, 0.05f, 10);
        if (sys_) sys_->apu.noiseBurst(0.16f, 180.f, 0.08f);
    }
    if (sys_ && !bot_ && add > 0) sys_->rumble(0.2f, 0.45f, 70);
}

void Game::stepResult() {
    if (runsThis_ >= 4 && (resultT_ % 8) == 0) {
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        int q = resultT_ / 8;
        if (q >= 0 && q < 4) tone(0, notes[q], 0.05f, 7);
    }
    if (resultT_ >= total_ - 1) {
        advance();
        return;
    }
    resultT_++;
}

void Game::advance() {
    // Six does not finish the ground. Leave only when a side has seven.
    if (you_ >= 7 || them_ >= 7) {
        finish(you_ >= 7 && them_ < 7);
        return;
    }
    ball_++;
    beginBall();
}

void Game::finish(bool win) {
    won_ = win && you_ >= 7 && them_ < 7;
    over_ = true;
    mode_ = won_ ? Mode::Leave : Mode::Lose;
    say_ = won_ ? "LEAVE" : (them_ >= 7 ? "THEY REACHED SEVEN" : "STILL SHORT");
    hold_ = 0;
    fanStep_ = -1;
    if (!sys_) return;
    if (won_) {
        tone(0, 523.f, 0.07f, 14);
        tone(1, 659.f, 0.05f, 16);
        tone(2, 784.f, 0.04f, 18);
        sys_->setLight(255, 196, 48);
        if (!bot_) sys_->rumble(0.35f, 0.7f, 140);
    } else {
        tone(0, 130.f, 0.06f, 16);
        tone(1, 98.f, 0.04f, 18);
        sys_->setLight(160, 32, 28);
    }
}

void Game::stepEnd() {
    hold_++;
    if (won_) {
        static const float notes[] = {392.f, 523.f, 659.f, 784.f, 1046.f};
        int step = hold_ / 9;
        if (step != fanStep_ && step >= 0 && step < 5) {
            fanStep_ = step;
            tone(0, notes[step], 0.06f, 8);
        }
    }
    if (!bot_ && (startPressed() || swingPressed())) beginMatch();
}

void Game::tick() {
    if (!sys_) return;
    if (mode_ == Mode::Title) {
        if (startPressed() || swingPressed()) beginMatch();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (startPressed() || swingPressed()) mode_ = Mode::Play;
        return;
    }
    if (mode_ == Mode::Leave || mode_ == Mode::Lose) {
        stepEnd();
        return;
    }
    if (!bot_ && startPressed()) {
        mode_ = Mode::Pause;
        return;
    }
    Del d = book();
    if (phase_ == Phase::Run) {
        steer(d);
        if (phaseFrame_ >= kRun) {
            phase_ = Phase::Bowl;
            phaseFrame_ = 0;
            bounced_ = false;
            tone(1, 720.f, 0.03f, 3);
        } else {
            phaseFrame_++;
        }
        return;
    }
    if (phase_ == Phase::Bowl) {
        if (!attempted_) steer(d);
        float bu = d.length == 0 ? 0.74f : d.length == 2 ? 0.30f : 0.52f;
        int bounceAt = int(bu * float(kPass));
        if (!bounced_ && phaseFrame_ == bounceAt) {
            bounced_ = true;
            sys_->apu.noiseBurst(0.08f, 280.f, 0.04f);
        }
        if (!attempted_) {
            if (cpu(d) && phaseFrame_ == kBotSwing) swing(d);
            else if (!cpu(d) && swingPressed()) swing(d);
        }
        if (phase_ == Phase::Bowl && !attempted_ && phaseFrame_ >= kPass) {
            flightPos(d, phaseFrame_, ballX_, ballY_);
            enterResult(d, Call::Beaten);
            return;
        }
        if (phase_ == Phase::Bowl) phaseFrame_++;
        return;
    }
    stepResult();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    tick();
    pumpAudio();
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
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

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip) {
    if (!sys_ || img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 72) {
            int r = 5 + y * 4 / 72;
            int g = 8 + y * 4 / 72;
            int b = 14 - y * 2 / 72;
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else if (y < 104) {
            v.lineBackdrop[y] = gs::rgb4(2, 6, 3);
        } else {
            int band = (y / 4) & 1;
            v.lineBackdrop[y] = gs::rgb4(2 + band, 9 + band, 2);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    const bool title = mode_ == Mode::Title;
    const bool ended = mode_ == Mode::Leave || mode_ == Mode::Lose;
    const bool live = mode_ == Mode::Play || mode_ == Mode::Pause;
    Del d = book();
    float feetYou = laneFeet(title ? 1 : line_);
    float feetBall = laneFeet(title ? 1 : d.line);
    float batScale = title ? 1.f : depthOf(line_);
    float bowlScale = title ? 1.f : depthOf(d.line);

    auto feetSpr = [&](const gs::Image& img, float cx, float fy, float w, float h, int pal, bool flip = false) {
        spr(img, cx, fy - h * 0.5f, w, h, pal, flip);
    };
    auto word = [&](const gs::Image& img, float y, int pal) {
        spr(img, 160.f, y, float(img.w), float(img.h), pal);
    };

    int pose = 0;
    if (shot_ == 3) pose = 3;
    else if (shot_ == 2) pose = 2;
    else if (shot_ == 1 || shot_ == 0) pose = 1;
    if (title) pose = 0;
    if (ended && mode_ == Mode::Leave) pose = 1;

    int bowlPose = 0;
    float bowlerX = kBowlerX;
    if (title) {
        bowlPose = (t_ / 8) % 2;
    } else if (live && phase_ == Phase::Run) {
        bowlPose = (phaseFrame_ / 6) % 2;
        float ur = clampf(phaseFrame_ / float(kRun), 0.f, 1.f);
        bowlerX = 250.f + (kBowlerX - 250.f) * ur;
    } else if (live && phase_ == Phase::Bowl && phaseFrame_ < 18) {
        bowlPose = 2;
    }

    float ballX = bowlerX - 18.f;
    float ballY = feetBall - 34.f;
    bool drawBall = true;
    if (title) {
        ballX = kBowlerX - 16.f;
        ballY = laneFeet(1) - 36.f + std::sin(t_ * 0.08f) * 2.f;
    } else if (live && phase_ == Phase::Run) {
        ballX = bowlerX - 18.f;
        ballY = feetBall - 34.f;
    } else if (live && phase_ == Phase::Bowl && !attempted_) {
        flightPos(d, phaseFrame_, ballX, ballY);
    } else if (call_ != Call::None) {
        float u = 1.f;
        if (!ended && phase_ == Phase::Result) u = std::min(1.f, resultT_ / float(std::max(1, anim_ - 1)));
        resultPos(d, u, ballX, ballY);
    } else {
        drawBall = false;
    }

    float shotU = 0.f;
    if (call_ != Call::None) {
        if (ended || phase_ != Phase::Result) shotU = 1.f;
        else shotU = std::min(1.f, resultT_ / float(std::max(1, anim_ - 1)));
    }
    float bailFly = 0.f;
    float bailSide = 0.f;
    if (broke_ && shotU > 0.55f) {
        float w = clampf((shotU - 0.55f) / 0.45f, 0.f, 1.f);
        bailFly = std::sin(w * kPi) * 16.f;
        bailSide = w * 10.f;
    }

    const gs::Image* banner = nullptr;
    int bannerPal = PAL_INK;
    if (title) {
        word(art_.wicketWord, 16.f, PAL_TITLE);
        word(art_.sevenWord, 42.f, PAL_GOLD);
    } else if (mode_ == Mode::Leave) {
        word(art_.leaveWord, 108.f, PAL_GOLD);
        word(art_.ruleWord, 132.f, PAL_TITLE);
    } else if (mode_ == Mode::Lose) {
        word(art_.ruleWord, 108.f, PAL_RED);
    } else if (phase_ == Phase::Result) {
        if (call_ == Call::One) {
            banner = &art_.oneWord;
            bannerPal = PAL_INK;
        } else if (call_ == Call::Two) {
            banner = &art_.twoWord;
            bannerPal = PAL_TITLE;
        } else if (call_ == Call::Four) {
            banner = &art_.fourWord;
            bannerPal = PAL_GREEN;
        } else if (call_ == Call::Six) {
            banner = &art_.sixWord;
            bannerPal = PAL_GOLD;
        } else if (call_ == Call::Caught) {
            banner = &art_.caughtWord;
            bannerPal = PAL_RED;
        } else if (call_ == Call::Beaten) {
            banner = &art_.beatenWord;
            bannerPal = PAL_RED;
        } else if (call_ == Call::Soon) {
            banner = &art_.soonWord;
            bannerPal = PAL_RED;
        } else if (call_ == Call::Late) {
            banner = &art_.lateWord;
            bannerPal = PAL_RED;
        } else if (call_ == Call::Line) {
            banner = &art_.lineWord;
            bannerPal = PAL_RED;
        } else if (call_ == Call::Misread) {
            banner = &art_.misWord;
            bannerPal = PAL_RED;
        }
        if (banner) word(*banner, 34.f, bannerPal);
    }

    if (live && phase_ == Phase::Bowl) {
        const float x0 = 72.f, x1 = 250.f, yb = 210.f;
        float u0 = kSweet0 / float(kPass);
        float u1 = (kSweet1 + 1) / float(kPass);
        float nu = clampf(phaseFrame_ / float(kPass), 0.f, 1.f);
        bool ready = fits(d.length, shot_) && line_ == d.line;
        spr(art_.blot, x0 + nu * (x1 - x0), yb, 4, 14, PAL_GOLD);
        float sw = (u1 - u0) * (x1 - x0);
        float sx = x0 + (u0 + u1) * 0.5f * (x1 - x0);
        spr(art_.blot, sx, yb, sw, 8, ready ? PAL_GREEN : PAL_RED);
        spr(art_.blot, (x0 + x1) * 0.5f, yb, x1 - x0, 4, PAL_INK);
    }

    float lampY = title ? 100.f : 58.f;
    for (int i = 0; i < 7; i++) {
        bool onY = i < you_;
        bool onT = i < them_;
        spr(art_.blot, 30.f + i * 12.f, lampY, onY ? 8.f : 5.f, onY ? 8.f : 5.f, onY ? PAL_GOLD : PAL_SHADE);
        spr(art_.blot, 206.f + i * 12.f, lampY, onT ? 8.f : 5.f, onT ? 8.f : 5.f, onT ? PAL_RED : PAL_SHADE);
    }

    if (drawBall) {
        int seam = ((phase_ == Phase::Bowl ? phaseFrame_ : t_) / 3) & 1;
        float bh = 13.f * (title ? 1.f : bowlScale);
        spr(art_.ball[seam], ballX, ballY, bh, bh, PAL_BALL);
    }
    if (broke_ && shotU > 0.55f) {
        float sy = laneFeet(1) - 40.f - bailFly;
        spr(art_.bail, kNearX - 6.f - bailSide, sy, 8, 4, PAL_STUMP);
        spr(art_.bail, kNearX + 6.f + bailSide, sy, 8, 4, PAL_STUMP);
    }

    int strikerPal = (!title && !d.yours) ? PAL_BOWL : PAL_BAT;
    int bowlerPal = (!title && !d.yours) ? PAL_BAT : PAL_BOWL;
    float walk = mode_ == Mode::Leave ? clampf(hold_ * 0.7f, 0.f, 80.f) : 0.f;
    float batH = 60.f * batScale;
    float batW = batH * (48.f / 60.f);
    float bowlH = 56.f * bowlScale;
    float bowlW = bowlH * (48.f / 60.f);
    feetSpr(art_.batsman[pose], kBatX - walk, feetYou, batW, batH, strikerPal);
    feetSpr(art_.bowler[bowlPose], bowlerX, feetBall, bowlW, bowlH, bowlerPal);
    feetSpr(art_.bowler[0], kKeepX, laneFeet(1) + 2.f, 30, 40, bowlerPal, true);
    int fieldPose = call_ == Call::Caught ? 2 : 0;
    feetSpr(art_.bowler[fieldPose], 274.f, feetBall + 4.f, 28, 38, bowlerPal);
    feetSpr(art_.stump[broke_ ? 1 : 0], kNearX, laneFeet(1), 28, 48, PAL_STUMP);
    feetSpr(art_.stump[0], kFarX, laneFeet(1) + 2.f, 22, 38, PAL_STUMP);

    spr(art_.shadow, kBatX - walk, feetYou + 2.f, 24, 7, PAL_SHADE);
    spr(art_.shadow, bowlerX, feetBall + 2.f, 20, 6, PAL_SHADE);

    if (live && (phase_ == Phase::Run || phase_ == Phase::Bowl)) {
        float markX = d.length == 0 ? 118.f : d.length == 2 ? 176.f : 148.f;
        spr(art_.blot, markX, feetBall - 2.f, 10, 4, PAL_GOLD);
        spr(art_.blot, kBatX, feetYou - 1.f, 16, 3, line_ == d.line ? PAL_GREEN : PAL_RED);
    }

    spr(art_.rope, kRopeX, 150.f, 34, 128, PAL_ROPE);
    spr(art_.pitch, 156.f, 172.f, 236, 82, PAL_PITCH);
    spr(art_.screen, 276.f, 78.f, 48, 36, PAL_HOUSE);
    spr(art_.crowd, 78.f, 112.f, 120, 24, PAL_CROWD);
    spr(art_.crowd, 188.f, 108.f, 100, 22, PAL_CROWD);
    spr(art_.house, 54.f, 96.f, 96, 50, PAL_HOUSE);
    const float trees[] = {18.f, 150.f, 214.f, 308.f};
    const float treeY[] = {102.f, 98.f, 104.f, 110.f};
    for (int i = 0; i < 4; i++) spr(art_.tree, trees[i], treeY[i] - 18.f, 46, 40, PAL_TREE);
    float drift = float(t_ % 480) * 0.12f;
    spr(art_.cloud, 54.f + drift * 0.15f, 20.f, 44, 16, PAL_CLOUD);
    spr(art_.cloud, 168.f - drift * 0.08f, 30.f, 36, 14, PAL_CLOUD);
    spr(art_.sun, 292.f, 18.f, 18, 18, PAL_GOLD);

    char buf[64];
    char py[8];
    char pt[8];
    pips(py, you_);
    pips(pt, them_);
    hud(1, 0, "S3 WICKET SEVEN", PAL_TITLE);
    if (!title) {
        std::snprintf(buf, sizeof buf, "BALL %d", ball_ + 1);
        hud(40 - int(std::strlen(buf)) - 1, 0, buf, PAL_INK);
        std::snprintf(buf, sizeof buf, "YOU %d %s", you_, py);
        hud(1, 1, buf, PAL_GREEN);
        std::snprintf(buf, sizeof buf, "THEM %d %s", them_, pt);
        hud(40 - int(std::strlen(buf)) - 1, 1, buf, PAL_RED);
        hudC(2, "FIRST TO SEVEN", you_ >= 7 ? PAL_GOLD : PAL_INK);
    }

    if (title) {
        hudC(20, "FIRST TO SEVEN. THEN LEAVE.", PAL_GOLD);
        hudC(21, "A SIX IS STILL SHORT.", PAL_INK);
        hudC(23, "FULL PUSH OR FOUR", PAL_GOLD);
        hudC(24, "GOOD ANY SHOT", PAL_GREEN);
        hudC(25, "SHORT TWO OR SIX", PAL_RED);
        hudC(26, "U-D LINE   L-R SHOT   Z SWING", PAL_INK);
        hudC(27, "ENTER STARTS", PAL_GREEN);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
        hudC(14, "ENTER RESUMES", PAL_INK);
    } else if (mode_ == Mode::Leave) {
        std::snprintf(buf, sizeof buf, "YOU %d   THEM %d", you_, them_);
        hudC(16, buf, PAL_GOLD);
        hudC(18, "LEAVE THE GROUND", PAL_TITLE);
        hudC(26, "ENTER PLAYS AGAIN", PAL_INK);
    } else if (mode_ == Mode::Lose) {
        std::snprintf(buf, sizeof buf, "YOU %d   THEM %d", you_, them_);
        hudC(14, buf, PAL_RED);
        hudC(16, say_, PAL_INK);
        hudC(18, "STILL SHORT OF SEVEN", PAL_RED);
        hudC(26, "ENTER PLAYS AGAIN", PAL_INK);
    } else {
        int lp = d.length == 0 ? PAL_GOLD : d.length == 2 ? PAL_RED : PAL_GREEN;
        int sp = fits(d.length, shot_) ? PAL_GREEN : PAL_RED;
        hud(1, 24, lengthName(d.length), lp);
        hud(8, 24, shotName(shot_), sp);
        std::snprintf(buf, sizeof buf, "YOU %s", lineName(line_));
        hud(1, 25, buf, line_ == d.line ? PAL_GREEN : PAL_RED);
        std::snprintf(buf, sizeof buf, "BALL %s", lineName(d.line));
        hud(16, 25, buf, PAL_INK);
        hudC(22, d.yours ? "YOU BAT" : "THEM BAT", d.yours ? PAL_GREEN : PAL_RED);
        if (phase_ == Phase::Result) hudC(23, say_, runsThis_ > 0 ? PAL_GOLD : PAL_RED);
        else if (phase_ == Phase::Bowl && phaseFrame_ >= kSweet0 && phaseFrame_ <= kSweet1 && line_ == d.line &&
                 fits(d.length, shot_) && d.yours && !bot_)
            hudC(23, "SWING", ((t_ / 4) & 1) ? PAL_GREEN : PAL_GOLD);
        else if (phase_ == Phase::Run)
            hudC(23, "READ THE BALL", PAL_INK);
        const char* hint = d.length == 0 ? "FULL TAKES PUSH OR FOUR" :
                           d.length == 2 ? "SHORT TAKES TWO OR SIX" :
                                           "GOOD TAKES ANY SHOT";
        hudC(27, hint, lp);
    }
}

}  // namespace wicketseven
