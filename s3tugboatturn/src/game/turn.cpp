#include "turn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace tugturn {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kTip = 1.02f;
constexpr float kHeelGain = 0.155f;
constexpr float kHeelK = 5.4f;
constexpr float kHeelDamp = 3.6f;
constexpr float kCapFwd = 9.2f;
constexpr float kCapRev = 3.4f;
constexpr float kYawBase = 0.40f;
constexpr float kYawPer = 0.058f;
constexpr float kCrew = 76.f;
constexpr float kFocal = 286.f;
constexpr float kBoatH = 4.4f;

// Three fairway turns. Each opens straight, hooks, and closes straight.
// exitZ sits on the straight after the hook. dir -1 is port.
struct Bend {
    float z0, z1, x0, x1, exitZ;
    int dir;
};

constexpr Bend kTurns[3] = {
    {46.f, 108.f, 0.f, -32.f, 132.f, -1},
    {158.f, 232.f, -32.f, 36.f, 258.f, 1},
    {286.f, 356.f, 36.f, -10.f, 384.f, -1},
};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float smooth(float t) {
    t = std::clamp(t, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

float courseX(float z) {
    if (z <= kTurns[0].z0) return kTurns[0].x0;
    for (const Bend& b : kTurns) {
        if (z < b.z1) {
            if (z < b.z0) return b.x0;
            float t = (z - b.z0) / (b.z1 - b.z0);
            return b.x0 + (b.x1 - b.x0) * smooth(t);
        }
    }
    return kTurns[2].x1;
}

float courseHeading(float z) {
    float dx = courseX(z + 2.6f) - courseX(z - 2.6f);
    return std::atan2(dx, 5.2f);
}

float courseHalf(float z) {
    float h = 13.8f;
    for (const Bend& b : kTurns) {
        if (z > b.z0 && z < b.z1) {
            float t = (z - b.z0) / (b.z1 - b.z0);
            h += 3.2f * std::sin(t * kPi);
        }
    }
    if (z < 24.f) h += (24.f - z) * 0.18f;
    return h;
}

bool inTurn(float z) {
    for (const Bend& b : kTurns)
        if (z >= b.z0 && z <= b.z1) return true;
    return false;
}

int bendAt(float z) {
    for (int i = 0; i < 3; i++)
        if (z >= kTurns[i].z0 && z <= kTurns[i].z1) return i;
    return -1;
}

const char* missWhy(int i) {
    if (i <= 0) return "missed the first turn";
    if (i == 1) return "missed the second turn";
    return "missed the third turn";
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

int hashN(int n) {
    uint32_t x = uint32_t(n) * 2246822519u;
    x ^= x >> 13;
    x *= 3266489917u;
    return int(x & 0x7fffffff);
}

}  // namespace

float Game::lateral() const { return x_ - courseX(z_); }
float Game::crewLeft() const { return std::max(0.f, kCrew - race_); }
float Game::eye() const { return mode_ == Mode::Title ? 5.05f : 4.35f; }
float Game::back() const { return mode_ == Mode::Title ? 12.4f : 10.4f; }
float Game::horizon() const { return mode_ == Mode::Title ? 86.f : 80.f; }

float Game::rivalZ() const {
    float start = 64.f;
    float end = kTurns[2].exitZ + 8.f;
    float u = 0.f;
    if (mode_ == Mode::Title) u = 0.04f;
    else u = std::clamp(race_ / kCrew, 0.f, 1.f);
    return start + (end - start) * u;
}

int Game::frameOf(float list) const {
    float u = std::clamp(list / 0.75f, -1.f, 1.f);
    int i = int(std::lround((u + 1.f) * 0.5f * 6.f));
    return std::clamp(i, 0, 6);
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (turns_ >= 2) return 3;
    if ((inTurn(z_) && std::fabs(heel_) > 0.16f) || turns_ >= 1) return 2;
    return 1;
}

const char* Game::tipWhy() const {
    if (turns_ >= 2 || z_ >= kTurns[1].exitZ) return "tipped on the third turn";
    if (turns_ >= 1 || z_ >= kTurns[0].exitZ) return "tipped on the second turn";
    return "tipped on the first turn";
}

void Game::begin() {
    z_ = 16.f;
    x_ = courseX(z_);
    heading_ = 0.f;
    speed_ = 0.f;
    throttle_ = 0.f;
    yaw_ = 0.f;
    heel_ = 0.f;
    heelVel_ = 0.f;
    race_ = 0.f;
    shake_ = 0.f;
    turns_ = 0;
    won_ = false;
    over_ = false;
    warned_ = false;
    why_ = "";
    chimeN_ = 0;
    chimeStep_ = 0;
    chimeT_ = 0.f;
    hornT_ = 0.f;
    tone0_ = 0.f;
    tone1_ = 0.f;
    wakeN_ = 0;
    puffN_ = 0;
    wakeT_ = 0.f;
    puffT_ = 0.f;
    for (int i = 0; i < 3; i++) made_[i] = false;
    for (Wake& w : wakes_) w = {};
    for (Puff& p : smokes_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    z_ = kTurns[0].z0 + 26.f;
    x_ = courseX(z_);
    heading_ = courseHeading(z_);
    heel_ = 0.52f;
    speed_ = 6.4f;
    for (int i = 0; i < 4; i++) {
        Wake w;
        w.x = x_ - std::sin(heading_) * (1.4f + float(i) * 1.1f);
        w.z = z_ - std::cos(heading_) * (1.4f + float(i) * 1.1f);
        w.life = 0.85f - float(i) * 0.16f;
        wakes_[i] = w;
    }
    wakeN_ = 4;
    for (int i = 0; i < 3; i++) {
        Puff p;
        p.x = x_ + std::sin(heading_) * 0.5f;
        p.z = z_ + std::cos(heading_) * 0.5f;
        p.y = 3.6f + float(i) * 0.45f;
        p.vx = -std::sin(heading_) * 0.4f;
        p.vz = -std::cos(heading_) * 0.4f;
        p.life = 0.8f - float(i) * 0.18f;
        smokes_[i] = p;
    }
    puffN_ = 3;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    horn();
    sys_->rumble(0.2f, 0.08f, 120);
    sys_->setLight(30, 70, 130);
}

void Game::buildCourse() {
    props_.clear();
    props_.reserve(96);
    for (int i = 0; i < 22; i++) {
        float z = 14.f + float(i) * 18.f;
        float c = courseX(z);
        float h = courseHalf(z);
        props_.push_back(Prop{c - h - 0.15f, z, 2.15f, Kind::BuoyP, PAL_RED, 0});
        props_.push_back(Prop{c + h + 0.15f, z + 9.f, 2.15f, Kind::BuoyS, PAL_GREEN, 0});
    }
    for (int i = 0; i < 12; i++) {
        float z = 22.f + float(i) * 32.f;
        float c = courseX(z);
        float h = courseHalf(z);
        int n = hashN(i * 13 + 5);
        float j = float(n % 100) / 100.f;
        props_.push_back(Prop{c - h - 3.4f - j, z, 4.4f, Kind::Lamp, PAL_STEEL, 0});
        props_.push_back(Prop{c + h + 3.2f + j * 0.6f, z + 14.f, 4.4f, Kind::Lamp, PAL_STEEL, 0});
    }
    const float sheds[] = {32.f, 92.f, 148.f, 206.f, 268.f, 332.f, 402.f};
    for (int i = 0; i < 7; i++) {
        float z = sheds[i];
        float side = (i & 1) ? 1.f : -1.f;
        float x = courseX(z) + side * (courseHalf(z) + 5.2f);
        props_.push_back(Prop{x, z, 6.4f, Kind::Shed, PAL_PIER, 0});
        if (i == 1 || i == 3 || i == 5) {
            props_.push_back(Prop{x + side * 1.4f, z + 8.f, 9.2f, Kind::Crane, PAL_STEEL, 0});
        }
    }
    for (int i = 0; i < 3; i++) {
        const Bend& b = kTurns[i];
        float z = 0.5f * (b.z0 + b.z1);
        float side = float(b.dir);
        props_.push_back(Prop{courseX(z) + side * (courseHalf(z) + 2.6f), z, 3.3f, Kind::Board, PAL_MARK, i});
    }
    for (int i = 0; i < 6; i++) {
        float z = 40.f + float(i) * 58.f;
        float side = (i & 1) ? -1.f : 1.f;
        props_.push_back(Prop{courseX(z) + side * (courseHalf(z) + 1.5f), z, 1.5f, Kind::Bollard, PAL_STEEL, 0});
    }
    float fz = kTurns[2].exitZ;
    float fc = courseX(fz);
    float fh = courseHalf(fz);
    props_.push_back(Prop{fc - fh - 0.4f, fz, 6.2f, Kind::Lamp, PAL_STEEL, 0});
    props_.push_back(Prop{fc + fh + 0.4f, fz, 6.2f, Kind::Lamp, PAL_STEEL, 0});
    props_.push_back(Prop{fc - fh - 4.8f, fz + 16.f, 7.2f, Kind::Shed, PAL_PIER, 0});

    gulls_[0] = Gull{10.f, 70.f, 8.2f, 0.4f};
    gulls_[1] = Gull{-24.f, 160.f, 9.4f, 1.6f};
    gulls_[2] = Gull{30.f, 250.f, 7.6f, 2.7f};
    gulls_[3] = Gull{-8.f, 340.f, 10.f, 3.8f};
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildCourse();
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.16f, 0.22f, 0.14f);
    if (bot_) startRun();
    else showTitle();
}

void Game::controls(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(p.axisX, -1.f, 1.f);
    const bool ahead = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C);
    const bool back = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    if (ahead) throttle_ = std::min(1.f, throttle_ + kDt * 0.7f);
    if (back) throttle_ = std::max(-0.5f, throttle_ - kDt * 1.1f);
    if (!ahead && !back) throttle_ += (0.f - throttle_) * (1.f - std::exp(-0.8f * kDt));
    if (p.accel > 0.12f) throttle_ = std::max(throttle_, p.accel);
    if (p.brake > 0.12f) throttle_ = std::min(throttle_, -p.brake * 0.5f);
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) const {
    float look = std::clamp(15.f + speed_ * 0.75f, 15.f, 26.f);
    float off = x_ - courseX(z_);
    float hDes = std::atan2(courseX(z_ + look) - x_, look);
    hDes -= std::clamp(off * 0.05f, -0.42f, 0.42f);
    float err = wrap(hDes - heading_);
    float cmd = std::clamp(err / 0.36f - yaw_ * 0.45f, -1.f, 1.f);

    float need = std::fabs(wrap(courseHeading(z_ + look) - heading_));
    float half = courseHalf(z_);
    float wide = std::fabs(off) / std::max(half, 1.f);
    float want = 8.5f;
    if (need > 0.55f) want = 5.7f;
    else if (need > 0.28f) want = 6.7f;
    if (wide > 0.48f) want = std::min(want, 5.1f);
    if (std::fabs(heel_) > 0.62f) {
        want = std::min(want, 5.0f);
        bool adding = (cmd > 0.f && heel_ < 0.f) || (cmd < 0.f && heel_ > 0.f);
        if (adding) cmd *= 0.55f;
    }
    steer = std::clamp(cmd, -0.78f, 0.78f);
    throttle = std::clamp(want / kCapFwd, 0.22f, 0.92f);
}

void Game::markTurns(float zPrev) {
    for (int i = 0; i < 3; i++) {
        if (made_[i]) continue;
        if (z_ < kTurns[i].exitZ) break;
        if (i > 0 && !made_[i - 1]) {
            fail("skipped a turn");
            return;
        }
        if (zPrev >= kTurns[i].exitZ) {
            fail("skipped a turn");
            return;
        }
        float off = std::fabs(x_ - courseX(z_));
        float herr = std::fabs(wrap(heading_ - courseHeading(z_)));
        bool inside = off <= courseHalf(z_) + 0.8f;
        bool aligned = herr <= 0.9f;
        bool moving = speed_ > 1.6f;
        if (!inside || !aligned || !moving) {
            fail(missWhy(i));
            return;
        }
        made_[i] = true;
        turns_ = i + 1;
        if (i == 2) {
            win();
            return;
        }
        blip(480.f + float(i) * 90.f);
        sys_->rumble(0.16f, 0.06f, 80);
    }
}

void Game::physics(float steer, float throttle) {
    if (mode_ != Mode::Run) return;
    steer = std::clamp(steer, -1.f, 1.f);
    throttle = std::clamp(throttle, -1.f, 1.f);
    float zPrev = z_;

    float rate = kYawBase + std::min(std::fabs(speed_), kCapFwd) * kYawPer;
    float yawCmd = steer * rate;
    yaw_ += (yawCmd - yaw_) * (1.f - std::exp(-3.2f * kDt));
    heading_ = wrap(heading_ + yaw_ * kDt);

    float cap = throttle >= 0.f ? kCapFwd : kCapRev;
    float target = throttle * cap;
    speed_ += (target - speed_) * (1.f - std::exp(-0.62f * kDt));
    speed_ = std::clamp(speed_, -kCapRev, kCapFwd);

    x_ += std::sin(heading_) * speed_ * kDt;
    z_ += std::cos(heading_) * speed_ * kDt;
    float slide = heel_ * std::fabs(speed_) * 0.055f;
    x_ += std::cos(heading_) * slide * kDt;
    z_ += -std::sin(heading_) * slide * kDt;
    if (z_ < 4.f) {
        z_ = 4.f;
        if (speed_ < 0.f) speed_ = 0.f;
    }

    // List is outward. A hard helm at speed, or a quay, walks her over.
    float demand = -yaw_ * speed_ * kHeelGain;
    float off = x_ - courseX(z_);
    float over = std::fabs(off) - courseHalf(z_);
    if (over > 0.f) {
        float sgn = off > 0.f ? 1.f : -1.f;
        x_ -= sgn * std::min(over, 0.42f);
        speed_ *= std::max(0.7f, 1.f - std::min(over, 2.5f) * 0.05f);
        demand += sgn * (1.05f + std::min(over, 2.2f) * 0.75f);
        if (over > 4.2f) {
            fail("fetched up on the quay");
            return;
        }
    }
    heelVel_ += ((demand - heel_) * kHeelK - heelVel_ * kHeelDamp) * kDt;
    heel_ += heelVel_ * kDt;
    heel_ = std::clamp(heel_, -1.45f, 1.45f);
    if (std::fabs(heel_) > kTip) {
        fail(tipWhy());
        return;
    }
    if (std::fabs(heel_) > 0.70f && !warned_) {
        warned_ = true;
        blip(150.f);
        sys_->rumble(0.28f, 0.1f, 90);
    } else if (std::fabs(heel_) < 0.42f) {
        warned_ = false;
    }

    markTurns(zPrev);
}

void Game::puffs() {
    bool live = mode_ == Mode::Run || mode_ == Mode::Title;
    if (live) {
        wakeT_ -= kDt;
        if (wakeT_ <= 0.f && std::fabs(speed_) > 2.f) {
            wakeT_ = 0.09f;
            Wake w;
            w.x = x_ - std::sin(heading_) * 2.2f;
            w.z = z_ - std::cos(heading_) * 2.2f;
            w.life = 1.f;
            wakes_[wakeN_] = w;
            wakeN_ = (wakeN_ + 1) % 12;
        }
        puffT_ -= kDt;
        if (puffT_ <= 0.f && (std::fabs(speed_) > 1.2f || std::fabs(throttle_) > 0.2f)) {
            puffT_ = 0.14f;
            Puff p;
            p.x = x_ + std::sin(heading_) * 0.55f;
            p.z = z_ + std::cos(heading_) * 0.55f;
            p.y = 4.15f;
            p.vx = -std::sin(heading_) * 0.55f + std::cos(heading_) * 0.15f;
            p.vz = -std::cos(heading_) * 0.55f - std::sin(heading_) * 0.15f;
            p.life = 1.f;
            smokes_[puffN_] = p;
            puffN_ = (puffN_ + 1) % 8;
        }
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= kDt * 0.7f;
    for (Puff& p : smokes_) {
        if (p.life <= 0.f) continue;
        p.life -= kDt * 0.55f;
        p.y += kDt * 1.35f;
        p.x += p.vx * kDt;
        p.z += p.vz * kDt;
    }
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "steady";
    chime();
    sys_->rumble(0.34f, 0.16f, 220);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.5f, 80.f, 0.42f);
    sys_->apu.tone(0, 62.f, 0.07f);
    tone0_ = 0.45f;
    sys_->rumble(0.72f, 0.22f, 240);
    sys_->setLight(190, 30, 20);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.1f;
}

void Game::horn() { hornT_ = 0.95f; }

void Game::chime() {
    chimeN_ = 4;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio() {
    bool drive = mode_ == Mode::Run || mode_ == Mode::Title;
    float water = drive ? 0.012f + std::fabs(speed_) * 0.0007f : 0.008f;
    sys_->apu.noise(water, 380.f, false);
    if (drive && (std::fabs(throttle_) > 0.04f || std::fabs(speed_) > 1.2f)) {
        float knock = 0.62f + 0.38f * std::sin(t_ * (9.f + std::fabs(throttle_) * 18.f));
        float vol = (0.014f + std::fabs(throttle_) * 0.03f + std::fabs(speed_) * 0.001f) * knock;
        sys_->apu.tone(2, 46.f + std::fabs(throttle_) * 28.f + std::fabs(speed_) * 0.6f, vol);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }

    if (hornT_ > 0.f) {
        hornT_ -= kDt;
        float vol = 0.075f;
        float f = 104.f;
        if (hornT_ < 0.55f && hornT_ > 0.40f) vol = 0.f;
        else if (hornT_ <= 0.40f) f = 138.f;
        sys_->apu.tone(0, f, vol);
        if (hornT_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    } else if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {330.f, 392.f, 494.f, 659.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 3)], 0.06f);
            tone0_ = 0.18f;
            chimeT_ = 0.16f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    } else if (mode_ == Mode::Run && std::fabs(heel_) > 0.62f) {
        float v = (std::fabs(heel_) - 0.62f) * 0.18f;
        sys_->apu.tone(0, 90.f + std::fabs(heel_) * 40.f, v);
    } else if (mode_ == Mode::Run) {
        sys_->apu.tone(0, 0.f, 0.f);
    }

    if (tone1_ > 0.f) {
        tone1_ -= kDt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.3f);
    bob_ = std::sin(t_ * 2.05f) * (mode_ == Mode::Run ? 0.04f + std::fabs(speed_) * 0.0035f : 0.03f);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
        puffs();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(420.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            if (!bot_ && (pad.pressed(gs::BTN_Z) || pad.pressed(gs::BTN_TURBO))) horn();
            race_ += kDt;
            float steer = 0.f, thr = throttle_;
            if (bot_) pilot(steer, thr);
            else controls(steer, thr);
            throttle_ = thr;
            physics(steer, thr);
            if (mode_ == Mode::Run && race_ >= kCrew) fail("the other crew took the berth");
            puffs();
            if (mode_ == Mode::Run) {
                float ah = std::fabs(heel_);
                if (crewLeft() < 12.f) sys.setLight(200, 40, 30);
                else if (ah > 0.75f) sys.setLight(210, 50, 20);
                else if (ah > 0.40f) sys.setLight(180, 110, 30);
                else if (turns_ > 0) sys.setLight(40, 140, 70);
                else sys.setLight(30, 70, 130);
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        if (sys.hasHome()) sys.eject();
        else showTitle();
    }

    audio();
    draw();
}

bool Game::project(float wx, float wy, float wz, float& sx, float& sy, float& scale, int& fog, float& rz) const {
    float camX = x_ - std::sin(heading_) * back();
    float camZ = z_ - std::cos(heading_) * back();
    float dx = wx - camX;
    float dz = wz - camZ;
    float S = std::sin(heading_);
    float C = std::cos(heading_);
    rz = dx * S + dz * C;
    float rx = dx * C - dz * S;
    if (rz < 1.4f) return false;
    scale = kFocal / rz;
    float jx = shake_ * std::sin(t_ * 47.f) * 5.f;
    float jy = shake_ * std::cos(t_ * 39.f) * 3.f;
    sx = 160.f + rx * scale + jx;
    sy = horizon() - (wy - (eye() + bob_)) * scale + jy;
    fog = 0;
    if (rz > 55.f) fog = std::clamp(int((rz - 55.f) / 16.f), 0, 14);
    return true;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -12 || cy + h * 0.5f < -12 || cx - w * 0.5f > gs::SCREEN_W + 12 ||
        cy - h * 0.5f > gs::SCREEN_H + 12)
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
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::drawFairway() {
    gs::VDP& v = sys_->vdp;
    const float hor = horizon();
    const float S = std::sin(heading_);
    const float C = std::cos(heading_);
    const float camX = x_ - S * back();
    const float camZ = z_ - C * back();
    const float camH = eye() + bob_;
    const float jx = shake_ * std::sin(t_ * 47.f) * 5.f;
    const uint16_t zenith = gs::rgb4(2, 4, 9);
    const uint16_t mid = gs::rgb4(6, 8, 12);
    const uint16_t haze = gs::rgb4(14, 11, 8);
    const uint16_t deep = gs::rgb4(1, 2, 5);
    v.roadTime = int(t_ * 24.f);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (float(y) < hor) {
            float u = float(y) / std::max(hor, 1.f);
            v.lineBackdrop[y] = u < 0.55f ? lerpC(zenith, mid, u / 0.55f) : lerpC(mid, haze, (u - 0.55f) / 0.45f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        // Fairway edges on this scanline, in camera-right units.
        float row = std::max(1.f, float(y) - hor);
        float dist = camH * kFocal / row;
        float x0 = camX + S * dist;
        float z0 = camZ + C * dist;
        float c0 = courseX(z0);
        float c1 = (courseX(z0 + 1.6f) - courseX(z0 - 1.6f)) / 3.2f;
        float half = courseHalf(z0);
        float e0 = x0 - c0;
        float denom = C + c1 * S;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.pal = uint8_t(PAL_HARBOR);
        r.left = r.right = gs::GROUND_LAND;
        r.style = 2;
        r.v = z0 * 36.f + t_ * 22.f;
        r.band = (int(std::floor(z0 * 0.32f)) & 1) ? 1 : 0;
        if (std::fabs(denom) < 0.05f) {
            if (std::fabs(e0) <= half) {
                r.cx = 160.f + jx;
                r.hw = 900.f;
            } else {
                r.cx = -4000.f;
                r.hw = 2.f;
            }
        } else {
            float uA = (half - e0) / denom;
            float uB = (-half - e0) / denom;
            float midU = 0.5f * (uA + uB);
            float halfU = 0.5f * std::fabs(uA - uB);
            float scl = kFocal / std::max(dist, 0.4f);
            r.cx = 160.f + midU * scl + jx;
            r.hw = std::min(4000.f, halfU * scl);
        }
        int fog = 0;
        if (dist > 50.f) fog = std::clamp(int((dist - 50.f) / 26.f), 0, 11);
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = deep;
    }
}

void Game::drawWorld() {
    struct Item {
        float rz;
        float sx, sy, sh;
        const gs::Mipped* img;
        int pal;
        int fog;
        bool shadow;
    };
    std::vector<Item> items;
    items.reserve(160);
    auto push = [&](float wx, float wy, float wz, const gs::Mipped& img, float worldH, int pal, bool shadow, float rzBias) {
        float sx, sy, scale;
        int fog;
        float rz;
        if (!project(wx, wy, wz, sx, sy, scale, fog, rz)) return;
        float sh = worldH * scale;
        if (sh < 2.f || rz > 200.f) return;
        items.push_back(Item{rz + rzBias, sx, sy, sh, &img, pal, fog, shadow});
    };

    for (const Prop& p : props_) {
        switch (p.kind) {
        case Kind::BuoyP:
        case Kind::BuoyS: push(p.x, p.h * 0.45f, p.z, art_.buoy, p.h, p.pal, false, 0); break;
        case Kind::Board: push(p.x, p.h * 0.5f, p.z, art_.board[p.num], p.h, p.pal, false, 0); break;
        case Kind::Lamp: push(p.x, p.h * 0.5f, p.z, art_.lamp, p.h, p.pal, false, 0); break;
        case Kind::Shed: push(p.x, p.h * 0.5f, p.z, art_.shed, p.h, p.pal, false, 0); break;
        case Kind::Crane: push(p.x, p.h * 0.5f, p.z, art_.crane, p.h, p.pal, false, 0); break;
        case Kind::Bollard: push(p.x, p.h * 0.5f, p.z, art_.bollard, p.h, p.pal, false, 0); break;
        }
    }

    int flap = int(t_ * 4.2f) & 1;
    for (const Gull& g : gulls_) {
        float gx = g.x + std::sin(t_ * 0.35f + g.ph) * 7.f;
        float gz = g.z + std::cos(t_ * 0.22f + g.ph) * 5.f;
        float gy = g.y + std::sin(t_ * 1.3f + g.ph) * 0.4f;
        push(gx, gy, gz, art_.gull[flap], 0.85f, PAL_BIRD, false, 0);
    }

    float rzv = rivalZ();
    float rturn = wrap(courseHeading(rzv + 10.f) - courseHeading(rzv - 10.f));
    float rlist = std::clamp(-rturn * 0.85f, -0.55f, 0.55f);
    push(courseX(rzv), kBoatH * 0.48f, rzv, art_.stern[frameOf(rlist)], kBoatH * 0.92f, PAL_RIVAL, false, 0.2f);

    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        float h = 0.55f + (1.f - w.life) * 1.3f;
        push(w.x, 0.1f, w.z, art_.foam, h, PAL_FOAM, false, 0);
    }
    for (const Puff& p : smokes_) {
        if (p.life <= 0.f) continue;
        float h = 0.7f + (1.f - p.life) * 1.4f;
        push(p.x, p.y, p.z, art_.smoke, h, PAL_FOAM, false, -0.35f);
    }

    float bob = std::sin(t_ * 2.3f) * 0.05f;
    bool tipped = mode_ == Mode::Fail && why_ && why_[0] == 't' && why_[1] == 'i';
    push(x_, 0.08f, z_, art_.shadow, 1.5f, PAL_TUG, true, 0.9f);
    if (tipped) {
        push(x_, 0.85f + bob, z_, art_.wreck, 2.8f, PAL_TUG, false, 0);
        push(x_ + 0.9f, 0.3f, z_ - 0.3f, art_.spray, 1.2f, PAL_FOAM, false, -0.2f);
    } else {
        push(x_, kBoatH * 0.5f + bob, z_, art_.stern[frameOf(heel_)], kBoatH, PAL_TUG, false, 0);
        if (std::fabs(heel_) > 0.32f && (mode_ == Mode::Run || mode_ == Mode::Title)) {
            float sgn = heel_ >= 0.f ? 1.f : -1.f;
            float sx = x_ + std::cos(heading_) * sgn * 1.35f;
            float sz = z_ - std::sin(heading_) * sgn * 1.35f;
            push(sx, 0.28f, sz, art_.spray, 0.9f, PAL_FOAM, false, -0.15f);
        }
        if (std::fabs(speed_) > 2.f) {
            float sx = x_ - std::sin(heading_) * 1.7f;
            float sz = z_ - std::cos(heading_) * 1.7f;
            push(sx, 0.12f, sz, art_.foam, 0.75f, PAL_FOAM, false, -0.1f);
        }
    }

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.rz < b.rz; });
    for (const Item& it : items) spr(*it.img, it.sx, it.sy, it.sh, it.pal, it.fog, it.shadow);
}

void Game::drawSky() {
    float hor = horizon();
    spr(art_.sun, 268.f, hor - 26.f, 18.f, PAL_SKY, 0, false);
    spr(art_.cloud, 48.f + std::sin(t_ * 0.12f) * 8.f, 26.f, 30.f, PAL_SKY, 0, false);
    spr(art_.cloud, 196.f + std::cos(t_ * 0.1f) * 10.f, 38.f, 20.f, PAL_SKY, 2, false);
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
    char buf[80];
    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, 0, false); };

    if (mode_ == Mode::Title) {
        banner(art_.title, 160.f, 20.f, PAL_BANNER);
        hudC(5, "MAKE THEM WITHOUT TIPPING", PAL_HUD);
        hudC(6, "THE CLOCK IS THE OTHER CREW", PAL_BANNER);
        hudC(7, "EASE THE HELM OR SHE GOES OVER", PAL_TAG);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "ARROWS HELM   UP AHEAD   DOWN ASTERN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 46.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        bool tipped = why_ && why_[0] == 't' && why_[1] == 'i';
        bool crew = why_ && why_[0] == 't' && why_[1] == 'h';
        banner(tipped ? art_.tipped : crew ? art_.beaten : art_.missed, 160.f, 40.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.steady, 160.f, 36.f, PAL_WIN);
    }

    hud(1, 0, "S3 TUGBOAT TURN", PAL_BANNER);
    int sec = std::max(0, int(std::ceil(crewLeft() - 1e-3f)));
    std::snprintf(buf, sizeof buf, "CREW %02d", sec);
    int crewPal = crewLeft() < 12.f ? PAL_ALERT : PAL_TAG;
    hud(32, 0, buf, crewPal);

    if (mode_ == Mode::Pause) {
        hudC(16, "START CONTINUES", PAL_HUD);
        hudC(17, "MODE RETURNS", PAL_TAG);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "AHEAD OF THE OTHER CREW  %.1fS", race_);
        hudC(8, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "CREW HAD %.0fS LEFT", crewLeft());
        hudC(9, buf, PAL_WIN);
        if (!bot_) hudC(26, "START RUNS IT AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(8, why_, PAL_ALERT);
        std::snprintf(buf, sizeof buf, "LIST %.0f   TURNS %d/3", heel_ * 57.2958f, turns_);
        hudC(9, buf, PAL_TAG);
        if (!bot_) hudC(26, "START TRIES AGAIN", PAL_HUD);
        return;
    }

    const char* line = "HOLD HER STEADY";
    int pal = PAL_WIN;
    int b = bendAt(z_);
    if (std::fabs(heel_) > 0.62f) {
        line = "EASE THE HELM";
        pal = PAL_ALERT;
    } else if (crewLeft() < 12.f) {
        line = "THE OTHER CREW IS CLOSING";
        pal = PAL_ALERT;
    } else if (b == 0) {
        line = "PORT TURN 1";
        pal = PAL_BANNER;
    } else if (b == 1) {
        line = "STARBOARD TURN 2";
        pal = PAL_BANNER;
    } else if (b == 2) {
        line = "PORT TURN 3";
        pal = PAL_BANNER;
    } else if (turns_ > 0) {
        std::snprintf(buf, sizeof buf, "TURN %d MADE", turns_);
        line = buf;
        pal = PAL_WIN;
    } else {
        line = "THREE TURNS TO THE BERTH";
        pal = PAL_TAG;
    }
    hud(1, 1, line, pal);
    std::snprintf(buf, sizeof buf, "SPD %.0f", std::fabs(speed_));
    hud(33, 1, buf, PAL_TAG);

    char bar[12];
    for (int i = 0; i < 11; i++) bar[i] = '-';
    bar[11] = 0;
    float n = std::clamp(heel_ / kTip, -1.f, 1.f);
    int at = int(std::lround((n + 1.f) * 0.5f * 10.f));
    at = std::clamp(at, 0, 10);
    bar[at] = 'O';
    int listPal = std::fabs(n) > 0.62f ? PAL_ALERT : PAL_HUD;
    std::snprintf(buf, sizeof buf, "LIST P%sS %+0.0f", bar, heel_ * 57.2958f);
    hud(1, 2, buf, listPal);
    std::snprintf(buf, sizeof buf, "%d/3", turns_);
    hud(37, 2, buf, PAL_TAG);
    hud(1, 26, "ARROWS HELM   UP AHEAD   DOWN ASTERN", PAL_HUD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    drawFairway();
    drawHud();
    drawWorld();
    drawSky();
}

}  // namespace tugturn
