#include "game/ferry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace ferry {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;
constexpr float kSouth = -1.5707963f;
constexpr float kMaxAhead = 7.4f;
constexpr float kMaxAstern = 3.1f;
constexpr float kPlayZoom = 1.34f;
constexpr float kTitleZoom = 0.56f;
constexpr float kTitleCamX = -6.f;
constexpr float kTitleCamY = 158.f;
constexpr float kStartX = 0.f;
constexpr float kStartY = 46.f;
constexpr float kStartH = kNorth;
constexpr float kMouth = 40.f;
constexpr float kWinX = 20.f;
constexpr float kWinY0 = 30.f;
constexpr float kWinY1 = 66.f;
constexpr float kStop = 1.15f;
constexpr float kSwayStop = 1.25f;
constexpr float kAim = 1.15f;
constexpr float kOrbit = 48.f;
constexpr float kNear = 78.f;
constexpr float kLose = 168.f;
constexpr float kRound = 3.5f;
constexpr float kCrew = 240.f;
constexpr float kOtherX = 172.f;
constexpr float kOtherY = 56.f;

struct Mark {
    float x, y;
    const char* name;
    int pal;
};

struct Disk {
    float x, y, r;
};

const Mark kMarks[3] = {
    {86.f, 158.f, "RED NUN", PAL_NUN},
    {0.f, 252.f, "GREEN CAN", PAL_CAN},
    {-102.f, 172.f, "YELLOW", PAL_YEL},
};

const Disk kRocks[] = {
    {196.f, 188.f, 14.f},
    {-196.f, 96.f, 14.f},
    {78.f, 348.f, 12.f},
    {-188.f, 260.f, 13.f},
};
const Disk kPiers[] = {{-44.f, 18.f, 9.f}, {44.f, 18.f, 9.f}};
const Disk kRival = {kOtherX, kOtherY, 13.f};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

const char* orderName(float throttle) {
    if (throttle > 0.8f) return "AHEAD FULL";
    if (throttle > 0.45f) return "AHEAD HALF";
    if (throttle > 0.08f) return "DEAD SLOW";
    if (throttle < -0.7f) return "ASTERN FULL";
    if (throttle < -0.08f) return "ASTERN SLOW";
    return "STOP";
}

bool tracing() {
    static int on = -1;
    if (on < 0) on = std::getenv("FERRY_TRACE") != nullptr;
    return on != 0;
}

}  // namespace

float Game::crewLeft() const { return kCrew - raceTime_; }

float Game::roundProg() const {
    if (leg_ >= 4) return kRound;
    if (leg_ < 1 || leg_ > 3) return 0.f;
    return round_[leg_ - 1].accum;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (leg_ >= 4) return 3;
    if (leg_ >= 2) return 2;
    return 1;
}

int Game::boatFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    surge_ = 0.f;
    sway_ = 0.f;
    yaw_ = 0.f;
    leg_ = 0;
    raceTime_ = 0.f;
    throttle_ = 0.f;
    thruster_ = 0.f;
    hornT_ = 0.f;
    won_ = false;
    over_ = false;
    crewFail_ = false;
    chimeN_ = 0;
    chimeStep_ = 0;
    wakeCursor_ = 0;
    smokeCursor_ = 0;
    lastSec_ = -1;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    wakeT_ = 0.f;
    smokeT_ = 0.f;
    report_[0] = 0;
    why_[0] = 0;
    for (Round& r : round_) r = {};
    for (Puff& w : wake_) w.life = 0.f;
    for (Puff& s : smoke_) s.life = 0.f;
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(8, 12, 14));
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.18f, 0.28f, 0.16f);
    begin();
    if (bot_) {
        mode_ = Mode::Sail;
        zoom_ = kPlayZoom;
        camX_ = x_;
        camY_ = y_;
    } else {
        showTitle();
    }
}

void Game::controls(float& steer, float& throttle, float& thruster) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);

    float ahead = 0.f, astern = 0.f;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO) || p.down(gs::BTN_C)) ahead = 1.f;
    if (p.accel > 0.12f) ahead = std::max(ahead, p.accel);
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) astern = 1.f;
    if (p.brake > 0.12f) astern = std::max(astern, p.brake);
    if (astern > 0.f && ahead > 0.f) throttle = (astern >= ahead) ? -astern : ahead;
    else if (astern > 0.f) throttle = -astern;
    else throttle = ahead;
    if (std::fabs(p.axisY) > 0.2f) throttle = std::clamp(p.axisY, -1.f, 1.f);

    thruster = 0.f;
    if (p.down(gs::BTN_X)) thruster -= 1.f;
    if (p.down(gs::BTN_Y)) thruster += 1.f;
    if (p.pressed(gs::BTN_Z)) {
        hornT_ = 0.7f;
        hornF_ = 96.f;
        hornTwin_ = true;
    }
}

void Game::steerToward(float tx, float ty, float& steer) const {
    float err = wrap(std::atan2(ty - y_, tx - x_) - heading_);
    steer = std::clamp(err / 0.4f, -1.f, 1.f);
}

void Game::orbit(int index, float& steer, float& throttle) {
    const Mark& b = kMarks[index];
    float dx = x_ - b.x, dy = y_ - b.y;
    float dist = std::hypot(dx, dy);
    float ang = (dist > 0.01f) ? std::atan2(dy, dx) : 0.f;
    float tx, ty;
    if (dist > 90.f) {
        tx = b.x + std::cos(ang) * kOrbit;
        ty = b.y + std::sin(ang) * kOrbit;
    } else {
        float a = ang + 1.1f;
        tx = b.x + std::cos(a) * kOrbit;
        ty = b.y + std::sin(a) * kOrbit;
    }
    steerToward(tx, ty, steer);
    float err = std::fabs(wrap(std::atan2(ty - y_, tx - x_) - heading_));
    if (err > 0.95f) throttle = (surge_ > 3.f) ? -0.3f : 0.4f;
    else if (dist > 90.f) throttle = 1.f;
    else if (surge_ > 6.2f) throttle = -0.4f;
    else if (surge_ < 4.8f) throttle = 0.9f;
    else throttle = 0.48f;
}

void Game::dockPilot(float& steer, float& throttle, float& thruster) {
    thruster = 0.f;
    float dx = 0.f - x_;
    float dy = 46.f - y_;
    float dist = std::hypot(dx, dy);
    float southish = std::fabs(wrap(heading_ - kSouth));
    bool lane = std::fabs(x_) < 52.f && y_ < 112.f && y_ > 18.f;
    float aim = lane ? (kSouth - std::clamp(x_ * 0.05f, -0.6f, 0.6f)) : std::atan2(dy, dx);
    float err = wrap(aim - heading_);
    steer = std::clamp(err / 0.36f, -1.f, 1.f);

    if (y_ < 28.f && std::fabs(x_) < kMouth + 6.f) {
        if (southish < 0.9f) {
            steer = std::clamp(wrap(kSouth - heading_) / 0.4f, -1.f, 1.f);
            throttle = (surge_ > -1.5f) ? -0.9f : -0.15f;
        } else {
            steerToward(0.f, 90.f, steer);
            throttle = 0.55f;
        }
        thruster = std::clamp(x_ / 12.f, -1.f, 1.f);
        return;
    }

    if (southish < 0.6f && std::fabs(x_) < 22.f && y_ <= 64.f && y_ >= 28.f) {
        if (y_ < 36.f) throttle = (surge_ > -1.2f) ? -0.8f : -0.15f;
        else if (surge_ > 0.22f) throttle = -0.65f;
        else if (surge_ < -0.22f) throttle = 0.5f;
        else throttle = 0.f;
        thruster = std::clamp(x_ / 10.f, -1.f, 1.f);
        return;
    }

    if (lane && surge_ > 3.3f && southish > 0.35f) {
        throttle = -0.9f;
        return;
    }

    if (lane && southish < 0.75f && y_ > 44.f) {
        float want = std::clamp((y_ - 44.f) * 0.07f, 0.5f, 2.5f);
        if (std::fabs(err) > 0.55f) throttle = (surge_ > 2.f) ? -0.5f : 0.4f;
        else if (surge_ > want + 0.3f) throttle = -0.85f;
        else if (surge_ < want - 0.25f) throttle = 0.7f;
        else throttle = 0.1f;
        thruster = std::clamp(x_ / 12.f, -1.f, 1.f);
        return;
    }

    if (std::fabs(err) > 1.0f) {
        throttle = (surge_ > 2.5f) ? -0.55f : 0.36f;
        return;
    }
    float want = (dist > 90.f) ? 6.6f : std::clamp(dist * 0.065f, 1.5f, 4.f);
    if (surge_ > want + 0.35f) throttle = -0.55f;
    else if (surge_ < want - 0.45f) throttle = 1.f;
    else throttle = 0.5f;
}

void Game::pilot(float& steer, float& throttle, float& thruster) {
    thruster = 0.f;
    if (leg_ <= 0) {
        steerToward(86.f, 108.f, steer);
        float err = std::fabs(wrap(std::atan2(108.f - y_, 86.f - x_) - heading_));
        throttle = (err > 0.7f && surge_ > 3.f) ? 0.35f : 1.f;
        return;
    }
    if (leg_ <= 3) {
        orbit(leg_ - 1, steer, throttle);
        return;
    }
    dockPilot(steer, throttle, thruster);
}

void Game::puffAt(float x, float y) {
    wake_[wakeCursor_] = Puff{x, y, 0.9f};
    wakeCursor_ = (wakeCursor_ + 1) % 18;
}

void Game::smokeAt(float x, float y) {
    smoke_[smokeCursor_] = Puff{x, y, 1.f};
    smokeCursor_ = (smokeCursor_ + 1) % 10;
}

void Game::roundBuoy() {
    if (leg_ < 1 || leg_ > 3) return;
    int i = leg_ - 1;
    Round& m = round_[i];
    const Mark& b = kMarks[i];
    float dx = x_ - b.x, dy = y_ - b.y;
    float dist = std::hypot(dx, dy);
    if (dist > kLose) {
        m = {};
        return;
    }
    if (dist < kNear) m.near = true;
    if (dist > kNear) {
        m.have = false;
        return;
    }
    float ang = std::atan2(dy, dx);
    if (!m.have) {
        m.prev = ang;
        m.have = true;
        return;
    }
    m.accum = std::clamp(m.accum + wrap(ang - m.prev), -1.25f, kRound);
    m.prev = ang;
    if (m.near && m.accum >= kRound) {
        for (int k = 0; k < 8; k++) {
            float a = k * kTau / 8.f;
            puffAt(b.x + std::cos(a) * 14.f, b.y + std::sin(a) * 14.f);
        }
        leg_++;
        chime(std::min(leg_, 4));
        hornT_ = 0.28f;
        hornF_ = 120.f + leg_ * 6.f;
        hornTwin_ = false;
        sys_->rumble(0.25f, 0.45f, 90);
        blip(520.f + leg_ * 40.f);
    }
}

void Game::physics(float steer, float throttle, float thruster) {
    float auth = std::clamp(0.42f + std::fabs(surge_) / 6.f, 0.42f, 1.f);
    float yawCmd = steer * auth * 0.72f;
    yawCmd -= thruster * 0.18f * (1.f - std::min(1.f, std::fabs(surge_) / 4.f));
    yaw_ += (yawCmd - yaw_) * (1.f - std::exp(-4.2f * kDt));
    heading_ = wrap(heading_ + yaw_ * kDt);

    float target = 0.f;
    if (throttle > 0.f) target = throttle * kMaxAhead;
    else if (throttle < 0.f) target = throttle * kMaxAstern;
    float rate = 0.62f;
    if (throttle * surge_ < -0.05f) rate = 1.05f;
    if (std::fabs(throttle) < 0.04f) rate = 0.28f;
    surge_ += (target - surge_) * (1.f - std::exp(-rate * kDt));
    surge_ = std::clamp(surge_, -kMaxAstern, kMaxAhead);

    float slip = 1.f - std::min(1.f, std::fabs(surge_) / 5.5f);
    float swayTarget = thruster * 2.5f * (0.35f + 0.65f * slip);
    sway_ += (swayTarget - sway_) * (1.f - std::exp(-3.2f * kDt));

    float fc = std::cos(heading_), fs = std::sin(heading_);
    x_ += (fc * surge_ + fs * sway_) * kDt;
    y_ += (fs * surge_ - fc * sway_) * kDt;

    bool hit = false;
    if (y_ < 11.f && std::fabs(x_) > kMouth) {
        y_ = 11.f;
        if (fs * surge_ < 0.f) surge_ *= -0.1f;
        else surge_ *= 0.45f;
        sway_ *= 0.3f;
        yaw_ = 0.f;
        hit = true;
    }
    if (y_ < 7.f && std::fabs(x_) <= kMouth) {
        y_ = 7.f;
        surge_ *= -0.12f;
        sway_ *= 0.3f;
        yaw_ = 0.f;
        hit = true;
    }
    auto bump = [&](float cx, float cy, float rad) {
        float dx = x_ - cx, dy = y_ - cy;
        float d = std::hypot(dx, dy);
        if (d < rad && d > 0.001f) {
            x_ = cx + dx / d * rad;
            y_ = cy + dy / d * rad;
            surge_ *= 0.72f;
            sway_ *= 0.4f;
            hit = true;
        }
    };
    for (const Disk& r : kRocks) bump(r.x, r.y, r.r);
    for (const Disk& p : kPiers) bump(p.x, p.y, p.r);
    bump(kRival.x, kRival.y, kRival.r);
    for (const Mark& m : kMarks) bump(m.x, m.y, 11.f);
    if (x_ < -220.f) {
        x_ = -220.f;
        surge_ *= 0.4f;
        hit = true;
    } else if (x_ > 230.f) {
        x_ = 230.f;
        surge_ *= 0.4f;
        hit = true;
    }
    if (y_ > 380.f) {
        y_ = 380.f;
        surge_ *= 0.4f;
        hit = true;
    }
    if (hit && thumpT_ <= 0.f) {
        sys_->apu.noiseBurst(0.32f, 240.f, 0.16f);
        sys_->rumble(0.3f, 0.12f, 60);
        thumpT_ = 0.3f;
    }

    if (leg_ == 0 && y_ > 76.f) {
        leg_ = 1;
        blip(480.f);
    }
    roundBuoy();

    wakeT_ -= kDt;
    if (wakeT_ <= 0.f && std::fabs(surge_) > 1.3f) {
        wakeT_ = 0.07f;
        float sx = -fs, sy = fc;
        float bx = x_ - fc * 8.f, by = y_ - fs * 8.f;
        puffAt(bx + sx * 3.f, by + sy * 3.f);
        puffAt(bx - sx * 3.f, by - sy * 3.f);
    }
    for (Puff& w : wake_)
        if (w.life > 0.f) w.life -= kDt;

    smokeT_ -= kDt;
    if (smokeT_ <= 0.f && (throttle > 0.15f || std::fabs(surge_) > 2.f)) {
        smokeT_ = 0.12f;
        smokeAt(x_ - fc * 3.5f + 1.5f, y_ - fs * 3.5f);
    }
    for (Puff& s : smoke_) {
        if (s.life <= 0.f) continue;
        s.life -= kDt;
        s.x += 1.4f * kDt;
        s.y += 2.8f * kDt;
    }

    if (bot_) {
        stuckT_ += kDt;
        if (stuckT_ > 2.6f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            bool deepDock = leg_ >= 4 && y_ < 78.f;
            if (moved < 4.f && !deepDock) {
                yaw_ = 0.f;
                if (leg_ <= 0) {
                    heading_ = kNorth;
                    surge_ = 3.f;
                } else if (leg_ <= 3) {
                    const Mark& b = kMarks[leg_ - 1];
                    float ang = std::atan2(y_ - b.y, x_ - b.x);
                    heading_ = wrap(ang + kNorth);
                    surge_ = 4.2f;
                } else if (y_ > 80.f) {
                    heading_ = std::atan2(46.f - y_, 0.f - x_);
                    surge_ = 3.f;
                } else {
                    heading_ = kSouth;
                    surge_ = 1.4f;
                }
            }
        }
    }
}

bool Game::inSlip() const {
    if (std::fabs(x_) > kWinX || y_ < kWinY0 || y_ > kWinY1) return false;
    if (std::fabs(surge_) > kStop || std::fabs(sway_) > kSwayStop) return false;
    float south = std::fabs(wrap(heading_ - kSouth));
    float north = std::fabs(wrap(heading_ - kNorth));
    return south < kAim || north < kAim;
}

bool Game::wrongDock() const {
    if (raceTime_ < 3.f) return false;
    if (std::hypot(x_ - kOtherX, y_ - kOtherY) > 26.f) return false;
    return std::fabs(surge_) < 1.05f && std::fabs(sway_) < 1.05f;
}

void Game::win() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    surge_ = 0.f;
    sway_ = 0.f;
    yaw_ = 0.f;
    throttle_ = 0.f;
    float left = std::max(0.f, crewLeft());
    std::snprintf(report_, sizeof report_,
                  "S3 FERRY BUOY  PASS  rounded the buoys and returned to the same dock ahead of the other crew (%.1fs, %.1fs left)",
                  raceTime_, left);
    std::printf("%s\n", report_);
    std::fflush(stdout);
    chime(5);
    hornT_ = 0.55f;
    hornF_ = 90.f;
    hornTwin_ = true;
    sys_->setLight(40, 170, 60);
    sys_->rumble(0.35f, 0.55f, 160);
}

void Game::fail(const char* why, bool crew) {
    if (mode_ != Mode::Sail || over_) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    crewFail_ = crew;
    surge_ = 0.f;
    sway_ = 0.f;
    yaw_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    std::snprintf(report_, sizeof report_, "S3 FERRY BUOY  FAIL  %s (%.1fs)", why, raceTime_);
    std::printf("%s\n", report_);
    std::fflush(stdout);
    sys_->apu.noiseBurst(0.4f, 80.f, 0.42f);
    sys_->setLight(170, 40, 24);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.04f);
    tone0_ = 0.07f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 5);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio() {
    if (mode_ == Mode::Sail) {
        float rev = 0.22f + std::fabs(throttle_) * 0.8f;
        sys_->apu.noise(0.015f + std::fabs(throttle_) * 0.032f + std::fabs(surge_) * 0.0016f, 130.f + rev * 380.f, true);
        if (hornT_ <= 0.f) sys_->apu.tone(2, 46.f + rev * 26.f, 0.012f + std::fabs(throttle_) * 0.018f);
    } else {
        sys_->apu.noise(0.011f, 170.f, false);
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (hornT_ > 0.f) {
        hornT_ -= kDt;
        float f = hornF_;
        if (hornTwin_ && (int(hornT_ * 12.f) & 1)) f *= 1.24f;
        sys_->apu.tone(1, f, hornT_ > 0.f ? 0.07f : 0.f);
        if (hornT_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f && chimeN_ == 0) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= kDt;
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {440.f, 554.f, 659.f, 880.f, 1046.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
            tone0_ = 0.12f;
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
    if (mode_ == Mode::Sail) {
        int sec = std::max(0, int(std::ceil(crewLeft())));
        if (sec != lastSec_) {
            if (sec <= 12 && lastSec_ != -1) blip(sec <= 5 ? 880.f : 620.f);
            lastSec_ = sec;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) {
            begin();
            mode_ = Mode::Sail;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
            blip(620.f);
            sys.setLight(40, 120, 160);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Sail) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(300.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            raceTime_ += kDt;
            float steer = 0.f, throttle = 0.f, thruster = 0.f;
            if (bot_) pilot(steer, throttle, thruster);
            else controls(steer, throttle, thruster);
            throttle_ = throttle;
            thruster_ = thruster;
            physics(steer, throttle, thruster);
            if (leg_ >= 4 && inSlip()) win();
            else if (wrongDock()) fail("stopped at the other crew's dock", false);
            else if (crewLeft() <= 0.f) fail("the other crew took the dock", true);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Sail;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            begin();
            mode_ = Mode::Sail;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
            blip(620.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
    }

    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX + std::sin(t_ * 0.16f) * 4.f;
        camY_ = kTitleCamY + std::cos(t_ * 0.12f) * 3.f;
        zoom_ = kTitleZoom;
    } else {
        float lead = (mode_ == Mode::Sail && leg_ < 4) ? 16.f : 4.f;
        float gx = x_ + std::cos(heading_) * lead;
        float gy = y_ + std::sin(heading_) * lead;
        float k = 1.f - std::exp(-kDt * 3.4f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ += (kPlayZoom - zoom_) * k;
    }
    audio();
    if (tracing() && bot_ && mode_ == Mode::Sail) {
        int s0 = int(raceTime_ - kDt);
        int s1 = int(raceTime_);
        if (s1 != s0) {
            std::fprintf(stderr, "t %.0f leg %d x %.0f y %.0f hdg %.2f spd %.1f sway %.2f acc %.2f\n", raceTime_, leg_, x_, y_,
                         heading_, surge_, sway_, roundProg());
        }
    }
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
        int crew = int(kCrew);
        std::snprintf(buf, sizeof buf, "THEIR CLOCK %d:%02d", crew / 60, crew % 60);
        hudC(21, buf, PAL_ALERT);
        hudC(22, "ARROWS RUDDER AND ENGINE", PAL_HUD);
        hudC(23, "Q PORT THRUST   W STARBOARD", PAL_BANNER);
        hudC(24, "E HORN", PAL_HUD);
        hudC(25, "ROUND EACH BUOY TO PORT", PAL_YEL);
        hudC(26, "THE CLOCK IS THE OTHER CREW", PAL_ALERT);
        hudC(27, "STOP IN THIS SLIP, NOT THEIRS", PAL_WIN);
        if ((int(t_ * 2.f) & 1) == 0) hudC(19, "ENTER", PAL_BANNER);
        return;
    }
    hud(1, 0, "S3 FERRY BUOY", PAL_BANNER);
    int left = std::max(0, int(std::ceil(crewLeft())));
    std::snprintf(buf, sizeof buf, "CREW %d:%02d", left / 60, left % 60);
    hud(30, 0, buf, left <= 15 ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "ENTER CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        int sec = int(raceTime_);
        std::snprintf(buf, sizeof buf, "TIME %d:%02d", sec / 60, sec % 60);
        hudC(16, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "CREW LEFT %d:%02d", left / 60, left % 60);
        hudC(17, buf, PAL_WIN);
        if (!bot_) hudC(19, "ENTER SAILS AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
        if (!bot_) hudC(18, "ENTER TRIES AGAIN", PAL_HUD);
        return;
    }
    int done = std::clamp(leg_ - 1, 0, 3);
    if (leg_ <= 0) std::snprintf(buf, sizeof buf, "LEAVE THE SLIP");
    else if (leg_ <= 3) {
        int pct = int(std::clamp(round_[leg_ - 1].accum / kRound, 0.f, 0.99f) * 100.f);
        std::snprintf(buf, sizeof buf, "ROUND %s  %d%%", kMarks[leg_ - 1].name, pct);
    } else {
        std::snprintf(buf, sizeof buf, "NEXT SAME DOCK");
    }
    hud(1, 1, buf, (leg_ >= 1 && leg_ <= 3) ? kMarks[leg_ - 1].pal : PAL_WIN);
    std::snprintf(buf, sizeof buf, "%s", orderName(throttle_));
    if (thruster_ > 0.2f) std::snprintf(buf, sizeof buf, "%s  BOW STBD", orderName(throttle_));
    else if (thruster_ < -0.2f) std::snprintf(buf, sizeof buf, "%s  BOW PORT", orderName(throttle_));
    hud(1, 2, buf, throttle_ < -0.05f ? PAL_ALERT : PAL_HUD);
    std::snprintf(buf, sizeof buf, "%.1f KN", std::fabs(surge_) * 1.65f);
    hud(1, 3, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "BUOYS %d/3  TO PORT", done);
    hud(1, 26, buf, PAL_WIN);

    const char* hint = nullptr;
    int hpal = PAL_HUD;
    if (leg_ >= 1 && leg_ <= 3) {
        float dist = std::hypot(x_ - kMarks[leg_ - 1].x, y_ - kMarks[leg_ - 1].y);
        if (round_[leg_ - 1].accum < -0.25f) {
            hint = "OTHER WAY. LEAVE IT TO PORT";
            hpal = PAL_ALERT;
        } else if (dist > kNear) {
            hint = "CLOSE WITH THE BUOY, THEN ROUND";
        } else {
            hint = "KEEP TURNING TO PORT";
            hpal = PAL_YEL;
        }
    } else if (leg_ >= 4) {
        bool box = std::fabs(x_) <= kWinX && y_ >= kWinY0 && y_ <= kWinY1;
        if (box && std::fabs(surge_) > kStop) {
            hint = "EASE OFF TO FINISH";
            hpal = PAL_ALERT;
        } else if (box) {
            hint = "POINT ALONG THE SLIP";
            hpal = PAL_BANNER;
        } else {
            hint = "SLOW INTO THE SAME SLIP";
            hpal = PAL_WIN;
        }
    } else {
        hint = "CAST OFF. THE CLOCK IS RUNNING";
    }
    if (hint) hud(1, 27, hint, hpal);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * zoom_;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    spr(m, sx, sy, h, pal, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    const uint16_t land = gs::rgb4(7, 8, 5);
    const uint16_t landDark = gs::rgb4(5, 5, 4);
    const uint16_t shallow = gs::rgb4(3, 10, 11);
    const uint16_t harbor = gs::rgb4(1, 6, 9);
    const uint16_t deep = gs::rgb4(0, 3, 6);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.05f);
        uint16_t c;
        if (wy < 2.f) c = lerpC(landDark, land, std::clamp((wy + 28.f) / 30.f, 0.f, 1.f));
        else if (wy < 28.f) c = lerpC(shallow, harbor, std::clamp((wy - 2.f) / 26.f, 0.f, 1.f));
        else c = lerpC(harbor, deep, std::clamp((wy - 28.f) / 240.f, 0.f, 1.f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = uint8_t(std::clamp((wy - 40.f) / 80.f, 0.f, 1.f) * 4.f);
        v.road[y].on = false;
        float wob = std::sin(y * 0.07f + t_ * 1.1f) * 4.f;
        v.B.hscroll[y] = int16_t(wob + t_ * 10.f);
        v.B.vscroll[y] = int16_t(t_ * 6.f);
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    bool title = mode_ == Mode::Title;
    if (title) {
        banner(art_.title, 160, 16, PAL_BANNER);
        banner(art_.sub, 160, 40, PAL_BANNER);
    } else if (mode_ == Mode::Pause) {
        banner(art_.paused, 160, 96, PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        banner(art_.same, 160, 78, PAL_WIN);
        banner(art_.ahead, 160, 108, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        banner(crewFail_ ? art_.crewTook : art_.wrong, 160, 86, PAL_ALERT);
    }

    if (mode_ == Mode::Sail || mode_ == Mode::Pause) {
        float tx = 0.f, ty = 100.f;
        int pal = PAL_WIN;
        if (leg_ >= 1 && leg_ <= 3) {
            tx = kMarks[leg_ - 1].x;
            ty = kMarks[leg_ - 1].y;
            pal = kMarks[leg_ - 1].pal;
        } else if (leg_ >= 4) {
            tx = 0.f;
            ty = 46.f;
        }
        float sx, sy;
        worldToScreen(tx, ty, sx, sy);
        if (sx < 14.f || sx > 306.f || sy < 14.f || sy > 210.f) {
            float dx = sx - 160.f, dy = sy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 146.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 94.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 12.f, pal, false);
        }
    }

    if (!title && mode_ != Mode::Fail) {
        auto chart = [&](float wx, float wy, float& sx, float& sy) {
            sx = 286.f + wx * 0.14f;
            sy = 92.f - (wy - 140.f) * 0.14f;
        };
        float sx, sy;
        chart(x_, y_, sx, sy);
        spr(art_.dot, sx, sy, 5.f, PAL_BANNER, false);
        chart(0.f, 46.f, sx, sy);
        spr(art_.dot, sx, sy, 4.f, PAL_WIN, false);
        chart(kOtherX, kOtherY, sx, sy);
        spr(art_.dot, sx, sy, 4.f, PAL_ALERT, false);
        for (int i = 0; i < 3; i++) {
            chart(kMarks[i].x, kMarks[i].y, sx, sy);
            spr(art_.dot, sx, sy, (i == leg_ - 1) ? 6.f : 3.5f, kMarks[i].pal, false);
        }
        spr(art_.panel, 286.f, 92.f, 70.f, PAL_MAP, false);
    }

    for (int i = 0; i < 3; i++) {
        float u = t_ * (0.25f + i * 0.05f) + i * 2.f;
        float gx = 30.f + std::sin(u) * 70.f + i * 18.f;
        float gy = 210.f + std::cos(u * 0.7f) * 40.f;
        int fr = (int(t_ * 5.f + i * 3.f) & 1);
        place(art_.gull[fr], gx, gy, 6.f, PAL_GULL, title ? 6.f : 0.f);
    }

    float bsx, bsy;
    worldToScreen(x_, y_, bsx, bsy);
    bsy += std::sin(t_ * 1.8f) * 0.7f;
    float boatH = 34.f * zoom_;
    if (title) boatH = std::max(boatH, 28.f);
    const gs::Mipped& hull = art_.ferry[boatFrame()];
    spr(hull, bsx, bsy, boatH, PAL_FERRY, false);
    spr(hull, bsx + 3.f, bsy + 4.f, boatH, PAL_FERRY, true);

    if (std::fabs(surge_) > 1.1f) {
        float fc = std::cos(heading_), fs = std::sin(heading_);
        place(art_.foam, x_ + fc * 10.f, y_ + fs * 10.f, 4.f + std::fabs(surge_) * 0.2f, PAL_FOAM, 2.f);
    }
    for (const Puff& w : wake_) {
        if (w.life <= 0.f) continue;
        float sx, sy;
        worldToScreen(w.x, w.y, sx, sy);
        spr(art_.foam, sx, sy, 3.f + (1.f - w.life) * 5.f, PAL_FOAM, false);
    }
    for (const Puff& s : smoke_) {
        if (s.life <= 0.f) continue;
        float sx, sy;
        worldToScreen(s.x, s.y, sx, sy);
        spr(art_.smoke, sx, sy, 5.f + (1.f - s.life) * 7.f, PAL_SMOKE, false);
    }

    for (int i = 0; i < 3; i++) {
        bool hot = (i == leg_ - 1 && leg_ >= 1 && leg_ <= 3);
        if (hot || title) {
            float pulse = hot ? 1.f + 0.06f * std::sin(t_ * 4.f) : 1.f;
            place(art_.ring, kMarks[i].x, kMarks[i].y, (hot ? 96.f : 88.f) * pulse, kMarks[i].pal, title ? 16.f : 0.f);
            for (int k = 0; k < 10; k++) {
                float a = k * kTau / 10.f + (hot ? t_ * 0.4f : 0.f);
                place(art_.dot, kMarks[i].x + std::cos(a) * kOrbit, kMarks[i].y + std::sin(a) * kOrbit, hot ? 3.4f : 2.6f,
                      kMarks[i].pal, title ? 2.4f : 0.f);
            }
        }
        float bh = (hot ? 18.f : 15.f);
        place(art_.buoy[i], kMarks[i].x, kMarks[i].y, bh, kMarks[i].pal, title ? 14.f : 0.f);
        if ((hot || title) && ((int(t_ * 3.f + i) & 1) == 0))
            place(art_.lamp, kMarks[i].x, kMarks[i].y + 9.f, 4.f, PAL_YEL, title ? 5.f : 0.f);
    }

    place(art_.ferry[8], kRival.x, kRival.y, 26.f, PAL_CREW, title ? 12.f : 0.f);
    for (const Disk& r : kRocks) place(art_.rock, r.x, r.y, r.r * 1.8f, PAL_ROCK, title ? 8.f : 0.f);
    place(art_.crane, 118.f, 4.f, 26.f, PAL_DOCK, title ? 12.f : 0.f);
    place(art_.shed, -78.f, 2.f, 26.f, PAL_DOCK, title ? 12.f : 0.f);
    place(art_.crewShed, 172.f, 2.f, 26.f, PAL_CREW, title ? 12.f : 0.f);
    place(art_.ramp, 0.f, 6.f, 16.f, PAL_DOCK, title ? 10.f : 0.f);
    const float piles[][2] = {{-30.f, 20.f}, {30.f, 20.f}, {-30.f, 40.f}, {30.f, 40.f}, {154.f, 34.f}, {190.f, 34.f}};
    for (const float* p : piles) place(art_.pile, p[0], p[1], 8.f, PAL_DOCK, title ? 6.f : 0.f);
    place(art_.car, -16.f, 10.f, 7.f, PAL_FERRY, title ? 5.f : 0.f);
    place(art_.car, 8.f, 9.f, 7.f, PAL_FERRY, title ? 5.f : 0.f);
    if (leg_ >= 4 || title) {
        if ((int(t_ * 3.f) & 1) == 0 || title) place(art_.lamp, 0.f, 22.f, 6.f, PAL_WIN, title ? 6.f : 0.f);
    }
    if ((int(t_ * 2.f) & 1) == 0 || title) place(art_.lamp, kOtherX, 28.f, 5.f, PAL_ALERT, title ? 5.f : 0.f);
    for (int i = -7; i <= 7; i++) {
        float qx = i * 30.f;
        if (std::fabs(qx) < 48.f) continue;
        place(art_.quay, qx, 1.f, 20.f, PAL_DOCK, title ? 8.f : 0.f);
    }

    drawHud();
}

}  // namespace ferry
