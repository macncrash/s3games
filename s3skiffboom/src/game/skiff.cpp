#include "skiff.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace skiffboom {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = kPi * 0.5f;

constexpr float kStartX = 0.f;
constexpr float kStartY = 20.f;
constexpr float kStartH = kNorth;

constexpr float kParkY = 158.f;
constexpr float kPocketX = 8.4f;
constexpr float kPocketY0 = 150.f;
constexpr float kPocketY1 = 166.f;
constexpr float kBoomMouth = 146.f;
constexpr float kHeadY0 = 170.f;
constexpr float kHeadY1 = 178.f;
constexpr float kStickIn = 13.2f;
constexpr float kNorthLimit = 180.f;

constexpr float kBow = 6.2f;
constexpr float kStern = 5.3f;
constexpr float kBeam = 2.7f;
constexpr float kBoatWorld = 18.5f;
constexpr float kDriveWorld = 7.6f;
constexpr float kDriveFwd = 3.15f;

constexpr float kCrewTime = 34.f;
constexpr float kStop = 0.72f;
constexpr float kHold = 0.42f;
constexpr float kAim = 0.9f;
constexpr float kCribHit = 8.6f;
constexpr float kBoomHit = 7.4f;
constexpr float kCribOff = 17.5f;
constexpr float kCribR = 3.25f;
constexpr float kRivalY0 = 48.f;
constexpr float kRivalGoal = 140.f;
constexpr float kRivalSide = 8.2f;

constexpr float kMaxSpd = 12.4f;
constexpr float kPlayZoom = 2.15f;
constexpr float kTitleZoom = 0.75f;
constexpr float kTitleCamX = 2.f;
constexpr float kTitleCamY = 92.f;

const float kCribY[4] = {40.f, 64.f, 90.f, 114.f};
const float kCribSide[4] = {-1.f, -1.f, 1.f, -1.f};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float smooth(float u) {
    u = std::clamp(u, 0.f, 1.f);
    return u * u * (3.f - 2.f * u);
}

float lerp(float a, float b, float u) { return a + (b - a) * u; }

float fairX(float y) {
    if (y < 44.f) return 0.f;
    if (y < 78.f) return 22.f * smooth((y - 44.f) / 34.f);
    if (y < 112.f) return 22.f - 30.f * smooth((y - 78.f) / 34.f);
    if (y < 132.f) return -8.f + 8.f * smooth((y - 112.f) / 20.f);
    return 0.f;
}

float waterW(float y) {
    if (y < 136.f) return 40.f;
    if (y < 150.f) return lerp(40.f, 18.f, smooth((y - 136.f) / 14.f));
    if (y < 172.f) return 18.f;
    if (y < 184.f) return lerp(18.f, 9.f, smooth((y - 172.f) / 12.f));
    return 9.f;
}

float fairW(float y) { return std::max(8.f, waterW(y) - 2.6f); }

struct Hazard {
    float x, y, r;
};

Hazard cribAt(int i) {
    float y = kCribY[i];
    return {fairX(y) + kCribSide[i] * kCribOff, y, kCribR};
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

int Game::frameOf(float heading) const {
    float u = std::fmod(heading, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (settle_ > 0.05f) return 3;
    if (inPocket_) return 2;
    return 1;
}

const char* Game::hint() const {
    if (driveOnBoom_) return "THE DRIVE IS ON THE BOOM";
    if (inPocket_ && std::fabs(speed_) > 1.f) return "BRAKE. LET THE DRIVE SETTLE";
    if (inPocket_) return "HOLD. THE BOOM TAKES THE DRIVE";
    if (y_ > kBoomMouth - 18.f) return "NOSE INTO THE BOOM AND STOP";
    if (crew_ < 12.f) return "THE OTHER CREW IS CLOSE";
    return "DELIVER THE DRIVE TO THE BOOM";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 0.f;
    throttle_ = 0.f;
    raceTime_ = 0.f;
    crew_ = kCrewTime;
    settle_ = 0.f;
    wakeT_ = 0.f;
    tickT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    wakeCursor_ = 0;
    inPocket_ = false;
    driveOnBoom_ = false;
    driveLost_ = false;
    entered_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    why_[0] = 0;
    report_[0] = 0;
    std::snprintf(why_, sizeof why_, "running");
    rivalVy_ = (kRivalGoal - kRivalY0) / kCrewTime;
    rivalY_ = kRivalY0;
    rivalX_ = fairX(rivalY_) + kRivalSide;
    rivalH_ = kNorth;
    lostX_ = lostY_ = 0.f;
    for (Wake& w : wakes_) w = {};
}

void Game::showTitle() {
    begin();
    rivalY_ = 78.f;
    rivalX_ = fairX(rivalY_) + kRivalSide;
    rivalH_ = std::atan2(4.f, fairX(rivalY_ + 4.f) + kRivalSide - rivalX_);
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    zoom_ = kPlayZoom;
    camX_ = x_;
    camY_ = y_;
    blip(580.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.74f);
    sys.apu.setEcho(0.14f, 0.2f, 0.08f);
    if (bot_) startRun();
    else showTitle();
}

void Game::human(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f || p.accel > 0.2f;
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f || p.brake > 0.2f;
    if (stop) throttle_ = std::max(-1.f, throttle_ - kDt * 2.2f);
    else if (go) throttle_ = std::min(1.f, throttle_ + kDt * 1.35f);
    else {
        float decay = std::fabs(speed_) < 0.55f ? 3.0f : 0.65f;
        if (throttle_ > 0.f) throttle_ = std::max(0.f, throttle_ - kDt * decay);
        else throttle_ = std::min(0.f, throttle_ + kDt * decay);
    }
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    if (y_ > 132.f) {
        float bias = std::clamp(x_ * 0.22f, -0.45f, 0.45f);
        if (std::fabs(x_) < 2.4f && std::fabs(speed_) < 1.3f) bias *= 0.2f;
        if (std::fabs(x_) > 6.f) bias = std::clamp(x_ * 0.32f, -0.6f, 0.6f);
        steer = std::clamp(wrap(kNorth + bias - heading_) / 0.28f, -1.f, 1.f);
        if (std::fabs(x_) > 7.f) {
            if (speed_ > 3.2f) throttle = -0.55f;
            else if (speed_ < 1.4f) throttle = 0.5f;
            else throttle = 0.1f;
            return;
        }
        if (y_ < 148.f) {
            if (speed_ > 4.0f) throttle = -0.6f;
            else if (speed_ < 2.4f) throttle = 0.72f;
            else throttle = 0.12f;
        } else if (y_ < kParkY - 3.f) {
            if (speed_ > 2.0f) throttle = -0.75f;
            else if (speed_ < 1.05f) throttle = 0.48f;
            else throttle = 0.05f;
        } else if (y_ <= kParkY + 5.f) {
            if (speed_ > 0.32f) throttle = -0.95f;
            else if (speed_ < -0.08f) throttle = 0.32f;
            else throttle = 0.f;
        } else {
            throttle = speed_ > -0.22f ? -0.6f : 0.f;
        }
        return;
    }

    float look = 14.f + std::max(0.f, speed_) * 0.9f;
    float ty = std::min(y_ + look, kParkY);
    float tx = ty > 108.f ? 0.f : fairX(ty);
    float dx = tx - x_, dy = ty - y_;
    float dist = std::hypot(dx, dy);
    float want = dist < 4.f ? kNorth : std::atan2(dy, dx);
    float err = wrap(want - heading_);
    steer = std::clamp(err / 0.32f, -1.f, 1.f);

    float avoid = 0.f;
    float c = std::cos(heading_), s = std::sin(heading_);
    for (int i = 0; i < 4; i++) {
        Hazard h = cribAt(i);
        float ox = h.x - x_, oy = h.y - y_;
        float d = std::hypot(ox, oy);
        if (d < 12.f && d > 0.05f) {
            float cross = c * oy - s * ox;
            float push = (12.f - d) / 12.f;
            avoid += (cross > 0.f ? -1.f : 1.f) * push * 1.7f;
        }
    }
    steer = std::clamp(steer + avoid, -1.f, 1.f);

    float cap = 0.92f;
    if (std::fabs(err) > 0.75f) cap = 0.28f;
    else if (std::fabs(err) > 0.4f) cap = 0.55f;
    if (y_ > 100.f) cap = std::min(cap, 0.7f);
    float spdCap = cap * kMaxSpd;
    if (speed_ > spdCap + 0.4f) throttle = -0.45f;
    else if (speed_ < spdCap - 1.1f) throttle = cap;
    else throttle = cap * 0.55f;
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    driveOnBoom_ = true;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "delivered");
    std::snprintf(report_, sizeof report_,
                  "S3 SKIFF BOOM  DELIVERED  the drive is on the boom ahead of the other crew  (%.1f s, %.1f s left)",
                  raceTime_, std::max(0.f, crew_));
    chime(4);
    sys_->rumble(0.35f, 0.55f, 160);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    std::snprintf(report_, sizeof report_, "S3 SKIFF BOOM  FAIL  %s (%.1f s)", why, raceTime_);
    sys_->apu.noiseBurst(0.42f, 90.f, 0.4f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.4f;
    sys_->rumble(0.6f, 0.2f, 180);
    sys_->setLight(180, 30, 20);
}

void Game::stepRival(float dt) {
    if (rivalY_ < kRivalGoal) rivalY_ = std::min(kRivalGoal, rivalY_ + rivalVy_ * dt);
    float side = kRivalSide;
    if (rivalY_ > 120.f) side = kRivalSide * (1.f - std::clamp((rivalY_ - 120.f) / 24.f, 0.f, 1.f));
    float nx = fairX(std::min(rivalY_ + 4.f, kParkY)) + side;
    rivalX_ = fairX(rivalY_) + side;
    rivalH_ = std::atan2(4.f, nx - rivalX_);
}

void Game::physics(float dt, float steer, float throttle) {
    raceTime_ += dt;
    crew_ -= dt;

    float rate = 1.55f + std::min(std::fabs(speed_), 12.f) * 0.07f;
    heading_ = wrap(heading_ + steer * rate * dt);

    if (throttle < -0.02f && speed_ > 0.f) {
        speed_ -= (-throttle) * 9.0f * dt;
        if (speed_ < 0.f) speed_ = std::max(speed_, throttle * 3.4f);
    } else {
        float cap = kMaxSpd;
        if (std::fabs(x_) < kStickIn && y_ > kBoomMouth) cap = 6.5f;
        float target = throttle >= 0.f ? throttle * cap : throttle * 4.2f;
        speed_ += (target - speed_) * (1.f - std::exp(-2.0f * dt));
    }
    if (std::fabs(throttle) < 0.05f && std::fabs(speed_) < 1.f) speed_ *= std::exp(-5.5f * dt);
    speed_ = std::clamp(speed_, -4.2f, 15.f);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * dt;
    y_ += s * speed_ * dt;

    float current = 0.55f;
    if (y_ > 136.f) current *= std::clamp((156.f - y_) / 20.f, 0.f, 1.f);
    y_ -= current * dt;

    if (std::fabs(x_) < kStickIn && y_ > kBoomMouth && y_ < kHeadY0) speed_ *= std::exp(-0.9f * dt);

    float hitSpd = std::fabs(speed_);
    bool cribHit = false, boomHit = false;
    float xBefore = x_, yBefore = y_;

    auto sample = [&](float along, float beam, float& px, float& py) {
        px = x_ + c * along - s * beam;
        py = y_ + s * along + c * beam;
    };
    float pts[5][3];
    sample(0.f, 0.f, pts[0][0], pts[0][1]);
    pts[0][2] = kBeam;
    sample(kBow, 0.f, pts[1][0], pts[1][1]);
    pts[1][2] = 1.6f;
    sample(-kStern, 0.f, pts[2][0], pts[2][1]);
    pts[2][2] = 1.5f;
    sample(0.f, -kBeam, pts[3][0], pts[3][1]);
    pts[3][2] = 1.15f;
    sample(0.f, kBeam, pts[4][0], pts[4][1]);
    pts[4][2] = 1.15f;

    auto pushCircle = [&](float cx, float cy, float rad, float px, float py) {
        float dx = px - cx, dy = py - cy;
        float d2 = dx * dx + dy * dy;
        float reach = rad + 0.15f;
        if (d2 >= reach * reach) return false;
        float d = std::sqrt(std::max(d2, 1e-6f));
        float pen = reach - d + 0.08f;
        x_ += dx / d * pen;
        y_ += dy / d * pen;
        return true;
    };
    for (int n = 0; n < 2; n++) {
        for (const auto& p : pts) {
            for (int i = 0; i < 4; i++) {
                Hazard h = cribAt(i);
                if (pushCircle(h.x, h.y, h.r, p[0], p[1])) cribHit = true;
            }
        }
        c = std::cos(heading_);
        s = std::sin(heading_);
        sample(0.f, 0.f, pts[0][0], pts[0][1]);
        sample(kBow, 0.f, pts[1][0], pts[1][1]);
        sample(-kStern, 0.f, pts[2][0], pts[2][1]);
        sample(0.f, -kBeam, pts[3][0], pts[3][1]);
        sample(0.f, kBeam, pts[4][0], pts[4][1]);
    }

    auto pushBox = [&](float px, float py, float rad, float x0, float x1, float y0, float y1) {
        float nx = std::clamp(px, x0, x1);
        float ny = std::clamp(py, y0, y1);
        float dx = px - nx, dy = py - ny;
        float d2 = dx * dx + dy * dy;
        if (d2 >= rad * rad) return false;
        if (d2 < 1e-4f) {
            float dl = px - x0, dr = x1 - px, db = py - y0, dtv = y1 - py;
            if (dl <= dr && dl <= db && dl <= dtv) x_ -= dl + rad;
            else if (dr <= db && dr <= dtv) x_ += dr + rad;
            else if (db <= dtv) y_ -= db + rad;
            else y_ += dtv + rad;
        } else {
            float d = std::sqrt(d2);
            float pen = rad - d + 0.08f;
            x_ += dx / d * pen;
            y_ += dy / d * pen;
        }
        return true;
    };
    c = std::cos(heading_);
    s = std::sin(heading_);
    sample(0.f, 0.f, pts[0][0], pts[0][1]);
    sample(kBow, 0.f, pts[1][0], pts[1][1]);
    sample(-kStern, 0.f, pts[2][0], pts[2][1]);
    sample(0.f, -kBeam, pts[3][0], pts[3][1]);
    sample(0.f, kBeam, pts[4][0], pts[4][1]);
    for (const auto& p : pts) {
        if (pushBox(p[0], p[1], p[2], -28.f, -kStickIn, kBoomMouth, kHeadY1)) boomHit = true;
        if (pushBox(p[0], p[1], p[2], kStickIn, 28.f, kBoomMouth, kHeadY1)) boomHit = true;
        if (pushBox(p[0], p[1], p[2], -28.f, 28.f, kHeadY0, kHeadY1 + 4.f)) boomHit = true;
    }

    float shove = std::hypot(x_ - xBefore, y_ - yBefore);
    if (shove > 2.4f) {
        x_ = xBefore + (x_ - xBefore) / shove * 2.4f;
        y_ = yBefore + (y_ - yBefore) / shove * 2.4f;
    }
    if (cribHit) {
        if (hitSpd > kCribHit) {
            driveLost_ = true;
            lostX_ = x_ - s * 4.8f;
            lostY_ = y_ + c * 4.8f;
            fail("the drive went over");
            return;
        }
        speed_ *= 0.42f;
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.28f, 180.f, 0.12f);
            thumpT_ = 0.28f;
            sys_->rumble(0.3f, 0.12f, 80);
        }
    }
    if (boomHit) {
        if (hitSpd > kBoomHit) {
            fail("broke the boom");
            return;
        }
        speed_ *= 0.35f;
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.32f, 140.f, 0.14f);
            thumpT_ = 0.3f;
            sys_->rumble(0.35f, 0.15f, 90);
        }
    }

    float cx = fairX(y_);
    float lim = fairW(y_);
    float off = x_ - cx;
    if (std::fabs(off) > lim) {
        x_ = cx + std::copysign(lim, off);
        speed_ *= 0.5f;
        if (hitSpd > 11.5f && y_ < kBoomMouth) {
            driveLost_ = true;
            lostX_ = x_;
            lostY_ = y_ - 3.f;
            fail("grounded the drive");
            return;
        }
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.16f, 240.f, 0.08f);
            thumpT_ = 0.22f;
        }
    }
    if (y_ < 10.f) {
        y_ = 10.f;
        if (s < 0.f) speed_ *= 0.4f;
    }
    if (y_ > kNorthLimit) {
        y_ = kNorthLimit;
        if (!inPocket_) {
            fail("missed the boom");
            return;
        }
        speed_ = 0.f;
    }

    c = std::cos(heading_);
    s = std::sin(heading_);
    float bowX = x_ + c * kBow, bowY = y_ + s * kBow;
    float sternX = x_ - c * kStern, sternY = y_ - s * kStern;
    float errH = std::fabs(wrap(kNorth - heading_));
    bool body = std::fabs(x_) <= kPocketX && y_ >= kPocketY0 && y_ <= kPocketY1;
    bool nose = std::fabs(bowX) <= kPocketX + 1.6f && bowY >= kPocketY0 - 1.f && bowY <= kHeadY0 - 0.8f;
    bool tail = std::fabs(sternX) <= kStickIn - 0.8f && sternY > kBoomMouth - 2.f;
    inPocket_ = body && nose && !driveLost_;
    bool posed = inPocket_ && tail && errH <= kAim;
    if (inPocket_ && !entered_) {
        entered_ = true;
        blip(490.f);
    }
    if (posed && std::fabs(speed_) <= kStop) {
        settle_ += dt;
        speed_ *= std::exp(-7.f * dt);
        if (settle_ >= kHold) {
            succeed();
            return;
        }
    } else {
        settle_ = 0.f;
    }

    if (crew_ <= 0.f) {
        fail("the other crew took the boom");
        return;
    }

    wakeT_ -= dt;
    if (wakeT_ <= 0.f && std::fabs(speed_) > 4.2f && y_ < kBoomMouth) {
        wakeT_ = 0.08f;
        Wake w;
        w.x = x_ - c * 6.5f;
        w.y = y_ - s * 6.5f;
        w.life = 1.f;
        wakes_[wakeCursor_] = w;
        wakeCursor_ = (wakeCursor_ + 1) % 16;
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= dt * 0.85f;

    stepRival(dt);

    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 2.2f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 1.6f && y_ < kPocketY0) {
                heading_ = std::atan2(kParkY - y_, -x_ * 0.4f);
                speed_ = std::max(speed_, 5.5f);
            }
        }
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.09f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 5);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio(float dt) {
    float water = mode_ == Mode::Run ? 0.012f + std::fabs(speed_) * 0.00045f : 0.008f;
    bool slack = inPocket_ || mode_ == Mode::Win;
    sys_->apu.noise(slack ? water * 0.4f : water, slack ? 280.f : 640.f, false);
    if (mode_ == Mode::Run && (throttle_ > 0.05f || std::fabs(speed_) > 1.4f)) {
        float wob = 0.65f + 0.35f * std::sin(t_ * (15.f + std::max(0.f, throttle_) * 18.f));
        float vol = (0.012f + std::max(0.f, throttle_) * 0.03f) * wob;
        sys_->apu.tone(2, 46.f + std::max(0.f, throttle_) * 34.f + std::fabs(speed_) * 0.3f, vol);
    } else if (tone0_ <= 0.f) {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= dt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= dt;
    if (mode_ == Mode::Run && crew_ < 8.f && crew_ > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) {
            blip(crew_ < 4.f ? 880.f : 520.f);
            tickT_ = crew_ < 4.f ? 0.25f : 0.5f;
        }
    }
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 523.f, 659.f, 784.f, 1046.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
            tone0_ = 0.12f;
            chimeT_ = 0.13f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(340.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            float steer = 0.f, thr = 0.f;
            if (bot_) pilot(steer, thr);
            else human(steer, thr);
            throttle_ = thr;
            physics(kDt, steer, throttle_);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    if (mode_ == Mode::Run) {
        if (crew_ < 8.f) sys.setLight(180, 70, 20);
        else sys.setLight(30, 110, 140);
    }
    camera();
    audio(kDt);
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
        return;
    }
    float lead = mode_ == Mode::Run ? 11.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float gz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        gx = 0.f;
        gy = (kBoomMouth + kHeadY0) * 0.5f;
        gz = 1.7f;
    }
    float k = 1.f - std::exp(-kDt * (mode_ == Mode::Run ? 4.6f : 2.8f));
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (gz - zoom_) * k;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
    gs::Sprite spt;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    spt.w = int16_t(sw);
    spt.h = int16_t(sh);
    spt.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    spt.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    spt.img = m.pick(float(sh));
    spt.pal = uint8_t(pal);
    spt.shadow = shadow;
    sys_->vdp.sprite(spt);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    int fog = int(std::clamp((std::fabs(wy - camY_) - 48.f) * 0.04f, 0.f, 8.f));
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (sx + w < -8 || sy + h < -8 || sx - w > gs::SCREEN_W + 8 || sy - h > gs::SCREEN_H + 8) return;
    gs::Sprite spt;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    spt.w = int16_t(sw);
    spt.h = int16_t(sh);
    spt.x = int16_t(std::clamp(std::lround(sx - sw * 0.5f), -2000L, 2000L));
    spt.y = int16_t(std::clamp(std::lround(sy - sh * 0.5f), -2000L, 2000L));
    spt.img = m.pick(float(sh));
    spt.pal = uint8_t(pal);
    spt.fog = uint8_t(fog);
    sys_->vdp.sprite(spt);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.roadTime = int(t_ * 36.f);
    float zoom = std::max(zoom_, 0.2f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / zoom;
        float u = std::clamp(wy / 200.f, 0.f, 1.f);
        v.lineBackdrop[y] = lerpC(gs::rgb4(4, 7, 3), gs::rgb4(2, 4, 2), u);
        int fog = int(std::clamp((std::fabs(wy - camY_) - 36.f) * 0.045f, 0.f, 7.f));
        v.lineFog[y] = uint8_t(fog);
        gs::RoadLine& r = v.road[y];
        r = {};
        r.on = true;
        float half = waterW(wy);
        float cxw = fairX(wy);
        r.cx = 160.f + (cxw - camX_) * zoom;
        r.hw = std::max(6.f, half * zoom);
        r.v = wy * 20.f;
        r.pal = uint8_t(PAL_WATER);
        r.band = (int(std::floor(wy * 0.45f)) & 1) ? 1 : 0;
        r.style = 2;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 13.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.missed;
        if (std::strcmp(why_, "the other crew took the boom") == 0) msg = &art_.crewTook;
        else if (std::strcmp(why_, "the drive went over") == 0) msg = &art_.driveLost;
        else if (std::strcmp(why_, "broke the boom") == 0) msg = &art_.broke;
        else if (std::strcmp(why_, "grounded the drive") == 0) msg = &art_.grounded;
        banner(*msg, 160.f, 18.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.delivered, 160.f, 16.f, PAL_WIN);
        banner(art_.onBoom, 160.f, 42.f, PAL_WIN);
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        spr(art_.panel, 292.f, 78.f, 78.f, PAL_MAP, false);
        auto dot = [&](float wx, float wy, int pal, float h) {
            float sx = 292.f + (wx - 4.f) * 0.42f;
            float sy = 78.f - (wy - 96.f) * 0.42f;
            spr(art_.dot, sx, sy, h, pal, false);
        };
        dot(0.f, kParkY, PAL_WIN, 4.f);
        dot(x_, y_, PAL_ALERT, 4.5f);
        dot(rivalX_, rivalY_, PAL_MARK, 3.5f);
        for (int i = 0; i < 4; i++) {
            Hazard h = cribAt(i);
            dot(h.x, h.y, PAL_CRIB, 2.5f);
        }
    }

    auto drawBoat = [&](float wx, float wy, float heading, int pal, bool drive, float bobAmp) {
        int fr = frameOf(heading);
        float sx = 160.f + (wx - camX_) * zoom_ ;
        float sy = 112.f - (wy - camY_) * zoom_ + bobAmp * std::sin(t_ * 2.3f + wx);
        float boatPx = kBoatWorld * zoom_;
        if (mode_ == Mode::Title && pal == PAL_HULL) boatPx = std::max(boatPx, 22.f);
        spr(art_.hull[fr], sx + 3.f, sy + 3.f, boatPx, pal, true);
        spr(art_.hull[fr], sx, sy, boatPx, pal, false);
        if (!drive) return;
        float ppw = boatPx / kBoatWorld;
        float c = std::cos(heading), s = std::sin(heading);
        float ox = c * kDriveFwd * ppw;
        float oy = -s * kDriveFwd * ppw;
        spr(art_.drive[fr], sx + ox, sy + oy, kDriveWorld * ppw, PAL_DRIVE, false);
    };

    if (!driveLost_ && !driveOnBoom_) drawBoat(x_, y_, heading_, PAL_HULL, true, inPocket_ ? 0.3f : 0.8f);
    else drawBoat(x_, y_, heading_, PAL_HULL, false, 0.4f);
    if (driveOnBoom_) {
        float sx = 160.f + (0.f - camX_) * zoom_;
        float sy = 112.f - (173.f - camY_) * zoom_;
        spr(art_.drive[0], sx, sy, 11.f * zoom_, PAL_DRIVE, false);
    } else if (driveLost_) {
        place(art_.drive[frameOf(heading_ + 0.8f)], lostX_, lostY_, 8.f, PAL_DRIVE, 6.f);
    }
    drawBoat(rivalX_, rivalY_, rivalH_, PAL_RIVAL, false, 0.7f);

    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        float h = (2.4f + (1.f - w.life) * 3.2f) * (zoom_ / kPlayZoom);
        float sx = 160.f + (w.x - camX_) * zoom_;
        float sy = 112.f - (w.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false);
    }
    if (std::fabs(speed_) > 3.5f && mode_ == Mode::Run) {
        float c = std::cos(heading_), s = std::sin(heading_);
        place(art_.foam, x_ + c * (kBow + 1.2f), y_ + s * (kBow + 1.2f), 2.4f, PAL_FOAM, 2.f);
    }

    place(art_.logH, -16.f, 174.f, 6.2f, PAL_BOOM, 3.f);
    place(art_.logH, 0.f, 174.f, 6.2f, PAL_BOOM, 3.f);
    place(art_.logH, 16.f, 174.f, 6.2f, PAL_BOOM, 3.f);
    place(art_.logV, -16.2f, 154.f, 20.f, PAL_BOOM, 6.f);
    place(art_.logV, -16.2f, 138.f, 16.f, PAL_BOOM, 5.f);
    place(art_.logV, 16.2f, 154.f, 20.f, PAL_BOOM, 6.f);
    place(art_.logV, 16.2f, 138.f, 16.f, PAL_BOOM, 5.f);
    for (int i = 0; i < 4; i++) {
        float y = 128.f + i * 4.5f;
        float x = waterW(y) - 3.f;
        place(art_.logH, -x, y, 4.4f, PAL_BOOM, 2.f);
        place(art_.logH, x, y, 4.4f, PAL_BOOM, 2.f);
    }
    place(art_.pile, -21.f, 136.f, 8.f, PAL_BOOM, 4.f);
    place(art_.pile, 21.f, 136.f, 8.f, PAL_BOOM, 4.f);
    place(art_.pile, -21.f, 172.f, 8.f, PAL_BOOM, 4.f);
    place(art_.pile, 21.f, 172.f, 8.f, PAL_BOOM, 4.f);
    place(art_.buoy, -10.2f, 144.f, 6.f, PAL_MARK, 4.f);
    place(art_.buoy, 10.2f, 144.f, 6.f, PAL_MARK, 4.f);
    place(art_.post, 30.f, 162.f, 11.f, PAL_BOOM, 5.f);
    place(art_.boomSign, 30.f, 170.f, 7.f, PAL_BANNER, mode_ == Mode::Title ? 12.f : 0.f);
    place(art_.shed, 36.f, 154.f, 15.f, PAL_SHED, 8.f);
    place(art_.tender[int(t_ * 1.5f) & 1], 8.f, 176.f, 9.f, PAL_CREW, 6.f);
    place(art_.heron, fairX(70.f) + waterW(70.f) + 2.f, 70.f, 12.f, PAL_BIRD, 6.f);

    for (float y = 18.f; y < 132.f; y += 16.f) {
        place(art_.reed, fairX(y) - waterW(y) - 1.2f, y, 7.f, PAL_WATER, 3.f);
        place(art_.reed, fairX(y) + waterW(y) + 1.2f, y + 6.f, 7.f, PAL_WATER, 3.f);
    }
    for (int i = 0; i < 4; i++) {
        Hazard h = cribAt(i);
        place(art_.crib, h.x, h.y, h.r * 2.3f, PAL_CRIB, 6.f);
    }

    int flap = int(t_ * 3.2f) & 1;
    place(art_.gull[flap], fairX(50.f) - 10.f + std::sin(t_ * 0.4f) * 12.f, 52.f + std::cos(t_ * 0.3f) * 4.f, 5.f, PAL_BIRD, 3.f);
    place(art_.gull[1 - flap], fairX(100.f) + 14.f + std::cos(t_ * 0.35f) * 10.f, 102.f, 4.5f, PAL_BIRD, 3.f);

    auto dashes = [&](float x0, float y0, float x1, float y1, int n) {
        for (int i = 0; i < n; i++) {
            float u = n == 1 ? 0.5f : float(i) / float(n - 1);
            place(art_.dash, x0 + (x1 - x0) * u, y0 + (y1 - y0) * u, 1.6f, PAL_MARK, 0.f);
        }
    };
    dashes(-kPocketX, kPocketY0, kPocketX, kPocketY0, 5);
    dashes(-kPocketX, kPocketY1, kPocketX, kPocketY1, 5);
    dashes(-kPocketX, kPocketY0, -kPocketX, kPocketY1, 4);
    dashes(kPocketX, kPocketY0, kPocketX, kPocketY1, 4);

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float psx = 160.f + (0.f - camX_) * zoom_;
        float psy = 112.f - (kParkY - camY_) * zoom_;
        if (psx < 18.f || psx > 300.f || psy < 16.f || psy > 208.f) {
            float dx = psx - 160.f, dy = psy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 130.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 80.f / std::fabs(dy));
            spr(art_.dot, 160.f + dx * k, 112.f + dy * k, 8.f, PAL_WIN, false);
        }
        float rsx = 160.f + (rivalX_ - camX_) * zoom_;
        float rsy = 112.f - (rivalY_ - camY_) * zoom_;
        if (rsy > -20.f && rsy < gs::SCREEN_H + 10.f) spr(art_.crewTag, rsx, rsy - 16.f, 10.f, PAL_ALERT, false);
    }

    char buf[48];
    int left = std::max(0, int(std::ceil(crew_ - 1e-3f)));
    if (mode_ == Mode::Title) {
        hudC(23, "DELIVER THE DRIVE TO THE BOOM", PAL_BANNER);
        hudC(24, "THE CLOCK IS THE OTHER CREW", PAL_ALERT);
        hudC(26, "ARROWS STEER   UP GO   DOWN BRAKE", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "ENTER STARTS", PAL_WIN);
        return;
    }
    hud(1, 0, "S3 SKIFF BOOM", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "CREW %d:%02d", left / 60, left % 60);
    hud(29, 0, buf, left <= 8 ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "ENTER CONTINUES", PAL_HUD);
        hudC(19, "ESC BACK TO TITLE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        int sec = int(raceTime_);
        std::snprintf(buf, sizeof buf, "TIME %d:%02d   CREW HAD %d:%02d", sec / 60, sec % 60, left / 60, left % 60);
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "ENTER RUNS THE LEG AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
        if (!bot_) hudC(18, "ENTER TRIES THE LEG AGAIN", PAL_HUD);
        return;
    }
    hud(1, 1, hint(), inPocket_ ? PAL_WIN : PAL_BANNER);
    int sp = int(std::lround(std::fabs(speed_)));
    std::snprintf(buf, sizeof buf, "SPD %02d   %s", sp, driveLost_ ? "DRIVE LOST" : "DRIVE ABOARD");
    hud(1, 2, buf, driveLost_ ? PAL_ALERT : PAL_WIN);
    if (settle_ > 0.f) {
        int n = std::clamp(int(settle_ / kHold * 6.f), 0, 6);
        std::snprintf(buf, sizeof buf, "HOLD %.*s", n, "******");
        hud(1, 25, buf, PAL_WIN);
    } else if (y_ > 120.f) {
        int dist = std::max(0, int(std::lround(kParkY - y_)));
        std::snprintf(buf, sizeof buf, "BOOM %d", dist);
        hud(1, 25, buf, PAL_MARK);
    } else {
        hud(1, 25, "THE REACH", PAL_HUD);
    }
    if (left <= 8) hud(1, 26, "THE OTHER CREW IS CLOSE", PAL_ALERT);
    hud(1, 27, "HIT A CRIB HARD AND THE DRIVE GOES OVER", PAL_ALERT);
}

}  // namespace skiffboom
