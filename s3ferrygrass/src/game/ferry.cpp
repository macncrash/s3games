#include "game/ferry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ferrygrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;
constexpr float kCx = 0.72f;
constexpr float kGrassL = -16.f;
constexpr float kGrassR = 16.f;
constexpr float kGrassS = 78.f;
constexpr float kGrassN = 122.f;
constexpr float kMargin = 0.35f;
constexpr float kStop = 0.22f;
constexpr float kHoldNeed = 0.50f;
constexpr float kOutNeed = 1.05f;
constexpr float kCrew0 = 46.f;
constexpr float kSurgeA = 4.6f;
constexpr float kSurgeB = 3.6f;
constexpr float kSway = 2.4f;
constexpr float kWest = -36.f;
constexpr float kEast = 36.f;
constexpr float kSouth = 10.f;
constexpr float kNorthLim = 140.f;
constexpr float kStartX = -8.f;
constexpr float kStartY = 30.f;
constexpr float kStartH = 1.15f;
constexpr float kGoalX = -0.4f;
constexpr float kGoalY = 98.f;

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

bool inGrassPt(float x, float y) {
    return x >= kGrassL + kMargin && x <= kGrassR - kMargin && y >= kGrassS + kMargin && y <= kGrassN - kMargin;
}

}  // namespace

bool Game::onGrass() const { return hullOnGrass(); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05f) return 3;
    if (grassCount() > 0) return 2;
    return 1;
}

int Game::shipFrame() const {
    float u = heading_;
    while (u < 0.f) u += kTau;
    while (u >= kTau) u -= kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
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

int Game::grassCount() const {
    float xs[4], ys[4];
    corners(xs, ys);
    int n = 0;
    for (int i = 0; i < 4; i++)
        if (inGrassPt(xs[i], ys[i])) n++;
    return n;
}

bool Game::hullOnGrass() const { return grassCount() == 4; }

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    surge_ = sway_ = yaw_ = 0;
    speed_ = 0;
    hold_ = outT_ = 0;
    race_ = 0;
    crew_ = kCrew0;
    won_ = over_ = announced_ = false;
    chimeN_ = chimeStep_ = 0;
    wakeCursor_ = smokeCursor_ = 0;
    hornT_ = tone0_ = chimeT_ = thumpT_ = 0;
    wakeT_ = smokeT_ = shake_ = thrustIn_ = 0;
    banner_ = nullptr;
    why_[0] = 0;
    for (Puff& p : wake_) p = {};
    for (Puff& p : smoke_) p = {};
    for (int i = 0; i < 6; i++) {
        foam_[i].x = -12.f + i * 5.f;
        foam_[i].y = 40.f + (i % 3) * 8.f;
        foam_[i].life = 0.6f + i * 0.1f;
    }
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 0.f;
    camY_ = 96.f;
    zoom_ = 1.35f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = x_;
    camY_ = y_ + 14.f;
    zoom_ = 1.9f;
    blip(620.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(2, 6, 3));
    sys.apu.setMaster(0.76f);
    sys.apu.setEcho(0.10f, 0.16f, 0.08f);
    t_ = 0;
    if (bot_) startRun();
    else showTitle();
}

void Game::horn() { hornT_ = 0.42f; }

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
    rudder = 0;
    if (p.down(gs::BTN_LEFT)) rudder += 1.f;
    if (p.down(gs::BTN_RIGHT)) rudder -= 1.f;
    if (std::fabs(p.axisX) > 0.20f) rudder = clampf(-p.axisX, -1.f, 1.f);
    bow = 0;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_X)) bow -= 1.f;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_Y)) bow += 1.f;
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO)) horn();
}

void Game::pilot(float& thrust, float& rudder, float& bow) {
    const bool in = hullOnGrass();
    auto pd = [&](float desH) {
        float hErr = wrap(desH - heading_);
        rudder = clampf(hErr * 2.8f - yaw_ * 1.8f, -1.f, 1.f);
        return hErr;
    };
    auto go = [&](float wantVx, float wantVy) {
        float water = y_ < kGrassS ? 1.f : 0.f;
        float ds, dw;
        worldToBody(heading_, wantVx - kCx * water, wantVy, ds, dw);
        float maxS = ds >= 0.f ? kSurgeA : kSurgeB;
        thrust = clampf(ds / maxS, -1.f, 1.f);
        bow = clampf(dw / kSway, -1.f, 1.f);
        rudder = clampf(rudder + bow * 0.25f, -1.f, 1.f);
    };

    if (in) {
        float hErr = pd(kNorth);
        float ax = std::fabs(kGoalX - x_);
        float ay = std::fabs(kGoalY - y_);
        if (ax < 0.45f && ay < 0.7f && std::fabs(hErr) < 0.2f && speed_ < 0.55f) {
            go(0.f, 0.f);
            thrust = 0.f;
            bow *= 0.2f;
            return;
        }
        go(clampf((kGoalX - x_) * 0.45f, -0.55f, 0.55f), clampf((kGoalY - y_) * 0.40f, -0.55f, 0.55f));
        return;
    }

    float aimX = y_ > 64.f ? kGoalX : kGoalX - 3.2f;
    float dx = aimX - x_;
    float dy = kGoalY - y_;
    float dist = std::max(0.2f, std::hypot(dx, dy));
    float hErr = pd(std::atan2(dy, dx));
    if (std::fabs(hErr) > 0.9f) {
        thrust = 0.22f;
        bow = 0.f;
        return;
    }
    float spd = 2.8f;
    if (y_ > 62.f) spd = 1.85f;
    if (y_ > kGrassS - 4.f) spd = 1.45f;
    go(dx / dist * spd, dy / dist * spd);
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
    surge_ *= 0.35f;
    sway_ *= 0.35f;
    yaw_ *= 0.5f;
    if (maxY > kGrassN + 1.f) {
        fail("ran off the grass");
        return;
    }
    if (thumpT_ > 0.f) return;
    thumpT_ = 0.28f;
    shake_ = std::max(shake_, 0.5f);
    sys_->apu.noiseBurst(0.22f, 200.f, 0.1f);
}

void Game::physics(float thrust, float rudder, float bow) {
    float wash = 0.75f + 0.25f * std::min(1.f, std::fabs(surge_) / 3.f);
    float yawTarget = rudder * 0.72f * wash - bow * 0.16f;
    auto lag = [](float cur, float target, float rate) {
        return cur + (target - cur) * (1.f - std::exp(-rate * kDt));
    };
    float surgeTarget = thrust >= 0.f ? thrust * kSurgeA : thrust * kSurgeB;
    surge_ = lag(surge_, surgeTarget, 1.7f);
    sway_ = lag(sway_, bow * kSway, 2.5f);
    yaw_ = lag(yaw_, yawTarget, 4.6f);

    int g = grassCount();
    float gf = g / 4.f;
    if (g == 4) {
        float damp = std::exp(-2.6f * kDt);
        surge_ *= damp;
        sway_ *= damp;
        yaw_ *= damp;
    } else if (g > 0) {
        surge_ *= std::exp(-0.35f * kDt);
    }
    heading_ = wrap(heading_ + yaw_ * kDt);

    float vx, vy;
    bodyToWorld(heading_, surge_, sway_, vx, vy);
    float water = 1.f - gf;
    vx += kCx * water;
    float ox = x_, oy = y_;
    x_ += vx * kDt;
    y_ += vy * kDt;
    if (!std::isfinite(x_) || !std::isfinite(y_) || !std::isfinite(heading_)) {
        fail("lost the channel");
        return;
    }
    resolveBanks();
    if (mode_ != Mode::Run) return;
    speed_ = std::hypot((x_ - ox) / kDt, (y_ - oy) / kDt);
    crew_ = std::max(0.f, crew_ - kDt);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    std::snprintf(why_, sizeof why_, "full stop");
    banner_ = &art_.stopped;
    chime(4);
    sys_->rumble(0.28f, 0.1f, 150);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    bool late = why && std::strstr(why, "other crew");
    banner_ = late ? &art_.late : &art_.missed;
    shake_ = 0.75f;
    sys_->rumble(0.5f, 0.25f, 170);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.38f, 140.f, 0.28f);
}

void Game::judge() {
    if (mode_ != Mode::Run) return;
    float xs[4], ys[4];
    corners(xs, ys);
    float minX = xs[0], maxX = xs[0], minY = ys[0], maxY = ys[0];
    for (int i = 1; i < 4; i++) {
        minX = std::min(minX, xs[i]);
        maxX = std::max(maxX, xs[i]);
        minY = std::min(minY, ys[i]);
        maxY = std::max(maxY, ys[i]);
    }
    bool in = hullOnGrass();
    if (in && !announced_) {
        announced_ = true;
        horn();
    }
    if (maxY > kGrassN - kMargin && minY > kGrassS) {
        fail("ran off the grass");
        return;
    }
    if (minY > kGrassS && (maxX > kGrassR + 1.2f || minX < kGrassL - 1.2f)) {
        fail("off the grass");
        return;
    }
    if (in && speed_ <= kStop) {
        outT_ = 0;
        hold_ += kDt;
        if (hold_ >= kHoldNeed) win();
    } else if (!in && speed_ <= kStop && y_ > 48.f) {
        hold_ = 0;
        outT_ += kDt;
        if (outT_ >= kOutNeed) {
            if (maxY < kGrassS + kMargin) fail("stopped short of the grass");
            else if (minY > kGrassN - kMargin) fail("stopped past the grass");
            else fail("not the grass");
        }
    } else {
        hold_ = 0;
        outT_ = 0;
    }
    if (mode_ == Mode::Run && crew_ <= 0.f) fail("the other crew has the grass");
}

void Game::audio() {
    float water = mode_ == Mode::Run ? 0.014f + std::fabs(speed_) * 0.002f : 0.010f;
    sys_->apu.noise(water, 400.f + std::fabs(speed_) * 16.f, false);
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
        float wob = 0.8f + 0.2f * std::sin(t_ * 9.f);
        sys_->apu.tone(2, 46.f + std::fabs(thrustIn_) * 26.f, (0.012f + std::fabs(thrustIn_) * 0.024f) * wob);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= kDt;
}

void Game::driftFx(float dt) {
    for (Puff& p : foam_) {
        if (p.life <= 0.f) {
            p.x = camX_ + std::sin(t_ * 0.6f + p.y) * 14.f;
            p.y = std::min(kGrassS - 4.f, camY_ + std::cos(t_ * 0.3f) * 10.f);
            p.life = 1.2f;
        }
        p.x += kCx * dt * 0.4f;
        p.life -= dt * 0.32f;
    }
    for (Puff& p : wake_)
        if (p.life > 0.f) p.life -= dt * 0.7f;
    for (Puff& p : smoke_)
        if (p.life > 0.f) {
            p.life -= dt * 0.45f;
            p.y += dt * 1.2f;
        }
    if (mode_ != Mode::Run) return;
    if (speed_ > 0.9f && y_ < kGrassS + 2.f) {
        wakeT_ -= dt;
        if (wakeT_ <= 0.f) {
            wakeT_ = 0.08f;
            float wx, wy;
            localWorld(0.f, -kHalfL * 0.8f, wx, wy);
            wake_[wakeCursor_] = {wx, wy, 1.f};
            wakeCursor_ = (wakeCursor_ + 1) % 12;
        }
    }
    if (std::fabs(thrustIn_) > 0.08f) {
        smokeT_ -= dt;
        if (smokeT_ <= 0.f) {
            smokeT_ = 0.12f;
            float wx, wy;
            localWorld(0.f, -kHalfL * 0.5f, wx, wy);
            smoke_[smokeCursor_] = {wx, wy, 1.f};
            smokeCursor_ = (smokeCursor_ + 1) % 6;
        }
    }
}

void Game::camera() {
    float wantX = x_, wantY = y_ + 6.f, wantZ = 2.2f;
    if (mode_ == Mode::Title) {
        wantX = std::sin(t_ * 0.16f) * 2.f;
        wantY = 98.f;
        wantZ = 1.32f;
    } else if (mode_ == Mode::Run && y_ < 70.f) {
        wantY = y_ + 18.f;
        wantX = x_ * 0.55f;
        wantZ = 1.85f;
    }
    float k = 1.f - std::exp(-kDt * 3.6f);
    camX_ += (wantX - camX_) * k;
    camY_ += (wantY - camY_) * k;
    zoom_ += (wantZ - zoom_) * k;
    if (shake_ > 0.f) {
        camX_ += std::sin(t_ * 44.f) * shake_;
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
    } else if (!bot_ && (mode_ == Mode::Win || mode_ == Mode::Fail)) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    }
    if (mode_ == Mode::Win || hold_ > 0.05f) sys.setLight(40, 180, 70);
    else if (mode_ == Mode::Fail) sys.setLight(180, 40, 28);
    else if (mode_ == Mode::Run && hullOnGrass()) sys.setLight(50, 160, 60);
    else sys.setLight(30, 90, 140);
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
        hudC(23, "LAND ON THE GRASS", PAL_BANNER);
        hudC(24, "COME TO A FULL STOP", PAL_HUD);
        hudC(25, "THE CLOCK IS THE OTHER CREW", PAL_ALERT);
        hudC(26, "ARROWS ENGINE  Z/X BOW  C HORN", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "RETURN", PAL_WIN);
        return;
    }
    int left = std::max(0, int(std::ceil(crew_)));
    std::snprintf(buf, sizeof buf, "CREW %02d", left);
    hud(1, 0, "S3 FERRY GRASS", PAL_BANNER);
    hud(31, 0, buf, left < 8 ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(17, "RETURN CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(16, "FULL STOP ON THE GRASS", PAL_WIN);
        if (!bot_) hudC(18, "RETURN SAILS AGAIN", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_[0] ? why_ : "MISSED", PAL_ALERT);
        if (!bot_) hudC(18, "RETURN TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "SPD %04.1f", speed_);
    hud(1, 1, buf, PAL_HUD);
    const char* line = "MAKE THE GRASS";
    int pal = PAL_HUD;
    if (hold_ > 0.02f) {
        line = "HOLD THE STOP";
        pal = PAL_WIN;
    } else if (hullOnGrass()) {
        line = "ON THE GRASS  STOP";
        pal = PAL_WIN;
    } else if (grassCount() > 0) {
        line = "NOT ALL ON THE GRASS";
        pal = PAL_ALERT;
    } else if (y_ > kGrassS - 10.f) {
        line = "EASE HER UP";
        pal = PAL_HUD;
    }
    hudC(26, line, pal);
    hud(1, 27, "Z PORT BOW  X STBD BOW  C HORN", PAL_DIM);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
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
    int n = std::max(1, int(std::lround(len / 5.f)));
    for (int i = 0; i <= n; i++) {
        float u = float(i) / float(n);
        place(vertical ? art_.vbar : art_.hbar, x0 + dx * u, y0 + dy * u, vertical ? 3.f : 1.1f, pal);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) / std::max(zoom_, 0.25f);
        uint16_t col;
        if (wy >= kGrassS) {
            float u = clampf((wy - kGrassS) / 50.f, 0.f, 1.f);
            col = lerpC(gs::rgb4(3, 9, 3), gs::rgb4(6, 12, 4), u);
            float sh = 0.5f + 0.5f * std::sin(y * 0.17f + wy * 0.3f);
            col = lerpC(col, gs::rgb4(8, 13, 5), sh * 0.12f);
        } else {
            float u = clampf((wy - 10.f) / 70.f, 0.f, 1.f);
            col = lerpC(gs::rgb4(1, 4, 8), gs::rgb4(3, 9, 12), u);
        }
        v.lineBackdrop[y] = col;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        spr(art_.title, 88.f, 18.f, float(art_.title.h), PAL_BANNER);
        spr(art_.grassWord, 220.f, 20.f, float(art_.grassWord.h), PAL_WIN);
    } else if (banner_ && (mode_ == Mode::Win || mode_ == Mode::Fail || mode_ == Mode::Pause)) {
        int pal = mode_ == Mode::Win ? PAL_WIN : mode_ == Mode::Fail ? PAL_ALERT : PAL_BANNER;
        spr(*banner_, 160.f, 34.f, float(banner_->h), pal);
    }

    float bsx, bsy;
    worldToScreen(x_, y_, bsx, bsy);
    const gs::Mipped& hull = art_.hull[shipFrame()];
    float shipH = kSpriteWorld * zoom_;
    spr(hull, bsx, bsy, shipH, PAL_FERRY);
    spr(hull, bsx + 2.f, bsy + 3.f, shipH, PAL_FERRY, true);

    int markPal = (hold_ > 0.02f || mode_ == Mode::Win) ? PAL_WIN : PAL_MARK;
    for (int i = 0; i < 7; i++) {
        place(art_.tuft, kGrassL + 4.f + i * 4.6f, kGrassS + 6.f + (i % 2) * 3.f, 3.2f, PAL_SHORE);
        place(art_.tuft, kGrassL + 6.f + i * 4.2f, kGrassN - 6.f, 3.4f, PAL_SHORE);
    }
    place(art_.shed, -22.f, 112.f, 12.f, PAL_SHORE);
    const float posts[4][2] = {{kGrassL, kGrassS}, {kGrassR, kGrassS}, {kGrassL, kGrassN}, {kGrassR, kGrassN}};
    for (const auto& c : posts) place(art_.post, c[0], c[1], 5.5f, PAL_POST);
    edge(kGrassL, kGrassS, kGrassR, kGrassS, markPal);
    edge(kGrassR, kGrassS, kGrassR, kGrassN, markPal);
    edge(kGrassR, kGrassN, kGrassL, kGrassN, markPal);
    edge(kGrassL, kGrassN, kGrassL, kGrassS, markPal);

    float u = 1.f - clampf(crew_ / kCrew0, 0.f, 1.f);
    float rx = 22.f - u * 18.f;
    float ry = 108.f - u * 6.f;
    place(art_.hull[2], rx, ry, kSpriteWorld * 0.72f, PAL_RIVAL);

    for (const Puff& p : smoke_)
        if (p.life > 0.05f) place(art_.smoke, p.x, p.y, 1.4f + p.life, PAL_FOAM);
    for (const Puff& p : wake_)
        if (p.life > 0.05f) place(art_.foam, p.x, p.y, 1.2f + p.life, PAL_FOAM);
    for (const Puff& p : foam_)
        if (p.life > 0.05f && p.y < kGrassS) place(art_.foam, p.x, p.y, 1.5f, PAL_FOAM);

    for (int i = 0; i < 2; i++) {
        float gx = 40.f + i * 70.f + std::sin(t_ * 0.5f + i) * 8.f;
        float gy = 22.f + std::cos(t_ * 0.4f + i) * 6.f;
        bool up = std::sin(t_ * 5.f + i) > 0.f;
        spr(art_.gull[up ? 0 : 1], gx, gy, 10.f, PAL_GULL);
    }
    drawHud();
}

}  // namespace ferrygrass
