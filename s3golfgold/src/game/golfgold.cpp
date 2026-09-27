#include "game/golfgold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace golfgold {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kTee = 72.f;
constexpr float kCup = 248.f;
constexpr float kGround = 168.f;
constexpr float kBallR = 6.f;
constexpr float kFriction = 100.f;
constexpr float kPocket0 = 0.40f;
constexpr float kPocket1 = 0.60f;
constexpr float kMeterRate = 0.72f;

// First three gold cups pay the line. Cream is one, and is refused on the line.
constexpr bool kGoldHole[kHoles] = {true, true, true, false, false, true};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

int rgbClamp(int v) { return std::max(0, std::min(15, v)); }

// Stop distance from the tee. The green band dies in the cup. Outside it is short or long.
float stopOf(float meter) {
    const float dx = kCup - kTee;
    if (meter >= kPocket0 && meter <= kPocket1) {
        float t = (meter - kPocket0) / (kPocket1 - kPocket0);
        return dx - 5.f + t * 10.f;
    }
    if (meter < kPocket0) return dx * (meter / kPocket0) * 0.86f;
    return dx + 18.f + (meter - kPocket1) * 160.f;
}

float speedOf(float meter) {
    float s = stopOf(meter);
    if (s < 4.f) s = 4.f;
    return std::sqrt(2.f * kFriction * s);
}

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Aim: return "aim";
    case Mode::Roll: return "roll";
    case Mode::Call: return "call";
    case Mode::Win: return "win";
    case Mode::Lose: return "lose";
    case Mode::Pause: return "pause";
    }
    return "?";
}

bool Game::paid() const {
    const int bare = gold_ + cream_;
    return finisherGold_ && gold_ >= 1 && score_ >= kLine && bare < kLine && score_ == gold_ * 2 + cream_;
}

bool Game::cupGold() const {
    if (shot_ < 0 || shot_ >= kHoles) return false;
    return kGoldHole[shot_];
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = std::max(beep_, 0.09f);
}

void Game::chord(float a, float b, float c, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.09f);
    sys_->apu.tone(1, b, 0.06f);
    sys_->apu.tone(2, c, 0.05f);
    beep_ = std::max(beep_, hold);
}

void Game::hush() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 4, 3));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.16f, 0.10f);
    for (int i = 0; i < kHoles; i++) result_[i] = -2;
    ballX_ = kTee;
    ballY_ = kGround - kBallR;
    mode_ = Mode::Title;
    clock_ = 0;
}

void Game::toTitle() {
    score_ = gold_ = cream_ = shot_ = 0;
    finisherGold_ = won_ = over_ = false;
    for (int i = 0; i < kHoles; i++) result_[i] = -2;
    say_ = "";
    ballX_ = kTee;
    ballY_ = kGround - kBallR;
    ballV_ = 0;
    mode_ = Mode::Title;
}

void Game::beginRound() {
    score_ = gold_ = cream_ = shot_ = 0;
    finisherGold_ = won_ = over_ = false;
    for (int i = 0; i < kHoles; i++) result_[i] = -2;
    say_ = "";
    beginHole();
}

void Game::beginHole() {
    if (shot_ < 0 || shot_ >= kHoles) {
        lose();
        return;
    }
    ballX_ = kTee;
    ballY_ = kGround - kBallR;
    ballV_ = 0;
    willHole_ = false;
    sunk_ = false;
    meter_ = 0;
    meterDir_ = 1.f;
    swingT_ = 0;
    mode_ = Mode::Aim;
    blip(cupGold() ? 523.f : 330.f);
    if (sys_) sys_->setLight(cupGold() ? 255 : 80, cupGold() ? 170 : 60, cupGold() ? 40 : 30);
}

void Game::launch(float meter) {
    meter = clampf(meter, 0.f, 1.f);
    willHole_ = meter >= kPocket0 && meter <= kPocket1;
    sunk_ = false;
    ballX_ = kTee;
    ballY_ = kGround - kBallR;
    ballV_ = speedOf(meter);
    rollT_ = 0;
    swingT_ = 0.22f;
    mode_ = Mode::Roll;
    blip(cupGold() ? 680.f : 440.f);
    if (sys_) sys_->apu.noiseBurst(0.12f, 280.f, 0.04f);
}

void Game::stepBall(float dt) {
    rollT_ += dt;
    if (sunk_) return;
    ballV_ -= kFriction * dt;
    if (ballV_ < 0.f) ballV_ = 0.f;
    ballX_ += ballV_ * dt;
    ballY_ = kGround - kBallR;
    const float mouth = 7.f;
    if (willHole_ && ballX_ >= kCup - mouth) {
        sunk_ = true;
        ballX_ = kCup;
        ballY_ = kGround - 1.f;
        ballV_ = 0;
        if (sys_) sys_->apu.noiseBurst(0.22f, 180.f, 0.08f);
        return;
    }
    if (ballV_ <= 0.4f || ballX_ > 312.f) finishPutt();
}

void Game::finishPutt() {
    int mark = 0;
    const bool gold = shot_ >= 0 && shot_ < kHoles && kGoldHole[shot_];
    if (!sunk_) {
        mark = 0;
        say_ = ballX_ < kCup ? "SHORT" : "LONG";
        blip(120.f);
    } else if (!gold) {
        if (score_ + 1 >= kLine) {
            mark = -1;
            say_ = "NOT DOUBLE";
            blip(98.f);
        } else {
            cream_++;
            score_ += 1;
            mark = 1;
            say_ = "CREAM";
            blip(392.f);
        }
    } else {
        gold_++;
        score_ += 2;
        mark = 2;
        say_ = "IN THE HOLE";
        if (score_ >= kLine) finisherGold_ = true;
        chord(523.f, 659.f, 784.f, 0.28f);
        if (sys_ && !bot_) sys_->rumble(0.25f, 0.55f, 80);
    }
    if (shot_ >= 0 && shot_ < kHoles) result_[shot_] = mark;
    shot_++;
    callT_ = 0.48f;
    mode_ = Mode::Call;
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    say_ = "DOUBLE";
    chord(392.f, 523.f, 784.f, 0.9f);
    if (sys_) {
        sys_->setLight(255, 190, 40);
        if (!bot_) sys_->rumble(0.4f, 0.7f, 160);
    }
}

void Game::lose() {
    won_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    say_ = "NO DOUBLE";
    blip(90.f);
    if (sys_) sys_->setLight(180, 30, 24);
}

void Game::afterCall() {
    if (paid()) win();
    else if (shot_ >= kHoles) lose();
    else beginHole();
}

void Game::tickMeter(const Input& in, float dt) {
    float prev = meter_;
    meter_ += meterDir_ * kMeterRate * dt;
    if (meter_ >= 1.f) {
        meter_ = 1.f;
        meterDir_ = -1.f;
    } else if (meter_ <= 0.f) {
        meter_ = 0.f;
        meterDir_ = 1.f;
    }
    if (bot_) {
        if (meterDir_ > 0.f && prev < 0.50f && meter_ >= 0.50f) launch(0.50f);
        return;
    }
    if (in.action) launch(meter_);
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && clock_ > 0.4f) in.start = true;
    return in;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) clock_ += kDt;
    if (swingT_ > 0.f) swingT_ = std::max(0.f, swingT_ - kDt);
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) hush();
    }

    const Input in = bot_ ? botInput() : readPad(sys.pad);

    if (mode_ == Mode::Title) {
        ballX_ = kTee;
        ballY_ = kGround - kBallR + std::fabs(std::sin(clock_ * 3.f)) * 2.f;
        meter_ = 0.5f + 0.5f * std::sin(clock_ * kMeterRate * 3.14159265f);
        if (in.back && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (in.start || in.action) {
            beginRound();
        }
    } else if (mode_ == Mode::Pause) {
        if (in.start || in.action) mode_ = held_;
        else if (in.back) toTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (in.start || in.action)) beginRound();
        else if (!bot_ && in.back) toTitle();
    } else if (mode_ == Mode::Call) {
        callT_ -= kDt;
        if (callT_ <= 0.f || (!bot_ && (in.action || in.start))) afterCall();
    } else if (!bot_ && in.start) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Aim) {
        tickMeter(in, kDt);
    } else if (mode_ == Mode::Roll) {
        stepBall(kDt);
        if (mode_ == Mode::Roll && sunk_ && rollT_ > 0.35f) finishPutt();
    }

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(1, m.h));
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::blob(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = (unsigned char)s[i];
        if (c >= 'a' && c <= 'z') c = (unsigned char)(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - (s ? (int)std::strlen(s) / 2 : 0), row, s, pal); }

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    const bool win = mode_ == Mode::Win;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 148) {
            float u = y / 148.f;
            int boost = win && y < 70 ? 2 : 0;
            v.lineBackdrop[y] = gs::rgb4(rgbClamp(3 + int(u * 6.f) + boost), rgbClamp(6 + int((1.f - u) * 4.f)),
                                          rgbClamp(12 - int(u * 6.f)));
        } else if (y < 176) {
            int stripe = ((y / 4) & 1) ? 0 : 1;
            v.lineBackdrop[y] = gs::rgb4(2 + stripe, 9 + stripe, 3);
        } else {
            v.lineBackdrop[y] = gs::rgb4(1, 5, 2);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    const bool goldCup = (mode_ == Mode::Title) ? true : (shot_ < kHoles ? kGoldHole[shot_] : true);
    const int flagPal = goldCup ? PAL_FLAG : PAL_CREAM;

    spr(art_.tree, 28.f, 132.f, 40.f, PAL_TREE);
    spr(art_.tree, 300.f, 128.f, 48.f, PAL_TREE);
    spr(art_.tree, 188.f, 136.f, 30.f, PAL_TREE);

    blob(art_.blot, 160.f, kGround + 2.f, 320.f, 6.f, PAL_GRASS);
    blob(art_.blot, kCup, kGround - 2.f, 36.f, 8.f, PAL_GRASS);
    spr(art_.cup, kCup, kGround - 1.f, 12.f, PAL_CUP);
    spr(art_.flag, kCup + 10.f, kGround - 22.f, 40.f, flagPal);

    blob(art_.shadow, ballX_, kGround + 1.f, 14.f, 5.f, PAL_SHADOW, true);
    spr(art_.ball, ballX_, ballY_, sunk_ ? 8.f : 12.f, PAL_BALL);

    int swing = (mode_ == Mode::Roll && swingT_ > 0.f) ? 1 : 0;
    spr(art_.golfer[swing], kTee - 18.f, kGround - 22.f, 44.f, PAL_MAN);

    if (mode_ == Mode::Aim || mode_ == Mode::Title) {
        blob(art_.blot, 160.f, 200.f, 160.f, 8.f, PAL_METER);
        float span = 156.f;
        float x0 = 160.f - span * 0.5f;
        float pocketW = span * (kPocket1 - kPocket0);
        float pocketX = x0 + span * kPocket0 + pocketW * 0.5f;
        blob(art_.blot, pocketX, 200.f, pocketW, 8.f, PAL_GOOD);
        float nx = x0 + span * clampf(meter_, 0.f, 1.f);
        blob(art_.blot, nx, 200.f, 3.f, 14.f, PAL_WORD);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(2, "S3 GOLF GOLD", PAL_WORD);
        hudC(4, "ONLY THE GOLD COUNTS DOUBLE", PAL_INK);
        hudC(6, "GOLD CUP 2    CREAM CUP 1", PAL_CREAM);
        hudC(8, "LINE 6    IN THE HOLE", PAL_GOOD);
        hudC(22, "A PUTT    START", PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_WORD);
    } else if (mode_ == Mode::Win) {
        hudC(2, "ONLY THE GOLD COUNTS DOUBLE", PAL_WORD);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hudC(4, buf, PAL_GOOD);
        hudC(8, "DOUBLE", PAL_WORD);
    } else if (mode_ == Mode::Lose) {
        hudC(2, "NO DOUBLE", PAL_BAD);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hudC(4, buf, PAL_INK);
    } else {
        hudC(1, "ONLY THE GOLD COUNTS DOUBLE", PAL_WORD);
        std::snprintf(buf, sizeof buf, "HOLE %d   %s", std::min(shot_ + 1, kHoles), goldCup ? "GOLD X2" : "CREAM X1");
        hudC(3, buf, goldCup ? PAL_GOLD : PAL_CREAM);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hudC(5, buf, PAL_INK);
        if (mode_ == Mode::Call && say_[0]) hudC(8, say_, (std::strcmp(say_, "NOT DOUBLE") == 0 || std::strcmp(say_, "SHORT") == 0 || std::strcmp(say_, "LONG") == 0) ? PAL_BAD : PAL_GOOD);
        if (mode_ == Mode::Aim) hudC(22, "A  PUTT THE GREEN", PAL_INK);
    }

    for (int i = 0; i < kHoles; i++) {
        const char* mark = ".";
        int pal = PAL_INK;
        if (result_[i] == 2) {
            mark = "G";
            pal = PAL_GOLD;
        } else if (result_[i] == 1) {
            mark = "C";
            pal = PAL_CREAM;
        } else if (result_[i] == 0 || result_[i] == -1) {
            mark = "X";
            pal = PAL_BAD;
        } else if (i == shot_ && mode_ != Mode::Title && mode_ != Mode::Win && mode_ != Mode::Lose) {
            mark = kGoldHole[i] ? "G" : "C";
            pal = kGoldHole[i] ? PAL_GOLD : PAL_CREAM;
        }
        hud(16 + i * 2, 24, mark, pal);
    }
}

}  // namespace golfgold
