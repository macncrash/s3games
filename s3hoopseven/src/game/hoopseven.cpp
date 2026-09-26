#include "game/hoopseven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace hoopseven {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kG = 520.f;
constexpr float kPi = 3.14159265f;
constexpr float kAng = 63.f * kPi / 180.f;
constexpr float kRimX = 248.f;
constexpr float kRimY = 110.f;
constexpr float kRelY = 62.f;
constexpr float kBase = 204.f;
constexpr float kHand = 12.f;
constexpr float kBallR = 6.5f;
constexpr float kCountR = 16.f;
constexpr float kTubeX = 22.f;
constexpr float kTubeR = 3.0f;
constexpr float kBoardX = 286.f;
constexpr float kArcX = 88.f;
constexpr float kPaintX = 150.f;
constexpr float kFeetMin = 50.f;
constexpr float kFeetMax = 178.f;
constexpr float kPocket0 = 0.38f;
constexpr float kPocket1 = 0.62f;
constexpr float kMeterRate = 0.72f;
constexpr int kLine = 7;
constexpr float kYouX[3] = {62.f, 62.f, 172.f};

struct Cue {
    float x;
    float meter;
    float off;
};

// Lane pattern. A make sits in the green with no offset. A miss is short and wide.
constexpr Cue kLaneCue[] = {
    {112.f, 0.14f, -36.f}, {172.f, 0.50f, 0.f}, {112.f, 0.50f, 0.f},
    {62.f, 0.12f, -40.f},  {112.f, 0.50f, 0.f}, {62.f, 0.50f, 0.f},
};
constexpr int kLaneN = 6;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
float Sy(float y) { return kBase - y; }

int worthAt(float x) {
    if (x <= kArcX) return 3;
    if (x >= kPaintX) return 1;
    return 2;
}

const char* zoneOf(int w) {
    if (w >= 3) return "ARC";
    if (w == 2) return "MID";
    return "PAINT";
}

float speedScale(float m) {
    if (m >= kPocket0 && m <= kPocket1) return 1.f;
    if (m > kPocket1) return 1.28f + (m - kPocket1) * 1.3f;
    return 0.70f - (kPocket0 - m) * 0.55f;
}

bool solve(float x0, float y0, float x1, float y1, float& vx, float& vy) {
    float dx = x1 - x0;
    float dy = y1 - y0;
    float cosA = std::cos(kAng);
    float sinA = std::sin(kAng);
    float rise = std::tan(kAng) * dx - dy;
    if (dx < 36.f || rise < 8.f) return false;
    float s2 = (0.5f * kG * dx * dx) / (cosA * cosA * rise);
    if (s2 < 1.f) return false;
    float s = std::sqrt(s2);
    vx = s * cosA;
    vy = s * sinA;
    return true;
}

int rgbClamp(int v) { return std::max(0, std::min(15, v)); }

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
        case Mode::Title: return "title";
        case Mode::Aim: return "aim";
        case Mode::Flight: return "flight";
        case Mode::Call: return "call";
        case Mode::Win: return "win";
        case Mode::Lose: return "lose";
        case Mode::Pause: return "pause";
    }
    return "?";
}

Game::Mode Game::shown() const { return mode_ == Mode::Pause ? held_ : mode_; }

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
    sys.vdp.setFogColor(gs::rgb4(3, 2, 6));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.11f, 0.20f, 0.12f);
    feetX_ = 74.f;
    yours_ = true;
    mode_ = Mode::Title;
    meter_ = 0.5f;
    dribble();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    you_ = lane_ = youN_ = laneN_ = worth_ = ending_ = 0;
    won_ = over_ = false;
    yours_ = true;
    feetX_ = 74.f;
    meter_ = 0.5f;
    call_ = Call::Air;
}

void Game::beginMatch() {
    you_ = lane_ = youN_ = laneN_ = ending_ = 0;
    won_ = over_ = false;
    yours_ = true;
    feetX_ = 74.f;
    if (sys_) sys_->apu.noiseBurst(0.18f, 1600.f, 0.05f);
    beginAim();
}

void Game::beginAim() {
    mode_ = Mode::Aim;
    meter_ = 0.f;
    meterDir_ = 1.f;
    aimT_ = 0.f;
    scored_ = false;
    rimHit_ = bank_ = crossed_ = rimSnd_ = bankSnd_ = false;
    clean_ = false;
    call_ = Call::Air;
    blip(yours_ ? 523.f : 294.f);
    if (sys_) sys_->setLight(yours_ ? 30 : 170, yours_ ? 120 : 36, yours_ ? 150 : 28);
}

void Game::launch(float meter) {
    float off = 0.f;
    if (!yours_) off = kLaneCue[laneN_ % kLaneN].off;
    if (meter < kPocket0 || meter > kPocket1) {
        if (std::fabs(off) < 0.5f) off = meter > kPocket1 ? 34.f : -36.f;
    }
    float vx = 0.f, vy = 0.f;
    float x0 = feetX_ + kHand;
    if (!solve(x0, kRelY, kRimX + off, kRimY, vx, vy)) return;
    float sc = speedScale(meter);
    ball_.x = x0;
    ball_.y = kRelY;
    ball_.vx = vx * sc;
    ball_.vy = vy * sc;
    clean_ = std::fabs(off) < 0.5f && meter >= kPocket0 && meter <= kPocket1;
    worth_ = worthAt(feetX_);
    scored_ = rimHit_ = bank_ = crossed_ = rimSnd_ = bankSnd_ = false;
    flight_ = 0.f;
    scoreAt_ = -1.f;
    crossX_ = x0;
    aimOff_ = off;
    mode_ = Mode::Flight;
    blip(yours_ ? 680.f : 392.f);
}

void Game::stepBall(float dt) {
    if (scored_) {
        ball_.y += ball_.vy * dt - 0.5f * kG * dt * dt;
        ball_.vy -= kG * dt;
        ball_.x += ball_.vx * dt;
        ball_.vx += (kRimX - ball_.x) * 7.f * dt;
        ball_.vx *= 0.94f;
        if (ball_.y < kBallR) {
            ball_.y = kBallR;
            if (ball_.vy < 0.f) ball_.vy = -ball_.vy * 0.28f;
            ball_.vx *= 0.45f;
        }
        return;
    }

    float px = ball_.x;
    float py = ball_.y;
    ball_.y += ball_.vy * dt - 0.5f * kG * dt * dt;
    ball_.vy -= kG * dt;
    ball_.x += ball_.vx * dt;

    if (!clean_) {
        if (std::fabs(ball_.x - kRimX) < 50.f && std::fabs(ball_.y - kRimY) < 30.f) {
            const float tubes[2] = {kRimX - kTubeX, kRimX + kTubeX};
            for (float tx : tubes) {
                float dx = ball_.x - tx;
                float dy = ball_.y - kRimY;
                float d = std::hypot(dx, dy);
                float minD = kBallR + kTubeR;
                if (d >= minD || d < 1e-3f) continue;
                float nx = dx / d;
                float ny = dy / d;
                ball_.x += nx * (minD - d);
                ball_.y += ny * (minD - d);
                float vn = ball_.vx * nx + ball_.vy * ny;
                if (vn < 0.f) {
                    ball_.vx -= 1.5f * vn * nx;
                    ball_.vy -= 1.5f * vn * ny;
                }
                rimHit_ = true;
            }
        }
        if (ball_.x + kBallR > kBoardX && ball_.vx > 0.f && ball_.y > kRimY - 10.f && ball_.y < kRimY + 48.f) {
            ball_.x = kBoardX - kBallR;
            ball_.vx = -std::fabs(ball_.vx) * 0.48f;
            ball_.vy *= 0.86f;
            bank_ = true;
        }
    }

    if (ball_.y < kBallR) {
        ball_.y = kBallR;
        if (ball_.vy < 0.f) ball_.vy = -ball_.vy * 0.42f;
        ball_.vx *= 0.68f;
    }

    if (py >= kRimY && ball_.y < kRimY) {
        float den = py - ball_.y;
        float u = den > 1e-5f ? (py - kRimY) / den : 0.f;
        float cx = px + (ball_.x - px) * u;
        crossed_ = true;
        crossX_ = cx;
        if (clean_ && std::fabs(cx - kRimX) <= kCountR) {
            scored_ = true;
            ball_.x = cx;
            ball_.y = kRimY - 0.3f;
            ball_.vx *= 0.08f;
            ball_.vy = std::min(ball_.vy, -30.f);
        }
    }
}

void Game::fly(float dt) {
    const int n = 6;
    float h = dt / float(n);
    for (int i = 0; i < n; i++) {
        bool wasRim = rimHit_;
        bool wasBank = bank_;
        flight_ += h;
        stepBall(h);
        if (scored_ && scoreAt_ < 0.f) scoreAt_ = flight_;
        if (rimHit_ && !wasRim && !rimSnd_) {
            rimSnd_ = true;
            if (sys_) sys_->apu.noiseBurst(0.36f, 2100.f, 0.07f);
            blip(170.f);
        }
        if (bank_ && !wasBank && !bankSnd_) {
            bankSnd_ = true;
            if (sys_) sys_->apu.noiseBurst(0.30f, 620.f, 0.08f);
        }
    }
    bool onFloor = ball_.y <= kBallR + 1.2f && std::fabs(ball_.vy) < 46.f && std::fabs(ball_.vx) < 55.f && flight_ > 0.35f;
    bool out = ball_.x < -24.f || ball_.x > 340.f || ball_.y > 260.f;
    if (scored_) {
        if (flight_ > scoreAt_ + 0.42f) finishShot();
    } else if (onFloor || out || flight_ > 2.8f) {
        finishShot();
    }
}

void Game::finishShot() {
    if (scored_) {
        if (yours_) you_ += worth_;
        else lane_ += worth_;
        if (bank_) call_ = Call::Bank;
        else if (rimHit_) call_ = Call::Count;
        else call_ = Call::Swish;
        if (yours_) chord(523.f, 659.f, 784.f, 0.30f);
        else chord(349.f, 440.f, 523.f, 0.26f);
        if (sys_ && yours_ && !bot_) sys_->rumble(0.22f, 0.5f, 70);
    } else {
        if (rimHit_) call_ = Call::Rim;
        else if (bank_ || (crossed_ && crossX_ > kRimX)) call_ = Call::Long;
        else if (crossed_ || ball_.x < kRimX - 6.f) call_ = Call::Short;
        else call_ = Call::Air;
        blip(98.f);
        if (sys_) sys_->apu.noiseBurst(0.12f, 280.f, 0.05f);
    }
    if (yours_) youN_++;
    else laneN_++;
    if (you_ >= kLine && lane_ < kLine) ending_ = 1;
    else if (lane_ >= kLine && you_ < kLine) ending_ = 2;
    else ending_ = 0;
    callT_ = scored_ ? 0.52f : 0.40f;
    flash_ = scored_ ? 0.16f : 0.04f;
    mode_ = Mode::Call;
}

void Game::afterCall() {
    if (ending_ == 1) win();
    else if (ending_ == 2) lose();
    else {
        yours_ = !yours_;
        beginAim();
    }
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    chord(392.f, 523.f, 784.f, 0.95f);
    if (sys_) {
        sys_->setLight(255, 190, 40);
        if (!bot_) sys_->rumble(0.4f, 0.75f, 180);
    }
}

void Game::lose() {
    won_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    blip(82.f);
    if (sys_) sys_->setLight(180, 28, 22);
}

void Game::steer(float dt, float axis) {
    const bool scripted = bot_ || !yours_;
    if (scripted) {
        float goal = yours_ ? kYouX[std::min(youN_, 2)] : kLaneCue[laneN_ % kLaneN].x;
        float d = goal - feetX_;
        float step = 210.f * dt;
        if (std::fabs(d) <= step) feetX_ = goal;
        else feetX_ += std::copysign(step, d);
    } else {
        feetX_ += axis * 156.f * dt;
    }
    feetX_ = clampf(feetX_, kFeetMin, kFeetMax);
}

void Game::tickMeter(bool shoot, float dt) {
    if (mode_ == Mode::Title) {
        meter_ = 0.50f + 0.045f * std::sin(clock_ * 2.4f);
        return;
    }
    float prev = meter_;
    meter_ += meterDir_ * kMeterRate * dt;
    if (meter_ >= 1.f) {
        meter_ = 1.f;
        meterDir_ = -1.f;
    } else if (meter_ <= 0.f) {
        meter_ = 0.f;
        meterDir_ = 1.f;
    }
    const bool scripted = bot_ || !yours_;
    if (scripted) {
        float want = yours_ ? 0.50f : kLaneCue[laneN_ % kLaneN].meter;
        float goal = yours_ ? kYouX[std::min(youN_, 2)] : kLaneCue[laneN_ % kLaneN].x;
        if (aimT_ > 0.20f && std::fabs(feetX_ - goal) < 1.6f && meterDir_ > 0.f && prev < want && meter_ >= want)
            launch(want);
        return;
    }
    if (shoot) launch(meter_);
}

void Game::dribble() {
    float s = std::sin(clock_ * 9.0f);
    ball_.x = feetX_ + 10.f;
    ball_.y = kBallR + std::fabs(s) * 26.f;
    ball_.vx = ball_.vy = 0.f;
    if (s >= 0.f && prevSin_ < 0.f && sys_) sys_->apu.noiseBurst(0.06f, 220.f, 0.025f);
    prevSin_ = s;
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z) ||
                pad.pressed(gs::BTN_TURBO);
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    if (pad.down(gs::BTN_LEFT)) in.x -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) in.x += 1.f;
    if (std::fabs(pad.axisX) > 0.22f) in.x = pad.axisX;
    in.x = clampf(in.x, -1.f, 1.f);
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && clock_ > 0.62f) in.start = true;
    return in;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) clock_ += kDt;
    if (flash_ > 0.f) flash_ = std::max(0.f, flash_ - kDt);
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) hush();
    }

    const Input in = bot_ ? botInput() : readPad(sys.pad);

    if (mode_ == Mode::Title) {
        dribble();
        tickMeter(false, kDt);
        if (in.back && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (in.start || in.action) {
            beginMatch();
        }
    } else if (mode_ == Mode::Pause) {
        if (in.start || in.action) mode_ = held_;
        else if (in.back) toTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (in.start || in.action)) beginMatch();
        else if (!bot_ && in.back) toTitle();
    } else if (mode_ == Mode::Call) {
        callT_ -= kDt;
        if (callT_ <= 0.f || (!bot_ && (in.action || in.start))) afterCall();
    } else if (!bot_ && in.start) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Aim) {
        aimT_ += kDt;
        steer(kDt, in.x);
        dribble();
        tickMeter(in.action, kDt);
    } else if (mode_ == Mode::Flight) {
        fly(kDt);
    }

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(1, m.h));
    (void)flip;
    stamp(m, cx, cy, w, h, pal);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (!sys_ || w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::blob(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::max(1, (int)std::lround(w)));
    s.h = int16_t(std::max(1, (int)std::lround(h)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::word(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w == 0) return;
    blob(img, cx, cy, float(img.w), float(img.h), pal, false);
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

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? (int)std::strlen(s) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    const bool win = mode_ == Mode::Win;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int boost = (flash_ > 0.f && y < 120) ? 2 : 0;
        if (win && y < 140) boost += 1;
        if (y < 150) {
            float u = y / 150.f;
            int r = rgbClamp(3 + int(u * 10.f) + boost);
            int g = rgbClamp(2 + int(u * 4.f));
            int b = rgbClamp(8 - int(u * 5.f));
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else if (y < 166) {
            v.lineBackdrop[y] = gs::rgb4(rgbClamp(11 + boost), 6, 3);
        } else {
            int stripe = ((y / 3) & 1) ? 1 : 0;
            v.lineBackdrop[y] = gs::rgb4(4 + stripe, 4, 5);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    backdrop();

    const Mode m = shown();
    const bool aiming = m == Mode::Aim || m == Mode::Title;
    const bool holding = aiming;
    const float rimSy = Sy(kRimY);
    const int who = (m == Mode::Title || yours_) ? PAL_YOU : PAL_LANE;
    const int pts = worthAt(feetX_);

    float off = 0.f;
    float showMeter = meter_;
    if (m == Mode::Aim && !yours_) off = kLaneCue[laneN_ % kLaneN].off;
    if (aiming && (showMeter < kPocket0 || showMeter > kPocket1)) {
        if (std::fabs(off) < 0.5f) off = showMeter > kPocket1 ? 34.f : -36.f;
    }
    const bool pocket = aiming && std::fabs(off) < 0.5f && showMeter >= kPocket0 && showMeter <= kPocket1;

    if (m == Mode::Title) {
        word(art_.hoop, 78.f, 58.f, PAL_WORD);
        word(art_.seven, 86.f, 86.f, PAL_WORD);
    } else if (m == Mode::Win) {
        word(art_.first, 118.f, 108.f, PAL_WORD);
    } else if (m == Mode::Call) {
        const gs::Image* img = &art_.airWord;
        int pal = PAL_BAD;
        if (call_ == Call::Swish) {
            img = &art_.swish;
            pal = PAL_GOOD;
        } else if (call_ == Call::Count) {
            img = &art_.count;
            pal = PAL_WORD;
        } else if (call_ == Call::Bank) {
            img = &art_.bank;
            pal = PAL_WORD;
        } else if (call_ == Call::Rim) {
            img = &art_.rimWord;
        } else if (call_ == Call::Short) {
            img = &art_.shortWord;
        } else if (call_ == Call::Long) {
            img = &art_.longWord;
        }
        word(*img, 108.f, 112.f, pal);
    }
    if (aiming) word(art_.num[pts - 1], feetX_, kBase - 70.f, pocket ? PAL_GOOD : PAL_WORD);

    if (aiming) blob(art_.bracket, kRimX, rimSy, 18.f, 20.f, pocket ? PAL_GOOD : PAL_RIM);

    spr(art_.rim, kRimX, rimSy, 18.f, PAL_RIM);
    int spin = int((holding ? clock_ : flight_) * 12.f) & 1;
    float ballH = 14.f;
    blob(art_.shadow, ball_.x, kBase + 3.f, clampf(16.f - ball_.y * 0.04f, 6.f, 16.f), 5.f, PAL_SHADOW, true);
    spr(art_.ball[spin], ball_.x, Sy(ball_.y), ballH, PAL_BALL);

    int netFrame = (scored_ && (m == Mode::Flight || m == Mode::Call)) ? 1 : (int(clock_ * 5.f) & 1);
    spr(art_.net[netFrame], kRimX - 1.f, rimSy + 20.f, 30.f, PAL_NET);

    int pose = (m == Mode::Flight || m == Mode::Call || m == Mode::Win) ? 1 : 0;
    blob(art_.shadow, feetX_, kBase + 4.f, 24.f, 6.f, PAL_SHADOW, true);
    spr(art_.player[pose], feetX_, kBase - 26.f, 52.f, who);

    spr(art_.board, 272.f, rimSy - 6.f, 40.f, PAL_BOARD);
    stamp(art_.pole, 278.f, rimSy + 2.f, 46.f, 5.f, PAL_IRON);
    spr(art_.pole, 304.f, (rimSy + kBase) * 0.5f, std::max(16.f, kBase - rimSy), PAL_IRON);

    if (aiming) {
        float vx = 0.f, vy = 0.f;
        float x0 = feetX_ + kHand;
        if (solve(x0, kRelY, kRimX + off, kRimY, vx, vy)) {
            float sc = speedScale(showMeter);
            vx *= sc;
            vy *= sc;
            float x = x0;
            float y = kRelY;
            int pal = pocket ? PAL_GOOD : PAL_BAD;
            for (int i = 0; i < 14; i++) {
                float h = 0.055f;
                y += vy * h - 0.5f * kG * h * h;
                vy -= kG * h;
                x += vx * h;
                if (y < 4.f || x < 0.f || x > 312.f) break;
                blob(art_.solid, x, Sy(y), 3.f, 3.f, pal);
            }
        }
    }

    const float barX = 96.f;
    const float barW = 128.f;
    const float barY = 214.f;
    if (aiming) {
        blob(art_.solid, barX + barW * 0.5f, barY, barW, 4.f, PAL_METER);
        float pocketW = barW * (kPocket1 - kPocket0);
        blob(art_.solid, barX + barW * ((kPocket0 + kPocket1) * 0.5f), barY, pocketW, 7.f, PAL_GOOD);
        float nx = barX + clampf(showMeter, 0.f, 1.f) * barW;
        blob(art_.solid, nx, barY, 3.f, 12.f, pocket ? PAL_GOOD : PAL_WORD);
    }

    for (int i = 0; i < 7; i++) {
        bool onY = i < you_;
        bool onL = i < lane_;
        blob(art_.solid, 16.f + i * 11.f, 40.f, onY ? 8.f : 6.f, onY ? 8.f : 6.f, onY ? PAL_GOOD : PAL_IRON);
        blob(art_.solid, 236.f + i * 11.f, 40.f, onL ? 8.f : 6.f, onL ? 8.f : 6.f, onL ? PAL_RIM : PAL_IRON);
    }

    word(art_.floor7, 214.f, 190.f, PAL_INK);
    blob(art_.solid, kArcX, 186.f, 2.f, 28.f, PAL_INK);
    blob(art_.solid, kPaintX, 186.f, 2.f, 28.f, PAL_INK);
    blob(art_.solid, 268.f, 186.f, 2.f, 28.f, PAL_INK);
    blob(art_.solid, 160.f, 172.f, 300.f, 2.f, PAL_INK);
    blob(art_.solid, (kPaintX + 268.f) * 0.5f, 188.f, 268.f - kPaintX, 30.f, PAL_COURT);

    spr(art_.seats, 126.f, 142.f, 28.f, PAL_SEAT);
    spr(art_.fence, 50.f, 150.f, 22.f, PAL_IRON);
    spr(art_.fence, 150.f, 150.f, 22.f, PAL_IRON);
    spr(art_.fence, 250.f, 148.f, 22.f, PAL_IRON);
    spr(art_.lamp, 300.f, 34.f, 18.f, PAL_LAMP);
    spr(art_.moon, 28.f, 30.f, 16.f, PAL_LAMP);

    char buf[48];
    const bool six = (you_ == 6 || lane_ == 6) && m != Mode::Win && m != Mode::Lose && m != Mode::Title;
    if (m == Mode::Title) {
        hudC(0, "FIRST TO SEVEN", PAL_WORD);
        hudC(1, "ARC 3   MID 2   PAINT 1", PAL_INK);
        hudC(2, "Z SHOOTS THE GREEN", PAL_GOOD);
    } else if (m == Mode::Pause) {
        hudC(0, "PAUSED", PAL_WORD);
        hudC(2, "ENTER RESUMES", PAL_INK);
    } else if (m == Mode::Win) {
        hudC(0, "FIRST TO SEVEN", PAL_WORD);
        std::snprintf(buf, sizeof buf, "YOU %d  LANE %d", you_, lane_);
        hudC(1, buf, PAL_GOOD);
        hudC(2, "ENTER PLAYS AGAIN", PAL_INK);
    } else if (m == Mode::Lose) {
        hudC(0, "LANE REACHED SEVEN", PAL_BAD);
        std::snprintf(buf, sizeof buf, "YOU %d  LANE %d", you_, lane_);
        hudC(1, buf, PAL_INK);
        hudC(2, "ENTER TRIES AGAIN", PAL_INK);
    } else {
        std::snprintf(buf, sizeof buf, "YOU %d", you_);
        hud(1, 0, buf, PAL_GOOD);
        std::snprintf(buf, sizeof buf, "LANE %d", lane_);
        hud(40 - (int)std::strlen(buf) - 1, 0, buf, PAL_BAD);
        hudC(0, "TO 7", PAL_WORD);
        std::snprintf(buf, sizeof buf, "%s +%d", zoneOf(holding ? pts : worth_), holding ? pts : worth_);
        hudC(1, buf, PAL_INK);
        if (six) hudC(2, "SIX IS STILL SHORT", PAL_WORD);
        else if (m == Mode::Aim && pocket && yours_) hudC(2, "POCKET", PAL_GOOD);
        else if (m == Mode::Aim && yours_) hudC(2, "TIME THE GREEN", PAL_INK);
        else if (m == Mode::Aim) hudC(2, "LANE SHOT", PAL_BAD);
        else if (m == Mode::Call && scored_) {
            std::snprintf(buf, sizeof buf, "+%d", worth_);
            hudC(2, buf, yours_ ? PAL_GOOD : PAL_BAD);
        }
    }
}

}  // namespace hoopseven
