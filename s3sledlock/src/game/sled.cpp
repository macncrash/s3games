#include "sled.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace sledlock {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr float TAU = 6.2831853f;
constexpr float GATE_LO = 24.f;
constexpr float GATE_HI = 50.f;
constexpr float LOCK_HALF = 2.55f;
constexpr float CANAL_HALF = 6.6f;
constexpr float MOUTH = 7.2f;
constexpr float MEET = 0.72f;
constexpr float HALF_L = 1.50f;
constexpr float HALF_B = 0.42f;
constexpr float HIT_R = 0.20f;
constexpr float HOLD_LO = GATE_LO - 9.0f;
constexpr float HOLD_HI = GATE_HI - 8.2f;
constexpr float PLAY_ZOOM = 15.5f;
constexpr float TITLE_ZOOM = 8.6f;
constexpr float CAM_REF = 132.f;

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

float Game::slide() const { return std::cos(hdg_) * speed_ + fall_; }
float Game::bowY() const { return y_ + std::cos(hdg_) * HALF_L; }
float Game::sternY() const { return y_ - std::cos(hdg_) * HALF_L; }

float Game::halfAt(float y) const {
    auto smooth = [](float u) {
        u = clampf(u, 0.f, 1.f);
        return u * u * (3.f - 2.f * u);
    };
    if (y < GATE_LO - MOUTH) return CANAL_HALF;
    if (y < GATE_LO - 0.35f) {
        float u = (y - (GATE_LO - MOUTH)) / (MOUTH - 0.35f);
        return CANAL_HALF + (LOCK_HALF - CANAL_HALF) * smooth(u);
    }
    if (y <= GATE_HI + 0.35f) return LOCK_HALF;
    if (y < GATE_HI + MOUTH) {
        float u = (y - (GATE_HI + 0.35f)) / (MOUTH - 0.35f);
        return LOCK_HALF + (CANAL_HALF - LOCK_HALF) * smooth(u);
    }
    return CANAL_HALF;
}

float Game::slopeAt() const {
    // The cut falls toward the lower gate, flattens in the pound, then falls away above it.
    if (y_ < GATE_LO - 0.8f) return 0.55f;
    if (y_ > GATE_HI + 0.6f) return 0.42f;
    if (phase_ == Phase::Rise) return 0.04f * (1.f - fill_);
    return 0.08f;
}

float Game::sx(float wx) const { return 160.f + (wx - camX_) * zoom_ + shx_; }
float Game::sy(float wy) const { return CAM_REF - (wy - camY_) * zoom_ + shy_; }
float Game::viewTop() const { return camY_ + (CAM_REF + 10.f) / zoom_; }
float Game::viewBot() const { return camY_ + (CAM_REF - gs::SCREEN_H - 10.f) / zoom_; }

void Game::leafGeom(int which, float open, float& hx, float& hy, float& tx, float& ty) const {
    // Closed leaf meets upstream of the hinge. Open leaf lies along the wall:
    // lower tips downstream, upper tips upstream, clear of a sled that holds back.
    const bool upper = which >= 2;
    const bool right = (which & 1) != 0;
    const float gy = upper ? GATE_HI : GATE_LO;
    const float sign = right ? 1.f : -1.f;
    const float len = std::sqrt(LOCK_HALF * LOCK_HALF + MEET * MEET);
    hx = sign * LOCK_HALF;
    hy = gy;
    const float cang = std::atan2(MEET, -sign * LOCK_HALF);
    const float oY = gy + (upper ? len : -len);
    const float oX = sign * (LOCK_HALF - 0.15f);
    const float oang = std::atan2(oY - hy, oX - hx);
    float d = wrapPi(oang - cang);
    const float ang = cang + d * clampf(open, 0.f, 1.f);
    tx = hx + std::cos(ang) * len;
    ty = hy + std::sin(ang) * len;
}

bool Game::throatClear(bool upper) const {
    const float open = upper ? openHi_ : openLo_;
    if (open < 0.92f) return false;
    for (int side = 0; side < 2; side++) {
        float hx, hy, tx, ty;
        leafGeom(upper ? 2 + side : side, open, hx, hy, tx, ty);
        if (std::fabs(tx) < LOCK_HALF - 0.55f) return false;
        for (int i = 0; i <= 8; i++) {
            float u = float(i) / 8.f;
            float px = hx + (tx - hx) * u;
            if (std::fabs(px) > LOCK_HALF - 0.35f) continue;
            if (std::fabs(px) < 1.20f) return false;
        }
    }
    return true;
}

bool Game::inChamber() const { return sternY() > GATE_LO + 1.35f && bowY() < GATE_HI - 2.2f; }

bool Game::lined() const { return std::fabs(x_) < 0.42f && std::fabs(hdg_) < 0.24f && std::fabs(lat_) < 0.50f; }

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
    if (phase_ == Phase::Rise) return 3;
    if (phase_ == Phase::Out) return 4;
    if (phase_ == Phase::Throat || phase_ == Phase::Shut) return 2;
    return 1;
}

void Game::seedFlakes() {
    for (int i = 0; i < 18; i++) {
        flakes_[i].x = float((i * 47) % 320);
        flakes_[i].y = float((i * 83) % 224);
        flakes_[i].sp = 16.f + float(i % 5) * 8.f;
        flakes_[i].sc = 2.2f + float(i % 3);
    }
}

void Game::resetPose() {
    phase_ = Phase::Glide;
    over_ = false;
    won_ = false;
    why_ = "";
    commitLo_ = false;
    commitHi_ = false;
    playT_ = 0;
    x_ = -0.95f;
    y_ = 6.8f;
    hdg_ = -0.20f;
    speed_ = 0.15f;
    fall_ = 1.15f;
    lat_ = 0.f;
    yawV_ = 0;
    kick_ = brake_ = lean_ = 0;
    openLo_ = openHi_ = 0;
    tgtLo_ = tgtHi_ = 0;
    fill_ = 0;
    dwell_ = 0;
    shake_ = 0;
    puffT_ = 0;
    ayeT_ = 0;
    chimeStep_ = -1;
    puffN_ = 0;
    deny_ = false;
    for (Puff& p : puffs_) p = {};
}

void Game::snapCamera() {
    if (mode_ == Mode::Title) {
        camX_ = 0.1f;
        camY_ = 15.5f;
        zoom_ = TITLE_ZOOM;
    } else if (mode_ == Mode::Win) {
        camX_ = x_ * 0.3f;
        camY_ = y_ + 0.2f;
        zoom_ = 10.f;
    } else if (mode_ == Mode::Fail) {
        camX_ = x_ * 0.4f;
        camY_ = y_ + 0.4f;
        zoom_ = 12.f;
    } else {
        camX_ = x_ * 0.45f;
        camY_ = y_ + 0.55f;
        zoom_ = PLAY_ZOOM;
    }
}

void Game::followCamera() {
    float tx = x_ * 0.45f, ty = y_ + 0.55f, tz = PLAY_ZOOM;
    if (mode_ == Mode::Title) {
        tx = 0.1f;
        ty = 15.5f;
        tz = TITLE_ZOOM;
    } else if (mode_ == Mode::Win) {
        tx = x_ * 0.25f;
        ty = y_ + 0.1f;
        tz = 10.2f;
    } else if (mode_ == Mode::Fail) {
        tx = x_ * 0.35f;
        ty = y_ + 0.3f;
        tz = 12.f;
    }
    float k = 1.f - std::exp(-DT * 5.5f);
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
    sys_->apu.noiseBurst(0.5f, 700.f, 0.22f);
    sys_->apu.tone(1, 98.f, 0.06f);
    sys_->rumble(0.75f, 0.35f, 200);
    sys_->setLight(170, 30, 24);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "clear";
    chime(true);
    sys_->rumble(0.28f, 0.12f, 160);
    sys_->setLight(40, 140, 90);
}

void Game::chime(bool big) {
    chimeBig_ = big;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::pilot(float& kick, float& brake, float& lean, bool& hail) const {
    const float mid = (GATE_LO + GATE_HI) * 0.5f;
    const float bow = bowY();
    const float along = slide();
    float ty = mid;
    float cap = 1.4f;
    bool waiting = false;
    if (phase_ == Phase::Glide || (phase_ == Phase::Throat && !commitLo_)) {
        waiting = true;
        ty = HOLD_LO;
        cap = 1.5f;
    } else if (phase_ == Phase::Rise) {
        waiting = true;
        ty = mid;
        cap = 0.4f;
    } else if (phase_ == Phase::Out && !commitHi_) {
        waiting = true;
        ty = HOLD_HI;
        cap = 1.2f;
    } else if (phase_ == Phase::Out) {
        ty = GATE_HI + 16.f;
        cap = 1.85f;
    } else {
        ty = mid;
        cap = 1.45f;
    }

    const bool lowerShut = phase_ != Phase::Out && openLo_ < 0.85f && !commitLo_;
    const bool upperShut = phase_ == Phase::Out && !commitHi_ && openHi_ < 0.85f;
    const float gateY = upperShut ? GATE_HI : GATE_LO;
    const float gap = gateY - bow;
    const bool gateShut = lowerShut || upperShut;

    float want = 0.f;
    float aim = 0.f;
    if (waiting && gateShut) {
        const bool crooked = std::fabs(x_) > 0.28f || std::fabs(hdg_) > 0.14f;
        if (crooked && gap < 7.2f) {
            aim = clampf(-x_ * 0.12f, -0.1f, 0.1f);
            want = std::fabs(hdg_) < 0.22f ? -0.55f : 0.f;
        } else if (crooked) {
            float look = std::max(2.2f, ty - y_);
            aim = clampf(std::atan2(-x_ - lat_ * 0.4f, look), -0.7f, 0.7f);
            want = 0.55f;
            if (gap < 9.f) want = std::min(want, std::max(0.15f, (gap - 7.2f) * 0.35f));
        } else {
            aim = clampf(-x_ * 1.15f - lat_ * 0.55f, -0.2f, 0.2f);
            float room = ty - y_;
            if (room > 0.45f) want = std::min(1.15f, room * 0.4f);
            else if (room < -0.45f) want = -0.35f;
            else want = 0.f;
        }
    } else {
        float room = ty - y_;
        aim = clampf(std::atan2(-x_ - lat_ * 0.35f, std::max(1.8f, room)), -0.4f, 0.4f);
        if (room > 0.35f) want = std::min(cap, std::max(0.35f, room * 0.48f));
        else want = cap;
        if (!lined()) want = std::min(want, 0.6f);
    }
    if (gateShut && gap < 5.2f && want > 0.f) want = std::min(want, std::max(0.f, (gap - 3.6f) * 0.4f));

    float err = want - along;
    kick = 0.f;
    brake = 0.f;
    if (want < -0.05f) kick = clampf(err * 1.3f, -1.f, 0.f);
    else if (err > 0.06f) kick = clampf(err * 0.85f, 0.f, 1.f);
    else if (err < -0.05f) brake = clampf(-err, 0.f, 1.f);
    if (std::fabs(want) < 0.05f && along > 0.06f) brake = std::max(brake, 0.85f);

    lean = clampf(wrapPi(aim - hdg_) * 3.8f - yawV_ * 1.05f, -1.f, 1.f);
    hail = phase_ == Phase::Glide && std::fabs(x_) < 0.4f && std::fabs(hdg_) < 0.18f && std::fabs(lat_) < 0.35f &&
           along < 0.45f && along > -0.2f && gap > 4.6f && gap < 12.f;
}

void Game::controls() {
    const gs::Pad& p = sys_->pad;
    float lean = 0.f;
    if (p.down(gs::BTN_LEFT)) lean -= 1.f;
    if (p.down(gs::BTN_RIGHT)) lean += 1.f;
    if (std::fabs(p.axisX) > 0.12f) lean = clampf(p.axisX, -1.f, 1.f);
    lean_ = lean;

    float kick = 0.f, brake = 0.f;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_C)) kick = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) brake = 1.f;
    if (p.accel > 0.08f) kick = std::max(kick, p.accel);
    if (p.brake > 0.08f) brake = std::max(brake, p.brake);
    if (brake > 0.2f && kick > 0.2f) kick = 0.f;
    kick_ = kick;
    brake_ = brake;
}

void Game::integrate(float dt) {
    float spd = std::min(3.f, std::fabs(speed_) + std::fabs(fall_));
    yawV_ += lean_ * (1.05f + spd * 0.38f) * dt;
    yawV_ -= yawV_ * 2.5f * dt;
    yawV_ = clampf(yawV_, -1.8f, 1.8f);
    hdg_ = wrapPi(hdg_ + yawV_ * dt);

    if (kick_ > 0.f) speed_ += kick_ * 2.05f * dt;
    else if (kick_ < 0.f) fall_ += kick_ * 2.4f * dt;
    speed_ -= speed_ * 0.65f * dt;
    // A heel kills the slide. It does not pole the nose backward — that crabs on ice.
    if (brake_ > 0.f) {
        float ds = brake_ * 3.4f * dt;
        if (speed_ > 0.f) speed_ = std::max(0.f, speed_ - ds);
        else speed_ = std::min(0.f, speed_ + ds * 0.4f);
    }
    speed_ = clampf(speed_, -0.2f, 3.1f);

    fall_ += slopeAt() * dt;
    fall_ -= fall_ * 0.16f * dt;
    if (brake_ > 0.f) {
        float df = brake_ * 2.0f * dt;
        if (fall_ > 0.f) fall_ = std::max(0.f, fall_ - df);
        else fall_ = std::min(0.f, fall_ + df * 0.4f);
        // Held heel, already stopped: inch back up the cut.
        if (brake_ > 0.92f && speed_ < 0.06f && fall_ < 0.06f) fall_ = std::max(-0.5f, fall_ - 0.65f * dt);
    }
    fall_ = clampf(fall_, -0.5f, 3.0f);

    lat_ += lean_ * (0.7f + std::fabs(speed_ + fall_) * 0.15f) * dt;
    if (brake_ > 0.25f) lat_ -= lat_ * brake_ * 4.2f * dt;
    lat_ -= lat_ * 2.4f * dt;
    lat_ = clampf(lat_, -1.7f, 1.7f);

    if (phase_ == Phase::Rise) {
        float s = std::sin(clampf(fill_, 0.f, 1.f) * PI);
        lat_ += std::sin(t_ * 2.15f) * s * 3.4f * dt;
    }

    float vx = std::sin(hdg_) * speed_ + lat_;
    float vy = std::cos(hdg_) * speed_ + fall_;
    float sp = std::sqrt(vx * vx + vy * vy);
    if (sp > 4.3f) {
        float k = 4.3f / sp;
        speed_ *= k;
        fall_ *= k;
        lat_ *= k;
        vx *= k;
        vy *= k;
    }
    x_ += vx * dt;
    y_ += vy * dt;
    if (y_ < 2.4f) {
        y_ = 2.4f;
        if (fall_ < 0.f) fall_ = 0.f;
        if (speed_ < 0.f) speed_ = 0.f;
    }
    if (y_ > 76.f) {
        y_ = 76.f;
        fall_ = std::min(fall_, 0.f);
        if (speed_ > 0.f) speed_ *= 0.2f;
    }
}

void Game::confine() {
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    const float along[3] = {0.92f, 0.f, -0.92f};
    const float side[2] = {-0.92f, 0.92f};
    for (int pass = 0; pass < 2; pass++) {
        float pushR = 0.f, pushL = 0.f;
        for (float a : along) {
            for (float b : side) {
                float wx = x_ + fx * a * HALF_L + rx * b * HALF_B;
                float lim = halfAt(y_ + fy * a * HALF_L + ry * b * HALF_B) - 0.12f;
                pushR = std::max(pushR, wx - lim);
                pushL = std::max(pushL, -lim - wx);
            }
        }
        if (pushR <= 0.f && pushL <= 0.f) break;
        if (pushR > pushL) {
            x_ -= pushR;
            if (lat_ > 0.f) lat_ *= 0.35f;
        } else {
            x_ += pushL;
            if (lat_ < 0.f) lat_ *= 0.35f;
        }
        if (std::max(pushR, pushL) > 0.04f) yawV_ *= 0.65f;
    }
}

const char* Game::gateHit() const {
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    const float along[5] = {0.96f, 0.45f, 0.f, -0.48f, -0.96f};
    const float side[3] = {-0.96f, 0.f, 0.96f};
    for (int which = 0; which < 4; which++) {
        const float open = which < 2 ? openLo_ : openHi_;
        float hx, hy, tx, ty;
        leafGeom(which, open, hx, hy, tx, ty);
        float dx = tx - hx, dy = ty - hy;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.5f) {
            float cut = 0.42f / len;
            hx += dx * cut;
            hy += dy * cut;
        }
        for (float a : along) {
            for (float b : side) {
                float wx = x_ + fx * a * HALF_L * 0.90f + rx * b * HALF_B * 0.90f;
                float wy = y_ + fy * a * HALF_L * 0.90f + ry * b * HALF_B * 0.90f;
                if (segDist(wx, wy, hx, hy, tx, ty) < HIT_R)
                    return which < 2 ? "scraped the lower gate" : "scraped the upper gate";
            }
        }
    }
    return nullptr;
}

void Game::step() {
    playT_ += DT;
    if (playT_ > 96.f) {
        fail("timed out");
        return;
    }

    bool hail = false;
    bool hailed = false;
    if (bot_) {
        pilot(kick_, brake_, lean_, hail);
    } else {
        controls();
        const gs::Pad& pad = sys_->pad;
        hailed = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z);
        hail = pad.down(gs::BTN_A) || pad.down(gs::BTN_Z);
    }

    integrate(DT * 0.5f);
    confine();
    integrate(DT * 0.5f);
    confine();

    auto ease = [&](float& o, float target) {
        const float rate = (target > o ? 0.46f : 0.62f) * DT;
        if (o < target) o = std::min(target, o + rate);
        else o = std::max(target, o - rate);
    };
    ease(openLo_, tgtLo_);
    ease(openHi_, tgtHi_);

    if (const char* hit = gateHit()) {
        fail(hit);
        return;
    }

    const float bow = bowY();
    const float along = slide();
    const bool pointed = std::fabs(hdg_) < 0.6f;
    if (phase_ == Phase::Glide) {
        const bool inRange = bow < GATE_LO - 4.2f && bow > GATE_LO - 13.f && std::fabs(x_) < 0.48f;
        const bool squared = std::fabs(hdg_) < 0.22f && std::fabs(lat_) < 0.4f;
        const bool eased = inRange && squared && along < 0.55f && along > -0.2f && pointed;
        const bool called = hail && inRange && squared && along < 0.9f;
        if (eased || called) {
            phase_ = Phase::Throat;
            tgtLo_ = 1.f;
            dwell_ = 0.f;
            commitLo_ = false;
            ayeT_ = 1.5f;
            chime(false);
        } else if (hailed) {
            deny_ = true;
        }
    } else if (phase_ == Phase::Throat) {
        if (!commitLo_ && throatClear(false) && lined() && along < 1.35f && along > -0.15f) commitLo_ = true;
        const bool outside = bow < GATE_LO - 0.2f;
        const bool hesitating = !commitLo_ && outside && openLo_ > 0.96f && along < 0.4f && bow < GATE_LO - 4.2f;
        if (hesitating) dwell_ += DT;
        else dwell_ = std::max(0.f, dwell_ - DT * 0.5f);
        if (dwell_ > 8.f) tgtLo_ = 0.f;
        if (tgtLo_ < 0.5f && openLo_ < 0.05f && bow < GATE_LO - 0.6f) {
            phase_ = Phase::Glide;
            commitLo_ = false;
            dwell_ = 0.f;
        }
        if (inChamber()) {
            phase_ = Phase::Shut;
            tgtLo_ = 0.f;
            dwell_ = 0.f;
        }
    } else if (phase_ == Phase::Shut) {
        tgtLo_ = 0.f;
        if (openLo_ < 0.04f) {
            phase_ = Phase::Rise;
            fill_ = 0.f;
            chime(false);
        }
    } else if (phase_ == Phase::Rise) {
        fill_ = std::min(1.f, fill_ + DT / 4.0f);
        if (fill_ >= 1.f) {
            phase_ = Phase::Out;
            tgtHi_ = 1.f;
            commitHi_ = false;
            ayeT_ = 1.3f;
            chime(false);
        }
    } else if (phase_ == Phase::Out) {
        if (!commitHi_ && throatClear(true) && lined() && bowY() < GATE_HI - 3.6f && along < 1.4f) commitHi_ = true;
        if (sternY() > GATE_HI + 4.0f && openHi_ > 0.75f) {
            win();
            return;
        }
    }

    if (ayeT_ > 0.f) ayeT_ -= DT;
    puffs(DT);
    if (brake_ > 0.65f && along > 1.6f) shake_ = std::max(shake_, 0.18f);
}

void Game::puffs(float dt) {
    float g = std::fabs(slide());
    if (g > 0.75f || brake_ > 0.45f) {
        puffT_ -= dt;
        if (puffT_ <= 0.f) {
            puffT_ = 0.045f;
            Puff& p = puffs_[puffN_++ % 16];
            p.x = x_ - std::sin(hdg_) * HALF_L * 0.85f;
            p.y = y_ - std::cos(hdg_) * HALF_L * 0.85f;
            p.vx = lat_ * 0.15f;
            p.vy = -0.12f;
            p.life = 1.f;
            p.sc = 0.32f + g * 0.08f + brake_ * 0.12f;
        }
    }
    for (Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.life -= dt * 0.85f;
    }
}

void Game::flakes(float dt) {
    for (Flake& f : flakes_) {
        f.y += f.sp * dt;
        f.x += std::sin(t_ * 0.7f + f.y * 0.02f) * 10.f * dt;
        if (f.y > 230.f) f.y = -6.f;
        if (f.x < -6.f) f.x += 332.f;
        if (f.x > 326.f) f.x -= 332.f;
    }
}

const char* Game::hint() const {
    if (mode_ == Mode::Win) return "THE SLED IS THROUGH";
    if (mode_ == Mode::Fail) return why_ && why_[0] ? why_ : "SCRAPED A GATE";
    if (mode_ == Mode::Pause) return "PAUSED";
    if (mode_ == Mode::Title) return "ENTER TO SHOVE OFF";
    const float gap = GATE_LO - bowY();
    switch (phase_) {
    case Phase::Glide:
        if (gap < 12.f && slide() > 1.6f) return "TOO FAST. DIG A HEEL";
        if (gap < 14.f) return "HAIL THE KEEPER";
        return "ICE FALLS TO THE GATE. EASE UP";
    case Phase::Throat:
        if (tgtLo_ < 0.5f) return "THE LOWER GATE IS CLOSING";
        if (!throatClear(false)) return "HOLD. THE LEAVES ARE SWINGING";
        return "KICK THROUGH. DON'T CLIP A LEAF";
    case Phase::Shut: return "LET THE LOWER GATE SHUT";
    case Phase::Rise: return "HOLD THE MIDDLE. ICE IS HEAVING";
    case Phase::Out:
        if (!throatClear(true)) return "WAIT FOR THE UPPER GATE";
        return "CLEAR THE UPPER LEAVES";
    }
    return "";
}

const char* Game::gateLabel() const {
    switch (phase_) {
    case Phase::Glide: return "LOWER SHUT";
    case Phase::Throat:
        if (tgtLo_ < 0.5f) return "LOWER CLOSING";
        return throatClear(false) ? "LOWER CLEAR" : "LOWER OPENING";
    case Phase::Shut: return "LOWER SHUTTING";
    case Phase::Rise: return "POUND RISING";
    case Phase::Out:
        if (throatClear(true)) return "UPPER CLEAR";
        return openHi_ > 0.02f ? "UPPER OPENING" : "UPPER SHUT";
    }
    return "";
}

void Game::audio() {
    if (deny_) {
        deny_ = false;
        sys_->apu.tone(1, 140.f, 0.04f);
    }
    if (chimeStep_ >= 0) {
        chimeT_ -= DT;
        if (chimeT_ <= 0.f) {
            static const float winN[] = {523.25f, 659.25f, 783.99f, 1046.5f};
            static const float gateN[] = {392.f, 494.f, 587.33f};
            const float* notes = chimeBig_ ? winN : gateN;
            const int count = chimeBig_ ? 4 : 3;
            if (chimeStep_ >= count) {
                sys_->apu.tone(0, 0, 0);
                chimeStep_ = -1;
            } else {
                sys_->apu.tone(0, notes[chimeStep_], chimeBig_ ? 0.07f : 0.045f);
                chimeStep_++;
                chimeT_ = chimeBig_ ? 0.16f : 0.13f;
            }
        }
    } else if (mode_ != Mode::Play) {
        sys_->apu.tone(0, 0, 0);
    }

    if (mode_ == Mode::Play) {
        float g = std::fabs(slide());
        float scrape = 0.012f + g * 0.011f + brake_ * 0.02f + std::fabs(lat_) * 0.012f;
        float rate = 280.f + g * 520.f + brake_ * 400.f;
        sys_->apu.noise(scrape, rate, false);
        if (kick_ > 0.4f) sys_->apu.tone(2, 180.f + kick_ * 40.f, 0.018f);
        else sys_->apu.tone(2, 0, 0);
    } else {
        sys_->apu.noise(0.01f, 180.f, false);
        sys_->apu.tone(2, 0, 0);
    }
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
    if (cx + w * 0.5f < -12.f || cx - w * 0.5f > gs::SCREEN_W + 12.f) return;
    if (cy + h * 0.5f < -12.f || cy - h * 0.5f > gs::SCREEN_H + 12.f) return;
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
    float scale = pix / float(LEAF_PX);
    spr(m, (xA + xB) * 0.5f, (yA + yB) * 0.5f, float(m.h) * scale, PAL_GATE);
}

void Game::drawGates() {
    for (int which = 0; which < 4; which++) {
        const float open = which < 2 ? openLo_ : openHi_;
        float hx, hy, tx, ty;
        leafGeom(which, open, hx, hy, tx, ty);
        drawLeaf(hx, hy, tx, ty);
        place(art_.post, hx, hy, 1.15f, PAL_GATE);
    }
}

void Game::drawSled() {
    const gs::Mipped& hull = art_.sled[yawFrame()];
    float h = float(hull.h) * (2.f * HALF_L * zoom_ / float(SLED_PX));
    float cx = sx(x_) + lean_ * 4.5f;
    float cy = sy(y_);
    spr(art_.blob, cx, cy + h * 0.06f, h * 0.22f, PAL_FX, true);
    spr(hull, cx, cy, h, PAL_SLED);
    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        float z = p.sc * (0.45f + (1.f - p.life) * 0.8f) * zoom_;
        spr(art_.spray, sx(p.x), sy(p.y), z, PAL_FX);
    }
}

void Game::drawWorld() {
    const float top = viewTop();
    const float bot = viewBot();
    const float hareX = 9.1f;
    const float hareY = 12.4f + std::sin(t_ * 1.6f) * 1.4f;
    place(art_.hare, hareX, hareY + std::fabs(std::sin(t_ * 6.f)) * 0.12f, 0.85f, PAL_HARE, std::sin(t_ * 1.6f) > 0.f);

    place(art_.keeper, -(LOCK_HALF + 1.25f), GATE_LO + 0.4f, 1.85f, PAL_KEEP);
    place(art_.sign, -8.6f, HOLD_LO - 0.4f, 2.05f, PAL_CABIN);
    place(art_.cabin, 8.5f, 37.2f, 3.5f, PAL_CABIN);
    place(art_.ladder, LOCK_HALF + 0.15f, 36.5f, 2.4f, PAL_STONE);

    const float pines[][2] = {{-9.4f, 6.f}, {9.6f, 11.f}, {-10.2f, 20.f}, {10.f, 28.f},
                               {-9.6f, 42.f}, {9.8f, 48.f}, {-10.f, 60.f}, {9.4f, 66.f}};
    for (auto& tr : pines) {
        if (tr[1] < bot - 3.f || tr[1] > top + 3.f) continue;
        place(art_.pine, tr[0], tr[1], 4.6f, PAL_SNOW);
    }

    float pulse = 1.f + 0.08f * std::sin(t_ * 4.f);
    const float lamps[][2] = {{-(LOCK_HALF + 0.7f), GATE_LO}, {(LOCK_HALF + 0.7f), GATE_LO},
                               {-(LOCK_HALF + 0.7f), GATE_HI}, {(LOCK_HALF + 0.7f), GATE_HI},
                               {-3.2f, HOLD_LO},      {3.2f, HOLD_LO}};
    for (auto& lp : lamps) {
        if (lp[1] < bot - 1.f || lp[1] > top + 1.f) continue;
        place(art_.lamp, lp[0], lp[1], 1.55f * pulse, PAL_LAMP);
    }

    if (phase_ == Phase::Rise || (phase_ == Phase::Out && openHi_ < 0.4f)) {
        for (int i = 0; i < 5; i++) {
            float wy = GATE_LO + 3.f + float(i) * 4.2f;
            float wob = std::sin(t_ * 3.f + float(i)) * 0.12f;
            spr(art_.spray, sx(-(LOCK_HALF - 0.45f) + wob), sy(wy), 8.f + float(i % 2), PAL_FX);
            spr(art_.spray, sx((LOCK_HALF - 0.45f) - wob), sy(wy + 0.6f), 7.f, PAL_FX);
        }
    }
    const float cracks[][2] = {{-0.8f, 18.f}, {1.1f, 30.f}, {-0.4f, 40.f}, {0.7f, 47.f}, {-1.2f, 56.f}};
    for (auto& cr : cracks) {
        if (cr[1] < bot || cr[1] > top) continue;
        float vis = 1.f;
        if (cr[1] > GATE_LO && cr[1] < GATE_HI) vis = 0.35f + fill_ * 0.65f;
        if (vis < 0.4f && phase_ != Phase::Rise && phase_ != Phase::Out) continue;
        place(art_.crack, cr[0], cr[1], 2.2f, PAL_FX);
    }

    const float stoneH = 1.25f;
    const float stoneW = stoneH * float(art_.stone.w) / float(std::max(art_.stone.h, 1));
    for (float wy = std::floor(std::max(GATE_LO - 8.f, bot) / 1.15f) * 1.15f; wy < std::min(GATE_HI + 8.f, top);
         wy += 1.15f) {
        float edge = halfAt(wy);
        place(art_.stone, -edge - stoneW * 0.5f + 0.04f, wy, stoneH, PAL_STONE, false);
        place(art_.stone, edge + stoneW * 0.5f - 0.04f, wy, stoneH, PAL_STONE, true);
    }

    const float snowH = 3.5f;
    const float snowW = snowH * float(art_.snow.w) / float(std::max(art_.snow.h, 1));
    for (float wy = std::floor(bot / 3.3f) * 3.3f; wy < top; wy += 3.3f) {
        float edge = halfAt(wy);
        float inner = edge + stoneW - 0.25f;
        for (int i = 0; i < 4; i++) {
            float shift = inner + snowW * 0.5f + float(i) * (snowW - 0.45f);
            place(art_.snow, -shift, wy, snowH, PAL_SNOW);
            place(art_.snow, shift, wy, snowH, PAL_SNOW, true);
        }
        if (wy < GATE_LO - 6.f || wy > GATE_HI + 6.f) {
            float sway = std::sin(t_ * 0.8f + wy) * 0.1f;
            place(art_.drift, -edge - 0.2f + sway, wy, 0.9f, PAL_SNOW);
            place(art_.drift, edge + 0.2f - sway, wy, 0.9f, PAL_SNOW, true);
        }
    }

    float smoke = std::fmod(t_ * 0.22f, 1.f);
    spr(art_.spray, sx(8.9f + smoke * 0.4f), sy(39.6f + smoke * 1.6f), 7.f * (1.f - smoke * 0.4f), PAL_FX);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(gs::rgb4(2, 4, 6));
    const uint16_t deep = gs::rgb4(2, 6, 9);
    const uint16_t deepHi = gs::rgb4(4, 8, 11);
    const uint16_t high = gs::rgb4(7, 12, 13);
    const uint16_t highHi = gs::rgb4(10, 14, 15);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (CAM_REF - float(y)) / zoom_;
        float grain = std::fmod(wy * 0.52f, 1.f);
        if (grain < 0.f) grain += 1.f;
        const bool stripe = grain < 0.11f;
        float k = 0.f;
        if (wy >= GATE_HI) k = 1.f;
        else if (wy >= GATE_LO && mode_ != Mode::Title) k = fill_;
        uint16_t lo = stripe ? deepHi : deep;
        uint16_t hi = stripe ? highHi : high;
        v.lineBackdrop[y] = lerpColor(lo, hi, k);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    shx_ = shy_ = 0.f;
    if (shake_ > 0.f) {
        shx_ = std::sin(t_ * 90.f) * 3.0f * shake_;
        shy_ = std::cos(t_ * 70.f) * 2.0f * shake_;
        shake_ = std::max(0.f, shake_ - DT * 1.5f);
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    backdrop();

    if (mode_ == Mode::Title) text("S3 SLED LOCK", 160.f, 24.f, 0.78f, PAL_AMBER);
    else if (mode_ == Mode::Win) text("CLEAR", 160.f, 36.f, 1.15f, PAL_GREEN);
    else if (mode_ == Mode::Fail) text("SCRAPED", 160.f, 36.f, 1.0f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f, 100.f, 1.f, PAL_HUD);

    if (ayeT_ > 0.f) {
        float kx = sx(-(LOCK_HALF + 1.25f));
        float ky = sy(GATE_LO + 0.4f) - 22.f;
        text("AYE", kx, ky, 0.5f, PAL_AMBER);
    }

    for (const Flake& f : flakes_) spr(art_.flake, f.x, f.y, f.sc, PAL_FX);

    drawSled();
    drawGates();
    drawWorld();

    char buf[64];
    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, "S3 SLED LOCK", PAL_AMBER);
        float g = slide();
        if (g < -0.08f) std::snprintf(buf, sizeof buf, "BACK %.1f", std::fabs(g));
        else std::snprintf(buf, sizeof buf, "%.1f M/S", std::fabs(g));
        hud(40 - int(std::strlen(buf)), 0, buf, PAL_HUD);
        int hpal = (phase_ == Phase::Glide && slide() > 1.6f && (GATE_LO - bowY()) < 12.f) ? PAL_RED : PAL_HUD;
        hud(1, 1, hint(), hpal);
        const char* side = "LINED UP";
        char sideBuf[24];
        if (!(std::fabs(x_) < 0.28f && std::fabs(hdg_) < 0.2f)) {
            std::snprintf(sideBuf, sizeof sideBuf, "%s %.1f", x_ >= 0.f ? "RIGHT" : "LEFT", std::fabs(x_));
            side = sideBuf;
        }
        std::snprintf(buf, sizeof buf, "%s  %s", side, gateLabel());
        hud(1, 2, buf, PAL_GREEN);
        if (phase_ == Phase::Rise) {
            int blocks = int(std::lround(fill_ * 10.f));
            char bar[16];
            int n = 0;
            bar[n++] = 'I';
            bar[n++] = 'C';
            bar[n++] = 'E';
            bar[n++] = ' ';
            for (int i = 0; i < 10; i++) bar[n++] = i < blocks ? '#' : '-';
            bar[n] = 0;
            hud(1, 3, bar, PAL_HUD);
        }
        hud(1, 26, "ARROWS  UP KICK  DOWN HEEL  Z HAIL", PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hudC(8, "THE SLED HAS ONE JOB", PAL_AMBER);
        hudC(9, "PASS THE LOCK", PAL_HUD);
        hudC(10, "DON'T SCRAPE A GATE", PAL_HUD);
        hudC(12, int(t_ * 2.f) % 2 == 0 ? "ENTER TO SHOVE OFF" : "UP KICKS   DOWN DIGS A HEEL", PAL_HUD);
        hudC(13, "Z HAILS THE KEEPER", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(10, "PASSED THE LOCK", PAL_GREEN);
        hudC(11, "NOT A GATE WAS SCRAPED", PAL_HUD);
        std::snprintf(buf, sizeof buf, "%.1f S ON THE ICE", playT_);
        hudC(12, buf, PAL_HUD);
        hudC(14, "ENTER RUNS IT AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(10, why_ && why_[0] ? why_ : "SCRAPED A GATE", PAL_RED);
        hudC(12, "A SCRAPE FAILS THE PASS", PAL_HUD);
        hudC(14, "ENTER TRIES THE LOCK AGAIN", PAL_HUD);
    }
    hud(40 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.14f, 0.22f, 0.12f);
    t_ = 0.f;
    seedFlakes();
    if (bot_) begin();
    else showTitle();
    sys.setLight(30, 80, 120);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    flakes(DT);

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || bot_) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
        else step();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    }

    followCamera();
    draw();
    audio();
}

}  // namespace sledlock
