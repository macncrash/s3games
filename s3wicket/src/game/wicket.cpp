#include "game/wicket.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace wicket {
namespace {

constexpr int kRun = 48;
constexpr int kSweet0 = 46;
constexpr int kSweet1 = 62;
constexpr int kBotSwing = 54;
constexpr int kPass = 70;
constexpr int kNeed = 2;

constexpr float kBatX = 78.f;
constexpr float kReleaseX = 188.f;
constexpr float kNearX = 44.f;
constexpr float kFarX = 232.f;
constexpr float kBowlerX = 206.f;
constexpr float kPitchX = 36.f;
constexpr float kPitchY = 118.f;
constexpr float kRopeX = 304.f;
constexpr float kRopeY = 128.f;

struct Del {
    int line;
    int length;  // 0 full, drive hits the wicket; 1 short, loft clears the rope
};

// A set over. The first two answers are one of each, so a true swing wins the match.
constexpr Del kOver[6] = {{1, 0}, {2, 1}, {0, 0}, {1, 1}, {2, 0}, {0, 1}};

struct Span {
    int anim;
    int total;
};

float laneFeet(int line) { return 132.f + float(line) * 16.f; }

const char* lineName(int line) { return line == 0 ? "OFF" : line == 1 ? "MIDDLE" : "LEG"; }

float clampf(float v, float a, float b) { return v < a ? a : (v > b ? v : b); }

}  // namespace

namespace {

Span spanFor(int kind) {
    int anim = 30;
    if (kind == 2) anim = 62;
    else if (kind == 1 || kind == 3) anim = 44;
    return {anim, anim + 28};
}

}  // namespace

int Game::callKind(Call c) {
    if (c == Call::Six) return 2;
    if (c == Call::Wicket) return 1;
    if (c == Call::Bowled) return 3;
    return 0;
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
    sys.vdp.setFogColor(gs::rgb4(6, 10, 14));
    sys.apu.setMaster(0.72f);
    ball_ = 0;
    line_ = 1;
    loft_ = false;
    call_ = Call::None;
    attempted_ = false;
    for (int i = 0; i < 6; i++) mark_[i] = '.';
    if (bot_) beginOver();
    else mode_ = Mode::Title;
}

void Game::beginOver() {
    ball_ = 0;
    wickets_ = 0;
    sixes_ = 0;
    won_ = false;
    over_ = false;
    line_ = 1;
    loft_ = false;
    for (int i = 0; i < 6; i++) mark_[i] = '.';
    mode_ = Mode::Play;
    beginBall();
    tone(0, 330.f, 0.05f, 6);
}

void Game::beginBall() {
    phase_ = Phase::Run;
    phaseFrame_ = 0;
    resultT_ = 0;
    attempted_ = false;
    call_ = Call::None;
    sixNote_ = -1;
    loft_ = false;
    if (bot_) {
        line_ = kOver[ball_].line;
        loft_ = kOver[ball_].length == 1;
    }
}

void Game::steer() {
    if (bot_) {
        line_ = kOver[ball_].line;
        loft_ = kOver[ball_].length == 1;
        return;
    }
    if (!sys_) return;
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_UP)) line_ = std::max(0, line_ - 1);
    if (p.pressed(gs::BTN_DOWN)) line_ = std::min(2, line_ + 1);
    if (p.pressed(gs::BTN_LEFT)) loft_ = false;
    if (p.pressed(gs::BTN_RIGHT)) loft_ = true;
}

void Game::flightPos(int frame, float& x, float& y) const {
    const Del& d = kOver[ball_];
    float u = clampf(frame / float(kPass), 0.f, 1.f);
    x = kReleaseX + (kBatX + 6.f - kReleaseX) * u;
    float feet = laneFeet(d.line);
    float yHand = feet - 34.f;
    float yBat = feet - (d.length ? 26.f : 16.f);
    float yBounce = feet - 4.f;
    float bu = d.length ? 0.24f : 0.78f;
    if (u < bu) {
        y = yHand + (yBounce - yHand) * (u / bu);
    } else {
        float w = (u - bu) / (1.f - bu);
        float lift = d.length ? 20.f : 3.f;
        y = yBounce + (yBat - yBounce) * w - std::sin(w * 3.14159265f) * lift;
    }
}

void Game::resultPos(float u, float& x, float& y) const {
    float x0 = ballX_;
    float y0 = ballY_;
    u = clampf(u, 0.f, 1.f);
    if (call_ == Call::Six) {
        x = x0 + (312.f - x0) * u;
        y = y0 + (74.f - y0) * u - std::sin(u * 3.14159265f) * 16.f;
    } else if (call_ == Call::Wicket) {
        x = x0 + (kFarX - x0) * u;
        y = y0 + ((laneFeet(1) - 16.f) - y0) * u - std::sin(u * 3.14159265f) * 6.f;
    } else if (call_ == Call::Bowled) {
        x = x0 + (kNearX - x0) * u;
        y = y0 + ((laneFeet(1) - 14.f) - y0) * u;
    } else if (call_ == Call::Sky) {
        int ln = kOver[ball_].line;
        x = x0 + (156.f - x0) * u;
        y = y0 + ((laneFeet(ln) - 6.f) - y0) * u - std::sin(u * 3.14159265f) * 46.f;
    } else if (call_ == Call::Block) {
        int ln = kOver[ball_].line;
        x = x0 + (154.f - x0) * std::min(1.f, u * 1.15f);
        y = laneFeet(ln) - 6.f - std::sin(u * 3.14159265f) * 8.f;
    } else {
        x = x0 + 34.f * u;
        y = y0 - std::sin(u * 3.14159265f) * 20.f;
    }
}

float Game::shotU() const {
    if (call_ == Call::None) return 0.f;
    if (mode_ == Mode::Win || mode_ == Mode::Lose) return 1.f;
    if (phase_ != Phase::Result) return 0.f;
    Span s = spanFor(callKind(call_));
    return std::min(1.f, resultT_ / float(std::max(1, s.anim - 1)));
}

void Game::enterResult(Call c) {
    call_ = c;
    phase_ = Phase::Result;
    resultT_ = 0;
    sixNote_ = -1;
    char mark = 'x';
    if (c == Call::Wicket) {
        wickets_++;
        mark = 'W';
        tone(0, 140.f, 0.08f, 8);
        tone(1, 90.f, 0.06f, 14);
        sys_->apu.noiseBurst(0.34f, 240.f, 0.11f);
    } else if (c == Call::Six) {
        sixes_++;
        mark = '6';
        tone(0, 392.f, 0.07f, 10);
        sys_->apu.noiseBurst(0.16f, 1400.f, 0.08f);
    } else if (c == Call::Bowled) {
        tone(0, 98.f, 0.07f, 12);
        sys_->apu.noiseBurst(0.3f, 180.f, 0.12f);
    } else {
        tone(0, c == Call::Soon || c == Call::Late ? 220.f : 170.f, 0.05f, 7);
        if (c == Call::Sky || c == Call::Block) sys_->apu.noiseBurst(0.1f, 700.f, 0.05f);
    }
    if (ball_ >= 0 && ball_ < 6) mark_[ball_] = mark;
}

void Game::swing() {
    if (phase_ != Phase::Bowl || attempted_) return;
    attempted_ = true;
    flightPos(phaseFrame_, ballX_, ballY_);
    const Del& d = kOver[ball_];
    Call c;
    if (line_ != d.line) c = Call::Bowled;
    else if (phaseFrame_ < kSweet0) c = Call::Soon;
    else if (phaseFrame_ > kSweet1) c = Call::Late;
    else if (loft_ != (d.length == 1)) c = loft_ ? Call::Sky : Call::Block;
    else c = loft_ ? Call::Six : Call::Wicket;
    enterResult(c);
}

void Game::stepResult() {
    Span s = spanFor(callKind(call_));
    if (call_ == Call::Six) {
        int q = resultT_ / 10;
        if (q != sixNote_ && q >= 0 && q < 5) {
            sixNote_ = q;
            static const float notes[] = {392.f, 494.f, 587.f, 740.f, 880.f};
            tone(0, notes[q], 0.06f, 9);
        }
    }
    if (resultT_ >= s.total - 1) {
        advance();
        return;
    }
    resultT_++;
}

void Game::advance() {
    if (wickets_ + sixes_ >= kNeed) {
        finish(true);
        return;
    }
    if (ball_ + 1 >= 6) {
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
    if (win) {
        tone(0, 523.f, 0.08f, 10);
        tone(1, 659.f, 0.05f, 10);
        sys_->apu.noiseBurst(0.1f, 1800.f, 0.22f);
    } else {
        tone(0, 146.f, 0.06f, 14);
        tone(1, 110.f, 0.04f, 16);
    }
}

void Game::stepEnd() {
    hold_++;
    if (mode_ == Mode::Win) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        int step = hold_ / 8;
        if (step != fanStep_ && step >= 0 && step < 4) {
            fanStep_ = step;
            tone(0, notes[step], 0.07f, 8);
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
            tone(1, 680.f, 0.04f, 4);
            sys_->apu.noiseBurst(0.05f, 500.f, 0.04f);
        } else {
            phaseFrame_++;
        }
        return;
    }
    if (phase_ == Phase::Bowl) {
        if (!attempted_) steer();
        if (!attempted_) {
            if (bot_ && phaseFrame_ == kBotSwing) swing();
            else if (!bot_ && swingPressed()) swing();
        }
        if (phase_ == Phase::Bowl && !attempted_ && phaseFrame_ >= kPass) {
            flightPos(phaseFrame_, ballX_, ballY_);
            enterResult(Call::Bowled);
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
        if (y < 78) {
            int r = 4 + y * 7 / 78;
            int g = 7 + y * 6 / 78;
            int b = 12 + y * 3 / 78;
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else if (y < 104) {
            v.lineBackdrop[y] = gs::rgb4(3, 8, 3);
        } else if (y < 196) {
            int s = (y / 6) & 1;
            v.lineBackdrop[y] = gs::rgb4(2 + s, 10 + s, 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(1, 7, 2);
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
    const Del& d = kOver[ball_ < 0 ? 0 : (ball_ > 5 ? 5 : ball_)];
    float feet = laneFeet(title ? 1 : line_);

    auto feetSpr = [&](const gs::Image& img, float cx, float fy, float w, float h, int pal, bool flip = false) {
        spr(img, cx, fy - h * 0.5f, w, h, pal, flip);
    };

    int clock = int(sys_->frame);
    float ballX = kReleaseX;
    float ballY = laneFeet(d.line) - 34.f;
    bool drawBall = true;
    if (title) {
        float bob = std::sin(clock * 0.08f) * 2.f;
        ballX = kBowlerX - 16.f;
        ballY = laneFeet(1) - 36.f + bob;
    } else if (phase_ == Phase::Run && !ended) {
        float u = clampf(phaseFrame_ / float(kRun), 0.f, 1.f);
        float bx = 236.f + (kBowlerX - 236.f) * u;
        ballX = bx - 16.f;
        ballY = laneFeet(d.line) - 34.f;
    } else if (phase_ == Phase::Bowl && !ended && !attempted_) {
        flightPos(phaseFrame_, ballX, ballY);
    } else if (call_ != Call::None) {
        resultPos(shotU(), ballX, ballY);
    } else {
        drawBall = false;
    }

    float uShot = shotU();
    bool farBroke = call_ == Call::Wicket && uShot > 0.72f;
    bool nearBroke = call_ == Call::Bowled && uShot > 0.72f;
    float fly = 0.f;
    float bailSide = 0.f;
    if (farBroke || nearBroke) {
        float w = clampf((uShot - 0.72f) / 0.28f, 0.f, 1.f);
        fly = std::sin(w * 3.14159265f) * 18.f;
        bailSide = w * 12.f;
    }

    int pose = loft_ ? 1 : 0;
    if (attempted_ && (phase_ == Phase::Result || ended)) pose = loft_ ? 3 : 2;
    if (title) pose = 0;

    int bowlPose = 2;
    float bowlerX = kBowlerX;
    if (title) {
        bowlPose = (clock / 7) & 1;
        bowlerX = kBowlerX;
    } else if (phase_ == Phase::Run && !ended) {
        bowlPose = (phaseFrame_ / 6) & 1;
        float ur = clampf(phaseFrame_ / float(kRun), 0.f, 1.f);
        bowlerX = 236.f + (kBowlerX - 236.f) * ur;
    }

    float depth = title ? 1.f : (0.84f + 0.08f * float(d.line));
    float batH = 54.f * (title ? 1.f : (0.86f + 0.07f * float(line_)));
    float batW = 40.f * (batH / 54.f);

    // Earlier sprites sit on top.
    auto banner = [&](const gs::Image& img, float y, int pal) {
        spr(img, 160.f, y, float(img.w), float(img.h), pal);
    };
    if (title) {
        banner(art_.title, 26.f, PAL_TITLE);
        banner(art_.sixBalls, 52.f, PAL_INK);
    } else if (mode_ == Mode::Win) {
        float y = 30.f;
        if (wickets_ > 0) {
            banner(art_.hitWicket, y, PAL_GOLD);
            y += 22.f;
        }
        if (sixes_ > 0) {
            banner(art_.clearRope, y, PAL_GREEN);
            y += 22.f;
        }
        banner(art_.ours, y + 4.f, PAL_TITLE);
    } else if (mode_ == Mode::Lose) {
        banner(art_.gone, 40.f, PAL_RED);
    } else if (phase_ == Phase::Result) {
        if (call_ == Call::Wicket) banner(art_.hitWicket, 36.f, PAL_GOLD);
        else if (call_ == Call::Six) banner(art_.clearRope, 34.f, PAL_GREEN);
        else if (call_ == Call::Bowled) banner(art_.bowledWord, 36.f, PAL_RED);
    }
    if (live && phase_ == Phase::Bowl) {
        const float x0 = 68.f, x1 = 252.f, yb = 200.f;
        float u0 = kSweet0 / float(kPass);
        float u1 = (kSweet1 + 1) / float(kPass);
        float nu = clampf(phaseFrame_ / float(kPass), 0.f, 1.f);
        spr(art_.blot, x0 + nu * (x1 - x0), yb, 4, 16, PAL_GOLD);
        float sw = (u1 - u0) * (x1 - x0);
        float sx = x0 + (u0 + u1) * 0.5f * (x1 - x0);
        spr(art_.blot, sx, yb, sw, 8, PAL_GREEN);
        spr(art_.blot, (x0 + x1) * 0.5f, yb, x1 - x0, 4, PAL_INK);
    }

    if (drawBall) {
        int seam = ((phase_ == Phase::Bowl ? phaseFrame_ : clock) / 3) & 1;
        float bh = 12.f * depth;
        spr(art_.ball[seam], ballX, ballY, bh, bh, PAL_BALL);
    }
    if (farBroke || nearBroke) {
        float sx = farBroke ? kFarX : kNearX;
        float sy = laneFeet(1) - 36.f - fly;
        spr(art_.bail, sx - 6.f - bailSide, sy, 8, 4, PAL_STUMP);
        spr(art_.bail, sx + 6.f + bailSide, sy, 8, 4, PAL_STUMP);
    }
    feetSpr(art_.batsman[pose], kBatX, feet, batW, batH, PAL_BAT);
    feetSpr(art_.bowler[bowlPose], bowlerX, laneFeet(title ? 1 : d.line), 36.f * depth, 50.f * depth, PAL_BOWL);
    feetSpr(art_.stump[nearBroke ? 1 : 0], kNearX, laneFeet(1), 26, 46, PAL_STUMP);
    feetSpr(art_.stump[farBroke ? 1 : 0], kFarX, laneFeet(1) + 2.f, 22, 40, PAL_STUMP);
    feetSpr(art_.bowler[0], 268.f, laneFeet(1) + 4.f, 26, 36, PAL_BOWL);

    if (live && (phase_ == Phase::Run || phase_ == Phase::Bowl)) {
        float markX = d.length ? 162.f : 104.f;
        spr(art_.blot, markX, laneFeet(d.line) - 2.f, 10, 4, PAL_GOLD);
        int youPal = line_ == d.line ? PAL_GREEN : PAL_RED;
        spr(art_.blot, kBatX, feet - 1.f, 16, 3, youPal);
    }

    spr(art_.shadow, kBatX, feet + 2.f, 22, 7, PAL_SHADE);
    spr(art_.shadow, bowlerX, laneFeet(title ? 1 : d.line) + 2.f, 18, 6, PAL_SHADE);

    spr(art_.screen, 252.f, 96.f, 40, 28, PAL_HOUSE);
    spr(art_.rope, kRopeX, kRopeY, 44, 120, PAL_ROPE);
    spr(art_.pitch, kPitchX + 120.f, kPitchY + 36.f, 240, 72, PAL_PITCH);
    spr(art_.crowd, 78.f, 100.f, 112, 22, PAL_CROWD);
    spr(art_.crowd, 210.f, 98.f, 100, 20, PAL_CROWD);
    spr(art_.house, 48.f, 102.f, 72, 40, PAL_HOUSE);
    const float trees[] = {18.f, 118.f, 168.f, 292.f};
    const float treeY[] = {104.f, 108.f, 106.f, 112.f};
    for (int i = 0; i < 4; i++) spr(art_.tree, trees[i], treeY[i] - 21.f, 48, 42, PAL_TREE);
    spr(art_.cloud, 70.f, 28.f, 40, 16, PAL_CLOUD);
    spr(art_.cloud, 150.f, 18.f, 34, 14, PAL_CLOUD);
    spr(art_.sun, 292.f, 20.f, 16, 16, PAL_GOLD);

    char buf[48];
    std::snprintf(buf, sizeof buf, "S3 WICKET");
    hud(1, 0, buf, PAL_TITLE);
    if (!title) {
        std::snprintf(buf, sizeof buf, "BALL %d/6", ball_ + 1);
        hud(40 - int(std::strlen(buf)) - 1, 0, buf, PAL_INK);
        char pips[7];
        for (int i = 0; i < 6; i++) {
            if (i == ball_ && live && mark_[i] == '.') pips[i] = '>';
            else pips[i] = mark_[i];
        }
        pips[6] = 0;
        std::snprintf(buf, sizeof buf, "%s  W%d S%d", pips, wickets_, sixes_);
        hud(1, 1, buf, PAL_GOLD);
        int need = kNeed - (wickets_ + sixes_);
        if (need < 0) need = 0;
        std::snprintf(buf, sizeof buf, "NEED %d", need);
        hud(40 - 6, 1, buf, need ? PAL_INK : PAL_GREEN);
    }

    if (title) {
        hudC(9, "HIT THE WICKET OR CLEAR THE ROPE", PAL_GOLD);
        hudC(11, "FULL DRIVE HITS THE WICKET", PAL_INK);
        hudC(12, "SHORT LOFT CLEARS THE ROPE", PAL_GREEN);
        hudC(14, "LAND TWO IN THE OVER", PAL_TITLE);
        hudC(25, "U-D LINE   L DRIVE   R LOFT   Z SWING", PAL_INK);
        hudC(26, "ENTER STARTS", PAL_GREEN);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
        hudC(14, "ENTER RESUMES", PAL_INK);
    } else if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "WICKET %d   SIX %d", wickets_, sixes_);
        hudC(12, buf, PAL_GOLD);
        hudC(26, "ENTER PLAYS AGAIN", PAL_INK);
    } else if (mode_ == Mode::Lose) {
        std::snprintf(buf, sizeof buf, "WICKET %d   SIX %d", wickets_, sixes_);
        hudC(8, buf, PAL_RED);
        hudC(10, "THE OVER GOT AWAY", PAL_INK);
        hudC(26, "ENTER PLAYS AGAIN", PAL_INK);
    } else {
        bool shotOk = loft_ == (d.length == 1);
        bool lineOk = line_ == d.line;
        hud(1, 24, d.length ? "SHORT" : "FULL", d.length ? PAL_GREEN : PAL_GOLD);
        hud(8, 24, loft_ ? "LOFT" : "DRIVE", shotOk ? PAL_GREEN : PAL_RED);
        std::snprintf(buf, sizeof buf, "YOU %s", lineName(line_));
        hud(1, 25, buf, lineOk ? PAL_GREEN : PAL_RED);
        std::snprintf(buf, sizeof buf, "BALL %s", lineName(d.line));
        hud(16, 25, buf, PAL_INK);
        if (phase_ == Phase::Bowl && phaseFrame_ >= kSweet0 && phaseFrame_ <= kSweet1 && lineOk && shotOk)
            hudC(23, "SWING", ((phaseFrame_ / 4) & 1) ? PAL_GREEN : PAL_GOLD);
        else if (phase_ == Phase::Run)
            hudC(23, "READ THE BALL", PAL_INK);
        else if (phase_ == Phase::Result) {
            const char* msg = "PLAYED";
            int pal = PAL_INK;
            if (call_ == Call::Soon) msg = "TOO SOON";
            else if (call_ == Call::Late) msg = "TOO LATE";
            else if (call_ == Call::Sky) {
                msg = "SKIED  NO ROPE";
                pal = PAL_RED;
            } else if (call_ == Call::Block) {
                msg = "BLOCKED  NO WICKET";
                pal = PAL_RED;
            } else if (call_ == Call::Wicket || call_ == Call::Six || call_ == Call::Bowled)
                msg = nullptr;
            if (msg) hudC(4, msg, pal);
        }
        hudC(26, "U-D LINE  L DRIVE  R LOFT  Z SWING", PAL_INK);
    }
}

}  // namespace wicket
