#include "tug.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace tuglock {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr float TAU = 6.2831853f;
constexpr float GATE_LO = 30.f;
constexpr float GATE_HI = 56.f;
constexpr float LOCK_HALF = 5.35f;
constexpr float CANAL_HALF = 11.f;
constexpr float MEET = 0.80f;
constexpr float HALF_L = 4.15f;
constexpr float HALF_B = 1.59f;
constexpr float HIT_R = 0.22f;
constexpr float HOLD_LO = 17.f;
constexpr float HOLD_HI = 44.f;
constexpr float CREW = 108.f;
constexpr float PLAY_ZOOM = 8.6f;
constexpr float TITLE_ZOOM = 3.4f;
constexpr float RIVAL_X = -13.4f;
constexpr float RIVAL_Y = 9.5f;
constexpr float HULL = 0.97f;

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

float Game::crewLeft() const { return std::max(0.f, CREW - playT_); }

float Game::bowY() const { return y_ + std::cos(hdg_) * HALF_L; }
float Game::sternY() const { return y_ - std::cos(hdg_) * HALF_L; }

float Game::halfAt(float y) const {
    auto smooth = [](float u) {
        u = clampf(u, 0.f, 1.f);
        return u * u * (3.f - 2.f * u);
    };
    const float mouth = 9.5f;
    if (y < GATE_LO - mouth) return CANAL_HALF;
    if (y < GATE_LO - 0.5f) {
        float u = (y - (GATE_LO - mouth)) / (mouth - 0.5f);
        return CANAL_HALF + (LOCK_HALF - CANAL_HALF) * smooth(u);
    }
    if (y <= GATE_HI + 0.5f) return LOCK_HALF;
    if (y < GATE_HI + mouth) {
        float u = (y - (GATE_HI + 0.5f)) / (mouth - 0.5f);
        return LOCK_HALF + (CANAL_HALF - LOCK_HALF) * smooth(u);
    }
    return CANAL_HALF;
}

float Game::sx(float wx) const { return 160.f + (wx - camX_) * zoom_ + shx_; }
float Game::sy(float wy) const { return 104.f - (wy - camY_) * zoom_ + shy_; }

void Game::leafGeom(int which, float open, float& hx, float& hy, float& tx, float& ty) const {
    const bool upper = which >= 2;
    const bool right = (which & 1) != 0;
    const float gy = upper ? GATE_HI : GATE_LO;
    const float sign = right ? 1.f : -1.f;
    const float len = std::sqrt(LOCK_HALF * LOCK_HALF + MEET * MEET);
    hx = sign * LOCK_HALF;
    hy = gy;
    const float cang = std::atan2(MEET, -sign * LOCK_HALF);
    const float oY = gy + (upper ? len : -len);
    const float oX = sign * (LOCK_HALF - 0.20f);
    const float oang = std::atan2(oY - hy, oX - hx);
    const float ang = cang + wrapPi(oang - cang) * clampf(open, 0.f, 1.f);
    tx = hx + std::cos(ang) * len;
    ty = hy + std::sin(ang) * len;
}

bool Game::throatClear(bool upper) const {
    const float open = upper ? openHi_ : openLo_;
    if (open < 0.90f) return false;
    float h0, g0, t0x, t0y, h1, g1, t1x, t1y;
    leafGeom(upper ? 2 : 0, open, h0, g0, t0x, t0y);
    leafGeom(upper ? 3 : 1, open, h1, g1, t1x, t1y);
    if (std::fabs(t0x) < 4.15f || std::fabs(t1x) < 4.15f) return false;
    if (!upper && (t0y > GATE_LO - 2.f || t1y > GATE_LO - 2.f)) return false;
    if (upper && (t0y < GATE_HI + 2.f || t1y < GATE_HI + 2.f)) return false;
    return true;
}

bool Game::inChamber() const {
    return sternY() > GATE_LO + 3.1f && bowY() < GATE_HI - 3.8f && std::fabs(x_) < LOCK_HALF - 1.2f;
}

int Game::yawFrameOf(float h) const {
    float u = std::fmod(h, TAU);
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
    if (phase_ == Phase::Throat && (commitLo_ || openLo_ > 0.35f)) return 2;
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
    x_ = 2.35f;
    y_ = 5.2f;
    hdg_ = 0.30f;
    speed_ = 0;
    yawV_ = 0;
    thrust_ = 0;
    steer_ = 0;
    throttle_ = 0;
    openLo_ = openHi_ = 0;
    tgtLo_ = tgtHi_ = 0;
    fill_ = 0;
    dwell_ = 0;
    shake_ = 0;
    wakeT_ = 0;
    smokeT_ = 0;
    hornT_ = 0;
    rubT_ = 0;
    wakeN_ = 0;
    smokeN_ = 0;
    chimeStep_ = -1;
    for (Puff& w : wakes_) w = {};
    for (Puff& s : smokes_) s = {};
}

void Game::snapCamera() {
    if (mode_ == Mode::Title) {
        camX_ = 0.4f;
        camY_ = 36.f;
        zoom_ = TITLE_ZOOM;
    } else if (mode_ == Mode::Win) {
        camX_ = x_ * 0.25f;
        camY_ = y_ - 1.2f;
        zoom_ = 6.4f;
    } else {
        camX_ = x_ * 0.35f;
        camY_ = y_ + 5.2f;
        zoom_ = PLAY_ZOOM;
    }
}

void Game::followCamera() {
    float tx = x_ * 0.35f, ty = y_ + 5.2f, tz = PLAY_ZOOM;
    if (mode_ == Mode::Title) {
        tx = 0.4f;
        ty = 36.f;
        tz = TITLE_ZOOM;
    } else if (mode_ == Mode::Win) {
        tx = x_ * 0.25f;
        ty = y_ - 0.6f;
        tz = 6.2f;
    } else if (mode_ == Mode::Fail) {
        tx = x_ * 0.4f;
        ty = y_ + 1.2f;
        tz = 7.4f;
    }
    float k = 1.f - std::exp(-DT * 5.5f);
    camX_ += (tx - camX_) * k;
    camY_ += (ty - camY_) * k;
    zoom_ += (tz - zoom_) * k;
}

void Game::begin() {
    resetPose();
    mode_ = Mode::Play;
    snapCamera();
    horn(0.32f);
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
    sys_->apu.noiseBurst(0.55f, 420.f, 0.30f);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->rumble(0.75f, 0.4f, 200);
    sys_->setLight(180, 30, 20);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "clear";
    chime(true);
    sys_->rumble(0.35f, 0.16f, 180);
    sys_->setLight(40, 150, 70);
}

void Game::chime(bool big) {
    chimeBig_ = big;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::horn(float seconds) {
    if (hornT_ < 0.05f) hornT_ = seconds;
}

void Game::pilot(float& thrust, float& steer) const {
    float targetY = HOLD_LO;
    float hurry = 1.35f;
    if (phase_ == Phase::Wait) {
        targetY = HOLD_LO;
        hurry = std::fabs(y_ - HOLD_LO) > 5.f ? 1.38f : 0.42f;
    } else if (phase_ == Phase::Throat) {
        if (!commitLo_ || !throatClear(false)) {
            targetY = HOLD_LO;
            hurry = 0.36f;
        } else {
            targetY = (GATE_LO + GATE_HI) * 0.5f;
            float crooked = std::fabs(hdg_) + std::fabs(x_) * 0.55f;
            hurry = crooked > 0.42f ? 0.40f : 1.05f;
        }
    } else if (phase_ == Phase::Settle || phase_ == Phase::Lift) {
        targetY = (GATE_LO + GATE_HI) * 0.5f;
        hurry = 0.55f;
    } else if (!commitHi_ || !throatClear(true)) {
        targetY = HOLD_HI;
        hurry = std::fabs(y_ - HOLD_HI) > 3.f ? 0.85f : 0.36f;
    } else {
        targetY = GATE_HI + 18.f;
        float crooked = std::fabs(hdg_) + std::fabs(x_) * 0.55f;
        hurry = crooked > 0.42f ? 0.42f : 1.40f;
    }

    const bool threading = (phase_ == Phase::Throat && commitLo_ && throatClear(false)) ||
                           (phase_ == Phase::Out && commitHi_ && throatClear(true));
    if (!threading && nearestLeaf() < 0.48f) {
        targetY = y_ - 4.f;
        hurry = 0.8f;
    }

    float along = targetY - y_;
    float wantSpd = clampf(along * 0.72f, -0.85f, hurry);
    if (std::fabs(along) < 0.35f) wantSpd = clampf(along * 0.45f, -0.22f, 0.22f);
    if ((phase_ == Phase::Wait || (phase_ == Phase::Throat && !commitLo_)) && bowY() > GATE_LO - 3.1f)
        wantSpd = std::min(wantSpd, 0.f);
    if (phase_ == Phase::Out && !commitHi_ && bowY() > GATE_HI - 3.3f) wantSpd = std::min(wantSpd, 0.f);
    const bool nearGate = std::fabs(y_ - GATE_LO) < 7.5f || std::fabs(y_ - GATE_HI) < 7.5f;
    if (nearGate && (std::fabs(hdg_) > 0.32f || std::fabs(x_) > 0.75f)) wantSpd = clampf(wantSpd, -0.40f, 0.42f);

    float wantH = clampf(-x_ * (nearGate ? 0.42f : 0.24f), -0.42f, 0.42f);
    float hErr = wrapPi(wantH - hdg_);
    float steerH = clampf(hErr * 3.3f - yawV_ * 1.15f, -1.f, 1.f);
    float steerX = clampf(-x_ * 1.25f, -1.f, 1.f);
    float slow = clampf(1.f - std::fabs(speed_) / 0.95f, 0.f, 1.f);
    steer = clampf(steerH * (1.f - 0.35f * slow) + steerX * (0.25f + 0.70f * slow), -1.f, 1.f);
    thrust = clampf((wantSpd - speed_) * 2.7f, -1.f, 1.f);
}

void Game::controls() {
    const gs::Pad& p = sys_->pad;
    steer_ = 0.f;
    if (p.down(gs::BTN_LEFT)) steer_ -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer_ += 1.f;
    if (std::fabs(p.axisX) > 0.15f) steer_ = clampf(p.axisX, -1.f, 1.f);

    float want = 0.f;
    const bool ahead = p.down(gs::BTN_UP) || p.down(gs::BTN_C);
    const bool astern = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B);
    if (ahead) want = 1.f;
    if (astern) want = -1.f;
    if (p.axisY > 0.22f) want = std::max(want, p.axisY);
    if (p.axisY < -0.22f) want = std::min(want, p.axisY);
    if (p.accel > 0.08f) want = std::max(want, p.accel);
    if (p.brake > 0.08f) want = std::min(want, -p.brake);
    float k = 1.f - std::exp(-3.4f * DT);
    throttle_ += (want - throttle_) * k;
    thrust_ = throttle_;
    if (p.pressed(gs::BTN_TURBO)) horn(0.22f);
}

void Game::integrate(float dt) {
    yawV_ += steer_ * (1.05f + std::fabs(speed_) * 0.40f) * dt;
    yawV_ -= yawV_ * 3.3f * dt;
    yawV_ = clampf(yawV_, -1.25f, 1.25f);
    hdg_ = wrapPi(hdg_ + yawV_ * dt);

    float drive = thrust_ * (thrust_ >= 0.f ? 2.05f : 1.45f);
    speed_ += drive * dt;
    speed_ -= speed_ * 0.72f * dt;
    speed_ = clampf(speed_, -1.0f, 1.50f);

    // Bow thruster: at low way a tug walks sideways instead of only yawing.
    float slide = steer_ * (std::fabs(speed_) < 0.9f ? 0.68f : 0.18f);
    float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    x_ += (std::sin(hdg_) * speed_ + rx * slide) * dt;
    y_ += (std::cos(hdg_) * speed_ + ry * slide) * dt;

    if (phase_ == Phase::Lift) {
        float s = std::sin(clampf(fill_, 0.f, 1.f) * PI);
        x_ += std::sin(t_ * 2.2f) * 0.22f * s * dt;
        y_ -= 0.30f * s * dt;
    }
    if (y_ < 2.2f) {
        y_ = 2.2f;
        if (speed_ < 0.f) speed_ = 0.f;
    }
    if (y_ > 78.f) {
        y_ = 78.f;
        if (speed_ > 0.f) speed_ *= 0.2f;
    }
}

void Game::confine() {
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    const float along[3] = {0.96f, 0.f, -0.96f};
    const float side[2] = {-0.96f, 0.96f};
    float worst = 0.f;
    for (int pass = 0; pass < 2; pass++) {
        float pushR = 0.f, pushL = 0.f;
        for (float a : along) {
            for (float b : side) {
                float wx = x_ + fx * a * HALF_L + rx * b * HALF_B;
                float wy = y_ + fy * a * HALF_L + ry * b * HALF_B;
                float pad = 0.40f;
                if (wy > GATE_LO - 2.f && wy < GATE_HI + 2.f) pad = 0.90f;
                else if (wy > GATE_LO - 10.f && wy < GATE_HI + 10.f) pad = 0.55f;
                float lim = halfAt(wy) - pad;
                pushR = std::max(pushR, wx - lim);
                pushL = std::max(pushL, -lim - wx);
            }
        }
        worst = std::max(worst, std::max(pushR, pushL));
        if (pushR <= 0.f && pushL <= 0.f) break;
        if (pushR > pushL) x_ -= pushR;
        else x_ += pushL;
        if (std::max(pushR, pushL) > 0.04f) {
            speed_ *= 0.86f;
            yawV_ *= 0.55f;
        }
    }
    if (worst > 0.10f && rubT_ <= 0.f) {
        rubT_ = 0.30f;
        sys_->apu.noiseBurst(0.16f, 180.f, 0.07f);
        sys_->rumble(0.25f, 0.1f, 40);
    }
}

void Game::separateRival() {
    float dx = x_ - RIVAL_X, dy = y_ - RIVAL_Y;
    float d = std::sqrt(dx * dx + dy * dy);
    if (d < 3.5f && d > 0.01f) {
        float push = 3.5f - d;
        x_ += dx / d * push;
        y_ += dy / d * push;
        speed_ *= 0.9f;
    }
}

const char* Game::gateHit() const {
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    const float along[6] = {0.98f, 0.62f, 0.28f, -0.08f, -0.48f, -0.98f};
    const float side[3] = {-0.98f, 0.f, 0.98f};
    for (int which = 0; which < 4; which++) {
        const float open = which < 2 ? openLo_ : openHi_;
        float hx, hy, tx, ty;
        leafGeom(which, open, hx, hy, tx, ty);
        float dx = tx - hx, dy = ty - hy;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.5f) {
            float cut = 0.48f / len;
            hx += dx * cut;
            hy += dy * cut;
        }
        for (float a : along) {
            for (float b : side) {
                float wx = x_ + fx * a * HALF_L * HULL + rx * b * HALF_B * HULL;
                float wy = y_ + fy * a * HALF_L * HULL + ry * b * HALF_B * HULL;
                if (segDist(wx, wy, hx, hy, tx, ty) < HIT_R)
                    return which < 2 ? "scraped the lower gate" : "scraped the upper gate";
            }
        }
    }
    return nullptr;
}

float Game::nearestLeaf() const {
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    const float along[5] = {0.98f, 0.5f, 0.f, -0.5f, -0.98f};
    const float side[3] = {-0.98f, 0.f, 0.98f};
    float best = 99.f;
    for (int which = 0; which < 4; which++) {
        const float open = which < 2 ? openLo_ : openHi_;
        float hx, hy, tx, ty;
        leafGeom(which, open, hx, hy, tx, ty);
        float dx = tx - hx, dy = ty - hy;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.5f) {
            float cut = 0.48f / len;
            hx += dx * cut;
            hy += dy * cut;
        }
        for (float a : along) {
            for (float b : side) {
                float wx = x_ + fx * a * HALF_L * HULL + rx * b * HALF_B * HULL;
                float wy = y_ + fy * a * HALF_L * HULL + ry * b * HALF_B * HULL;
                best = std::min(best, segDist(wx, wy, hx, hy, tx, ty));
            }
        }
    }
    return best;
}

void Game::step() {
    playT_ += DT;
    if (playT_ >= CREW) {
        fail("the other crew took the lock");
        return;
    }

    bool call = false;
    if (bot_) pilot(thrust_, steer_);
    else {
        controls();
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z)) {
            call = true;
            horn(0.26f);
        }
    }

    integrate(DT);
    confine();
    separateRival();

    auto ease = [&](float& o, float target) {
        const float rate = (target > o ? 0.58f : 0.78f) * DT;
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
    const bool pointed = std::fabs(hdg_) < 0.75f;
    if (phase_ == Phase::Wait) {
        const bool inRange = dist < 12.f && dist > 3.0f && std::fabs(x_) < 3.4f && pointed;
        const bool eased = std::fabs(speed_) < 0.85f;
        const bool hailed = call && std::fabs(speed_) < 1.35f && std::fabs(hdg_) < 0.9f;
        if (inRange && (eased || hailed)) {
            phase_ = Phase::Throat;
            tgtLo_ = 1.f;
            dwell_ = 0.f;
            commitLo_ = false;
            chime(false);
        }
    } else if (phase_ == Phase::Throat) {
        if (!commitLo_ && throatClear(false) && std::fabs(x_) < 0.65f && std::fabs(hdg_) < 0.30f && speed_ > -0.08f)
            commitLo_ = true;
        const bool lined = std::fabs(x_) < 0.75f && std::fabs(hdg_) < 0.36f;
        const bool hesitating = !commitLo_ && openLo_ > 0.96f && lined && speed_ < 0.32f && bowY() < GATE_LO - 4.f;
        if (hesitating) dwell_ += DT;
        else dwell_ = std::max(0.f, dwell_ - DT);
        if (!commitLo_ && dwell_ > 9.f) tgtLo_ = 0.f;
        if (tgtLo_ < 0.5f && openLo_ < 0.04f && bowY() < GATE_LO - 0.4f) {
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
        } else if (openLo_ < 0.08f && bowY() < GATE_LO - 0.5f) {
            phase_ = Phase::Wait;
            tgtLo_ = 0.f;
            commitLo_ = false;
        }
    } else if (phase_ == Phase::Lift) {
        fill_ = std::min(1.f, fill_ + DT / 3.6f);
        if (fill_ >= 1.f) {
            phase_ = Phase::Out;
            tgtHi_ = 1.f;
            commitHi_ = false;
            chime(false);
        }
    } else if (phase_ == Phase::Out) {
        if (!commitHi_ && throatClear(true) && std::fabs(x_) < 0.65f && std::fabs(hdg_) < 0.30f && bowY() < GATE_HI - 2.6f &&
            speed_ > -0.08f)
            commitHi_ = true;
        if (sternY() > GATE_HI + 6.6f && openHi_ > 0.75f) {
            win();
            return;
        }
    }

    wakeT_ -= DT;
    if (wakeT_ <= 0.f && std::fabs(speed_) > 0.28f) {
        wakeT_ = 0.08f;
        Puff& w = wakes_[wakeN_++ % 12];
        w.x = x_ - std::sin(hdg_) * HALF_L * 0.92f;
        w.y = y_ - std::cos(hdg_) * HALF_L * 0.92f;
        w.life = 1.f;
    }
    for (Puff& w : wakes_)
        if (w.life > 0.f) w.life -= DT * 0.85f;

    smokeT_ -= DT;
    if (smokeT_ <= 0.f && mode_ == Mode::Play) {
        smokeT_ = 0.14f;
        Puff& s = smokes_[smokeN_++ % 8];
        s.x = x_ - std::sin(hdg_) * 0.85f;
        s.y = y_ - std::cos(hdg_) * 0.85f;
        s.life = 1.f;
    }
    for (Puff& s : smokes_)
        if (s.life > 0.f) s.life -= DT * 0.55f;
}

const char* Game::hint() const {
    if (mode_ == Mode::Win) return "THE TUG IS THROUGH";
    if (mode_ == Mode::Fail) return why_ && why_[0] ? why_ : "SCRAPED A GATE";
    if (mode_ == Mode::Pause) return "PAUSED";
    if (mode_ == Mode::Title) return "ENTER TO TAKE THE TUG";
    const float dist = GATE_LO - bowY();
    switch (phase_) {
    case Phase::Wait:
        if (dist < 3.2f) return "TOO CLOSE. BACK HER OFF";
        if (dist < 12.f && std::fabs(speed_) > 0.9f) return "TOO MUCH WAY. EASE OFF";
        if (dist < 13.f) return "EASE UP. THE KEEPER HAS THE GATE";
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
            static const float gateN[] = {349.2f, 523.25f};
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
    if (hornT_ > 0.f) {
        hornT_ -= DT;
        sys_->apu.tone(1, 146.f, 0.055f);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
    if (mode_ == Mode::Play && (std::fabs(thrust_) > 0.04f || std::fabs(speed_) > 0.15f)) {
        float wob = 0.78f + 0.22f * std::sin(t_ * (16.f + std::fabs(thrust_) * 9.f));
        float vol = (0.016f + std::fabs(thrust_) * 0.040f) * wob;
        sys_->apu.tone(2, 42.f + std::fabs(thrust_) * 26.f + std::fabs(speed_) * 8.f, vol);
    } else {
        sys_->apu.tone(2, 0, 0);
    }
    float water = mode_ == Mode::Play ? 0.012f + std::fabs(speed_) * 0.012f : 0.008f;
    if (phase_ == Phase::Lift && mode_ == Mode::Play) water += 0.03f * std::sin(clampf(fill_, 0.f, 1.f) * PI);
    float rate = 140.f + (phase_ == Phase::Lift ? fill_ * 280.f : 0.f);
    sys_->apu.noise(water, rate, false);
    if (rubT_ > 0.f) rubT_ -= DT;
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

void Game::hudR(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(40 - n, row, s, pal);
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
        spr(g, left + (float(i) + 0.5f) * adv, y, float(g.h) * scale, pal);
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
    spr(m, (xA + xB) * 0.5f, (yA + yB) * 0.5f, float(m.h) * (pix / float(LEAF_PX)), PAL_GATE);
}

void Game::drawGates() {
    for (int which = 0; which < 4; which++) {
        const float open = which < 2 ? openLo_ : openHi_;
        float hx, hy, tx, ty;
        leafGeom(which, open, hx, hy, tx, ty);
        drawLeaf(hx, hy, tx, ty);
        place(art_.post, hx, hy, 1.35f, PAL_GATE);
    }
}

void Game::drawBoat(float wx, float wy, float hdg, int pal, float bob) {
    const gs::Mipped& hull = art_.tug[yawFrameOf(hdg)];
    float h = float(hull.h) * ((HALF_L * 2.f) * zoom_ / float(TUG_PX));
    float cx = sx(wx), cy = sy(wy) + bob;
    spr(art_.blob, cx + 3.f, cy + h * 0.08f, h * 0.22f, PAL_FX, true);
    spr(hull, cx, cy, h, pal);
}

void Game::drawWorld() {
    const float top = camY_ + 160.f / zoom_;
    const float bot = camY_ - 150.f / zoom_;

    for (const Puff& w : wakes_) {
        if (w.life <= 0.f) continue;
        float z = (8.f + (1.f - w.life) * 14.f) * (zoom_ / PLAY_ZOOM);
        spr(art_.wake, sx(w.x), sy(w.y), z, PAL_FX);
    }
    if (std::fabs(speed_) > 0.25f && mode_ != Mode::Title) {
        float fx = std::sin(hdg_), fy = std::cos(hdg_);
        spr(art_.foam, sx(x_ - fx * HALF_L * 0.95f), sy(y_ - fy * HALF_L * 0.95f), 7.f + std::fabs(speed_) * 5.f, PAL_FX);
    }
    if (phase_ == Phase::Lift || (phase_ == Phase::Out && openHi_ < 0.4f)) {
        for (int i = 0; i < 5; i++) {
            float fy = GATE_HI - 0.8f - float(i) * 0.85f;
            float fx = (i & 1 ? 1.1f : -1.1f) + std::sin(t_ * 3.f + float(i)) * 0.25f;
            spr(art_.foam, sx(fx), sy(fy), 8.f + float(i), PAL_FX);
        }
    }

    float gullX = 2.4f * std::sin(t_ * 0.37f);
    float gullY = 40.f + 8.f * std::cos(t_ * 0.23f);
    spr(art_.gull[int(t_ * 3.f) & 1], sx(gullX), sy(gullY), 14.f, PAL_BIRD);

    float bob = std::sin(t_ * 1.6f);
    place(art_.buoy, -3.2f, HOLD_LO + bob * 0.06f, 1.35f, PAL_PORT);
    place(art_.buoy, 3.2f, HOLD_LO + std::sin(t_ * 1.6f + 1.f) * 0.06f, 1.35f, PAL_STBD);
    place(art_.buoy, -3.2f, GATE_HI + 5.f + bob * 0.05f, 1.35f, PAL_PORT);
    place(art_.buoy, 3.2f, GATE_HI + 5.f + std::sin(t_ * 1.6f + 0.7f) * 0.05f, 1.35f, PAL_STBD);

    int loPal = throatClear(false) ? PAL_PORT : PAL_STBD;
    int hiPal = throatClear(true) ? PAL_PORT : PAL_STBD;
    place(art_.lamp, -(LOCK_HALF + 0.15f), GATE_LO + 1.1f, 2.3f, loPal);
    place(art_.lamp, LOCK_HALF + 0.15f, GATE_LO + 1.1f, 2.3f, loPal);
    place(art_.lamp, -(LOCK_HALF + 0.15f), GATE_HI - 1.1f, 2.3f, hiPal);
    place(art_.lamp, LOCK_HALF + 0.15f, GATE_HI - 1.1f, 2.3f, hiPal);

    place(art_.keeper, -(LOCK_HALF + 1.7f), GATE_LO + 0.4f, 1.85f, PAL_KEEP);
    place(art_.post, RIVAL_X - 1.7f, RIVAL_Y - 2.3f, 1.5f, PAL_GATE);
    place(art_.post, RIVAL_X + 1.7f, RIVAL_Y + 2.3f, 1.5f, PAL_GATE);
    place(art_.shed, LOCK_HALF + 6.4f, 46.f, 4.6f, PAL_YARD);
    place(art_.crane, LOCK_HALF + 5.2f, GATE_HI + 3.5f, 4.2f, PAL_YARD);
    place(art_.tank, -(CANAL_HALF + 3.6f), 42.f, 3.3f, PAL_YARD);

    const float stoneH = 2.05f;
    const float stoneW = stoneH * float(art_.brick.w) / float(std::max(art_.brick.h, 1));
    for (float wy = std::floor((GATE_LO - 10.f) / 1.7f) * 1.7f; wy < GATE_HI + 10.f; wy += 1.7f) {
        if (wy < bot - 2.f || wy > top + 2.f) continue;
        float edge = halfAt(wy);
        place(art_.brick, -edge - stoneW * 0.5f + 0.2f, wy, stoneH, PAL_BRICK, true);
        place(art_.brick, edge + stoneW * 0.5f - 0.2f, wy, stoneH, PAL_BRICK, false);
    }

    for (float wy = std::floor(bot / 4.2f) * 4.2f; wy < top; wy += 4.2f) {
        bool masonry = wy > GATE_LO - 10.f && wy < GATE_HI + 10.f;
        if (masonry) continue;
        float edge = halfAt(wy);
        float sway = std::sin(t_ * 1.2f + wy) * 0.08f;
        place(art_.reed, -edge - 0.2f + sway, wy, 1.55f, PAL_BANK);
        place(art_.reed, edge + 0.2f - sway, wy, 1.55f, PAL_BANK);
    }

    const float pathH = 4.6f;
    const float pathW = pathH * float(art_.path.w) / float(std::max(art_.path.h, 1));
    const float fieldW = pathH * float(art_.field.w) / float(std::max(art_.field.h, 1));
    for (float wy = std::floor(bot / 4.4f) * 4.4f; wy < top; wy += 4.4f) {
        float edge = halfAt(wy);
        float shift0 = stoneH * 0.3f;
        bool masonry = wy > GATE_LO - 9.f && wy < GATE_HI + 9.f;
        float inner = edge + (masonry ? stoneW * 0.85f : 0.3f);
        place(art_.path, -inner - pathW * 0.5f + 0.3f, wy, pathH, PAL_BANK, true);
        place(art_.path, inner + pathW * 0.5f - 0.3f, wy, pathH, PAL_BANK, false);
        for (int i = 1; i <= 3; i++) {
            float shift = pathW * 0.85f + float(i - 1) * (fieldW * 0.92f) + shift0;
            place(art_.field, -inner - pathW * 0.15f - shift, wy, pathH, PAL_BANK, false);
            place(art_.field, inner + pathW * 0.15f + shift, wy, pathH, PAL_BANK, false);
        }
    }
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(gs::rgb4(1, 3, 4));
    const uint16_t low = gs::rgb4(1, 4, 6);
    const uint16_t lowHi = gs::rgb4(2, 6, 8);
    const uint16_t up = gs::rgb4(2, 8, 10);
    const uint16_t upHi = gs::rgb4(3, 11, 12);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (104.f - float(y)) / std::max(zoom_, 0.2f);
        float band = std::fmod(wy * 0.35f - t_ * 0.28f, 1.f);
        if (band < 0.f) band += 1.f;
        const bool streak = band < 0.14f;
        float k = 0.f;
        if (wy >= GATE_HI) k = 1.f;
        else if (wy >= GATE_LO) k = fill_;
        v.lineBackdrop[y] = lerpColor(streak ? lowHi : low, streak ? upHi : up, k);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    shx_ = shy_ = 0.f;
    if (shake_ > 0.f) {
        shx_ = std::sin(t_ * 90.f) * 3.4f * shake_;
        shy_ = std::cos(t_ * 70.f) * 2.2f * shake_;
        shake_ = std::max(0.f, shake_ - DT * 1.5f);
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    backdrop();

    if (mode_ == Mode::Title) text("S3 TUGBOAT LOCK", 160.f, 16.f, 0.58f, PAL_AMBER);
    else if (mode_ == Mode::Win) text("CLEAR", 160.f, 36.f, 1.15f, PAL_GREEN);
    else if (mode_ == Mode::Fail) text(why_ && std::strcmp(why_, "the other crew took the lock") == 0 ? "TOO LATE" : "SCRAPED",
                                        160.f, 40.f, 1.0f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f, 96.f, 1.f, PAL_HUD);

    float crewY = sy(RIVAL_Y);
    if (crewY > -30.f && crewY < gs::SCREEN_H + 30.f) text("CREW", sx(RIVAL_X), crewY - 26.f, 0.42f, PAL_AMBER);

    for (const Puff& s : smokes_) {
        if (s.life <= 0.f) continue;
        float rise = (1.f - s.life) * 16.f;
        spr(art_.smoke, sx(s.x) + std::sin(t_ + s.y) * 3.f, sy(s.y) - rise, 6.f + (1.f - s.life) * 8.f, PAL_FX);
    }
    float bob = std::sin(t_ * (phase_ == Phase::Lift ? 6.5f : 2.1f)) * (phase_ == Phase::Lift ? 1.6f : 0.45f);
    drawBoat(x_, y_, hdg_, PAL_TUG, bob);
    drawBoat(RIVAL_X, RIVAL_Y, 0.04f, PAL_RIVAL, std::sin(t_ * 1.7f + 1.2f) * 0.4f);
    drawGates();
    drawWorld();

    char buf[64];
    int sec = std::max(0, int(std::ceil(crewLeft() - 1e-3f)));
    std::snprintf(buf, sizeof buf, "CREW %d:%02d", sec / 60, sec % 60);
    const int crewPal = sec <= 15 ? PAL_RED : PAL_AMBER;

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, "S3 TUGBOAT LOCK", PAL_AMBER);
        hudR(0, buf, crewPal);
        int hpal = PAL_HUD;
        if (phase_ == Phase::Wait && std::fabs(speed_) > 0.9f && (GATE_LO - bowY()) < 12.f) hpal = PAL_RED;
        hud(1, 1, hint(), hpal);
        const char* side = "LINED UP";
        char sideBuf[24];
        if (!(std::fabs(x_) < 0.30f && std::fabs(hdg_) < 0.20f)) {
            std::snprintf(sideBuf, sizeof sideBuf, "%s %.1f", x_ >= 0.f ? "STBD" : "PORT", std::fabs(x_));
            side = sideBuf;
        }
        char line[48];
        std::snprintf(line, sizeof line, "%s  %s", side, gateLabel());
        hud(1, 2, line, PAL_GREEN);
        if (speed_ < -0.08f) std::snprintf(buf, sizeof buf, "ASTERN %.1f", std::fabs(speed_));
        else if (speed_ < 0.08f) std::snprintf(buf, sizeof buf, "STOPPED");
        else std::snprintf(buf, sizeof buf, "AHEAD %.1f", speed_);
        hudR(2, buf, PAL_HUD);
        hud(1, 26, "ARROWS DRIVE   Z CALLS THE LOCK", PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hudC(8, "TAKE THE TUG THROUGH THE LOCK", PAL_AMBER);
        hudC(9, "PASS IT WITHOUT SCRAPING A GATE", PAL_HUD);
        hudC(10, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        std::snprintf(buf, sizeof buf, "THEIR CLOCK %d:%02d", sec / 60, sec % 60);
        hudC(12, buf, PAL_AMBER);
        hudC(14, int(t_ * 2.f) % 2 == 0 ? "ENTER TO TAKE THE TUG" : "ARROWS STEER AND DRIVE", PAL_HUD);
        hudC(15, "Z CALLS THE KEEPER", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(9, "PASSED THE LOCK", PAL_GREEN);
        hudC(10, "NOT A GATE WAS SCRAPED", PAL_HUD);
        int took = int(std::lround(playT_));
        int left = int(std::floor(crewLeft()));
        std::snprintf(buf, sizeof buf, "%d:%02d  CREW HAD %d:%02d", took / 60, took % 60, left / 60, left % 60);
        hudC(12, buf, PAL_AMBER);
        hudC(14, "ENTER TAKES THE NEXT LOCK", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(10, why_ && why_[0] ? why_ : "SCRAPED A GATE", PAL_RED);
        hudC(12, "ENTER TRIES THE LOCK AGAIN", PAL_HUD);
    }
    hudR(27, S3_VERSION_STRING, PAL_HUD);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.14f, 0.20f, 0.12f);
    t_ = 0.f;
    if (bot_) begin();
    else showTitle();
    sys.setLight(30, 90, 110);
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

}  // namespace tuglock
