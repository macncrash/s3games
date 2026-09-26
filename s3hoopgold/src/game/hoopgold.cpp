#include "game/hoopgold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace hoopgold {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kG = 520.f;
constexpr float kPi = 3.14159265f;
constexpr float kAng = 63.f * kPi / 180.f;
constexpr float kShootX = 72.f;
constexpr float kShootY = 58.f;
constexpr float kRimX = 228.f;
constexpr float kRimY = 142.f;
constexpr float kBase = 202.f;
constexpr float kCountR = 14.f;
constexpr float kMouth = 9.f;
constexpr float kTubeX = 28.f;
constexpr float kTubeR = 3.2f;
constexpr float kBallR = 6.2f;
constexpr float kBoardX = 278.f;
constexpr float kMeterRate = 0.64f;
constexpr float kPocket0 = 0.45f;
constexpr float kPocket1 = 0.55f;
constexpr float kAimMin = kRimX - 56.f;
constexpr float kAimMax = kRimX + 40.f;

// Rack order. The first three gold rims pay 6 if they go in.
// Cream counts one and is refused when it would reach the line.
constexpr bool kGoldShot[kRack] = {true, true, true, false, false, true};
constexpr float kBias[kRack] = {20.f, -16.f, 12.f, -22.f, 18.f, -10.f};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float Sy(float y) { return kBase - y; }

// Outside the green band the solved arc misses the mouth on purpose.
float speedScale(float m) {
    if (m >= kPocket0 && m <= kPocket1) {
        float u = (m - kPocket0) / (kPocket1 - kPocket0);
        return 0.994f + u * 0.012f;
    }
    if (m > kPocket1) return 1.08f + (m - kPocket1) * 0.85f;
    return 0.90f - (kPocket0 - m) * 0.70f;
}

bool solveShot(float targetX, float targetY, float& vx, float& vy) {
    float dx = targetX - kShootX;
    float dy = targetY - kShootY;
    float cosA = std::cos(kAng);
    float sinA = std::sin(kAng);
    float rise = std::tan(kAng) * dx - dy;
    if (rise < 8.f || dx < 24.f) return false;
    float s2 = (0.5f * kG * dx * dx) / (cosA * cosA * rise);
    if (s2 <= 1.f) return false;
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

Game::Mode Game::view() const { return mode_ == Mode::Pause ? held_ : mode_; }

bool Game::paid() const {
    const int bare = gold_ + cream_;
    return finisherGold_ && gold_ >= 1 && score_ >= kLine && bare < kLine && score_ == gold_ * 2 + cream_;
}

bool Game::pocketNow() const {
    return meter_ >= kPocket0 && meter_ <= kPocket1 && std::fabs(aim_ - kRimX) <= kMouth;
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
    sys.vdp.setFogColor(gs::rgb4(3, 2, 4));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.14f, 0.18f, 0.12f);
    for (int i = 0; i < kRack; i++) result_[i] = -2;
    aim_ = kRimX;
    callGold_ = true;
    mode_ = Mode::Title;
    clock_ = 0;
    dribble();
}

void Game::toTitle() {
    score_ = gold_ = cream_ = shot_ = refused_ = 0;
    finisherGold_ = won_ = over_ = false;
    for (int i = 0; i < kRack; i++) result_[i] = -2;
    say_ = "";
    call_ = Call::None;
    callGold_ = true;
    aim_ = kRimX;
    mode_ = Mode::Title;
}

void Game::beginRound() {
    score_ = gold_ = cream_ = shot_ = refused_ = 0;
    finisherGold_ = won_ = over_ = false;
    for (int i = 0; i < kRack; i++) result_[i] = -2;
    say_ = "";
    call_ = Call::None;
    beginAim();
}

void Game::beginAim() {
    if (shot_ < 0 || shot_ >= kRack) {
        lose();
        return;
    }
    callGold_ = kGoldShot[shot_];
    aim_ = kRimX + kBias[shot_];
    meter_ = 0;
    meterDir_ = 1.f;
    mode_ = Mode::Aim;
    blip(callGold_ ? 523.f : 330.f);
    if (sys_) sys_->setLight(callGold_ ? 255 : 80, callGold_ ? 170 : 60, callGold_ ? 40 : 30);
}

void Game::launch(float meter) {
    float vx = 0, vy = 0;
    if (!solveShot(aim_, kRimY, vx, vy)) return;
    float sc = speedScale(meter);
    ball_.x = kShootX;
    ball_.y = kShootY;
    ball_.vx = vx * sc;
    ball_.vy = vy * sc;
    scored_ = false;
    rimHit_ = false;
    bank_ = false;
    crossed_ = false;
    rimSnd_ = false;
    bankSnd_ = false;
    flight_ = 0;
    scoreAt_ = -1.f;
    crossX_ = 0;
    mode_ = Mode::Flight;
    blip(callGold_ ? 680.f : 440.f);
}

void Game::stepBall(float dt) {
    float px = ball_.x;
    float py = ball_.y;
    ball_.y += ball_.vy * dt - 0.5f * kG * dt * dt;
    ball_.vy -= kG * dt;
    ball_.x += ball_.vx * dt;

    if (scored_) {
        ball_.vx *= 0.90f;
        ball_.vx += (kRimX - ball_.x) * 8.f * dt;
        if (ball_.y < kBallR) {
            ball_.y = kBallR;
            ball_.vy = 0;
            ball_.vx *= 0.5f;
        }
        return;
    }

    if (std::fabs(ball_.x - kRimX) < 46.f && std::fabs(ball_.y - kRimY) < 28.f) {
        const float tubes[2] = {kRimX - kTubeX, kRimX + kTubeX};
        for (float tx : tubes) {
            float dx = ball_.x - tx;
            float dy = ball_.y - kRimY;
            float d = std::hypot(dx, dy);
            float minD = kBallR + kTubeR;
            if (d >= minD || d < 1e-4f) continue;
            float nx = dx / d;
            float ny = dy / d;
            float pen = minD - d;
            ball_.x += nx * pen;
            ball_.y += ny * pen;
            float vn = ball_.vx * nx + ball_.vy * ny;
            if (vn < 0.f) {
                ball_.vx -= 1.55f * vn * nx;
                ball_.vy -= 1.55f * vn * ny;
            }
            rimHit_ = true;
        }
    }

    if (ball_.x + kBallR > kBoardX && ball_.vx > 0.f && ball_.y > kRimY - 8.f && ball_.y < kRimY + 52.f) {
        ball_.x = kBoardX - kBallR;
        ball_.vx = -std::fabs(ball_.vx) * 0.52f;
        ball_.vy *= 0.92f;
        bank_ = true;
    }

    if (ball_.y < kBallR) {
        ball_.y = kBallR;
        if (ball_.vy < 0.f) ball_.vy = -ball_.vy * 0.45f;
        ball_.vx *= 0.7f;
    }

    if (py >= kRimY && ball_.y < kRimY) {
        float u = (py - kRimY) / (py - ball_.y + 1e-8f);
        float cx = px + (ball_.x - px) * u;
        crossed_ = true;
        crossX_ = cx;
        if (std::fabs(cx - kRimX) <= kCountR) {
            scored_ = true;
            ball_.x = cx;
            ball_.y = kRimY - 0.4f;
            ball_.vx *= 0.10f;
        }
    }
}

void Game::fly(float dt) {
    const int n = 8;
    float h = dt / float(n);
    for (int i = 0; i < n; i++) {
        bool wasRim = rimHit_;
        bool wasBank = bank_;
        flight_ += h;
        stepBall(h);
        if (scored_ && scoreAt_ < 0.f) scoreAt_ = flight_;
        if (rimHit_ && !wasRim && !rimSnd_) {
            rimSnd_ = true;
            if (sys_) sys_->apu.noiseBurst(0.38f, 2200.f, 0.08f);
            blip(180.f);
        }
        if (bank_ && !wasBank && !bankSnd_) {
            bankSnd_ = true;
            if (sys_) sys_->apu.noiseBurst(0.32f, 640.f, 0.09f);
        }
    }
    bool onFloor = ball_.y <= kBallR + 1.4f && std::fabs(ball_.vy) < 48.f && std::fabs(ball_.vx) < 60.f && flight_ > 0.4f;
    bool out = ball_.x < -30.f || ball_.x > 360.f || ball_.y > 280.f;
    if (scored_) {
        if (flight_ > scoreAt_ + 0.40f) finishShot();
    } else if (onFloor || out || flight_ > 3.1f) {
        finishShot();
    }
}

void Game::finishShot() {
    int mark = 0;
    if (!scored_) {
        mark = 0;
        if (bank_) call_ = Call::Long;
        else if (rimHit_) call_ = Call::Rim;
        else if (!crossed_) call_ = Call::Air;
        else if (crossX_ < kRimX) call_ = Call::Short;
        else call_ = Call::Long;
        say_ = call_ == Call::Rim ? "RIM" : call_ == Call::Short ? "SHORT" : call_ == Call::Air ? "AIR" : "LONG";
        blip(120.f);
        if (sys_) sys_->apu.noiseBurst(0.16f, 360.f, 0.06f);
    } else if (!callGold_) {
        if (score_ + 1 >= kLine) {
            refused_++;
            mark = -1;
            call_ = Call::Refuse;
            say_ = "NOT DOUBLE";
            blip(98.f);
        } else {
            cream_++;
            score_ += 1;
            mark = 1;
            call_ = Call::Cream;
            say_ = "CREAM";
            blip(392.f);
        }
    } else {
        gold_++;
        score_ += 2;
        mark = 2;
        if (bank_) call_ = Call::Bank;
        else if (rimHit_) call_ = Call::Count;
        else call_ = Call::Swish;
        say_ = call_ == Call::Bank ? "BANK" : call_ == Call::Count ? "COUNT" : "SWISH";
        if (score_ >= kLine) finisherGold_ = true;
        chord(523.f, 659.f, 784.f, 0.28f);
        if (sys_ && !bot_) sys_->rumble(0.28f, 0.62f, 90);
    }
    if (shot_ >= 0 && shot_ < kRack) result_[shot_] = mark;
    shot_++;
    callT_ = 0.42f;
    flash_ = scored_ && mark > 0 ? 0.16f : 0.04f;
    mode_ = Mode::Call;
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    say_ = "DOUBLE";
    callT_ = 0;
    chord(392.f, 523.f, 784.f, 0.9f);
    if (sys_) {
        sys_->setLight(255, 190, 40);
        if (!bot_) sys_->rumble(0.4f, 0.75f, 180);
    }
}

void Game::lose() {
    won_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    say_ = "NO DOUBLE";
    callT_ = 0;
    blip(90.f);
    if (sys_) sys_->setLight(180, 30, 24);
}

void Game::afterCall() {
    if (paid()) win();
    else if (shot_ >= kRack) lose();
    else beginAim();
}

void Game::aimControl(const Input& in, float dt) {
    if (bot_) {
        float d = kRimX - aim_;
        float step = 110.f * dt;
        if (d > step) aim_ += step;
        else if (d < -step) aim_ -= step;
        else aim_ = kRimX;
        return;
    }
    aim_ += in.x * 128.f * dt;
    aim_ = clampf(aim_, kAimMin, kAimMax);
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
        if (meterDir_ > 0.f && prev < 0.50f && meter_ >= 0.50f && std::fabs(aim_ - kRimX) < 0.75f) launch(0.50f);
        return;
    }
    if (in.action) launch(meter_);
}

void Game::dribble() {
    float s = std::sin(clock_ * 8.2f);
    ball_.x = 86.f;
    ball_.y = 8.f + std::fabs(s) * 24.f;
    ball_.vx = ball_.vy = 0;
    if (s > 0.f && std::sin((clock_ - kDt) * 8.2f) <= 0.f && sys_) sys_->apu.noiseBurst(0.08f, 240.f, 0.03f);
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
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
    if (mode_ == Mode::Title && clock_ > 0.45f) in.start = true;
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
        meter_ = 0.5f + 0.5f * std::sin(clock_ * kMeterRate * kPi);
        if (meter_ < 0.f) meter_ = 0.f;
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
        dribble();
        aimControl(in, kDt);
        tickMeter(in, kDt);
    } else if (mode_ == Mode::Flight) {
        fly(kDt);
    }

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(1, m.h));
    stamp(m, cx, cy, w, h, pal, shadow);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.hflip = false;
    s.shadow = shadow;
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
        if (y < 168) {
            float u = y / 168.f;
            int boost = (flash_ > 0.f || win) && y < 90 ? (win ? 3 : 2) : 0;
            int r = rgbClamp(2 + int(u * 11.f) + boost);
            int g = rgbClamp(2 + int(u * 5.f) + (win ? 1 : 0));
            int b = rgbClamp(9 - int(u * 6.f));
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else if (y < 204) {
            int stripe = ((y / 3) & 1) ? 1 : 0;
            v.lineBackdrop[y] = gs::rgb4(4 + stripe, 3, 3);
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 2, 3);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    const Mode m = view();
    const bool showArc = m == Mode::Aim || m == Mode::Title;
    const bool holding = m == Mode::Aim || m == Mode::Title;
    const float rimSy = Sy(kRimY);
    const int rimPal = callGold_ ? PAL_GOLD : PAL_CREAM;
    const bool pocket = m == Mode::Aim && pocketNow();

    spr(art_.sun, 28.f, 26.f, 20.f, PAL_LAMP);
    spr(art_.fence, 250.f, 158.f, 30.f, PAL_FENCE);
    spr(art_.fence, 100.f, 160.f, 26.f, PAL_FENCE);

    blob(art_.blot, 160.f, 200.f, 300.f, 2.f, PAL_COURT);
    blob(art_.blot, 228.f, 184.f, 86.f, 2.f, PAL_COURT);
    blob(art_.blot, 188.f, 184.f, 2.f, 32.f, PAL_COURT);
    blob(art_.blot, 268.f, 184.f, 2.f, 32.f, PAL_COURT);

    spr(art_.pole, 292.f, (rimSy + 200.f) * 0.5f, std::max(12.f, 200.f - rimSy), PAL_IRON);
    spr(art_.board, 262.f, rimSy - 8.f, 42.f, PAL_BOARD);
    int netFrame = int(clock_ * 6.f) & 1;
    if (scored_ && m == Mode::Flight) netFrame = 1;
    spr(art_.net[netFrame], kRimX - 1.f, rimSy + 18.f, 34.f, PAL_NET);
    spr(art_.rim, kRimX, rimSy, 16.f, rimPal);

    word(callGold_ ? art_.x2 : art_.x1, kRimX, rimSy - 22.f, callGold_ ? PAL_WORD : PAL_CREAM);

    for (int i = 0; i < kRack; i++) {
        float cy = 48.f + i * 16.f;
        float h = (i == shot_ && m == Mode::Aim) ? 13.f : 10.f;
        if (i == shot_ && m == Mode::Aim) cy += std::sin(clock_ * 6.f) * 1.4f;
        int pal = kGoldShot[i] ? PAL_GOLD : PAL_CREAM;
        spr(art_.chip, 16.f, cy, h, pal);
        if (result_[i] == 2) blob(art_.blot, 16.f, cy, 4.f, 4.f, PAL_GOOD);
        else if (result_[i] == 1) blob(art_.blot, 16.f, cy, 4.f, 4.f, PAL_CREAM);
        else if (result_[i] == 0 || result_[i] == -1) blob(art_.blot, 16.f, cy, 4.f, 4.f, PAL_BAD);
    }

    if (showArc) {
        float vx = 0, vy = 0;
        float target = (m == Mode::Title) ? kRimX : aim_;
        float meter = (m == Mode::Title) ? 0.50f : meter_;
        if (solveShot(target, kRimY, vx, vy)) {
            float sc = speedScale(meter);
            vx *= sc;
            vy *= sc;
            float x = kShootX;
            float y = kShootY;
            int pal = pocket ? PAL_GOOD : rimPal;
            for (int i = 0; i < 14; i++) {
                float h = 0.07f;
                y += vy * h - 0.5f * kG * h * h;
                vy -= kG * h;
                x += vx * h;
                if (y < 4.f || x > 310.f) break;
                blob(art_.blot, x, Sy(y), 4.f, 4.f, pal);
            }
        }
    }

    if (m == Mode::Aim || m == Mode::Title) {
        int pal = pocket ? PAL_GOOD : rimPal;
        blob(art_.bracket, aim_, rimSy, 14.f, 16.f, pal);
    }

    const float barX = 96.f;
    const float barW = 140.f;
    const float barY = 188.f;
    if (m == Mode::Aim || m == Mode::Title) {
        blob(art_.blot, barX + barW * 0.5f, barY, barW, 5.f, PAL_METER);
        float pocketW = barW * (kPocket1 - kPocket0);
        blob(art_.blot, barX + barW * 0.5f, barY, pocketW, 7.f, PAL_GOOD);
        float nx = barX + clampf(meter_, 0.f, 1.f) * barW;
        blob(art_.blot, nx, barY, 3.f, 11.f, pocket ? PAL_GOOD : PAL_WORD);
    }

    int pose = (m == Mode::Flight || m == Mode::Call) ? 1 : 0;
    spr(art_.player[pose], 50.f, 202.f - 28.f, 56.f, PAL_YOU);

    float bx = holding ? ball_.x : ball_.x;
    float by = holding ? ball_.y : ball_.y;
    int spin = int((holding ? clock_ : flight_) * 12.f) & 1;
    float ballH = 16.f;
    blob(art_.shadow, bx, kBase + 2.f, clampf(18.f - (by - kBallR) * 0.05f, 7.f, 18.f), 5.f, PAL_SHADOW, true);
    spr(art_.ball[spin], bx, Sy(by), ballH, PAL_BALL);
    blob(art_.shadow, 50.f, kBase + 3.f, 26.f, 6.f, PAL_SHADOW, true);

    if (m == Mode::Title) word(art_.hoop, 78.f, 40.f, PAL_WORD);
    if (m == Mode::Win) word(art_.doubled, 168.f, 96.f, PAL_WORD);
    if (m == Mode::Lose) word(art_.noDouble, 168.f, 96.f, PAL_BAD);
    if (m == Mode::Pause) word(art_.hoop, 168.f, 96.f, PAL_INK);
    if (m == Mode::Call) {
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
        } else if (call_ == Call::Cream) {
            img = &art_.x1;
            pal = PAL_CREAM;
        } else if (call_ == Call::Refuse) {
            img = &art_.notWord;
            pal = PAL_BAD;
        } else if (call_ == Call::Rim) {
            img = &art_.rimWord;
        } else if (call_ == Call::Short) {
            img = &art_.shortWord;
        } else if (call_ == Call::Long) {
            img = &art_.longWord;
        } else if (call_ == Call::Air) {
            img = &art_.airWord;
        }
        word(*img, 150.f, 108.f, pal);
    }

    char buf[48];
    if (m == Mode::Title) {
        hudC(0, "ONLY THE GOLD COUNTS DOUBLE", PAL_WORD);
        hudC(1, "CREAM CANNOT FINISH THE LINE", PAL_CREAM);
        hudC(25, "LINE 6   GOLD 2   CREAM 1", PAL_INK);
        hudC(26, "LEFT RIGHT ONTO THE RIM", PAL_INK);
        hudC(27, "Z SHOOTS THE GREEN", PAL_GOOD);
    } else if (m == Mode::Pause) {
        hudC(0, "PAUSED", PAL_WORD);
        hudC(27, "ENTER RESUMES", PAL_INK);
    } else if (m == Mode::Win) {
        hudC(0, "ONLY THE GOLD COUNTS DOUBLE", PAL_WORD);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hudC(1, buf, PAL_GOOD);
        hudC(27, "ENTER PLAYS AGAIN", PAL_INK);
    } else if (m == Mode::Lose) {
        hudC(0, "CREAM DOES NOT BUY THE DOUBLE", PAL_BAD);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hudC(1, buf, PAL_INK);
        hudC(27, "ENTER TRIES AGAIN", PAL_INK);
    } else {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hud(1, 0, buf, PAL_WORD);
        std::snprintf(buf, sizeof buf, "LINE %d", kLine);
        hud(40 - (int)std::strlen(buf) - 1, 0, buf, PAL_INK);
        std::snprintf(buf, sizeof buf, "BARE %d", gold_ + cream_);
        hud(1, 1, buf, bare() < kLine ? PAL_GOOD : PAL_BAD);
        hudC(1, callGold_ ? "GOLD RIM X2" : "CREAM RIM X1", callGold_ ? PAL_WORD : PAL_CREAM);
        std::snprintf(buf, sizeof buf, "SHOT %d/%d", std::min(shot_ + 1, kRack), kRack);
        hud(40 - (int)std::strlen(buf) - 1, 1, buf, PAL_INK);
        if (m == Mode::Aim) {
            hudC(26, pocket ? "POCKET" : "LEFT RIGHT ONTO THE RIM", pocket ? PAL_GOOD : PAL_INK);
            hudC(27, "Z SHOOTS", PAL_INK);
        } else if (m == Mode::Call && call_ == Call::Refuse) {
            hudC(12, "CREAM IS NOT THE DOUBLE", PAL_BAD);
        } else if (m == Mode::Call && scored_ && call_ != Call::Refuse) {
            std::snprintf(buf, sizeof buf, "+%d", callGold_ ? 2 : 1);
            hudC(12, buf, callGold_ ? PAL_WORD : PAL_CREAM);
        }
    }
}

}  // namespace hoopgold
