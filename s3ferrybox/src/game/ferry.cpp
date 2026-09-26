#include "game/ferry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ferrybox {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;
constexpr float kCx = 0.95f;
constexpr float kCy = 0.f;
constexpr float kBoxL = -8.7f;
constexpr float kBoxR = 8.7f;
constexpr float kBoxS = 96.f;
constexpr float kBoxN = 128.f;
constexpr float kMargin = 0.30f;
constexpr float kStop = 0.40f;
constexpr float kHoldNeed = 0.58f;
constexpr float kOutNeed = 1.15f;
constexpr float kTimeLimit = 100.f;
constexpr float kSurgeA = 5.2f;
constexpr float kSurgeB = 4.0f;
constexpr float kSway = 2.6f;
constexpr float kWest = -30.f;
constexpr float kEast = 30.f;
constexpr float kSouth = 8.f;
constexpr float kNorthLim = 156.f;
constexpr float kStartX = 8.f;
constexpr float kStartY = 64.f;
constexpr float kStartH = 0.70f;
constexpr float kGoalY = 112.f;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t + 0.5f), int(ag + (bg - ag) * t + 0.5f), int(ab + (bb - ab) * t + 0.5f));
}

void bodyToWorld(float h, float surge, float sway, float& vx, float& vy) {
    float c = std::cos(h), s = std::sin(h);
    vx = c * surge + s * sway;
    vy = s * surge - c * sway;
}

void worldToBody(float h, float vx, float vy, float& surge, float& sway) {
    float c = std::cos(h), s = std::sin(h);
    surge = c * vx + s * vy;
    sway = s * vx - c * vy;
}

const char* compass(float h) {
    static const char* name[] = {"E", "NE", "N", "NW", "W", "SW", "S", "SE"};
    float u = h;
    while (u < 0.f) u += kTau;
    while (u >= kTau) u -= kTau;
    int i = int((u + kPi / 8.f) / (kPi / 4.f)) & 7;
    return name[i];
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05f) return 3;
    if (hullInside()) return 2;
    return 1;
}

int Game::shipFrame() const {
    float u = heading_;
    while (u < 0.f) u += kTau;
    while (u >= kTau) u -= kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

void Game::corners(float xs[4], float ys[4]) const {
    float c = std::cos(heading_), s = std::sin(heading_);
    const float fl[4] = {kHalfL, kHalfL, -kHalfL, -kHalfL};
    const float fb[4] = {kHalfB, -kHalfB, -kHalfB, kHalfB};
    for (int i = 0; i < 4; i++) {
        xs[i] = x_ + c * fl[i] + s * fb[i];
        ys[i] = y_ + s * fl[i] - c * fb[i];
    }
}

int Game::insideCount() const {
    float xs[4], ys[4];
    corners(xs, ys);
    int n = 0;
    for (int i = 0; i < 4; i++) {
        if (xs[i] >= kBoxL + kMargin && xs[i] <= kBoxR - kMargin && ys[i] >= kBoxS + kMargin &&
            ys[i] <= kBoxN - kMargin)
            n++;
    }
    return n;
}

bool Game::hullInside() const { return insideCount() == 4; }

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    surge_ = sway_ = yaw_ = 0;
    speed_ = 0;
    hold_ = 0;
    outT_ = 0;
    race_ = 0;
    phase_ = 0;
    won_ = false;
    over_ = false;
    announced_ = false;
    chimeN_ = chimeStep_ = 0;
    wakeCursor_ = smokeCursor_ = 0;
    hornT_ = tone0_ = chimeT_ = thumpT_ = 0;
    wakeT_ = smokeT_ = shake_ = 0;
    thrustIn_ = 0;
    banner_ = nullptr;
    why_[0] = 0;
    for (Puff& p : wake_) p = {};
    for (Puff& p : smoke_) p = {};
    for (int i = 0; i < 8; i++) {
        foam_[i].x = -16.f + i * 4.6f;
        foam_[i].y = 50.f + (i % 4) * 14.f;
        foam_[i].life = 0.55f + i * 0.12f;
    }
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 1.2f;
    camY_ = 84.f;
    zoom_ = 1.25f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = x_;
    camY_ = y_ + 12.f;
    zoom_ = 1.78f;
    blip(640.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(1, 4, 8));
    sys.apu.setMaster(0.76f);
    sys.apu.setEcho(0.10f, 0.16f, 0.08f);
    t_ = 0;
    if (bot_) startRun();
    else showTitle();
}

void Game::horn() { hornT_ = 0.46f; }

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    tone0_ = 0.10f;
}

void Game::chime(int notes) {
    chimeN_ = std::max(1, std::min(notes, 6));
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::controls(float& thrust, float& rudder, float& bow) {
    const gs::Pad& p = sys_->pad;
    thrust = 0;
    if (p.down(gs::BTN_UP)) thrust += 1.f;
    if (p.down(gs::BTN_DOWN)) thrust -= 1.f;
    if (p.accel > 0.12f) thrust = p.accel;
    if (p.brake > 0.12f) thrust = -p.brake;
    if (std::fabs(p.axisY) > 0.22f) thrust = clampf(p.axisY, -1.f, 1.f);
    thrust = clampf(thrust, -1.f, 1.f);
    rudder = 0;
    if (p.down(gs::BTN_LEFT)) rudder += 1.f;
    if (p.down(gs::BTN_RIGHT)) rudder -= 1.f;
    if (std::fabs(p.axisX) > 0.20f) rudder = clampf(-p.axisX, -1.f, 1.f);
    bow = 0;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_X)) bow -= 1.f;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_Y)) bow += 1.f;
    bow = clampf(bow, -1.f, 1.f);
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO)) horn();
}

void Game::pilot(float& thrust, float& rudder, float& bow) {
    const bool in = hullInside();
    auto pd = [&](float desH) {
        float hErr = wrap(desH - heading_);
        rudder = clampf(hErr * 3.1f - yaw_ * 2.1f, -1.f, 1.f);
        return hErr;
    };
    auto go = [&](float wantVx, float wantVy) {
        float ds, dw;
        worldToBody(heading_, wantVx - kCx, wantVy - kCy, ds, dw);
        float maxS = ds >= 0.f ? kSurgeA : kSurgeB;
        thrust = clampf(ds / maxS, -1.f, 1.f);
        bow = clampf(dw / kSway, -1.f, 1.f);
        rudder = clampf(rudder + bow * 0.34f, -1.f, 1.f);
    };

    if (!in && y_ > kBoxN + 3.f) {
        phase_ = 3;
        pd(-kNorth);
        go(clampf(-x_ * 0.45f, -1.1f, 1.1f), -1.7f);
        return;
    }
    if (in) {
        phase_ = 2;
        float hErr = pd(kNorth);
        float ax = std::fabs(x_);
        float ay = std::fabs(kGoalY - y_);
        if (ax < 0.32f && ay < 0.32f && std::fabs(hErr) < 0.14f) {
            go(0.f, 0.f);
            return;
        }
        go(clampf(-x_ * 0.55f, -0.72f, 0.72f), clampf((kGoalY - y_) * 0.55f, -0.72f, 0.72f));
        return;
    }

    float aimX = (y_ > 100.f || (std::fabs(x_) < 1.1f && y_ > 88.f)) ? 0.f : -1.3f;
    float dx = aimX - x_;
    float dy = kGoalY - y_;
    float dist = std::hypot(dx, dy);
    float desH = std::atan2(dy, dx);
    float hErr = pd(desH);
    phase_ = y_ > 86.f ? 1 : 0;
    if (std::fabs(hErr) > 0.85f) {
        thrust = 0.30f;
        bow = 0.f;
        return;
    }
    float spd = 3.6f;
    if (dist < 36.f) spd = 2.4f;
    if (dist < 22.f) spd = 1.65f;
    if (dist < 12.f) spd = 1.22f;
    if (std::fabs(hErr) > 0.55f) spd = std::min(spd, 1.4f);
    float inv = dist < 0.25f ? 0.f : 1.f / dist;
    go(dx * inv * spd, dy * inv * spd);
}

void Game::resolveBanks() {
    float xs[4], ys[4];
    corners(xs, ys);
    float minX = xs[0], maxX = xs[0], minY = ys[0], maxY = ys[0];
    for (int i = 1; i < 4; i++) {
        minX = std::min(minX, xs[i]);
        maxX = std::max(maxX, xs[i]);
        minY = std::min(minY, ys[i]);
        maxY = std::max(maxY, ys[i]);
    }
    bool hit = false;
    if (minX < kWest) {
        x_ += kWest - minX;
        hit = true;
    }
    if (maxX > kEast) {
        x_ += kEast - maxX;
        hit = true;
    }
    if (minY < kSouth) {
        y_ += kSouth - minY;
        hit = true;
    }
    if (maxY > kNorthLim) {
        y_ += kNorthLim - maxY;
        hit = true;
    }
    if (!hit) return;
    surge_ *= 0.40f;
    sway_ *= 0.40f;
    yaw_ *= 0.60f;
    if (thumpT_ > 0.f) return;
    thumpT_ = 0.32f;
    shake_ = std::max(shake_, 0.55f);
    sys_->apu.noiseBurst(0.24f, 220.f, 0.12f);
    sys_->rumble(0.22f, 0.08f, 70);
}

void Game::physics(float thrust, float rudder, float bow) {
    float wash = 0.72f + 0.28f * std::min(1.f, std::fabs(surge_) / 3.2f);
    float yawTarget = rudder * 0.78f * wash - bow * 0.18f;
    auto lag = [](float cur, float target, float rate) {
        return cur + (target - cur) * (1.f - std::exp(-rate * kDt));
    };
    float surgeTarget = thrust >= 0.f ? thrust * kSurgeA : thrust * kSurgeB;
    surge_ = lag(surge_, surgeTarget, 1.85f);
    sway_ = lag(sway_, bow * kSway, 2.7f);
    yaw_ = lag(yaw_, yawTarget, 5.0f);
    heading_ = wrap(heading_ + yaw_ * kDt);

    float vx, vy;
    bodyToWorld(heading_, surge_, sway_, vx, vy);
    vx += kCx;
    vy += kCy;
    float ox = x_, oy = y_;
    x_ += vx * kDt;
    y_ += vy * kDt;
    if (!std::isfinite(x_) || !std::isfinite(y_) || !std::isfinite(heading_)) {
        fail("lost the channel");
        return;
    }
    resolveBanks();
    speed_ = std::hypot((x_ - ox) / kDt, (y_ - oy) / kDt);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    std::snprintf(why_, sizeof why_, "stopped inside the box");
    banner_ = &art_.stopped;
    chime(4);
    sys_->rumble(0.32f, 0.12f, 160);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    banner_ = (why && std::strcmp(why, "too late") == 0) ? &art_.late : &art_.outside;
    shake_ = 0.8f;
    sys_->rumble(0.55f, 0.28f, 180);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.40f, 140.f, 0.32f);
    sys_->apu.tone(0, 82.f, 0.05f);
    tone0_ = 0.36f;
}

void Game::judge() {
    if (mode_ != Mode::Run) return;
    bool in = hullInside();
    if (in && !announced_) {
        announced_ = true;
        horn();
    }
    if (in && speed_ <= kStop) {
        outT_ = 0;
        hold_ += kDt;
        if (hold_ >= kHoldNeed) win();
    } else if (!in && speed_ <= kStop) {
        hold_ = 0;
        outT_ += kDt;
        if (outT_ >= kOutNeed) {
            float xs[4], ys[4];
            corners(xs, ys);
            float minY = ys[0], maxY = ys[0];
            for (int i = 1; i < 4; i++) {
                minY = std::min(minY, ys[i]);
                maxY = std::max(maxY, ys[i]);
            }
            if (maxY < kBoxS + kMargin) fail("stopped short of the box");
            else if (minY > kBoxN - kMargin) fail("stopped past the box");
            else fail("stopped outside the box");
        }
    } else {
        hold_ = 0;
        outT_ = 0;
    }
    if (mode_ == Mode::Run && race_ >= kTimeLimit) fail("too late");
}

void Game::audio() {
    float water = mode_ == Mode::Run ? 0.015f + std::fabs(speed_) * 0.002f : 0.010f;
    sys_->apu.noise(water, 420.f + std::fabs(speed_) * 18.f, false);
    if (hornT_ > 0.f) {
        hornT_ -= kDt;
        float v = hornT_ > 0.08f ? 0.07f : std::max(0.f, hornT_) * 0.8f;
        sys_->apu.tone(1, hornF_, v);
    } else if (chimeN_ == 0 && tone0_ <= 0.f) {
        sys_->apu.tone(1, 0.f, 0.f);
    }
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f};
            int n = std::min(chimeStep_, 3);
            sys_->apu.tone(0, notes[n], 0.055f);
            tone0_ = 0.16f;
            chimeT_ = 0.16f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && (std::fabs(thrustIn_) > 0.04f || std::fabs(surge_) > 0.4f)) {
        float wob = 0.78f + 0.22f * std::sin(t_ * (8.f + std::fabs(thrustIn_) * 14.f));
        float vol = (0.012f + std::fabs(thrustIn_) * 0.026f) * wob;
        float f = 48.f + std::fabs(thrustIn_) * 28.f + std::fabs(surge_) * 1.4f;
        sys_->apu.tone(2, f, vol);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= kDt;
}

void Game::driftFx(float dt) {
    for (Puff& p : foam_) {
        if (p.life <= 0.f) {
            p.x = camX_ + std::sin(t_ * 0.7f + p.y) * 18.f - 10.f;
            p.y = camY_ + std::cos(t_ * 0.4f + p.x * 0.1f) * 16.f;
            p.life = 1.3f;
        }
        p.x += kCx * dt;
        p.life -= dt * 0.35f;
    }
    for (Puff& p : wake_)
        if (p.life > 0.f) p.life -= dt * 0.7f;
    for (Puff& p : smoke_)
        if (p.life > 0.f) {
            p.life -= dt * 0.45f;
            p.y += dt * 1.4f;
            p.x += dt * 0.35f;
        }
    if (mode_ != Mode::Run) return;
    if (speed_ > 1.05f) {
        wakeT_ -= dt;
        if (wakeT_ <= 0.f) {
            wakeT_ = 0.07f;
            float wx, wy;
            localWorld(0.f, -kHalfL * 0.82f, wx, wy);
            wake_[wakeCursor_].x = wx;
            wake_[wakeCursor_].y = wy;
            wake_[wakeCursor_].life = 1.f;
            wakeCursor_ = (wakeCursor_ + 1) % 14;
        }
    }
    if (std::fabs(thrustIn_) > 0.08f || std::fabs(surge_) > 0.5f) {
        smokeT_ -= dt;
        if (smokeT_ <= 0.f) {
            smokeT_ = 0.10f;
            float wx, wy;
            localWorld(0.f, -16.f / kPaintL * kHalfL, wx, wy);
            smoke_[smokeCursor_].x = wx;
            smoke_[smokeCursor_].y = wy;
            smoke_[smokeCursor_].life = 1.f;
            smokeCursor_ = (smokeCursor_ + 1) % 8;
        }
    }
}

void Game::camera() {
    float wantX = x_;
    float wantY = y_;
    float wantZ = 2.45f;
    if (mode_ == Mode::Title) {
        wantX = 1.2f + std::sin(t_ * 0.18f) * 1.2f;
        wantY = 84.f;
        wantZ = 1.25f;
    } else if (mode_ == Mode::Run && y_ < 90.f && !hullInside()) {
        wantY = y_ + 16.f;
        wantX = x_ * 0.6f;
        wantZ = 1.78f;
    } else {
        wantX = x_ + std::cos(heading_) * 2.5f;
        wantY = y_ + std::sin(heading_) * 2.5f;
        wantZ = 2.55f;
    }
    float k = 1.f - std::exp(-kDt * 3.8f);
    camX_ += (wantX - camX_) * k;
    camY_ += (wantY - camY_) * k;
    zoom_ += (wantZ - zoom_) * k;
    if (shake_ > 0.f) {
        camX_ += std::sin(t_ * 46.f) * shake_ * 1.5f;
        camY_ += std::cos(t_ * 37.f) * shake_;
        shake_ = std::max(0.f, shake_ - kDt * 1.4f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            banner_ = &art_.paused;
            blip(320.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            float thrust = 0, rudder = 0, bow = 0;
            if (bot_) pilot(thrust, rudder, bow);
            else controls(thrust, rudder, bow);
            thrustIn_ = thrust;
            physics(thrust, rudder, bow);
            if (mode_ == Mode::Run) judge();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Run;
            banner_ = nullptr;
        } else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) startRun();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
    }

    if (mode_ == Mode::Win || hold_ > 0.05f) sys.setLight(40, 180, 70);
    else if (mode_ == Mode::Fail) sys.setLight(180, 40, 28);
    else if (mode_ == Mode::Run && hullInside()) sys.setLight(40, 150, 80);
    else sys.setLight(30, 80, 130);

    camera();
    driftFx(kDt);
    audio();
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::drawHud() {
    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(23, "THE FERRY HAS ONE JOB", PAL_BANNER);
        hudC(24, "STOP INSIDE THE BOX", PAL_HUD);
        hudC(25, "CLOSE IS STILL OUTSIDE", PAL_ALERT);
        hudC(26, "ARROWS ENGINE  Z/X BOW  C HORN", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "RETURN", PAL_WIN);
        return;
    }
    int sec = std::max(0, int(race_));
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hud(1, 0, "S3 FERRY BOX", PAL_BANNER);
    hud(34, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(17, "RETURN CONTINUES", PAL_HUD);
        hudC(18, "ESC TO THE TITLE", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(16, "INSIDE THE BOX", PAL_WIN);
        std::snprintf(buf, sizeof buf, "HELD  %d:%02d", sec / 60, sec % 60);
        hudC(17, buf, PAL_HUD);
        if (!bot_) hudC(19, "RETURN SAILS AGAIN", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_[0] ? why_ : "OUTSIDE", PAL_ALERT);
        if (!bot_) hudC(18, "RETURN TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "SPD %04.1f  %s", speed_, compass(heading_));
    hud(1, 1, buf, speed_ <= kStop + 0.15f && !hullInside() ? PAL_ALERT : PAL_HUD);
    hud(28, 1, "SET EAST", PAL_BANNER);
    int nIn = insideCount();
    const char* line = "MAKE THE BOX";
    int pal = PAL_HUD;
    if (hold_ > 0.02f) {
        int n = std::max(1, std::min(5, int(hold_ / kHoldNeed * 5.f + 0.001f)));
        char pips[8];
        for (int i = 0; i < 5; i++) pips[i] = i < n ? '#' : '-';
        pips[5] = 0;
        std::snprintf(buf, sizeof buf, "HOLD %s", pips);
        line = buf;
        pal = PAL_WIN;
    } else if (nIn == 4) {
        line = "ALL INSIDE  STOP";
        pal = PAL_WIN;
    } else if (nIn > 0) {
        line = "CLOSE IS STILL OUTSIDE";
        pal = PAL_ALERT;
    } else if (speed_ < 0.75f) {
        line = "TOO SLOW  OUTSIDE";
        pal = PAL_ALERT;
    } else if (y_ > kBoxN) {
        line = "BACK HER DOWN";
        pal = PAL_BANNER;
    } else if (y_ > kBoxS - 8.f) {
        line = "EASE HER IN";
        pal = PAL_HUD;
    }
    hudC(26, line, pal);
    hud(1, 27, "Z PORT BOW  X STBD BOW  C HORN", PAL_DIM);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -20 || cy + h * 0.5f < -20 || cx - w * 0.5f > gs::SCREEN_W + 20 || cy - h * 0.5f > gs::SCREEN_H + 20)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * zoom_;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * zoom_, pal, false);
}

void Game::localWorld(float lx, float ly, float& wx, float& wy) const {
    float c = std::cos(heading_), s = std::sin(heading_);
    wx = x_ + ly * c + lx * s;
    wy = y_ + ly * s - lx * c;
}

void Game::edge(float x0, float y0, float x1, float y1, int pal) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = std::hypot(dx, dy);
    bool vertical = std::fabs(dy) > std::fabs(dx);
    int n = std::max(1, int(std::lround(len / 4.4f)));
    for (int i = 0; i <= n; i++) {
        float u = float(i) / float(n);
        place(vertical ? art_.vbar : art_.hbar, x0 + dx * u, y0 + dy * u, vertical ? 3.3f : 1.15f, pal);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) / std::max(zoom_, 0.25f);
        float u = clampf((wy - 30.f) / 130.f, 0.f, 1.f);
        uint16_t water = lerpC(gs::rgb4(1, 5, 9), gs::rgb4(3, 10, 13), u);
        float sh = 0.5f + 0.5f * std::sin(y * 0.11f + t_ * 1.7f + wy * 0.04f);
        water = lerpC(water, gs::rgb4(8, 14, 15), sh * 0.10f);
        v.lineBackdrop[y] = water;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        spr(art_.title, 96.f, 16.f, float(art_.title.h), PAL_BANNER);
        spr(art_.boxWord, 230.f, 18.f, float(art_.boxWord.h), PAL_BANNER);
    } else if (banner_ && (mode_ == Mode::Win || mode_ == Mode::Fail || mode_ == Mode::Pause)) {
        int pal = mode_ == Mode::Win ? PAL_WIN : mode_ == Mode::Fail ? PAL_ALERT : PAL_BANNER;
        spr(*banner_, 160.f, 36.f, float(banner_->h), pal);
    }

    for (int i = 0; i < 3; i++) {
        float gx = 36.f + i * 92.f + std::sin(t_ * 0.5f + i) * 10.f;
        float gy = 18.f + std::cos(t_ * 0.35f + i * 1.4f) * 6.f;
        bool up = std::sin(t_ * 6.f + i * 2.f) > 0.f;
        spr(art_.gull[up ? 0 : 1], gx, gy, 11.f, PAL_GULL, false);
    }

    float bsx, bsy;
    worldToScreen(x_, y_, bsx, bsy);
    bsy += std::sin(t_ * 2.1f + x_ * 0.04f) * 0.8f;
    const gs::Mipped& hull = art_.hull[shipFrame()];
    float shipH = kSpriteWorld * zoom_;
    spr(hull, bsx, bsy, shipH, PAL_FERRY);
    spr(hull, bsx + 3.f, bsy + 4.f, shipH, PAL_FERRY, true);

    for (const Puff& p : smoke_)
        if (p.life > 0.05f) place(art_.smoke, p.x, p.y, 1.5f + p.life, PAL_FOAM);
    for (const Puff& p : wake_)
        if (p.life > 0.05f) place(art_.foam, p.x, p.y, 1.3f + p.life * 1.4f, PAL_FOAM);

    int markPal = (hold_ > 0.02f || mode_ == Mode::Win) ? PAL_WIN : PAL_MARK;
    const float posts[4][2] = {{kBoxL - 1.6f, kBoxS - 1.6f},
                               {kBoxR + 1.6f, kBoxS - 1.6f},
                               {kBoxL - 1.6f, kBoxN + 1.6f},
                               {kBoxR + 1.6f, kBoxN + 1.6f}};
    for (const auto& c : posts) place(art_.post, c[0], c[1], 6.4f, PAL_POST);
    edge(kBoxL, kBoxS, kBoxR, kBoxS, markPal);
    edge(kBoxR, kBoxS, kBoxR, kBoxN, markPal);
    edge(kBoxR, kBoxN, kBoxL, kBoxN, markPal);
    edge(kBoxL, kBoxN, kBoxL, kBoxS, markPal);
    place(art_.north, 0.f, kBoxN + 3.4f, 2.6f, PAL_BANNER);

    float qh = 20.f;
    float qw = qh * float(art_.quay.w) / float(std::max(1, art_.quay.h));
    for (int i = 0; i < 8; i++) {
        float qy = 28.f + i * 16.f;
        place(art_.quay, kWest - qw * 0.5f + 0.6f, qy, qh, PAL_SHORE);
        place(art_.quay, kEast + qw * 0.5f - 0.6f, qy, qh, PAL_SHORE);
    }
    place(art_.shed, kEast + 6.f, 108.f, 14.f, PAL_SHORE);
    place(art_.light, kWest - 5.f, 142.f, 18.f, PAL_LIGHT);

    float pcx, pcy, psx, psy;
    worldToScreen(0.f, (kBoxS + kBoxN) * 0.5f, pcx, pcy);
    psx = (kBoxR - kBoxL) * zoom_;
    psy = (kBoxN - kBoxS) * zoom_;
    sprBox(art_.pad, pcx, pcy, psx, psy, markPal);

    for (const Puff& p : foam_)
        if (p.life > 0.05f) place(art_.foam, p.x, p.y, 1.6f, PAL_FOAM);
    for (int i = 0; i < 6; i++) {
        float span = 36.f;
        float px = -18.f + std::fmod(t_ * kCx * 3.2f + i * 11.f, span);
        float py = 46.f + i * 14.f;
        place(art_.chev, px, py, 2.1f, PAL_MARK);
    }

    drawHud();
}

}  // namespace ferrybox
