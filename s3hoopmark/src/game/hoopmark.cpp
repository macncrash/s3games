#include "game/hoopmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace hoopmark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kG = 9.81f;
constexpr float kPi = 3.14159265f;
constexpr float kAngle = 52.f * kPi / 180.f;
constexpr float kRimX = 11.05f;
constexpr float kRimY = 3.05f;
constexpr float kBallR = 0.121f;
constexpr float kTubeR = 0.012f;
constexpr float kRimR = 0.237f;
constexpr float kInner = kRimR - kTubeR;
constexpr float kCountR = kInner - kBallR * 0.35f;
constexpr float kBoardX = kRimX + 0.48f;
constexpr float kReleaseY = 2.08f;
constexpr float kThree = 6.25f;
constexpr float kBotDist = 6.9f;
constexpr float kMarkX = kRimX - kBotDist;
constexpr float kMarkZ = 0.f;
constexpr float kMeterRate = 0.82f;
constexpr float kWorldL = 1.5f;
constexpr float kPx = 26.8f;
constexpr float kScr0 = 10.f;
constexpr float kFloor = 190.f;
constexpr float kSetR = 0.32f;
constexpr float kLiftR = 0.30f;
constexpr float kCoinV = 2.2f;
constexpr float kWalkV = 2.5f;
constexpr float kLeaveV = 3.3f;
constexpr float kStep = 0.95f;
constexpr int kMaxShots = 4;
constexpr float kGcx = 36.f;
constexpr float kGcy = 98.f;
constexpr float kGs = 64.f;
constexpr float kIn = 40.f;

float scrX(float x) { return kScr0 + (x - kWorldL) * kPx; }
float scrY(float y) { return kFloor - y * kPx; }
float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float speedScale(float m) {
    if (m >= 0.40f && m <= 0.60f) return 0.998f + ((m - 0.40f) / 0.20f) * 0.014f;
    if (m > 0.60f) return 1.012f + (m - 0.60f) * 0.62f;
    return 0.998f - (0.40f - m) * 0.78f;
}

bool inPocket(float m) { return m >= 0.40f && m <= 0.60f; }

bool solveShot(float x0, float lat, float& vx, float& vy, float& vz) {
    float dx = kRimX - (x0 + 0.22f);
    float dz = lat;
    float dist = std::hypot(dx, dz);
    float cosA = std::cos(kAngle);
    float sinA = std::sin(kAngle);
    float tanA = std::tan(kAngle);
    float rise = tanA * dist - (kRimY - kReleaseY);
    if (rise < 0.05f || dist < 0.3f) return false;
    float s2 = (0.5f * kG * dist * dist) / (cosA * cosA * rise);
    float s = std::sqrt(s2);
    float vh = s * cosA;
    vx = vh * dx / dist;
    vz = vh * dz / dist;
    vy = s * sinA;
    return true;
}

int rgbClamp(int v) { return std::max(0, std::min(15, v)); }

}  // namespace

void Game::tone(int ch, float freq, float vol, float hold) {
    if (!sys_ || ch < 0 || ch > 2) return;
    sys_->apu.tone(ch, freq, vol);
    toneUntil_[ch] = t_ + hold;
}

void Game::chord(float a, float b, float c, float hold) {
    tone(0, a, 0.09f, hold);
    tone(1, b, 0.07f, hold);
    tone(2, c, 0.05f, hold);
}

void Game::pumpAudio() {
    if (!sys_) return;
    for (int ch = 0; ch < 3; ch++) {
        if (toneUntil_[ch] > 0.f && t_ >= toneUntil_[ch]) {
            sys_->apu.tone(ch, 0, 0);
            toneUntil_[ch] = 0;
        }
    }
}

void Game::say(const char* s, float time) {
    say_ = s ? s : "";
    sayT_ = time;
}

int Game::worth() const { return (kRimX - feetX_) >= kThree ? 3 : 2; }

float Game::markDist() const { return std::hypot(coinX_ - kMarkX, coinZ_ - kMarkZ); }

void Game::resetCourt() {
    shots_ = 0;
    shotPts_ = 3;
    onMark_ = opened_ = lifted_ = finished_ = left_ = false;
    won_ = over_ = false;
    scored_ = rimHit_ = bank_ = crossed_ = false;
    rimSnd_ = bankSnd_ = wasIn_ = faceLeft_ = false;
    feetX_ = kMarkX;
    aimZ_ = 0;
    coinX_ = kMarkX - 1.05f;
    coinZ_ = 0.48f;
    meter_ = 0;
    meterDir_ = 1;
    watch_ = 0;
    callT_ = 0;
    flash_ = 0;
    flight_ = 0;
    scoreAt_ = 0;
    crossPast_ = 0;
    sayT_ = 0;
    say_ = "";
    prevDrib_ = 0;
    result_ = "none";
    call_ = Call::Air;
    ball_.x = kMarkX + 0.28f;
    ball_.y = 0.4f;
    ball_.z = 0;
    ball_.vx = ball_.vy = ball_.vz = 0;
    toneUntil_[0] = toneUntil_[1] = toneUntil_[2] = 0;
    if (sys_) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    sys.apu.setMaster(0.70f);
    t_ = 0;
    resetCourt();
    mode_ = Mode::Title;
}

void Game::begin() {
    resetCourt();
    mode_ = Mode::Place;
    tone(0, 392.f, 0.05f, 0.08f);
}

void Game::trySet() {
    if (markDist() > kSetR) {
        say("NOT THE MARK", 0.55f);
        tone(0, 110.f, 0.07f, 0.12f);
        return;
    }
    coinX_ = kMarkX;
    coinZ_ = kMarkZ;
    feetX_ = kMarkX;
    aimZ_ = 0;
    onMark_ = true;
    meter_ = 0;
    meterDir_ = 1;
    mode_ = Mode::Aim;
    say("SHOOT FROM THE MARK", 0.55f);
    tone(0, 523.f, 0.07f, 0.12f);
}

void Game::launch(float meter) {
    float vx, vy, vz;
    if (bot_) {
        feetX_ = kMarkX;
        aimZ_ = 0;
    }
    if (!solveShot(feetX_, aimZ_, vx, vy, vz)) return;
    float sc = speedScale(meter);
    ball_.x = feetX_ + 0.22f;
    ball_.y = kReleaseY;
    ball_.z = 0;
    ball_.vx = vx * sc;
    ball_.vy = vy * sc;
    ball_.vz = vz * sc;
    scored_ = false;
    rimHit_ = false;
    bank_ = false;
    crossed_ = false;
    rimSnd_ = false;
    bankSnd_ = false;
    crossPast_ = 0;
    shotPts_ = worth();
    flight_ = 0;
    scoreAt_ = 0;
    shots_++;
    mode_ = Mode::Flight;
    tone(0, 620.f, 0.07f, 0.10f);
    tone(1, 880.f, 0.04f, 0.12f);
}

void Game::stepBall(float dt) {
    float px = ball_.x, py = ball_.y, pz = ball_.z;
    ball_.y += ball_.vy * dt - 0.5f * kG * dt * dt;
    ball_.vy -= kG * dt;
    ball_.x += ball_.vx * dt;
    ball_.z += ball_.vz * dt;

    if (scored_) {
        ball_.vx += (kRimX - ball_.x) * 3.5f * dt;
        ball_.vz += (0.f - ball_.z) * 3.5f * dt;
        ball_.vx *= 0.90f;
        ball_.vz *= 0.90f;
        if (ball_.y < kRimY - 0.2f) ball_.vy *= 0.94f;
        if (ball_.y < kBallR) {
            ball_.y = kBallR;
            ball_.vy = 0;
        }
        return;
    }

    for (int n = 0; n < 2; n++) {
        float dx = ball_.x - kRimX;
        float dz = ball_.z;
        float d = std::hypot(dx, dz);
        if (d < 1e-6f) break;
        float cx = kRimX + kRimR * dx / d;
        float cz = kRimR * dz / d;
        float ox = ball_.x - cx;
        float oy = ball_.y - kRimY;
        float oz = ball_.z - cz;
        float od = std::sqrt(ox * ox + oy * oy + oz * oz);
        float minD = kBallR + kTubeR;
        if (od >= minD || od < 1e-8f) break;
        float nx = ox / od, ny = oy / od, nz = oz / od;
        float pen = minD - od;
        ball_.x += nx * pen;
        ball_.y += ny * pen;
        ball_.z += nz * pen;
        float vn = ball_.vx * nx + ball_.vy * ny + ball_.vz * nz;
        bool inside = d < kRimR;
        if (vn < 0.f) {
            float rest = inside ? 0.18f : 0.62f;
            ball_.vx -= (1.f + rest) * vn * nx;
            ball_.vy -= (1.f + rest) * vn * ny;
            ball_.vz -= (1.f + rest) * vn * nz;
            if (inside && ball_.vy > -0.4f) ball_.vy = std::min(ball_.vy, -0.8f);
            float fr = inside ? 0.86f : 0.94f;
            ball_.vx *= fr;
            ball_.vz *= fr;
            if (inside) ball_.vy *= 0.92f;
        }
        rimHit_ = true;
    }

    if (ball_.x + kBallR > kBoardX && ball_.vx > 0.f && std::fabs(ball_.z) < 0.92f && ball_.y > kRimY - 0.5f &&
        ball_.y < kRimY + 1.1f) {
        ball_.x = kBoardX - kBallR;
        ball_.vx = -std::fabs(ball_.vx) * 0.58f;
        ball_.vy *= 0.94f;
        ball_.vz *= 0.88f;
        bank_ = true;
    }

    if (ball_.y < kBallR) {
        ball_.y = kBallR;
        if (ball_.vy < 0.f) ball_.vy = -ball_.vy * 0.64f;
        ball_.vx *= 0.78f;
        ball_.vz *= 0.78f;
    }

    if (py >= kRimY && ball_.y < kRimY && !scored_) {
        float u = (py - kRimY) / (py - ball_.y + 1e-12f);
        float cxp = px + (ball_.x - px) * u;
        float czp = pz + (ball_.z - pz) * u;
        float rad = std::hypot(cxp - kRimX, czp);
        crossed_ = true;
        crossPast_ = cxp - kRimX;
        if (rad <= kCountR) {
            scored_ = true;
            scoreAt_ = flight_;
        }
    }
}

void Game::fly(float dt) {
    const int n = 8;
    float h = dt / float(n);
    for (int i = 0; i < n; i++) {
        flight_ += h;
        stepBall(h);
    }
    if (rimHit_ && !rimSnd_) {
        rimSnd_ = true;
        sys_->apu.noiseBurst(0.42f, 2400.f, 0.09f);
        tone(2, 210.f, 0.06f, 0.07f);
    }
    if (bank_ && !bankSnd_) {
        bankSnd_ = true;
        sys_->apu.noiseBurst(0.36f, 700.f, 0.10f);
    }
    bool settled = ball_.y <= kBallR + 0.04f && std::fabs(ball_.vy) < 0.4f && std::hypot(ball_.vx, ball_.vz) < 0.45f &&
                   flight_ > 0.45f;
    bool out = ball_.x < 0.6f || ball_.x > kBoardX + 1.1f || std::fabs(ball_.z) > 3.4f;
    if (scored_) {
        if (flight_ > scoreAt_ + 0.48f) finishShot();
    } else if (settled || out || flight_ > 3.6f) {
        finishShot();
    }
}

const char* Game::nameOf(Call c) {
    switch (c) {
    case Call::Swish: return "swish";
    case Call::Count: return "count";
    case Call::Bank: return "bank";
    case Call::Rim: return "rim";
    case Call::Short: return "short";
    case Call::Long: return "long";
    case Call::Air: return "air";
    }
    return "none";
}

Game::Call Game::classify() const {
    if (scored_) {
        if (bank_) return Call::Bank;
        if (rimHit_) return Call::Count;
        return Call::Swish;
    }
    if (bank_) return Call::Long;
    if (rimHit_) return Call::Rim;
    if (!crossed_ || crossPast_ < -0.05f) return Call::Short;
    if (crossPast_ > 0.2f) return Call::Long;
    return Call::Air;
}

void Game::finishShot() {
    call_ = classify();
    result_ = nameOf(call_);
    if (scored_) {
        opened_ = true;
        flash_ = call_ == Call::Swish ? 0.18f : 0.10f;
        if (call_ == Call::Swish) {
            sys_->apu.noiseBurst(0.16f, 3200.f, 0.08f);
            tone(0, 784.f, 0.08f, 0.12f);
            tone(1, 1175.f, 0.05f, 0.16f);
        } else if (call_ == Call::Bank) {
            tone(0, 523.f, 0.08f, 0.12f);
            tone(1, 659.f, 0.05f, 0.14f);
        } else {
            tone(0, 659.f, 0.08f, 0.12f);
            tone(1, 880.f, 0.05f, 0.14f);
        }
        if (!bot_) sys_->rumble(0.25f, 0.55f, 90);
        sys_->setLight(80, 200, 90);
    } else {
        sys_->apu.noiseBurst(0.20f, 400.f, 0.08f);
        tone(0, 146.f, 0.06f, 0.14f);
    }
    callT_ = 0.62f;
    mode_ = Mode::Call;
}

void Game::afterCall() {
    if (scored_) {
        feetX_ = kMarkX;
        watch_ = 0.48f;
        faceLeft_ = false;
        mode_ = Mode::Lift;
        say("LIFT THE COIN", 1.1f);
        return;
    }
    if (shots_ >= kMaxShots) {
        fail();
        return;
    }
    meter_ = 0;
    meterDir_ = 1;
    mode_ = Mode::Aim;
    say("STILL OPEN", 0.70f);
}

void Game::tryLift() {
    if (std::fabs(feetX_ - coinX_) > kLiftR) {
        say("WALK TO THE COIN", 0.45f);
        tone(0, 120.f, 0.06f, 0.10f);
        return;
    }
    finish();
}

void Game::finish() {
    lifted_ = true;
    finished_ = true;
    won_ = true;
    faceLeft_ = true;
    mode_ = Mode::Leave;
    chord(659.25f, 783.99f, 1046.5f, 0.70f);
    if (sys_) {
        sys_->rumble(0.40f, 0.80f, 180);
        sys_->setLight(255, 200, 60);
    }
    say("LEAVE", 0.8f);
}

void Game::fail() {
    won_ = false;
    finished_ = false;
    lifted_ = false;
    over_ = true;
    mode_ = Mode::Over;
    if (sys_) {
        sys_->apu.tone(0, 98.f, 0.10f);
        toneUntil_[0] = t_ + 0.40f;
        sys_->setLight(180, 40, 30);
    }
}

void Game::aimHuman(float dt, const Input& in) {
    if (in.y < -0.2f) aimZ_ -= 0.50f * dt;
    if (in.y > 0.2f) aimZ_ += 0.50f * dt;
    aimZ_ = clampf(aimZ_, -0.55f, 0.55f);
    if (std::fabs(in.x) > 0.2f) say("FROM THE MARK", 0.35f);
    meter_ += meterDir_ * kMeterRate * dt;
    if (meter_ >= 1.f) {
        meter_ = 1.f;
        meterDir_ = -1.f;
    } else if (meter_ <= 0.f) {
        meter_ = 0.f;
        meterDir_ = 1.f;
    }
    const gs::Pad& p = sys_->pad;
    bool tap = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    if (tap) launch(meter_);
}

void Game::autoRelease(float dt) {
    float target = 0.50f;
    float prev = meter_;
    meter_ += meterDir_ * kMeterRate * dt;
    if (meter_ >= 1.f) {
        meter_ = 1.f;
        meterDir_ = -1.f;
    } else if (meter_ <= 0.f) {
        meter_ = 0.f;
        meterDir_ = 1.f;
    }
    bool up = meter_ >= prev;
    if ((up && prev < target && meter_ >= target) || (!up && prev > target && meter_ <= target)) launch(target);
}

void Game::dribble() {
    float s = std::sin(t_ * 7.2f);
    ball_.x = feetX_ + 0.28f;
    ball_.y = 0.16f + std::fabs(s) * 0.90f;
    ball_.z = 0;
    ball_.vx = ball_.vy = ball_.vz = 0;
    if (s > 0.f && prevDrib_ <= 0.f) sys_->apu.noiseBurst(0.10f, 280.f, 0.035f);
    prevDrib_ = s;
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    if (pad.down(gs::BTN_LEFT)) in.x -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) in.x += 1.f;
    if (pad.down(gs::BTN_UP)) in.y -= 1.f;
    if (pad.down(gs::BTN_DOWN)) in.y += 1.f;
    if (std::fabs(pad.axisX) > 0.20f) in.x = pad.axisX;
    if (std::fabs(pad.axisY) > 0.20f) in.y = -pad.axisY;
    float len = std::hypot(in.x, in.y);
    if (len > 1.f) {
        in.x /= len;
        in.y /= len;
    }
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && t_ > 0.45f) {
        in.start = true;
        return in;
    }
    if (mode_ == Mode::Place) {
        float dx = kMarkX - coinX_;
        float dz = kMarkZ - coinZ_;
        float d = std::hypot(dx, dz);
        if (d <= kSetR) in.action = true;
        else if (d > 1e-4f) {
            in.x = dx / d;
            in.y = dz / d;
        }
        return in;
    }
    if (mode_ == Mode::Lift && watch_ <= 0.f) {
        float dx = coinX_ - feetX_;
        if (std::fabs(dx) <= kLiftR) in.action = true;
        else in.x = dx < 0.f ? -1.f : 1.f;
    }
    return in;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (flash_ > 0.f) flash_ = std::max(0.f, flash_ - kDt);
    if (sayT_ > 0.f) sayT_ = std::max(0.f, sayT_ - kDt);

    Input in = bot_ ? botInput() : readPad(sys.pad);

    if (mode_ == Mode::Title) {
        dribble();
        if (in.back && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (in.start || in.action) begin();
    } else if (mode_ == Mode::Pause) {
        if (in.start || in.action) mode_ = held_;
        else if (in.back) {
            resetCourt();
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (in.start || in.action)) begin();
        else if (!bot_ && in.back) {
            resetCourt();
            mode_ = Mode::Title;
        }
    } else if (!bot_ && (in.start || in.back)) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Place) {
        dribble();
        coinX_ += in.x * kCoinV * kDt;
        coinZ_ += in.y * kCoinV * kDt;
        coinX_ = clampf(coinX_, 2.4f, 7.2f);
        coinZ_ = clampf(coinZ_, -0.9f, 0.9f);
        bool inside = markDist() <= kSetR;
        if (inside && !wasIn_) tone(1, 880.f, 0.05f, 0.08f);
        wasIn_ = inside;
        if (in.action) trySet();
    } else if (mode_ == Mode::Aim) {
        if (bot_) autoRelease(kDt);
        else aimHuman(kDt, in);
        if (mode_ == Mode::Aim) dribble();
    } else if (mode_ == Mode::Flight) {
        fly(kDt);
    } else if (mode_ == Mode::Call) {
        callT_ -= kDt;
        bool skip = !bot_ && callT_ < 0.40f && in.action;
        if (callT_ <= 0.f || skip) afterCall();
    } else if (mode_ == Mode::Lift) {
        if (watch_ > 0.f) {
            watch_ -= kDt;
            feetX_ = std::min(kMarkX + kStep, feetX_ + 2.6f * kDt);
            faceLeft_ = false;
        } else {
            feetX_ += in.x * kWalkV * kDt;
            feetX_ = clampf(feetX_, kMarkX - 0.45f, kMarkX + kStep + 0.15f);
            if (in.x < -0.2f) faceLeft_ = true;
            else if (in.x > 0.2f) faceLeft_ = false;
            if (in.action) tryLift();
        }
    } else if (mode_ == Mode::Leave) {
        faceLeft_ = true;
        feetX_ -= kLeaveV * kDt;
        if (scrX(feetX_) < -30.f) {
            left_ = true;
            over_ = true;
            mode_ = Mode::Over;
        }
    }

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

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 156) {
            float u = y / 156.f;
            int boost = flash_ > 0.f && y < 70 ? 2 : 0;
            int r = rgbClamp(int(1 + u * 7) + boost);
            int g = rgbClamp(int(1 + u * 4) + (flash_ > 0.f ? 1 : 0));
            int b = rgbClamp(int(9 - u * 5));
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else if (y < int(kFloor)) {
            v.lineBackdrop[y] = gs::rgb4(1, 2, 3);
        } else {
            int w = 4 + ((y & 8) ? 1 : 0);
            v.lineBackdrop[y] = gs::rgb4(w + 1, w, 2);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    auto banner = [&](const gs::Image& img, int pal, float y) { spr(img, 160.f, y, float(img.w), float(img.h), pal); };

    if (mode_ == Mode::Title) banner(art_.title, PAL_GOLD, 34.f);
    else if (mode_ == Mode::Pause) banner(art_.pauseWord, PAL_INK, 42.f);
    else if (mode_ == Mode::Over) banner(won_ ? art_.finished : art_.stillOpen, won_ ? PAL_GOLD : PAL_RED, 42.f);
    else if (mode_ == Mode::Leave) banner(art_.leaveWord, PAL_GREEN, 40.f);
    else if (mode_ == Mode::Lift) banner(art_.liftWord, PAL_GOLD, 40.f);
    else if (mode_ == Mode::Call) {
        const gs::Image* img = &art_.airWord;
        int pal = PAL_RED;
        if (call_ == Call::Swish) {
            img = &art_.swish;
            pal = PAL_GREEN;
        } else if (call_ == Call::Count) {
            img = &art_.count;
            pal = PAL_GOLD;
        } else if (call_ == Call::Bank) {
            img = &art_.bank;
            pal = PAL_GOLD;
        } else if (call_ == Call::Rim) img = &art_.rimWord;
        else if (call_ == Call::Short) img = &art_.shortWord;
        else if (call_ == Call::Long) img = &art_.longWord;
        banner(*img, pal, 40.f);
    }

    bool showAim = mode_ == Mode::Aim || mode_ == Mode::Title || mode_ == Mode::Place ||
                   (mode_ == Mode::Pause && (held_ == Mode::Aim || held_ == Mode::Place));
    if (showAim) spr(art_.bracket, kGcx + aimZ_ * kIn, kGcy, 11, 11, PAL_GOLD);
    if (mode_ == Mode::Flight || mode_ == Mode::Call) {
        float along = kRimX - ball_.x;
        float lat = ball_.z;
        if (along < 1.35f && along > -0.7f) {
            float cx = kGcx + clampf(lat, -0.55f, 0.55f) * kIn;
            float cy = kGcy + clampf(along, -0.5f, 0.5f) * kIn;
            spr(art_.pip, cx, cy, 10, 10, PAL_BALL);
        }
    }
    spr(art_.glass, kGcx, kGcy, kGs, kGs, PAL_JUDGE);

    if (mode_ == Mode::Aim || (mode_ == Mode::Pause && held_ == Mode::Aim)) {
        const float barX = 104.f, barW = 112.f, barY = 20.f;
        spr(art_.blot[3], barX + clampf(meter_, 0.f, 1.f) * barW, barY, 3, 11, PAL_METER);
        spr(art_.blot[2], barX + 0.50f * barW, barY, barW * 0.20f, 7, PAL_METER);
        spr(art_.blot[1], barX + barW * 0.5f, barY, barW, 5, PAL_METER);
    }

    bool showPlayer = !(mode_ == Mode::Over && left_);
    if (lifted_ && showPlayer) {
        float hx = scrX(feetX_) + (faceLeft_ ? -12.f : 14.f);
        spr(art_.coin, hx, kFloor - 38.f, 12, 12, PAL_COIN);
    }

    spr(art_.rim, scrX(kRimX), scrY(kRimY), 40, 16, PAL_IRON);
    int netFrame = int(t_ * 6.f) & 1;
    spr(art_.net[netFrame], scrX(kRimX) - 2.f, scrY(kRimY) + 16.f, 30, 28, PAL_IRON);

    bool spin = (mode_ == Mode::Flight || mode_ == Mode::Call) ? (int(flight_ * 14.f) & 1) != 0 : (int(t_ * 6.f) & 1) != 0;
    spr(art_.ball[spin ? 1 : 0], scrX(ball_.x), scrY(ball_.y) + ball_.z * 8.f, 18, 18, PAL_BALL);

    if (mode_ == Mode::Aim) {
        float vx, vy, vz;
        if (solveShot(feetX_, aimZ_, vx, vy, vz)) {
            float sc = speedScale(meter_);
            vx *= sc;
            vy *= sc;
            vz *= sc;
            float x = feetX_ + 0.22f, y = kReleaseY, z = 0;
            int pal = inPocket(meter_) ? PAL_GREEN : PAL_GOLD;
            for (int i = 0; i < 14; i++) {
                float h = 0.07f;
                y += vy * h - 0.5f * kG * h * h;
                vy -= kG * h;
                x += vx * h;
                z += vz * h;
                if (y < 0.25f || x > kBoardX) break;
                spr(art_.blot[1], scrX(x), scrY(y) + z * 8.f, 3, 3, pal);
            }
        }
    }

    if (showPlayer) {
        float ph = float(art_.shooter.h);
        float pw = float(art_.shooter.w);
        spr(art_.shooter, scrX(feetX_), kFloor - ph * 0.5f, pw, ph, PAL_YOU, faceLeft_);
    }

    if (!lifted_) spr(art_.coin, scrX(coinX_), kFloor - 1.f + coinZ_ * 14.f, 14, 10, PAL_COIN);

    bool coinIn = markDist() <= kSetR && !lifted_;
    float pulse = coinIn ? 1.f : 1.f + 0.05f * std::sin(t_ * 4.f);
    spr(art_.ring, scrX(kMarkX), kFloor + 1.f, 46.f * pulse, 16.f, coinIn ? PAL_GREEN : PAL_GOLD);

    spr(art_.board, scrX(kBoardX), scrY(kRimY + 0.42f), 50, 34, PAL_GLASS);
    float poleTop = scrY(kRimY - 0.1f);
    spr(art_.pole, scrX(kBoardX) + 8.f, (poleTop + kFloor) * 0.5f, 8, std::max(8.f, kFloor - poleTop), PAL_IRON);

    spr(art_.blot[1], scrX(kRimX - kThree), kFloor - 8.f, 2, 16, PAL_GREEN);
    spr(art_.blot[1], 160.f, kFloor, 300, 2, PAL_INK);

    spr(art_.lamp, 214.f, 132.f, 16, 70, PAL_LAMP);
    spr(art_.moon, 292.f, 24.f, 16, 16, PAL_LAMP);
    spr(art_.trees, 160.f, 162.f, 320, 46, PAL_PARK);

    float sh = clampf(16.f - ball_.y * 1.6f, 7.f, 16.f);
    spr(art_.shadow, scrX(ball_.x), kFloor + 3.f, sh, 5, PAL_INK, false, true);
    if (showPlayer) spr(art_.shadow, scrX(feetX_), kFloor + 4.f, 22, 6, PAL_INK, false, true);

    char buf[40];
    if (mode_ != Mode::Title) {
        int n = (mode_ == Mode::Aim || mode_ == Mode::Place || mode_ == Mode::Title) ? shots_ + 1 : shots_;
        if (n < 1) n = 1;
        if (n > kMaxShots) n = kMaxShots;
        std::snprintf(buf, sizeof buf, "SHOT %d OF %d", n, kMaxShots);
        hud(1, 0, buf, PAL_GOLD);
        const char* st = opened_ ? "OPEN" : onMark_ ? "SET" : "MARK";
        hud(39 - int(std::strlen(st)), 0, st, opened_ ? PAL_GREEN : PAL_GOLD);
    }

    if (mode_ == Mode::Title) {
        hudC(8, "THE COIN IS THE MARK", PAL_GOLD);
        hudC(9, "A COUNT OPENS IT", PAL_INK);
        hudC(10, "LIFT IT AND LEAVE", PAL_INK);
        hudC(12, "THE MARK IS BEYOND THE ARC", PAL_GREEN);
        hudC(13, "FOUR MISSES LEAVE IT OPEN", PAL_RED);
        hudC(25, "ARROWS MOVE THE COIN", PAL_INK);
        hudC(26, "Z SHOOTS   ENTER STARTS", PAL_GREEN);
    } else if (mode_ == Mode::Pause) {
        hudC(8, "ENTER RESUMES", PAL_INK);
        hudC(9, "ESC TITLE", PAL_INK);
    } else if (mode_ == Mode::Over && won_) {
        hudC(8, "THE MARK IS FINISHED", PAL_GOLD);
        hudC(9, "YOU LEFT", PAL_GREEN);
        hudC(26, "ENTER PLAYS AGAIN", PAL_INK);
    } else if (mode_ == Mode::Over) {
        hudC(8, "NO FINISHED MARK", PAL_RED);
        hudC(9, "FOUR SHOTS AND NO COUNT", PAL_INK);
        hudC(26, "ENTER PLAYS AGAIN", PAL_INK);
    } else if (sayT_ > 0.f) {
        hudC(1, say_, PAL_GOLD);
    } else if (mode_ == Mode::Place) {
        bool on = markDist() <= kSetR;
        hudC(1, on ? "ON THE MARK" : "SET THE COIN ON THE MARK", on ? PAL_GREEN : PAL_INK);
        hudC(25, "Z SETS THE MARK", PAL_INK);
    } else if (mode_ == Mode::Aim) {
        bool pocket = inPocket(meter_);
        hudC(1, pocket ? "POCKET" : "U-D AIM   Z SHOOTS", pocket ? PAL_GREEN : PAL_INK);
        std::snprintf(buf, sizeof buf, "WORTH %d   FROM THE MARK", worth());
        hudC(25, buf, PAL_GOLD);
    } else if (mode_ == Mode::Call && scored_) {
        std::snprintf(buf, sizeof buf, "+%d  MARK OPEN", shotPts_);
        hudC(7, buf, PAL_GREEN);
    } else if (mode_ == Mode::Call) {
        hudC(7, "NO COUNT", PAL_RED);
    } else if (mode_ == Mode::Lift) {
        hudC(25, "WALK BACK   Z LIFTS", PAL_INK);
    } else if (mode_ == Mode::Leave) {
        hudC(25, "THE MARK IS FINISHED", PAL_GREEN);
    }
}

}  // namespace hoopmark
