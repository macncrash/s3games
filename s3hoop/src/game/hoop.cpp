#include "game/hoop.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace hoop {
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
constexpr float kLaneDist = 4.9f;
constexpr float kMinDist = 3.5f;
constexpr float kMaxDist = 7.6f;
constexpr float kMeterRate = 0.82f;
constexpr float kWorldL = 1.5f;
constexpr float kPx = 26.8f;
constexpr float kScr0 = 10.f;
constexpr float kFloor = 190.f;
constexpr float kGlassX = 4.f;
constexpr float kGlassY = 32.f;
constexpr float kGlassC = 42.5f;
constexpr float kInPx = 64.f;

constexpr int kLaneN = 10;
constexpr float kLaneMeter[kLaneN] = {0.50f, 0.22f, 0.50f, 0.86f, 0.47f, 0.16f, 0.55f, 0.93f, 0.44f, 0.28f};
constexpr float kLaneLat[kLaneN] = {0.02f, 0.00f, 0.48f, -0.12f, 0.05f, 0.28f, -0.04f, 0.52f, 0.00f, -0.36f};

float scrX(float x) { return kScr0 + (x - kWorldL) * kPx; }
float scrY(float y) { return kFloor - y * kPx; }

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

// Green band is the pocket the rim actually accepts. Outside it the arc is short or long.
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

void Game::pumpAudio() {
    if (!sys_) return;
    for (int ch = 0; ch < 3; ch++) {
        if (toneUntil_[ch] > 0.f && t_ >= toneUntil_[ch]) {
            sys_->apu.tone(ch, 0, 0);
            toneUntil_[ch] = 0;
        }
    }
}

bool Game::shootPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
}

bool Game::startPressed() const { return sys_->pad.pressed(gs::BTN_START); }

int Game::worth() const { return (kRimX - feetX_) >= kThree ? 3 : 2; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    sys.apu.setMaster(0.72f);
    feetX_ = kRimX - kBotDist;
    aimZ_ = 0;
    you_ = lane_ = 0;
    youTurn_ = true;
    over_ = won_ = false;
    if (bot_) beginMatch();
    else mode_ = Mode::Title;
}

void Game::beginMatch() {
    you_ = 0;
    lane_ = 0;
    laneIx_ = 0;
    youTurn_ = true;
    won_ = false;
    over_ = false;
    fanStep_ = -1;
    feetX_ = kRimX - kBotDist;
    aimZ_ = 0;
    beginAim();
}

void Game::beginAim() {
    if (!youTurn_) {
        feetX_ = kRimX - kLaneDist;
        aimZ_ = kLaneLat[laneIx_ % kLaneN];
    } else if (bot_) {
        feetX_ = kRimX - kBotDist;
        aimZ_ = 0;
    }
    feetX_ = clampf(feetX_, kRimX - kMaxDist, kRimX - kMinDist);
    aimZ_ = clampf(aimZ_, -0.62f, 0.62f);
    meter_ = 0;
    meterDir_ = 1;
    mode_ = Mode::Aim;
    prevDrib_ = 0;
    tone(0, youTurn_ ? 392.f : 294.f, 0.05f, 0.08f);
}

void Game::launch(float meter) {
    float vx, vy, vz;
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
    if (!youTurn_) laneIx_++;
    mode_ = Mode::Flight;
    tone(0, 620.f, 0.07f, 0.1f);
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
        sys_->apu.noiseBurst(0.36f, 700.f, 0.1f);
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
    if (scored_) {
        if (youTurn_) you_ += shotPts_;
        else lane_ += shotPts_;
        flash_ = call_ == Call::Swish ? 0.18f : 0.1f;
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
    } else {
        sys_->apu.noiseBurst(0.2f, 400.f, 0.08f);
        tone(0, 146.f, 0.06f, 0.14f);
    }
    callT_ = 0.62f;
    mode_ = Mode::Call;
}

void Game::afterCall() {
    if (you_ >= 21) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        fanStep_ = -1;
        callT_ = 0;
        sys_->setLight(40, 220, 80);
        return;
    }
    if (lane_ >= 21) {
        won_ = false;
        if (bot_) over_ = true;
        mode_ = Mode::Lose;
        fanStep_ = -1;
        callT_ = 0;
        sys_->setLight(220, 40, 30);
        return;
    }
    youTurn_ = !youTurn_;
    beginAim();
}

void Game::aimControl(float dt) {
    bool human = youTurn_ && !bot_;
    if (!human) return;
    const gs::Pad& p = sys_->pad;
    float ax = p.axisX;
    if (p.down(gs::BTN_LEFT) || ax < -0.25f) feetX_ -= 2.5f * dt;
    if (p.down(gs::BTN_RIGHT) || ax > 0.25f) feetX_ += 2.5f * dt;
    if (p.down(gs::BTN_UP)) aimZ_ -= 0.42f * dt;
    if (p.down(gs::BTN_DOWN)) aimZ_ += 0.42f * dt;
    feetX_ = clampf(feetX_, kRimX - kMaxDist, kRimX - kMinDist);
    aimZ_ = clampf(aimZ_, -0.62f, 0.62f);
}

void Game::autoRelease(float dt) {
    float target = 0.50f;
    if (!youTurn_) target = kLaneMeter[laneIx_ % kLaneN];
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

void Game::dribble(float dt) {
    float s = std::sin(t_ * 7.2f);
    ball_.x = feetX_ + 0.46f;
    ball_.y = 0.16f + std::fabs(s) * 0.92f;
    ball_.z = 0;
    ball_.vx = ball_.vy = ball_.vz = 0;
    if (s > 0.f && prevDrib_ <= 0.f) sys_->apu.noiseBurst(0.1f, 280.f, 0.035f);
    prevDrib_ = s;
    (void)dt;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (flash_ > 0.f) flash_ = std::max(0.f, flash_ - kDt);

    if (mode_ == Mode::Title) {
        dribble(kDt);
        if (startPressed() || shootPressed()) beginMatch();
    } else if (mode_ == Mode::Aim) {
        dribble(kDt);
        if (bot_ || !youTurn_) {
            autoRelease(kDt);
        } else {
            meter_ += meterDir_ * kMeterRate * kDt;
            if (meter_ >= 1.f) {
                meter_ = 1.f;
                meterDir_ = -1.f;
            } else if (meter_ <= 0.f) {
                meter_ = 0.f;
                meterDir_ = 1.f;
            }
            aimControl(kDt);
            if (startPressed()) {
                held_ = Mode::Aim;
                mode_ = Mode::Pause;
            } else if (shootPressed()) {
                launch(meter_);
            }
        }
    } else if (mode_ == Mode::Flight) {
        if (!bot_ && startPressed()) {
            held_ = Mode::Flight;
            mode_ = Mode::Pause;
        } else {
            fly(kDt);
        }
    } else if (mode_ == Mode::Call) {
        callT_ -= kDt;
        bool skip = !bot_ && callT_ < 0.40f && (shootPressed() || startPressed());
        if (callT_ <= 0.f || skip) afterCall();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        int step = int(callT_ / 0.18f);
        if (step != fanStep_ && step >= 0 && step < 4) {
            fanStep_ = step;
            float f = notes[step];
            if (mode_ == Mode::Lose) f *= 0.5f;
            tone(0, f, 0.08f, 0.16f);
            tone(1, f * 0.5f, 0.04f, 0.16f);
        }
        callT_ += kDt;
        if (!bot_ && (startPressed() || shootPressed())) beginMatch();
    } else if (mode_ == Mode::Pause) {
        if (startPressed() || shootPressed()) mode_ = held_;
    }

    pumpAudio();
    draw();
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

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
        if (y < 168) {
            float u = y / 168.f;
            int boost = flash_ > 0.f && y < 80 ? 2 : 0;
            int r = rgbClamp(int(1 + u * 11) + boost);
            int g = rgbClamp(int(1 + u * 5) + (flash_ > 0.f ? 1 : 0));
            int b = rgbClamp(int(8 - u * 5));
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else if (y < int(kFloor)) {
            v.lineBackdrop[y] = gs::rgb4(1, 2, 2);
        } else {
            int w = 5 + ((y & 4) ? 1 : 0);
            v.lineBackdrop[y] = gs::rgb4(w + 2, w - 1, 1);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    auto banner = [&](const gs::Image& img, int pal, float y) { spr(img, 188.f, y, float(img.w), float(img.h), pal); };

    if (mode_ == Mode::Title) banner(art_.title, PAL_GOLD, 46.f);
    if (mode_ == Mode::Pause) banner(art_.pause, PAL_INK, 70.f);
    if (mode_ == Mode::Win) {
        banner(art_.count, PAL_GOLD, 46.f);
        banner(art_.youWin, PAL_GREEN, 72.f);
    } else if (mode_ == Mode::Lose) {
        banner(art_.laneWin, PAL_RED, 56.f);
    } else if (mode_ == Mode::Call) {
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
        } else if (call_ == Call::Rim) {
            img = &art_.rimWord;
        } else if (call_ == Call::Short) {
            img = &art_.shortWord;
        } else if (call_ == Call::Long) {
            img = &art_.longWord;
        }
        banner(*img, pal, 48.f);
    }

    float ix = kGlassX + kGlassC + aimZ_ * kInPx;
    float iy = kGlassY + kGlassC;
    if (mode_ == Mode::Aim || mode_ == Mode::Title || mode_ == Mode::Pause) spr(art_.bracket, ix, iy, 11, 11, PAL_GOLD);

    if (mode_ == Mode::Flight || mode_ == Mode::Call || mode_ == Mode::Win) {
        float along = kRimX - ball_.x;
        float lat = ball_.z;
        if (along < 1.35f && along > -0.7f) {
            float cx = kGlassX + kGlassC + clampf(lat, -0.58f, 0.58f) * kInPx;
            float cy = kGlassY + kGlassC + clampf(along, -0.55f, 0.55f) * kInPx;
            spr(art_.pip, cx, cy, 10, 10, PAL_BALL);
        }
    }
    spr(art_.glass, kGlassX + kGlassC, kGlassY + kGlassC, 86, 86, PAL_JUDGE);

    if (mode_ == Mode::Aim || mode_ == Mode::Pause || mode_ == Mode::Title) {
        const float barX = 108.f, barW = 112.f, barY = 3.f;
        float nx = barX + clampf(meter_, 0.f, 1.f) * barW;
        spr(art_.blot[3], nx, barY + 3.f, 3, 9, PAL_METER);
        spr(art_.blot[2], barX + 0.50f * barW, barY + 3.f, barW * 0.20f, 7, PAL_METER);
        spr(art_.blot[1], barX + barW * 0.5f, barY + 3.f, barW, 5, PAL_METER);
    }

    spr(art_.rim, scrX(kRimX), scrY(kRimY), 38, 16, PAL_IRON);
    int netFrame = int(t_ * 6.f) & 1;
    spr(art_.net[netFrame], scrX(kRimX) - 2.f, scrY(kRimY) + 18.f, 32, 30, PAL_IRON);

    int ballPal = PAL_BALL;
    float ballW = 16.f, ballH = 16.f;
    bool spin = (int(flight_ * 14.f) & 1) != 0;
    spr(art_.ball[spin ? 1 : 0], scrX(ball_.x), scrY(ball_.y) + ball_.z * 8.f, ballW, ballH, ballPal);

    float px = scrX(feetX_);
    float waitX = scrX(feetX_ - 1.15f);
    if (youTurn_) {
        spr(art_.shooter, waitX, kFloor - 22.f, 32, 46, PAL_LANE, true);
        spr(art_.shooter, px, kFloor - 28.f, 40, 56, PAL_YOU);
    } else {
        spr(art_.shooter, waitX, kFloor - 22.f, 32, 46, PAL_YOU, true);
        spr(art_.shooter, px, kFloor - 28.f, 40, 56, PAL_LANE);
    }

    spr(art_.board, scrX(kBoardX), scrY(kRimY + 0.38f), 52, 36, PAL_GLASS);
    float poleTop = scrY(kRimY - 0.15f);
    spr(art_.pole, scrX(kBoardX), (poleTop + kFloor) * 0.5f, 8, std::max(8.f, kFloor - poleTop), PAL_IRON);

    if (mode_ == Mode::Aim) {
        float vx, vy, vz;
        if (solveShot(feetX_, aimZ_, vx, vy, vz)) {
            float sc = speedScale(meter_);
            vx *= sc;
            vy *= sc;
            vz *= sc;
            float x = feetX_ + 0.22f, y = kReleaseY, z = 0;
            int pal = inPocket(meter_) ? PAL_GREEN : PAL_GOLD;
            for (int i = 0; i < 16; i++) {
                float h = 0.07f;
                y += vy * h - 0.5f * kG * h * h;
                vy -= kG * h;
                x += vx * h;
                z += vz * h;
                if (y < 0.2f || x > kBoardX) break;
                spr(art_.blot[1], scrX(x), scrY(y) + z * 8.f, 3, 3, pal);
            }
        }
    }

    spr(art_.lamp, 214.f, 128.f, 18, 78, PAL_LAMP);
    spr(art_.moon, 286.f, 28.f, 18, 18, PAL_LAMP);
    spr(art_.trees, 160.f, 164.f, 320, 52, PAL_PARK);

    float lineX = scrX(kRimX - kThree);
    spr(art_.blot[1], lineX, kFloor - 2.f, 2, 18, PAL_GREEN);
    spr(art_.blot[1], scrX(kRimX - kLaneDist), kFloor - 1.f, 2, 10, PAL_INK);
    spr(art_.blot[1], 160.f, kFloor, 300, 2, PAL_INK);

    float sh = clampf(16.f - ball_.y * 1.5f, 7.f, 16.f);
    spr(art_.shadow, scrX(ball_.x), kFloor + 3.f, sh, 5, PAL_INK, false, true);
    spr(art_.shadow, px, kFloor + 4.f, 22, 6, PAL_INK, false, true);

    char buf[40];
    std::snprintf(buf, sizeof buf, "YOU %d", you_);
    hud(1, 0, buf, PAL_GOLD);
    std::snprintf(buf, sizeof buf, "LANE %d", lane_);
    hud(40 - int(std::strlen(buf)) - 1, 0, buf, PAL_RED);

    if (mode_ == Mode::Title) {
        hudC(4, "FIRST TO 21", PAL_INK);
        hudC(5, "THE RIM IS THE JUDGE", PAL_GOLD);
        hudC(25, "L-R RANGE   U-D AIM   Z SHOOT", PAL_INK);
        hudC(26, "ENTER STARTS", PAL_GREEN);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "ENTER RESUMES", PAL_INK);
    } else if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "%d - %d", you_, lane_);
        hudC(11, buf, PAL_GOLD);
        hudC(26, "ENTER PLAYS AGAIN", PAL_INK);
    } else if (mode_ == Mode::Lose) {
        std::snprintf(buf, sizeof buf, "%d - %d", you_, lane_);
        hudC(10, buf, PAL_RED);
        hudC(12, "THE RIM COUNTED LANE", PAL_INK);
        hudC(26, "ENTER PLAYS AGAIN", PAL_INK);
    } else {
        const char* who = youTurn_ ? "YOUR BALL" : "LANE";
        int whoPal = youTurn_ ? PAL_GOLD : PAL_RED;
        std::snprintf(buf, sizeof buf, "%s   WORTH %d", who, (mode_ == Mode::Flight || mode_ == Mode::Call) ? shotPts_ : worth());
        hudC(2, buf, whoPal);
        hud(1, 3, "RIM", PAL_GOLD);
        if (mode_ == Mode::Aim) {
            hudC(25, inPocket(meter_) ? "POCKET" : "L-R RANGE  U-D AIM  Z SHOOT", inPocket(meter_) ? PAL_GREEN : PAL_INK);
        } else if (mode_ == Mode::Call && scored_) {
            std::snprintf(buf, sizeof buf, "+%d", shotPts_);
            hudC(8, buf, PAL_GREEN);
        }
    }
}

}  // namespace hoop
