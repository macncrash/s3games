#include "game/cuegold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cuegold {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kCueHome = 78.f;
constexpr float kObjHome = 168.f;
constexpr float kPocket = 268.f;
constexpr float kRow = 128.f;
constexpr float kPocket0 = 0.38f;
constexpr float kPocket1 = 0.62f;
constexpr float kMeterRate = 0.70f;

// Gold, cream, gold finishes the line at 5. Bare pots stay at 3.
constexpr bool kGoldBall[kShots] = {true, false, true, false, true};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

int rgbClamp(int v) { return std::max(0, std::min(15, v)); }

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

bool Game::objectGold() const {
    if (shot_ < 0 || shot_ >= kShots) return false;
    return kGoldBall[shot_];
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
    sys.vdp.setFogColor(gs::rgb4(1, 3, 2));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.10f, 0.14f, 0.08f);
    for (int i = 0; i < kShots; i++) result_[i] = -2;
    cueX_ = kCueHome;
    objX_ = kObjHome;
    mode_ = Mode::Title;
    clock_ = 0;
}

void Game::toTitle() {
    score_ = gold_ = cream_ = shot_ = 0;
    finisherGold_ = won_ = over_ = false;
    for (int i = 0; i < kShots; i++) result_[i] = -2;
    say_ = "";
    cueX_ = kCueHome;
    objX_ = kObjHome;
    mode_ = Mode::Title;
}

void Game::beginRound() {
    score_ = gold_ = cream_ = shot_ = 0;
    finisherGold_ = won_ = over_ = false;
    for (int i = 0; i < kShots; i++) result_[i] = -2;
    say_ = "";
    beginShot();
}

void Game::beginShot() {
    if (shot_ < 0 || shot_ >= kShots) {
        lose();
        return;
    }
    cueX_ = kCueHome;
    objX_ = kObjHome;
    willPot_ = false;
    potted_ = false;
    meter_ = 0;
    meterDir_ = 1.f;
    swingT_ = 0;
    mode_ = Mode::Aim;
    blip(objectGold() ? 494.f : 330.f);
    if (sys_) sys_->setLight(objectGold() ? 255 : 90, objectGold() ? 160 : 70, objectGold() ? 30 : 28);
}

void Game::stroke(float meter) {
    meter = clampf(meter, 0.f, 1.f);
    willPot_ = meter >= kPocket0 && meter <= kPocket1;
    potted_ = false;
    cueX_ = kCueHome;
    objX_ = kObjHome;
    rollT_ = 0;
    swingT_ = 0.28f;
    mode_ = Mode::Roll;
    blip(objectGold() ? 660.f : 420.f);
    if (sys_) sys_->apu.noiseBurst(0.08f, 420.f, 0.03f);
}

void Game::stepRoll(float dt) {
    rollT_ += dt;
    if (potted_) return;
    if (rollT_ < 0.28f) {
        float t = rollT_ / 0.28f;
        cueX_ = kCueHome + (kObjHome - 16.f - kCueHome) * t;
        return;
    }
    if (rollT_ < 0.32f) {
        if (sys_) sys_->apu.noiseBurst(0.10f, 700.f, 0.04f);
        cueX_ = kObjHome - 16.f;
        return;
    }
    float u = clampf((rollT_ - 0.32f) / 0.45f, 0.f, 1.f);
    cueX_ = kObjHome - 16.f - u * 10.f;
    if (willPot_) {
        objX_ = kObjHome + (kPocket - kObjHome) * u;
        if (u >= 1.f) {
            potted_ = true;
            objX_ = kPocket;
            if (sys_) sys_->apu.noiseBurst(0.18f, 160.f, 0.07f);
        }
        return;
    }
    if (meter_ < kPocket0) {
        objX_ = kObjHome + (kPocket - kObjHome) * 0.45f * u;
    } else {
        objX_ = kObjHome + (kPocket - kObjHome + 40.f) * u;
    }
    if (u >= 1.f) finishShot();
}

void Game::finishShot() {
    int mark = 0;
    const bool gold = shot_ >= 0 && shot_ < kShots && kGoldBall[shot_];
    if (!potted_) {
        mark = 0;
        say_ = meter_ < kPocket0 ? "SHORT" : "LONG";
        blip(110.f);
    } else if (!gold) {
        if (score_ + 1 >= kLine) {
            mark = -1;
            say_ = "NOT DOUBLE";
            blip(96.f);
        } else {
            cream_++;
            score_ += 1;
            mark = 1;
            say_ = "CREAM";
            blip(370.f);
        }
    } else {
        gold_++;
        score_ += 2;
        mark = 2;
        say_ = "POTTED";
        if (score_ >= kLine) finisherGold_ = true;
        chord(494.f, 622.f, 784.f, 0.28f);
        if (sys_ && !bot_) sys_->rumble(0.22f, 0.5f, 70);
    }
    if (shot_ >= 0 && shot_ < kShots) result_[shot_] = mark;
    shot_++;
    callT_ = 0.46f;
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
        if (!bot_) sys_->rumble(0.35f, 0.65f, 150);
    }
}

void Game::lose() {
    won_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    say_ = "NO DOUBLE";
    blip(88.f);
    if (sys_) sys_->setLight(170, 28, 22);
}

void Game::afterCall() {
    if (paid()) win();
    else if (shot_ >= kShots) lose();
    else beginShot();
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
        if (meterDir_ > 0.f && prev < 0.50f && meter_ >= 0.50f) stroke(0.50f);
        return;
    }
    if (in.action) stroke(meter_);
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
    if (mode_ == Mode::Title && clock_ > 0.35f) in.start = true;
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
        cueX_ = kCueHome - 6.f + std::sin(clock_ * 2.2f) * 4.f;
        objX_ = kObjHome;
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
        float pull = meter_ * 18.f;
        cueX_ = kCueHome - pull;
        tickMeter(in, kDt);
    } else if (mode_ == Mode::Roll) {
        stepRoll(kDt);
        if (mode_ == Mode::Roll && potted_ && rollT_ > 0.85f) finishShot();
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
        int shade = (y / 8) & 1;
        int g = win && y < 40 ? 6 : 3 + shade;
        v.lineBackdrop[y] = gs::rgb4(rgbClamp(2 + shade), rgbClamp(g), rgbClamp(2));
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    const bool goldBall = (mode_ == Mode::Title) ? true : (shot_ < kShots ? kGoldBall[shot_] : true);
    const int objPal = goldBall ? PAL_GOLD : PAL_CREAM;

    blob(art_.blot, 160.f, kRow, 250.f, 78.f, PAL_RAIL);
    blob(art_.blot, 160.f, kRow, 232.f, 60.f, PAL_FELT);
    blob(art_.blot, 52.f, kRow, 18.f, 18.f, PAL_POCKET);
    blob(art_.blot, 268.f, kRow, 18.f, 18.f, PAL_POCKET);
    blob(art_.blot, 160.f, 94.f, 10.f, 8.f, PAL_POCKET);
    blob(art_.blot, 160.f, 162.f, 10.f, 8.f, PAL_POCKET);

    for (int i = 0; i < 4; i++) {
        blob(art_.blot, 90.f + i * 36.f, 102.f, 4.f, 3.f, PAL_RAIL);
        blob(art_.blot, 90.f + i * 36.f, 154.f, 4.f, 3.f, PAL_RAIL);
    }

    float stickX = cueX_ - 28.f;
    if (mode_ == Mode::Roll && swingT_ > 0.12f) stickX += 16.f;
    spr(art_.cue, stickX, kRow, 10.f, PAL_CUE);

    if (!(potted_ && mode_ != Mode::Title)) {
        blob(art_.shadow, objX_, kRow + 8.f, 16.f, 5.f, PAL_SHADOW, true);
        spr(art_.ball, objX_, kRow, potted_ ? 8.f : 16.f, objPal);
    } else if (mode_ == Mode::Roll) {
        spr(art_.ball, kPocket, kRow + 2.f, 8.f, objPal);
    }
    blob(art_.shadow, cueX_, kRow + 8.f, 16.f, 5.f, PAL_SHADOW, true);
    spr(art_.ball, cueX_, kRow, 16.f, PAL_CUEBALL);

    if (mode_ == Mode::Aim || mode_ == Mode::Title) {
        blob(art_.blot, 160.f, 196.f, 168.f, 8.f, PAL_METER);
        float span = 164.f;
        float x0 = 160.f - span * 0.5f;
        float pocketW = span * (kPocket1 - kPocket0);
        float pocketX = x0 + span * kPocket0 + pocketW * 0.5f;
        blob(art_.blot, pocketX, 196.f, pocketW, 8.f, PAL_GOOD);
        float nx = x0 + span * clampf(meter_, 0.f, 1.f);
        blob(art_.blot, nx, 196.f, 3.f, 14.f, PAL_WORD);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(1, "S3 CUE GOLD", PAL_WORD);
        hudC(3, "ONLY THE GOLD COUNTS DOUBLE", PAL_INK);
        hudC(5, "GOLD POT 2    CREAM POT 1", PAL_CREAM);
        hudC(7, "LINE 4    CORNER POCKET", PAL_GOOD);
        hudC(24, "A STROKE    START", PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_WORD);
    } else if (mode_ == Mode::Win) {
        hudC(1, "ONLY THE GOLD COUNTS DOUBLE", PAL_WORD);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hudC(3, buf, PAL_GOOD);
        hudC(6, "DOUBLE", PAL_WORD);
    } else if (mode_ == Mode::Lose) {
        hudC(1, "NO DOUBLE", PAL_BAD);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hudC(3, buf, PAL_INK);
    } else {
        hudC(1, "ONLY THE GOLD COUNTS DOUBLE", PAL_WORD);
        std::snprintf(buf, sizeof buf, "SHOT %d   %s", std::min(shot_ + 1, kShots), goldBall ? "GOLD X2" : "CREAM X1");
        hudC(3, buf, goldBall ? PAL_GOLD : PAL_CREAM);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hudC(5, buf, PAL_INK);
        if (mode_ == Mode::Call && say_[0]) {
            bool bad = std::strcmp(say_, "NOT DOUBLE") == 0 || std::strcmp(say_, "SHORT") == 0 ||
                       std::strcmp(say_, "LONG") == 0;
            hudC(8, say_, bad ? PAL_BAD : PAL_GOOD);
        }
        if (mode_ == Mode::Aim) hudC(24, "A  STROKE THE CUE", PAL_INK);
    }

    for (int i = 0; i < kShots; i++) {
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
            mark = kGoldBall[i] ? "G" : "C";
            pal = kGoldBall[i] ? PAL_GOLD : PAL_CREAM;
        }
        hud(15 + i * 2, 22, mark, pal);
    }
}

}  // namespace cuegold
