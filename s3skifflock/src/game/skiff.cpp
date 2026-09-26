#include "skiff.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace skifflock {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr float TAU = 6.2831853f;
constexpr float GATE_LO = 26.f;
constexpr float GATE_HI = 48.f;
constexpr float LOCK_HALF = 2.32f;
constexpr float CANAL_HALF = 6.1f;
constexpr float MEET = 0.68f;
constexpr float HALF_L = 2.25f;
constexpr float HALF_B = 0.70f;
constexpr float BOAT_M = 4.50f;
constexpr float HOLD_LO = GATE_LO - 8.6f;
constexpr float HOLD_HI = GATE_HI - 7.4f;
constexpr float PLAY_ZOOM = 11.5f;
constexpr float TITLE_ZOOM = 4.7f;
constexpr float HIT_R = 0.16f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float wrapPi(float a) {
    while (a > PI) a -= TAU;
    while (a < -PI) a += TAU;
    return a;
}

uint16_t lerpColor(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

float segDist(float px, float py, float ax, float ay, float bx, float by) {
    float abx = bx - ax, aby = by - ay;
    float den = abx * abx + aby * aby;
    float t = den < 1e-6f ? 0.f : ((px - ax) * abx + (py - ay) * aby) / den;
    t = clampf(t, 0.f, 1.f);
    float dx = px - (ax + abx * t);
    float dy = py - (ay + aby * t);
    return std::sqrt(dx * dx + dy * dy);
}

}  // namespace

float Game::bowY() const { return y_ + std::cos(hdg_) * HALF_L; }
float Game::sternY() const { return y_ - std::cos(hdg_) * HALF_L; }

float Game::halfAt(float y) const {
    auto smooth = [](float u) {
        u = clampf(u, 0.f, 1.f);
        return u * u * (3.f - 2.f * u);
    };
    const float mouth = 7.6f;
    if (y < GATE_LO - mouth) return CANAL_HALF;
    if (y < GATE_LO - 0.4f) {
        float u = (y - (GATE_LO - mouth)) / (mouth - 0.4f);
        return CANAL_HALF + (LOCK_HALF - CANAL_HALF) * smooth(u);
    }
    if (y <= GATE_HI + 0.4f) return LOCK_HALF;
    if (y < GATE_HI + mouth) {
        float u = (y - (GATE_HI + 0.4f)) / (mouth - 0.4f);
        return LOCK_HALF + (CANAL_HALF - LOCK_HALF) * smooth(u);
    }
    return CANAL_HALF;
}

float Game::sx(float wx) const { return 160.f + (wx - camX_) * zoom_ + shx_; }
float Game::sy(float wy) const { return 118.f - (wy - camY_) * zoom_ + shy_; }

void Game::leafGeom(int which, float open, float& hx, float& hy, float& tx, float& ty) const {
    // Closed leaf meets upstream of the hinge. Open leaf lies on the wall:
    // lower tips downstream, upper tips upstream.
    const bool upper = which >= 2;
    const bool right = (which & 1) != 0;
    const float gy = upper ? GATE_HI : GATE_LO;
    const float sign = right ? 1.f : -1.f;
    const float len = std::sqrt(LOCK_HALF * LOCK_HALF + MEET * MEET);
    hx = sign * LOCK_HALF;
    hy = gy;
    const float meetX = 0.f;
    const float meetY = gy + MEET;
    const float cang = std::atan2(meetY - hy, meetX - hx);
    const float oX = sign * (LOCK_HALF - 0.16f);
    const float oY = gy + (upper ? len : -len);
    const float oang = std::atan2(oY - hy, oX - hx);
    const float ang = cang + wrapPi(oang - cang) * clampf(open, 0.f, 1.f);
    tx = hx + std::cos(ang) * len;
    ty = hy + std::sin(ang) * len;
}

bool Game::throatClear(bool upper) const {
    const float open = upper ? openHi_ : openLo_;
    if (open < 0.82f) return false;
    float h0, g0, t0x, t0y, h1, g1, t1x, t1y;
    leafGeom(upper ? 2 : 0, open, h0, g0, t0x, t0y);
    leafGeom(upper ? 3 : 1, open, h1, g1, t1x, t1y);
    if (std::fabs(t0x) < 1.70f || std::fabs(t1x) < 1.70f) return false;
    if (!upper && (t0y > GATE_LO - 1.35f || t1y > GATE_LO - 1.35f)) return false;
    if (upper && (t0y < GATE_HI + 1.35f || t1y < GATE_HI + 1.35f)) return false;
    return true;
}

bool Game::inChamber() const { return sternY() > GATE_LO + 2.5f && bowY() < GATE_HI - 3.0f; }

int Game::yawFrame() const {
    float u = std::fmod(hdg_, TAU);
    if (u < 0.f) u += TAU;
    int i = int(std::lround(u / TAU * float(YAWS))) % YAWS;
    if (i < 0) i += YAWS;
    return i;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail || over_) return 5;
    if (phase_ == Phase::Lift) return 3;
    if (phase_ == Phase::Out) return 4;
    if (phase_ == Phase::Throat && (commitLo_ || openLo_ > 0.4f)) return 2;
    return 1;
}

void Game::resetPose() {
    phase_ = Phase::Wait;
    over_ = false;
    won_ = false;
    why_ = "";
    commitLo_ = false;
    commitHi_ = false;
    playT_ = 0;
    x_ = 1.15f;
    y_ = 9.5f;
    hdg_ = 0.18f;
    speed_ = 0;
    yawV_ = 0;
    driftX_ = 0;
    driftY_ = 0;
    thrust_ = 0;
    steer_ = 0;
    throttle_ = 0;
    openLo_ = openHi_ = 0;
    tgtLo_ = tgtHi_ = 0;
    fill_ = 0;
    dwell_ = 0;
    shake_ = 0;
    wakeT_ = 0;
    wakeN_ = 0;
    chimeStep_ = -1;
    for (Wake& w : wakes_) w = {};
}

void Game::snapCamera() {
    if (mode_ == Mode::Title) {
        camX_ = 0.15f;
        camY_ = 32.f;
        zoom_ = TITLE_ZOOM;
    } else if (mode_ == Mode::Win) {
        camX_ = x_ * 0.35f;
        camY_ = y_ - 0.2f;
        zoom_ = 7.4f;
    } else {
        camX_ = x_ * 0.55f;
        camY_ = y_ + 0.8f;
        zoom_ = PLAY_ZOOM;
    }
}

void Game::followCamera() {
    float tx = x_ * 0.55f, ty = y_ + 0.8f, tz = PLAY_ZOOM;
    if (mode_ == Mode::Title) {
        tx = 0.15f;
        ty = 32.f;
        tz = TITLE_ZOOM;
    } else if (mode_ == Mode::Win) {
        tx = x_ * 0.35f;
        ty = y_ - 0.4f;
        tz = 7.2f;
    }
    float k = 1.f - std::exp(-DT * 6.f);
    camX_ += (tx - camX_) * k;
    camY_ += (ty - camY_) * k;
    zoom_ += (tz - zoom_) * k;
    if (zoom_ < 1.f) zoom_ = 1.f;
}

void Game::begin() {
    resetPose();
    mode_ = Mode::Play;
    snapCamera();
}

void Game::showTitle() {
    resetPose();
    mode_ = Mode::Title;
    snapCamera();
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.55f, 520.f, 0.28f);
    sys_->apu.tone(2, 0, 0);
    sys_->rumble(0.7f, 0.4f, 180);
    sys_->setLight(180, 30, 20);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "clear";
    chime(true);
    sys_->rumble(0.35f, 0.15f, 160);
    sys_->setLight(40, 160, 70);
}

void Game::chime(bool big) {
    chimeBig_ = big;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::pilot(float& thrust, float& steer) const {
    const float mid = (GATE_LO + GATE_HI) * 0.5f;
    float ty = HOLD_LO;
    float hurry = 1.4f;
    if (phase_ == Phase::Wait) {
        ty = HOLD_LO;
        hurry = std::fabs(y_ - ty) > 3.f ? 1.65f : 0.40f;
    } else if (phase_ == Phase::Throat) {
        if (!commitLo_) {
            ty = HOLD_LO;
            hurry = std::fabs(y_ - ty) > 2.f ? 1.05f : 0.32f;
        } else {
            ty = mid + 0.4f;
            float crooked = std::fabs(hdg_) + std::fabs(x_) * 0.85f;
            hurry = crooked > 0.34f ? 0.38f : 1.10f;
        }
    } else if (phase_ == Phase::Settle || phase_ == Phase::Lift) {
        ty = mid;
        hurry = 0.85f;
    } else if (!commitHi_) {
        ty = HOLD_HI;
        hurry = std::fabs(y_ - ty) > 2.f ? 0.95f : 0.30f;
    } else {
        ty = GATE_HI + 16.f;
        float crooked = std::fabs(hdg_) + std::fabs(x_) * 0.85f;
        hurry = crooked > 0.34f ? 0.36f : 1.45f;
    }

    float dx = 0.f - x_;
    float dy = ty - y_;
    float dist = std::sqrt(dx * dx + dy * dy);
    float sp = dist < 0.18f ? 0.f : std::min(hurry, dist * 0.72f);
    float desVx = dist > 0.05f ? dx / dist * sp : 0.f;
    float desVy = dist > 0.05f ? dy / dist * sp : 0.f;
    float dvx = desVx - driftX_;
    float dvy = desVy - driftY_;
    float wantH = std::atan2(dvx, dvy);
    float lim = (phase_ == Phase::Wait) ? 0.55f : 0.40f;
    wantH = clampf(wantH, -lim, lim);
    float wantSpd = clampf(dvx * std::sin(wantH) + dvy * std::cos(wantH), -1.15f, 2.05f);
    float hErr = wrapPi(wantH - hdg_);
    steer = clampf(hErr * 3.1f - yawV_ * 0.85f, -1.f, 1.f);
    thrust = clampf((wantSpd - speed_) * 2.0f, -1.f, 1.f);
}

void Game::controls() {
    const gs::Pad& p = sys_->pad;
    steer_ = 0.f;
    if (p.down(gs::BTN_LEFT)) steer_ -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer_ += 1.f;
    if (std::fabs(p.axisX) > 0.15f) steer_ = clampf(p.axisX, -1.f, 1.f);

    float want = 0.f;
    const bool ahead = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO);
    const bool astern = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B);
    if (ahead) want = 1.f;
    if (astern) want = -1.f;
    if (p.accel > 0.08f) want = std::max(want, p.accel);
    if (p.brake > 0.08f) want = std::min(want, -p.brake);
    float k = 1.f - std::exp(-5.5f * DT);
    throttle_ += (want - throttle_) * k;
    thrust_ = throttle_;
}

void Game::integrate(float dt) {
    yawV_ += steer_ * (1.85f + std::fabs(speed_) * 0.50f) * dt;
    yawV_ -= yawV_ * 3.4f * dt;
    yawV_ = clampf(yawV_, -1.6f, 1.6f);
    hdg_ = wrapPi(hdg_ + yawV_ * dt);

    float drive = thrust_ * (thrust_ >= 0.f ? 3.6f : 2.8f);
    speed_ += drive * dt;
    speed_ -= speed_ * 1.25f * dt;
    speed_ = clampf(speed_, -1.35f, 2.15f);

    float sway = 0.f, surge = 0.f;
    if (phase_ == Phase::Lift) {
        float s = std::sin(clampf(fill_, 0.f, 1.f) * PI);
        sway += 0.95f * s;
        surge -= 0.55f * s;
    } else if (phase_ == Phase::Settle) {
        sway += 0.22f * (1.f - openLo_);
    }
    driftX_ += sway * dt;
    driftY_ += surge * dt;
    // A skiff's outboard walks the stern, so a held helm crabs as well as yaws.
    driftX_ += std::cos(hdg_) * steer_ * 0.90f * dt;
    driftY_ -= std::sin(hdg_) * steer_ * 0.90f * dt;
    driftX_ -= driftX_ * 2.6f * dt;
    driftY_ -= driftY_ * 2.6f * dt;

    x_ += (std::sin(hdg_) * speed_ + driftX_) * dt;
    y_ += (std::cos(hdg_) * speed_ + driftY_) * dt;
    if (y_ < 3.2f) {
        y_ = 3.2f;
        if (speed_ < 0.f) speed_ = 0.f;
        driftY_ = std::max(0.f, driftY_);
    }
    if (y_ > 66.f) {
        y_ = 66.f;
        if (speed_ > 0.f) speed_ *= 0.2f;
    }
}

void Game::confine() {
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    const float along[3] = {0.94f, 0.f, -0.94f};
    const float side[2] = {-0.94f, 0.94f};
    for (int pass = 0; pass < 2; pass++) {
        float pushR = 0.f, pushL = 0.f;
        for (float a : along) {
            for (float b : side) {
                float wx = x_ + fx * a * HALF_L + rx * b * HALF_B;
                float wy = y_ + fy * a * HALF_L + ry * b * HALF_B;
                float lim = halfAt(wy) - 0.08f;
                pushR = std::max(pushR, wx - lim);
                pushL = std::max(pushL, -lim - wx);
            }
        }
        if (pushR <= 0.f && pushL <= 0.f) break;
        if (pushR > pushL) x_ -= pushR;
        else x_ += pushL;
        if (pushR > pushL && driftX_ > 0.f) driftX_ *= 0.35f;
        if (pushL > pushR && driftX_ < 0.f) driftX_ *= 0.35f;
        if (std::max(pushR, pushL) > 0.05f) yawV_ *= 0.7f;
    }
}

const char* Game::gateHit() const {
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    const float along[6] = {0.96f, 0.55f, 0.15f, -0.2f, -0.6f, -0.96f};
    const float side[3] = {-0.96f, 0.f, 0.96f};
    for (int which = 0; which < 4; which++) {
        const float open = which < 2 ? openLo_ : openHi_;
        float hx, hy, tx, ty;
        leafGeom(which, open, hx, hy, tx, ty);
        float dx = tx - hx, dy = ty - hy;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.4f) {
            float cut = 0.34f / len;
            hx += dx * cut;
            hy += dy * cut;
        }
        for (float a : along) {
            for (float b : side) {
                float wx = x_ + fx * a * HALF_L * 0.94f + rx * b * HALF_B * 0.94f;
                float wy = y_ + fy * a * HALF_L * 0.94f + ry * b * HALF_B * 0.94f;
                if (segDist(wx, wy, hx, hy, tx, ty) < HIT_R) return which < 2 ? "scraped the lower gate" : "scraped the upper gate";
            }
        }
    }
    return nullptr;
}

void Game::step() {
    playT_ += DT;
    if (playT_ > 72.f) {
        fail("timed out");
        return;
    }

    bool call = false;
    if (bot_) {
        pilot(thrust_, steer_);
        throttle_ = thrust_;
    } else {
        controls();
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z)) call = true;
    }

    integrate(DT * 0.5f);
    confine();
    integrate(DT * 0.5f);
    confine();

    auto ease = [&](float& o, float target) {
        const float rate = (target > o ? 0.52f : 0.70f) * DT;
        if (o < target) o = std::min(target, o + rate);
        else o = std::max(target, o - rate);
    };
    ease(openLo_, tgtLo_);
    ease(openHi_, tgtHi_);

    if (const char* hit = gateHit()) {
        fail(hit);
        return;
    }

    const float dist = GATE_LO - bowY();
    const bool pointed = std::fabs(hdg_) < 0.65f;
    if (phase_ == Phase::Wait) {
        const bool inRange = dist < 9.5f && dist > 2.4f && std::fabs(x_) < 2.4f;
        const bool eased = std::fabs(speed_) < 1.0f && pointed;
        const bool hailed = call && std::fabs(speed_) < 1.35f && std::fabs(hdg_) < 0.75f;
        if (inRange && (eased || hailed)) {
            phase_ = Phase::Throat;
            tgtLo_ = 1.f;
            dwell_ = 0.f;
            commitLo_ = false;
            chime(false);
        }
    } else if (phase_ == Phase::Throat) {
        if (!commitLo_ && throatClear(false) && std::fabs(x_) < 0.38f && std::fabs(hdg_) < 0.30f) commitLo_ = true;
        const bool outside = bowY() < GATE_LO - 0.4f;
        const bool hesitating = outside && openLo_ > 0.96f && speed_ < 0.45f && bowY() < GATE_LO - 3.5f;
        if (hesitating) dwell_ += DT;
        else dwell_ = std::max(0.f, dwell_ - DT);
        if (dwell_ > 7.f) tgtLo_ = 0.f;
        if (tgtLo_ < 0.5f && openLo_ < 0.04f && bowY() < GATE_LO - 0.5f) {
            phase_ = Phase::Wait;
            commitLo_ = false;
            dwell_ = 0.f;
        }
        if (inChamber()) {
            phase_ = Phase::Settle;
            tgtLo_ = 0.f;
            dwell_ = 0.f;
        }
    } else if (phase_ == Phase::Settle) {
        if (openLo_ < 0.05f && inChamber()) {
            phase_ = Phase::Lift;
            fill_ = 0.f;
            chime(false);
        }
    } else if (phase_ == Phase::Lift) {
        fill_ = std::min(1.f, fill_ + DT / 3.3f);
        if (fill_ >= 1.f) {
            phase_ = Phase::Out;
            tgtHi_ = 1.f;
            commitHi_ = false;
            chime(false);
        }
    } else if (phase_ == Phase::Out) {
        if (!commitHi_ && throatClear(true) && std::fabs(x_) < 0.38f && std::fabs(hdg_) < 0.30f && bowY() < GATE_HI - 3.2f)
            commitHi_ = true;
        if (sternY() > GATE_HI + 3.2f && openHi_ > 0.75f) {
            win();
            return;
        }
    }

    wakeT_ -= DT;
    if (wakeT_ <= 0.f && std::fabs(speed_) > 0.35f) {
        wakeT_ = 0.09f;
        Wake& w = wakes_[wakeN_++ % 14];
        w.x = x_ - std::sin(hdg_) * HALF_L * 0.9f;
        w.y = y_ - std::cos(hdg_) * HALF_L * 0.9f;
        w.life = 1.f;
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= DT;
}

const char* Game::hint() const {
    if (mode_ == Mode::Win) return "THE SKIFF IS THROUGH";
    if (mode_ == Mode::Fail) return why_ && why_[0] ? why_ : "SCRAPED A GATE";
    if (mode_ == Mode::Pause) return "PAUSED";
    if (mode_ == Mode::Title) return "ENTER TO CAST OFF";
    const float dist = GATE_LO - bowY();
    switch (phase_) {
    case Phase::Wait:
        if (dist < 10.f && std::fabs(speed_) > 1.05f) return "TOO FAST - EASE UP";
        if (dist < 11.f) return "EASE UP. THE KEEPER HAS THE GATE";
        return "ONE LOCK AHEAD. DON'T SCRAPE A GATE";
    case Phase::Throat:
        if (tgtLo_ < 0.5f) return "THE LOWER GATE IS CLOSING";
        if (!throatClear(false)) return "HOLD FOR THE GAP";
        return "THROUGH. DON'T CLIP A LEAF";
    case Phase::Settle:
        return "LET THE LOWER GATE SHUT BEHIND YOU";
    case Phase::Lift:
        return "HOLD HER. THE POUND IS RISING";
    case Phase::Out:
        if (!throatClear(true)) return "WAIT FOR THE UPPER GATE";
        return "CLEAR THE UPPER BUOYS";
    }
    return "";
}

const char* Game::gateLabel() const {
    switch (phase_) {
    case Phase::Wait: return "LOWER SHUT";
    case Phase::Throat:
        if (tgtLo_ < 0.5f) return "LOWER CLOSING";
        return throatClear(false) ? "LOWER CLEAR" : "LOWER OPENING";
    case Phase::Settle: return "LOWER SHUTTING";
    case Phase::Lift: return "POUND RISING";
    case Phase::Out:
        if (throatClear(true)) return "UPPER CLEAR";
        return openHi_ > 0.02f ? "UPPER OPENING" : "UPPER SHUT";
    }
    return "";
}

void Game::audio() {
    if (chimeStep_ >= 0) {
        chimeT_ -= DT;
        if (chimeT_ <= 0.f) {
            static const float winN[] = {523.25f, 659.25f, 783.99f, 1046.5f};
            static const float gateN[] = {392.f, 523.25f};
            const float* notes = chimeBig_ ? winN : gateN;
            const int count = chimeBig_ ? 4 : 2;
            if (chimeStep_ >= count) {
                sys_->apu.tone(0, 0, 0);
                chimeStep_ = -1;
            } else {
                sys_->apu.tone(0, notes[chimeStep_], chimeBig_ ? 0.07f : 0.05f);
                chimeStep_++;
                chimeT_ = 0.16f;
            }
        }
    }
    if (mode_ == Mode::Play && (std::fabs(thrust_) > 0.05f || std::fabs(speed_) > 0.25f)) {
        float wob = 0.72f + 0.28f * std::sin(t_ * 28.f);
        float vol = (0.018f + std::fabs(thrust_) * 0.032f) * wob;
        sys_->apu.tone(2, 56.f + std::fabs(thrust_) * 34.f + std::fabs(speed_) * 8.f, vol);
    } else {
        sys_->apu.tone(2, 0, 0);
    }
    float water = mode_ == Mode::Play ? 0.015f + std::fabs(speed_) * 0.01f : 0.01f;
    if (phase_ == Phase::Lift) water += 0.028f * std::sin(clampf(fill_, 0.f, 1.f) * PI);
    float rate = 160.f + (phase_ == Phase::Lift ? fill_ * 260.f : 0.f);
    sys_->apu.noise(water, rate, false);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow, bool flip) {
    if (h < 0.8f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(m.h, 1));
    if (cx + w * 0.5f < -8.f || cx - w * 0.5f > gs::SCREEN_W + 8.f) return;
    if (cy + h * 0.5f < -8.f || cy - h * 0.5f > gs::SCREEN_H + 8.f) return;
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
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip) {
    spr(m, sx(wx), sy(wy), worldH * zoom_, pal, false, flip);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !*s) return;
    const float adv = 18.f * scale;
    const float left = x - float(std::strlen(s)) * adv * 0.5f;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        float h = float(g.h) * scale;
        spr(g, left + (float(i) + 0.5f) * adv, y, h, pal);
    }
}

void Game::drawLeaf(float x0, float y0, float x1, float y1) {
    float xA = sx(x0), yA = sy(y0), xB = sx(x1), yB = sy(y1);
    float pix = std::sqrt((xB - xA) * (xB - xA) + (yB - yA) * (yB - yA));
    if (pix < 2.f) return;
    float ang = std::atan2(yB - yA, xB - xA);
    int bin = int(std::lround(ang * float(GATE_DIRS) / TAU));
    bin = (bin % GATE_DIRS + GATE_DIRS) % GATE_DIRS;
    const gs::Mipped& m = art_.leaf[bin];
    float scale = pix / float(LEAF_PX);
    spr(m, (xA + xB) * 0.5f, (yA + yB) * 0.5f, float(m.h) * scale, PAL_GATE);
}

void Game::drawGates() {
    for (int which = 0; which < 4; which++) {
        const float open = which < 2 ? openLo_ : openHi_;
        float hx, hy, tx, ty;
        leafGeom(which, open, hx, hy, tx, ty);
        float dx = tx - hx, dy = ty - hy;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.2f) continue;
        float bx = hx - dx / len * 1.45f;
        float by = hy - dy / len * 1.45f;
        drawLeaf(hx, hy, bx, by);
        drawLeaf(hx, hy, tx, ty);
    }
}

void Game::drawBoat() {
    const float bob = std::sin(t_ * (phase_ == Phase::Lift ? 7.f : 2.2f)) * (phase_ == Phase::Lift ? 1.5f : 0.45f);
    const gs::Mipped& hull = art_.skiff[yawFrame()];
    float h = float(hull.h) * (BOAT_M * zoom_ / float(SKIFF_PX));
    float cx = sx(x_), cy = sy(y_) + bob;
    spr(art_.blob, cx, cy + h * 0.08f, h * 0.28f, PAL_FX, true);
    spr(hull, cx, cy, h, PAL_BOAT);
    if (std::fabs(thrust_) > 0.2f) {
        float fx = std::sin(hdg_), fy = std::cos(hdg_);
        spr(art_.foam, sx(x_ - fx * HALF_L * 0.95f), sy(y_ - fy * HALF_L * 0.95f), 8.f + std::fabs(thrust_) * 6.f, PAL_FX);
    }
    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        float z = (6.f + (1.f - w.life) * 10.f) * (zoom_ / PLAY_ZOOM);
        spr(art_.ripple, sx(w.x), sy(w.y), z, PAL_FX);
    }
}

void Game::drawWorld() {
    const float top = camY_ + 150.f / zoom_;
    const float bot = camY_ - 140.f / zoom_;

    // Earlier sprites sit on top. Props and walls go in before the bank tiles.
    float gx = 1.6f * std::sin(t_ * 0.33f);
    float gy = 34.f + 7.5f * std::cos(t_ * 0.27f);
    spr(art_.gull[int(t_ * 3.f) & 1], sx(gx), sy(gy), 16.f, PAL_BIRD);

    float bob = std::sin(t_ * 1.7f);
    place(art_.buoy, -1.85f, HOLD_LO + bob * 0.05f, 1.15f, PAL_PORT);
    place(art_.buoy, 1.85f, HOLD_LO + std::sin(t_ * 1.7f + 1.f) * 0.05f, 1.15f, PAL_STBD);
    place(art_.buoy, -1.85f, GATE_HI + 3.2f + bob * 0.04f, 1.15f, PAL_PORT);
    place(art_.buoy, 1.85f, GATE_HI + 3.2f + std::sin(t_ * 1.7f + 0.6f) * 0.04f, 1.15f, PAL_STBD);
    place(art_.post, -3.35f, HOLD_LO, 1.9f, PAL_GATE);
    place(art_.post, 3.35f, HOLD_LO, 1.9f, PAL_GATE);
    place(art_.keeper, -(LOCK_HALF + 1.35f), GATE_LO + 0.2f, 1.7f, PAL_KEEP);
    place(art_.sign, -(CANAL_HALF + 1.5f), GATE_LO - 11.f, 2.3f, PAL_HOUSE);
    place(art_.cottage, LOCK_HALF + 4.2f, GATE_HI + 2.2f, 3.4f, PAL_HOUSE);
    place(art_.heron, -(CANAL_HALF + 0.35f), 14.5f, 1.5f, PAL_BIRD);
    const float trees[][2] = {{-8.6f, 8.f}, {8.4f, 14.f}, {-9.0f, 22.f}, {8.8f, 36.f}, {-8.4f, 52.f}, {8.6f, 58.f}};
    for (auto& tr : trees) place(art_.tree, tr[0], tr[1], 4.2f, PAL_BANK);

    if (phase_ == Phase::Lift || (phase_ == Phase::Out && openHi_ < 0.35f)) {
        for (int i = 0; i < 4; i++) {
            float fy = GATE_HI - 1.2f - float(i) * 0.7f - fill_ * 1.4f;
            float fx = (i & 1 ? 0.7f : -0.7f) + std::sin(t_ * 3.f + float(i)) * 0.2f;
            spr(art_.foam, sx(fx), sy(fy), 9.f + float(i), PAL_FX);
        }
    }
    for (int i = 0; i < 4; i++) {
        float wy = y_ - 3.f + float(i) * 2.4f;
        float wx = std::sin(t_ * 0.5f + float(i) * 1.3f) * 0.8f;
        spr(art_.ripple, sx(wx), sy(wy), 12.f, PAL_FX);
    }

    const float stoneH = 1.75f;
    const float stoneW = stoneH * float(art_.stone.w) / float(std::max(art_.stone.h, 1));
    for (float wy = std::floor((GATE_LO - 9.f) / 1.55f) * 1.55f; wy < GATE_HI + 9.f; wy += 1.55f) {
        if (wy < bot - 2.f || wy > top + 2.f) continue;
        float edge = halfAt(wy);
        place(art_.stone, -edge - stoneW * 0.5f + 0.05f, wy, stoneH, PAL_STONE, false);
        place(art_.stone, edge + stoneW * 0.5f - 0.05f, wy, stoneH, PAL_STONE, true);
    }

    for (float wy = std::floor(bot / 4.5f) * 4.5f; wy < top; wy += 4.5f) {
        if (wy > GATE_LO - 9.f && wy < GATE_HI + 9.f) continue;
        float edge = halfAt(wy);
        float sway = std::sin(t_ * 1.3f + wy) * 0.08f;
        place(art_.reed, -edge - 0.15f + sway, wy, 1.6f, PAL_BANK);
        place(art_.reed, edge + 0.15f - sway, wy, 1.6f, PAL_BANK);
    }

    const float grassH = 5.2f;
    const float grassW = grassH * float(art_.grass.w) / float(std::max(art_.grass.h, 1));
    const float fieldW = grassH * float(art_.field.w) / float(std::max(art_.field.h, 1));
    for (float wy = std::floor(bot / 4.6f) * 4.6f; wy < top; wy += 4.6f) {
        float edge = halfAt(wy);
        float halfG = grassW * 0.5f;
        place(art_.grass, -edge - halfG + 0.15f, wy, grassH, PAL_BANK, false);
        place(art_.grass, edge + halfG - 0.15f, wy, grassH, PAL_BANK, true);
        for (int i = 1; i <= 3; i++) {
            float shift = halfG + float(i) * (fieldW - 0.4f);
            place(art_.field, -edge - shift, wy, grassH, PAL_BANK, false);
            place(art_.field, edge + shift, wy, grassH, PAL_BANK, false);
        }
    }
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(gs::rgb4(1, 3, 4));
    const uint16_t low = gs::rgb4(1, 4, 5);
    const uint16_t lowHi = gs::rgb4(2, 6, 7);
    const uint16_t up = gs::rgb4(2, 8, 10);
    const uint16_t upHi = gs::rgb4(4, 12, 13);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (118.f - float(y)) / zoom_;
        float band = std::fmod(wy * 0.38f - t_ * 0.35f, 1.f);
        if (band < 0.f) band += 1.f;
        const bool streak = band < 0.16f;
        float k = 0.f;
        if (wy >= GATE_HI) k = 1.f;
        else if (wy >= GATE_LO) k = fill_;
        uint16_t lo = streak ? lowHi : low;
        uint16_t hi = streak ? upHi : up;
        v.lineBackdrop[y] = lerpColor(lo, hi, k);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    shx_ = shy_ = 0.f;
    if (shake_ > 0.f) {
        shx_ = std::sin(t_ * 90.f) * 3.2f * shake_;
        shy_ = std::cos(t_ * 73.f) * 2.2f * shake_;
        shake_ = std::max(0.f, shake_ - DT * 1.4f);
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    backdrop();

    if (mode_ == Mode::Title) text("S3 SKIFF LOCK", 160.f, 28.f, 0.92f, PAL_AMBER);
    else if (mode_ == Mode::Win) text("CLEAR", 160.f, 54.f, 1.15f, PAL_GREEN);
    else if (mode_ == Mode::Fail) text("SCRAPED", 160.f, 54.f, 1.05f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f, 96.f, 1.f, PAL_HUD);

    drawBoat();
    drawGates();
    drawWorld();

    char buf[64];
    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, "S3 SKIFF LOCK", PAL_AMBER);
        if (speed_ < -0.08f) std::snprintf(buf, sizeof buf, "ASTERN %.1f", std::fabs(speed_));
        else std::snprintf(buf, sizeof buf, "%.1f M/S", std::fabs(speed_));
        hud(40 - int(std::strlen(buf)), 0, buf, PAL_HUD);
        int hpal = (phase_ == Phase::Wait && std::fabs(speed_) > 1.05f && (GATE_LO - bowY()) < 10.f) ? PAL_RED : PAL_HUD;
        hud(1, 1, hint(), hpal);

        const char* side = "LINED UP";
        char sideBuf[24];
        if (!(std::fabs(x_) < 0.28f && std::fabs(hdg_) < 0.22f)) {
            std::snprintf(sideBuf, sizeof sideBuf, "%s %.1f", x_ >= 0.f ? "RIGHT" : "LEFT", std::fabs(x_));
            side = sideBuf;
        }
        std::snprintf(buf, sizeof buf, "%s  %s", side, gateLabel());
        hud(1, 2, buf, PAL_GREEN);
        hud(1, 26, "ARROWS DRIVE    Z CALLS THE LOCK", PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hudC(8, "THE SKIFF HAS ONE JOB", PAL_AMBER);
        hudC(9, "PASS THE LOCK", PAL_HUD);
        hudC(10, "DON'T SCRAPE A GATE", PAL_HUD);
        hudC(12, int(t_ * 2.f) % 2 == 0 ? "ENTER TO CAST OFF" : "ARROWS STEER AND DRIVE", PAL_HUD);
        hudC(13, "Z CALLS THE KEEPER", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(10, "PASSED THE LOCK", PAL_GREEN);
        hudC(11, "NOT A GATE WAS SCRAPED", PAL_HUD);
        hudC(13, "ENTER SAILS IT AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(10, why_ && why_[0] ? why_ : "SCRAPED A GATE", PAL_RED);
        hudC(12, "ENTER TRIES THE LOCK AGAIN", PAL_HUD);
    }
    hud(40 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.18f, 0.10f);
    t_ = 0.f;
    if (bot_) begin();
    else showTitle();
    sys.setLight(20, 70, 90);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || bot_) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else step();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    }

    followCamera();
    draw();
    audio();
}

}  // namespace skifflock
