#include "game/wicketgold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace wicketgold {
namespace {

constexpr int kRun = 36;
constexpr int kSweet0 = 40;
constexpr int kSweet1 = 52;
constexpr int kBotSwing = 46;
constexpr int kPass = 64;
constexpr int kBalls = 6;

constexpr float kBatX = 112.f;
constexpr float kReleaseX = 184.f;
constexpr float kMeetX = 104.f;
constexpr float kNearX = 90.f;
constexpr float kFarX = 218.f;
constexpr float kBowlerX = 198.f;
constexpr float kRopeX = 300.f;
constexpr float kRopeY = 132.f;
constexpr float kPitchX = 156.f;
constexpr float kPitchY = 156.f;
constexpr float kPi = 3.14159265f;

struct Del {
    int line;    // 0 off, 1 middle, 2 leg
    int length;  // 0 full, drive the wicket; 1 short, loft the rope
    int gold;    // 1 counts two, 0 counts one
};

// Cream, gold, cream, gold finishes a perfect over on 6.
constexpr Del kOver[kBalls] = {{1, 0, 0}, {2, 1, 1}, {0, 0, 0}, {1, 1, 1}, {2, 0, 1}, {0, 1, 0}};

float laneFeet(int line) { return 152.f + float(line) * 10.f; }

const char* lineName(int line) { return line == 0 ? "OFF" : line == 1 ? "MIDDLE" : "LEG"; }

float clampf(float v, float a, float b) { return v < a ? a : (v > b ? v : b); }

const Del& delivery(int ball) {
    if (ball < 0) ball = 0;
    if (ball >= kBalls) ball = kBalls - 1;
    return kOver[ball];
}

}  // namespace

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

bool Game::paid() const {
    return finisherGold_ && gold_ >= 1 && score_ >= kLine && bare() < kLine && score_ == gold_ * 2 + cream_;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(6, 10, 14));
    sys.apu.setMaster(0.7f);
    ball_ = 0;
    linePick_ = 1;
    loft_ = false;
    call_ = Call::None;
    say_ = "SHORT WICKET";
    attempted_ = false;
    for (int i = 0; i < kBalls; i++) card_[i] = '.';
    if (bot_) beginOver();
    else mode_ = Mode::Title;
}

void Game::beginOver() {
    ball_ = 0;
    gold_ = 0;
    cream_ = 0;
    score_ = 0;
    refused_ = 0;
    faced_ = 0;
    won_ = false;
    over_ = false;
    finisherGold_ = false;
    linePick_ = 1;
    loft_ = false;
    say_ = "SHORT WICKET";
    for (int i = 0; i < kBalls; i++) card_[i] = '.';
    mode_ = Mode::Play;
    beginBall();
    tone(0, 330.f, 0.05f, 6);
}

void Game::beginBall() {
    phase_ = Phase::Run;
    phaseFrame_ = 0;
    resultT_ = 0;
    attempted_ = false;
    bounced_ = false;
    call_ = Call::None;
    const Del& d = delivery(ball_);
    if (bot_) {
        linePick_ = d.line;
        loft_ = d.length == 1;
    }
}

void Game::steer() {
    const Del& d = delivery(ball_);
    if (bot_) {
        linePick_ = d.line;
        loft_ = d.length == 1;
        return;
    }
    if (!sys_) return;
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_UP)) linePick_ = std::max(0, linePick_ - 1);
    if (p.pressed(gs::BTN_DOWN)) linePick_ = std::min(2, linePick_ + 1);
    if (p.pressed(gs::BTN_LEFT)) loft_ = false;
    if (p.pressed(gs::BTN_RIGHT)) loft_ = true;
}

void Game::flightPos(int frame, float& x, float& y) const {
    const Del& d = delivery(ball_);
    float u = clampf(frame / float(kPass), 0.f, 1.f);
    x = kReleaseX + (kMeetX - kReleaseX) * u;
    float feet = laneFeet(d.line);
    float yHand = feet - 32.f;
    float yBat = feet - (d.length ? 26.f : 14.f);
    float yBounce = feet - 3.f;
    float bu = d.length ? 0.30f : 0.70f;
    if (u < bu) {
        y = yHand + (yBounce - yHand) * (u / bu);
    } else {
        float w = (u - bu) / (1.f - bu);
        float lift = d.length ? 18.f : 4.f;
        y = yBounce + (yBat - yBounce) * w - std::sin(w * kPi) * lift;
    }
}

void Game::resultPos(float u, float& x, float& y) const {
    const Del& d = delivery(ball_);
    float x0 = ballX_;
    float y0 = ballY_;
    u = clampf(u, 0.f, 1.f);
    bool scored = call_ == Call::Gold || call_ == Call::Cream || call_ == Call::Refuse;
    if (scored && d.length == 0) {
        x = x0 + (kFarX - x0) * u;
        y = y0 + ((laneFeet(1) - 16.f) - y0) * u - std::sin(u * kPi) * 7.f;
    } else if (scored && d.length == 1) {
        x = x0 + (kRopeX + 6.f - x0) * u;
        y = y0 + (78.f - y0) * u - std::sin(u * kPi) * 26.f;
    } else if (call_ == Call::Bowled) {
        x = x0 + (kNearX - x0) * u;
        y = y0 + ((laneFeet(1) - 14.f) - y0) * u;
    } else if (call_ == Call::Sky) {
        x = x0 + (150.f - x0) * u;
        y = y0 - std::sin(u * kPi) * 48.f;
    } else if (call_ == Call::Block) {
        x = x0 + 18.f * u;
        y = laneFeet(d.line) - 6.f - std::sin(u * kPi) * 8.f;
    } else {
        x = x0 + (kNearX - 10.f - x0) * u;
        y = y0 + ((laneFeet(d.line) - 10.f) - y0) * u;
    }
}

float Game::shotU() const {
    if (call_ == Call::None) return 0.f;
    if (mode_ == Mode::Win || mode_ == Mode::Lose) return 1.f;
    if (phase_ != Phase::Result) return 0.f;
    int anim = 28;
    if (call_ == Call::Gold) anim = 54;
    else if (call_ == Call::Cream || call_ == Call::Refuse) anim = 44;
    else if (call_ == Call::Bowled) anim = 40;
    return std::min(1.f, resultT_ / float(std::max(1, anim - 1)));
}

void Game::enterResult(Call c) {
    call_ = c;
    phase_ = Phase::Result;
    resultT_ = 0;
    faced_ = ball_ + 1;
    char mark = 'x';
    if (c == Call::Gold) {
        gold_++;
        score_ += 2;
        mark = '2';
        say_ = "DOUBLE";
        if (score_ >= kLine) finisherGold_ = true;
        tone(0, 523.f, 0.07f, 8);
        tone(1, 659.f, 0.05f, 10);
        sys_->apu.noiseBurst(0.22f, 900.f, 0.08f);
        if (!bot_) sys_->rumble(0.25f, 0.55f, 80);
    } else if (c == Call::Cream) {
        cream_++;
        score_ += 1;
        mark = '1';
        say_ = "CREAM";
        tone(0, 392.f, 0.06f, 8);
        sys_->apu.noiseBurst(0.12f, 700.f, 0.05f);
    } else if (c == Call::Refuse) {
        refused_++;
        mark = 'R';
        say_ = "NOT DOUBLE";
        tone(0, 110.f, 0.06f, 12);
        tone(1, 92.f, 0.04f, 14);
    } else if (c == Call::Bowled) {
        mark = 'B';
        say_ = "BOWLED";
        tone(0, 98.f, 0.07f, 12);
        sys_->apu.noiseBurst(0.28f, 180.f, 0.1f);
    } else if (c == Call::Soon) {
        say_ = "TOO SOON";
        tone(0, 220.f, 0.05f, 6);
    } else if (c == Call::Late) {
        say_ = "TOO LATE";
        tone(0, 180.f, 0.05f, 6);
    } else if (c == Call::Sky) {
        say_ = "SKIED";
        tone(0, 240.f, 0.05f, 7);
        sys_->apu.noiseBurst(0.1f, 600.f, 0.05f);
    } else if (c == Call::Block) {
        say_ = "BLOCKED";
        tone(0, 160.f, 0.05f, 6);
        sys_->apu.noiseBurst(0.1f, 400.f, 0.04f);
    } else {
        say_ = "BEATEN";
        tone(0, 140.f, 0.04f, 6);
    }
    if (ball_ >= 0 && ball_ < kBalls) card_[ball_] = mark;
}

void Game::swing() {
    if (phase_ != Phase::Bowl || attempted_) return;
    attempted_ = true;
    flightPos(phaseFrame_, ballX_, ballY_);
    const Del& d = delivery(ball_);
    Call c;
    if (linePick_ != d.line) c = Call::Bowled;
    else if (phaseFrame_ < kSweet0) c = Call::Soon;
    else if (phaseFrame_ > kSweet1) c = Call::Late;
    else if (loft_ != (d.length == 1)) c = loft_ ? Call::Sky : Call::Block;
    else if (!d.gold && score_ + 1 >= kLine) c = Call::Refuse;
    else c = d.gold ? Call::Gold : Call::Cream;
    enterResult(c);
}

void Game::stepResult() {
    int anim = 28;
    if (call_ == Call::Gold) anim = 54;
    else if (call_ == Call::Cream || call_ == Call::Refuse) anim = 44;
    else if (call_ == Call::Bowled) anim = 40;
    int total = anim + 18;
    if (call_ == Call::Gold) {
        int q = resultT_ / 8;
        if (q >= 0 && q < 4 && resultT_ % 8 == 0) {
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
            tone(0, notes[q], 0.06f, 7);
        }
    }
    if (resultT_ >= total - 1) {
        advance();
        return;
    }
    resultT_++;
}

void Game::advance() {
    if (paid()) {
        finish(true);
        return;
    }
    if (ball_ + 1 >= kBalls) {
        finish(false);
        return;
    }
    ball_++;
    beginBall();
}

void Game::finish(bool win) {
    won_ = win;
    over_ = true;
    mode_ = win ? Mode::Win : Mode::Lose;
    fanStep_ = -1;
    hold_ = 0;
    say_ = win ? "DOUBLE" : "NO DOUBLE";
    if (win) {
        tone(0, 523.f, 0.08f, 12);
        tone(1, 659.f, 0.05f, 14);
        tone(2, 784.f, 0.04f, 16);
        sys_->setLight(255, 190, 40);
        if (!bot_) sys_->rumble(0.4f, 0.7f, 160);
    } else {
        tone(0, 130.f, 0.06f, 16);
        tone(1, 98.f, 0.04f, 18);
        sys_->setLight(160, 30, 24);
    }
}

void Game::stepEnd() {
    hold_++;
    if (mode_ == Mode::Win) {
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        int step = hold_ / 10;
        if (step != fanStep_ && step >= 0 && step < 4) {
            fanStep_ = step;
            tone(0, notes[step], 0.06f, 9);
        }
    }
    if (!bot_ && (startPressed() || swingPressed())) beginOver();
}

void Game::tick() {
    if (mode_ == Mode::Title) {
        if (startPressed() || swingPressed()) beginOver();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (startPressed() || swingPressed()) mode_ = Mode::Play;
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        stepEnd();
        return;
    }
    if (!bot_ && startPressed()) {
        mode_ = Mode::Pause;
        return;
    }
    if (phase_ == Phase::Run) {
        steer();
        if (phaseFrame_ >= kRun) {
            phase_ = Phase::Bowl;
            phaseFrame_ = 0;
            tone(1, 640.f, 0.035f, 4);
            sys_->apu.noiseBurst(0.05f, 480.f, 0.04f);
        } else {
            phaseFrame_++;
        }
        return;
    }
    if (phase_ == Phase::Bowl) {
        if (!attempted_) steer();
        const Del& d = delivery(ball_);
        int bounceAt = int((d.length ? 0.30f : 0.70f) * kPass);
        if (!bounced_ && phaseFrame_ == bounceAt) {
            bounced_ = true;
            sys_->apu.noiseBurst(0.08f, 320.f, 0.04f);
        }
        if (!attempted_) {
            if (bot_ && phaseFrame_ >= kBotSwing && phaseFrame_ <= kSweet1) swing();
            else if (!bot_ && swingPressed()) swing();
        }
        if (phase_ == Phase::Bowl && !attempted_ && phaseFrame_ >= kPass) {
            flightPos(phaseFrame_, ballX_, ballY_);
            enterResult(Call::Beaten);
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
        if (y < 96) {
            int r = 5 + y * 5 / 96;
            int g = 8 + y * 5 / 96;
            int b = 13 + y * 2 / 96;
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else {
            int mow = (y / 5) & 1;
            v.lineBackdrop[y] = gs::rgb4(2 + mow, 9 + mow, 2);
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
    const bool ended = mode_ == Mode::Win || mode_ == Mode::Lose;
    const bool live = mode_ == Mode::Play || mode_ == Mode::Pause;
    const Del& d = delivery(ball_);
    int clock = int(sys_->frame);
    bool showGold = title ? ((clock / 36) & 1) : d.gold != 0;
    float feet = laneFeet(title ? 1 : linePick_);

    auto feetSpr = [&](const gs::Image& img, float cx, float fy, float w, float h, int pal, bool flip = false) {
        spr(img, cx, fy - h * 0.5f, w, h, pal, flip);
    };
    auto banner = [&](const gs::Image& img, float y, int pal) {
        spr(img, 160.f, y, float(img.w), float(img.h), pal);
    };

    float ballX = kReleaseX;
    float ballY = laneFeet(d.line) - 32.f;
    bool drawBall = true;
    if (title) {
        ballX = kBowlerX - 18.f;
        ballY = laneFeet(1) - 34.f + std::sin(clock * 0.08f) * 2.f;
    } else if (phase_ == Phase::Run && !ended) {
        float u = clampf(phaseFrame_ / float(kRun), 0.f, 1.f);
        float bx = 258.f + (kBowlerX - 258.f) * u;
        ballX = bx - 16.f;
        ballY = laneFeet(d.line) - 32.f;
    } else if (phase_ == Phase::Bowl && !ended && !attempted_) {
        flightPos(phaseFrame_, ballX, ballY);
    } else if (call_ != Call::None) {
        resultPos(shotU(), ballX, ballY);
    } else {
        drawBall = false;
    }

    float uShot = shotU();
    bool drive = (call_ == Call::Gold || call_ == Call::Cream) && d.length == 0;
    bool farBroke = drive && uShot > 0.72f;
    bool nearBroke = call_ == Call::Bowled && uShot > 0.72f;
    float fly = 0.f;
    float bailSide = 0.f;
    if (farBroke || nearBroke) {
        float w = clampf((uShot - 0.72f) / 0.28f, 0.f, 1.f);
        fly = std::sin(w * kPi) * 16.f;
        bailSide = w * 10.f;
    }

    int pose = loft_ ? 2 : 0;
    if (attempted_ && (phase_ == Phase::Result || ended)) {
        if (call_ == Call::Bowled || call_ == Call::Soon || call_ == Call::Late || call_ == Call::Beaten) pose = 3;
        else pose = loft_ ? 2 : 1;
    }
    if (title) pose = 0;

    int bowlPose = 2;
    float bowlerX = kBowlerX;
    if (title) {
        bowlPose = (clock / 8) & 1;
    } else if (phase_ == Phase::Run && !ended) {
        bowlPose = (phaseFrame_ / 6) & 1;
        float ur = clampf(phaseFrame_ / float(kRun), 0.f, 1.f);
        bowlerX = 258.f + (kBowlerX - 258.f) * ur;
    } else if (phase_ == Phase::Bowl && !attempted_) {
        bowlPose = 2;
    }

    int ump = 0;
    if (call_ == Call::Gold || mode_ == Mode::Win) ump = 1;
    else if (call_ == Call::Refuse) ump = 2;

    if (title) {
        banner(art_.wordWicket, 28.f, PAL_GOLDTEXT);
        banner(art_.wordGold, 54.f, PAL_GOLDTEXT);
    } else if (mode_ == Mode::Win) {
        banner(art_.wordDouble, 30.f, PAL_GOLDTEXT);
        banner(art_.wordGold, 54.f, PAL_GOLDTEXT);
    } else if (mode_ == Mode::Lose) {
        banner(art_.wordNot, 36.f, PAL_RED);
    } else if (phase_ == Phase::Result) {
        if (call_ == Call::Gold) banner(art_.wordDouble, 32.f, PAL_GOLDTEXT);
        else if (call_ == Call::Cream) banner(d.length ? art_.wordRope : art_.wordWicket, 32.f, PAL_INK);
        else if (call_ == Call::Refuse) banner(art_.wordNot, 32.f, PAL_RED);
        else if (call_ == Call::Bowled) banner(art_.wordBowled, 32.f, PAL_RED);
        else if (call_ == Call::Sky || call_ == Call::Block) banner(art_.wordCream, 32.f, PAL_RED);
    }

    if (live && phase_ == Phase::Bowl) {
        const float x0 = 78.f, x1 = 250.f, yb = 196.f;
        float u0 = kSweet0 / float(kPass);
        float u1 = (kSweet1 + 1) / float(kPass);
        float nu = clampf(phaseFrame_ / float(kPass), 0.f, 1.f);
        spr(art_.blot, x0 + nu * (x1 - x0), yb, 4, 14, PAL_GOLDTEXT);
        float sw = (u1 - u0) * (x1 - x0);
        float sx = x0 + (u0 + u1) * 0.5f * (x1 - x0);
        spr(art_.blot, sx, yb, sw, 8, PAL_GREEN);
        spr(art_.blot, (x0 + x1) * 0.5f, yb, x1 - x0, 3, PAL_INK);
    }

    if (drawBall && showGold) {
        float pulse = 1.f + 0.12f * std::sin(clock * 0.3f);
        spr(art_.star, ballX - 8.f, ballY - 8.f, 9.f * pulse, 9.f * pulse, PAL_GOLD);
    }
    if (drawBall) {
        int seam = ((phase_ == Phase::Bowl ? phaseFrame_ : clock) / 3) & 1;
        spr(art_.ball[seam], ballX, ballY, 13, 13, showGold ? PAL_GOLD : PAL_CREAM);
    }

    int bailPal = showGold ? PAL_GOLD : PAL_CREAM;
    if (farBroke || nearBroke) {
        float sx = farBroke ? kFarX : kNearX;
        float sy = laneFeet(1) - 34.f - fly;
        spr(art_.bail, sx - 6.f - bailSide, sy, 8, 3, bailPal);
        spr(art_.bail, sx + 6.f + bailSide, sy, 8, 3, bailPal);
    } else if (!nearBroke) {
        float sy = laneFeet(1) - 36.f;
        spr(art_.bail, kNearX - 5.f, sy, 8, 3, PAL_STUMP);
        spr(art_.bail, kNearX + 5.f, sy, 8, 3, PAL_STUMP);
        if (!farBroke) {
            bool gild = showGold && (title || d.length == 0);
            spr(art_.bail, kFarX - 5.f, sy + 2.f, 8, 3, gild ? PAL_GOLD : PAL_STUMP);
            spr(art_.bail, kFarX + 5.f, sy + 2.f, 8, 3, gild ? PAL_GOLD : PAL_STUMP);
        }
    }

    feetSpr(art_.batsman[pose], kBatX, feet, 40, 48, PAL_BAT);
    feetSpr(art_.bowler[bowlPose], bowlerX, laneFeet(title ? 1 : d.line), 36, 46, PAL_BOWL);
    feetSpr(art_.umpire[ump], 262.f, laneFeet(1) - 2.f, 30, 40, PAL_BOWL);
    feetSpr(art_.stump[nearBroke ? 1 : 0], kNearX, laneFeet(1), 22, 40, PAL_STUMP);
    feetSpr(art_.stump[farBroke ? 1 : 0], kFarX, laneFeet(1) + 1.f, 20, 36, PAL_STUMP);

    if (live && (phase_ == Phase::Run || phase_ == Phase::Bowl)) {
        float bu = d.length ? 0.30f : 0.70f;
        float markX = kReleaseX + (kMeetX - kReleaseX) * bu;
        spr(art_.blot, markX, laneFeet(d.line) - 1.f, 10, 4, showGold ? PAL_GOLD : PAL_CREAM);
        int youPal = linePick_ == d.line ? PAL_GREEN : PAL_RED;
        spr(art_.blot, kBatX, feet + 1.f, 16, 3, youPal);
    }

    bool ropeGold = showGold && (title || d.length == 1);
    spr(art_.rope, kRopeX, kRopeY, 36, 96, ropeGold ? PAL_ROPEGOLD : PAL_ROPE);
    spr(art_.plate, 46.f, 118.f, 34, 16, PAL_GOLD);
    spr(art_.pitch, kPitchX, kPitchY, 168, 46, PAL_PITCH);
    spr(art_.crowd, 70.f, 108.f, 90, 18, PAL_CROWD);
    spr(art_.crowd, 200.f, 106.f, 80, 16, PAL_CROWD);
    spr(art_.house, 40.f, 104.f, 64, 36, PAL_HOUSE);
    spr(art_.tree, 16.f, 100.f, 40, 36, PAL_TREE);
    spr(art_.tree, 128.f, 98.f, 36, 32, PAL_TREE);
    spr(art_.tree, 292.f, 104.f, 40, 36, PAL_TREE);
    spr(art_.cloud, 64.f, 22.f, 36, 14, PAL_CLOUD);
    spr(art_.cloud, 150.f, 14.f, 30, 12, PAL_CLOUD);
    spr(art_.sun, 286.f, 18.f, 14, 14, PAL_GOLD);

    char buf[48];
    hud(1, 0, "S3 WICKET GOLD", PAL_GOLDTEXT);
    if (!title) {
        std::snprintf(buf, sizeof buf, "BALL %d/%d", std::min(ball_ + 1, kBalls), kBalls);
        hud(40 - int(std::strlen(buf)) - 1, 0, buf, PAL_INK);
        char pips[7];
        for (int i = 0; i < kBalls; i++) {
            if (i == ball_ && live && card_[i] == '.') pips[i] = '>';
            else pips[i] = card_[i];
        }
        pips[kBalls] = 0;
        hud(1, 1, pips, PAL_GOLDTEXT);
        std::snprintf(buf, sizeof buf, "G%d C%d %d/%d", gold_, cream_, score_, kLine);
        hud(40 - int(std::strlen(buf)) - 1, 1, buf, score_ >= kLine ? PAL_GREEN : PAL_INK);
    }

    if (title) {
        hudC(22, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLDTEXT);
        hudC(23, "GOLD 2   CREAM 1   LINE 6", PAL_INK);
        hudC(24, "CREAM CANNOT FINISH", PAL_RED);
        hudC(25, "FULL DRIVE HITS   SHORT LOFT CLEARS", PAL_GREEN);
        hudC(26, "U-D LINE  L DRIVE  R LOFT  Z SWING", PAL_INK);
        hudC(27, "ENTER STARTS", PAL_GOLDTEXT);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLDTEXT);
        hudC(14, "ENTER RESUMES", PAL_INK);
    } else if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hudC(10, buf, PAL_GOLDTEXT);
        hudC(12, "ONLY THE GOLD COUNTS DOUBLE", PAL_GREEN);
        hudC(27, "ENTER PLAYS AGAIN", PAL_INK);
    } else if (mode_ == Mode::Lose) {
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hudC(10, buf, PAL_RED);
        hudC(12, "THE DOUBLE GOT AWAY", PAL_INK);
        hudC(27, "ENTER PLAYS AGAIN", PAL_INK);
    } else {
        bool shotOk = loft_ == (d.length == 1);
        bool lineOk = linePick_ == d.line;
        hud(1, 25, showGold ? "GOLD" : "CREAM", showGold ? PAL_GOLDTEXT : PAL_INK);
        hud(8, 25, d.length ? "SHORT" : "FULL", d.length ? PAL_GREEN : PAL_GOLDTEXT);
        hud(16, 25, loft_ ? "LOFT" : "DRIVE", shotOk ? PAL_GREEN : PAL_RED);
        std::snprintf(buf, sizeof buf, "YOU %s", lineName(linePick_));
        hud(1, 26, buf, lineOk ? PAL_GREEN : PAL_RED);
        std::snprintf(buf, sizeof buf, "BALL %s", lineName(d.line));
        hud(16, 26, buf, PAL_INK);
        if (phase_ == Phase::Bowl && phaseFrame_ >= kSweet0 && phaseFrame_ <= kSweet1 && lineOk && shotOk)
            hudC(24, "SWING", ((phaseFrame_ / 4) & 1) ? PAL_GREEN : PAL_GOLDTEXT);
        else if (phase_ == Phase::Run)
            hudC(24, "READ THE BALL", PAL_INK);
        else if (phase_ == Phase::Result && call_ != Call::Gold && call_ != Call::Cream && call_ != Call::Refuse &&
                 call_ != Call::Bowled)
            hudC(24, say_, PAL_RED);
        hudC(27, "U-D LINE  L DRIVE  R LOFT  Z SWING", PAL_INK);
    }
}

}  // namespace wicketgold
