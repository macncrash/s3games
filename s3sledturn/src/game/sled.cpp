#include "sled.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace sledturn {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kTip = 1.02f;
constexpr float kLeanK = 8.2f;
constexpr float kLeanDamp = 5.4f;
constexpr float kCapFwd = 12.6f;
constexpr float kCapRev = 2.4f;
constexpr float kYawBase = 0.42f;
constexpr float kYawPer = 0.052f;
constexpr float kClock = 96.f;
constexpr float kFocal = 270.f;
constexpr float kSledH = 2.85f;

// Three hooks. Each opens straight, bends, and is judged on the straight after.
struct Bend {
    float z0, z1, x0, x1, exitZ;
    int dir;
};

constexpr Bend kBends[3] = {
    {46.f, 118.f, 0.f, 22.f, 142.f, 1},
    {168.f, 246.f, 22.f, -26.f, 272.f, -1},
    {300.f, 378.f, -26.f, 12.f, 404.f, 1},
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
    if (z <= kBends[0].z0) return kBends[0].x0;
    for (int i = 0; i < 3; i++) {
        const Bend& b = kBends[i];
        if (z < b.z1) {
            if (z < b.z0) return b.x0;
            float t = (z - b.z0) / (b.z1 - b.z0);
            return b.x0 + (b.x1 - b.x0) * smooth(t);
        }
    }
    return kBends[2].x1;
}

float courseHeading(float z) {
    float dx = courseX(z + 2.6f) - courseX(z - 2.6f);
    return std::atan2(dx, 5.2f);
}

float courseHalf(float z) {
    float h = 9.2f;
    for (const Bend& b : kBends) {
        if (z > b.z0 && z < b.z1) {
            float t = (z - b.z0) / (b.z1 - b.z0);
            h += 2.8f * std::sin(t * kPi);
        }
    }
    if (z < 24.f) h += (24.f - z) * 0.16f;
    return h;
}

bool inBend(float z) {
    for (const Bend& b : kBends)
        if (z >= b.z0 && z <= b.z1) return true;
    return false;
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

float Game::eye() const { return mode_ == Mode::Title ? 5.7f : 4.55f; }
float Game::back() const { return mode_ == Mode::Title ? 13.4f : 8.7f; }
float Game::horizon() const { return mode_ == Mode::Title ? 68.f : 78.f; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (turns_ >= 2) return 3;
    if (turns_ >= 1 || inBend(z_)) return 2;
    return 1;
}

int Game::leanFrame() const {
    float u = std::clamp(lean_ / 0.86f, -1.f, 1.f);
    int i = int(std::lround((u + 1.f) * 0.5f * 6.f));
    return std::clamp(i, 0, 6);
}

const char* Game::tipWhy() const {
    int n = 1;
    if (z_ >= kBends[0].exitZ) n = 2;
    if (z_ >= kBends[1].exitZ) n = 3;
    if (n == 1) return "tipped on the first turn";
    if (n == 2) return "tipped on the second turn";
    return "tipped on the third turn";
}

void Game::seedFlakes() {
    for (int i = 0; i < 22; i++) {
        flakes_[i].x = float(hashN(i * 3 + 1) % 320);
        flakes_[i].y = float(hashN(i * 5 + 2) % 224);
        flakes_[i].s = 2.f + float(hashN(i + 9) % 3);
        flakes_[i].v = 16.f + float(hashN(i * 7 + 4) % 20);
    }
}

void Game::begin() {
    z_ = 14.f;
    x_ = courseX(z_);
    heading_ = 0.f;
    speed_ = 0.f;
    throttle_ = 0.f;
    yaw_ = 0.f;
    lean_ = 0.f;
    leanVel_ = 0.f;
    race_ = 0.f;
    shake_ = 0.f;
    puffT_ = 0.f;
    puffN_ = 0;
    turns_ = 0;
    won_ = false;
    over_ = false;
    warned_ = false;
    setting_ = false;
    why_ = "";
    chimeN_ = 0;
    chimeStep_ = 0;
    chimeT_ = 0.f;
    tone0_ = 0.f;
    tone1_ = 0.f;
    for (int i = 0; i < 3; i++) made_[i] = false;
    for (Puff& p : puffs_) p = {};
    seedFlakes();
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    z_ = 0.5f * (kBends[0].z0 + kBends[0].z1);
    x_ = courseX(z_);
    heading_ = courseHeading(z_);
    lean_ = 0.48f;
    speed_ = 8.f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    blip(620.f);
}

void Game::buildCourse() {
    props_.clear();
    props_.reserve(140);
    for (int i = 0; i < 26; i++) {
        float z = 8.f + float(i) * 16.f;
        float c = courseX(z);
        float h = courseHalf(z);
        props_.push_back(Prop{c + h + 0.35f, z, 1.7f, Kind::StakeR, PAL_RED, 0});
        props_.push_back(Prop{c - h - 0.35f, z + 8.f, 1.7f, Kind::StakeL, PAL_BLUE, 0});
    }
    for (int i = 0; i < 34; i++) {
        float z = 12.f + float(i) * 12.f;
        float c = courseX(z);
        float h = courseHalf(z);
        int n = hashN(i * 19 + 5);
        float j = float(n % 100) / 100.f;
        float side = (i & 1) ? 1.f : -1.f;
        props_.push_back(Prop{c + side * (h + 2.4f + j * 2.6f), z + float(n % 5) * 0.3f, 4.4f + j * 1.4f, Kind::Tree,
                               PAL_TREE, 0});
    }
    for (int i = 0; i < 3; i++) {
        const Bend& b = kBends[i];
        float z = 0.5f * (b.z0 + b.z1);
        float side = float(-b.dir);
        props_.push_back(Prop{courseX(z) + side * (courseHalf(z) + 2.6f), z, 2.35f, Kind::Sign, PAL_SIGN, i});
        float ez = b.exitZ;
        props_.push_back(Prop{courseX(ez) + courseHalf(ez) + 0.2f, ez, 2.5f, Kind::Post, PAL_RED, 0});
        props_.push_back(Prop{courseX(ez) - courseHalf(ez) - 0.2f, ez, 2.5f, Kind::Post, PAL_BLUE, 0});
    }
    props_.push_back(Prop{courseHalf(28.f) + 4.2f, 28.f, 3.1f, Kind::Cabin, PAL_CABIN, 0});
    props_.push_back(Prop{courseX(210.f) - courseHalf(210.f) - 3.4f, 210.f, 1.5f, Kind::Cache, PAL_CABIN, 0});

    ravens_[0] = Raven{10.f, 70.f, 7.2f, 0.4f};
    ravens_[1] = Raven{-16.f, 190.f, 8.f, 1.8f};
    ravens_[2] = Raven{8.f, 340.f, 6.6f, 3.1f};
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildCourse();
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.12f, 0.22f, 0.12f);
    if (bot_) startRun();
    else showTitle();
}

void Game::controls(float& steer, float& throttle, bool& set) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(p.axisX, -1.f, 1.f);
    const bool up = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_Y);
    const bool down = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    if (up) throttle_ = std::min(1.f, throttle_ + kDt * 0.95f);
    if (down) throttle_ = std::max(-0.55f, throttle_ - kDt * 1.35f);
    if (!up && !down) throttle_ *= std::exp(-0.45f * kDt);
    if (p.axisY > 0.25f) throttle_ = std::max(throttle_, p.axisY);
    if (p.axisY < -0.25f) throttle_ = std::min(throttle_, p.axisY * 0.55f);
    if (p.accel > 0.08f) throttle_ = std::max(throttle_, p.accel);
    if (p.brake > 0.08f) throttle_ = std::min(throttle_, -p.brake * 0.5f);
    throttle = throttle_;
    set = p.down(gs::BTN_A) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO);
}

void Game::pilot(float& steer, float& throttle, bool& set) {
    float look = std::clamp(15.f + speed_ * 0.35f, 15.f, 24.f);
    float ahead = courseX(z_ + look);
    float off = x_ - courseX(z_);
    float half = courseHalf(z_);
    float hDes = std::atan2(ahead - x_, look);
    hDes -= std::clamp(off * 0.075f, -0.42f, 0.42f);
    float err = wrap(hDes - heading_);
    float cmd = std::clamp(err / 0.42f, -1.f, 1.f);

    float curv = std::fabs(wrap(courseHeading(z_ + 22.f) - courseHeading(z_)));
    float want = 10.6f;
    if (curv > 0.12f) want = 7.8f;
    if (curv > 0.30f) want = 6.5f;
    if (std::fabs(off) > half * 0.48f) want = std::min(want, 5.8f);
    if (std::fabs(lean_) > 0.48f) want = std::min(want, 6.0f);

    float cap = 0.78f;
    if (speed_ > 10.f) cap = 0.42f;
    else if (speed_ > 8.2f) cap = 0.60f;
    if (std::fabs(lean_) > 0.55f) {
        bool adding = (cmd > 0.f && lean_ < 0.f) || (cmd < 0.f && lean_ > 0.f);
        if (adding) cmd *= 0.35f;
        cap = std::min(cap, 0.48f);
    }
    steer = std::clamp(cmd, -cap, cap);
    throttle = std::clamp((want - speed_) * 0.42f, -0.30f, 0.92f);
    if (std::fabs(lean_) > 0.52f && throttle < 0.f) throttle = 0.f;
    set = std::fabs(lean_) > 0.58f;
}

void Game::markTurns(float zPrev) {
    for (int i = 0; i < 3; i++) {
        if (made_[i]) continue;
        if (z_ < kBends[i].exitZ) break;
        if (i > 0 && !made_[i - 1]) {
            fail("skipped a turn");
            return;
        }
        if (zPrev >= kBends[i].exitZ) {
            fail("skipped a turn");
            return;
        }
        float off = std::fabs(x_ - courseX(z_));
        float herr = std::fabs(wrap(heading_ - courseHeading(z_)));
        bool inside = off <= courseHalf(z_) + 0.8f;
        bool aligned = herr <= 0.95f;
        bool moving = speed_ > 2.1f;
        if (!inside || !aligned || !moving) {
            if (i == 0) fail("missed the first turn");
            else if (i == 1) fail("missed the second turn");
            else fail("missed the third turn");
            return;
        }
        made_[i] = true;
        turns_ = i + 1;
        if (i == 2) {
            win();
            return;
        }
        blip(500.f + float(i) * 120.f);
        sys_->rumble(0.16f, 0.08f, 80);
    }
}

void Game::physics(float steer, float throttle, bool set) {
    if (mode_ != Mode::Run) return;
    steer = std::clamp(steer, -1.f, 1.f);
    throttle = std::clamp(throttle, -1.f, 1.f);
    setting_ = set;
    float zPrev = z_;

    float used = steer;
    if (set) used *= 0.58f;
    float rate = kYawBase + std::min(std::fabs(speed_), kCapFwd) * kYawPer;
    if (std::fabs(speed_) < 2.4f) rate *= 0.55f;
    float yawCmd = used * rate * (speed_ >= 0.f ? 1.f : 0.45f);
    yaw_ += (yawCmd - yaw_) * (1.f - std::exp(-8.f * kDt));
    heading_ = wrap(heading_ + yaw_ * kDt);

    float cap = throttle >= 0.f ? kCapFwd : kCapRev;
    float target = throttle * cap;
    speed_ += (target - speed_) * (1.f - std::exp(-2.4f * kDt));
    speed_ *= 1.f - std::fabs(steer) * std::min(std::fabs(speed_), 12.f) * 0.0035f;
    speed_ = std::clamp(speed_, -kCapRev, kCapFwd);

    x_ += std::sin(heading_) * speed_ * kDt;
    z_ += std::cos(heading_) * speed_ * kDt;
    if (z_ < 2.f) {
        z_ = 2.f;
        if (speed_ < 0.f) speed_ = 0.f;
    }

    float spd = std::max(0.f, speed_);
    float demand = -steer * (0.26f + std::min(spd, 12.f) * 0.060f);
    demand -= yaw_ * spd * 0.045f;
    if (set) demand *= 0.38f;
    if (throttle < -0.2f && std::fabs(lean_) > 0.34f) {
        float sgn = lean_ > 0.f ? 1.f : -1.f;
        demand += sgn * (-throttle) * 0.72f;
    }

    float off = x_ - courseX(z_);
    float over = std::fabs(off) - courseHalf(z_);
    if (over > 0.f) {
        float sgn = off > 0.f ? 1.f : -1.f;
        x_ -= sgn * std::min(over, 0.45f);
        speed_ *= std::max(0.68f, 1.f - std::min(over, 2.5f) * 0.05f);
        demand += sgn * (1.15f + std::min(over, 2.f) * 0.45f);
    }

    float slide = lean_ * std::min(std::fabs(speed_), 12.f) * 0.04f;
    x_ += std::cos(heading_) * slide * kDt;
    z_ -= std::sin(heading_) * slide * kDt;

    leanVel_ += ((demand - lean_) * kLeanK - leanVel_ * kLeanDamp) * kDt;
    lean_ += leanVel_ * kDt;
    lean_ = std::clamp(lean_, -1.45f, 1.45f);
    if (std::fabs(lean_) > kTip) {
        fail(tipWhy());
        return;
    }

    if (std::fabs(lean_) > 0.64f && !warned_) {
        warned_ = true;
        blip(150.f);
    } else if (std::fabs(lean_) < 0.38f) {
        warned_ = false;
    }

    markTurns(zPrev);
    if (mode_ != Mode::Run) return;
    if (z_ > kBends[2].exitZ + 28.f) {
        fail("missed the turns");
        return;
    }

    puffT_ -= kDt;
    if (puffT_ <= 0.f && std::fabs(speed_) > 2.4f) {
        puffT_ = 0.07f;
        Puff w;
        w.x = x_ - std::sin(heading_) * 1.7f;
        w.z = z_ - std::cos(heading_) * 1.7f;
        w.life = 1.f;
        puffs_[puffN_] = w;
        puffN_ = (puffN_ + 1) % 10;
    }
    for (Puff& w : puffs_)
        if (w.life > 0.f) w.life -= kDt * 0.9f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "upright";
    chime();
    sys_->rumble(0.30f, 0.16f, 200);
    sys_->setLight(40, 180, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.5f, 80.f, 0.42f);
    sys_->apu.tone(0, 58.f, 0.08f);
    tone0_ = 0.5f;
    sys_->rumble(0.75f, 0.25f, 240);
    sys_->setLight(190, 30, 20);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.09f;
}

void Game::chime() {
    chimeN_ = 4;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio() {
    float hiss = mode_ == Mode::Run ? 0.012f + std::fabs(speed_) * 0.0011f : 0.008f;
    sys_->apu.noise(hiss, 900.f + std::fabs(speed_) * 40.f, false);
    if (mode_ == Mode::Run && (std::fabs(throttle_) > 0.05f || std::fabs(speed_) > 1.8f)) {
        float wob = 0.75f + 0.25f * std::sin(t_ * (8.f + std::fabs(speed_) * 0.7f));
        float vol = (0.012f + std::min(std::fabs(speed_), 12.f) * 0.002f) * wob;
        sys_->apu.tone(2, 70.f + std::fabs(speed_) * 3.2f, vol);
    } else if (tone0_ <= 0.f) {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && tone0_ <= 0.f) {
        if (std::fabs(lean_) > 0.58f) {
            float v = (std::fabs(lean_) - 0.58f) * 0.18f;
            sys_->apu.tone(0, 110.f + std::fabs(lean_) * 70.f, v);
        } else {
            sys_->apu.tone(0, 0.f, 0.f);
        }
    }
    if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f && mode_ != Mode::Run) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= kDt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 494.f, 587.f, 784.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 3)], 0.06f);
            tone0_ = 0.18f;
            chimeT_ = 0.16f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.5f);
    for (Flake& f : flakes_) {
        f.y += f.v * kDt;
        f.x += std::sin(t_ * 0.7f + f.y * 0.02f) * 10.f * kDt;
        if (f.y > 230.f) {
            f.y = -6.f;
            f.x = float(hashN(int(t_ * 60.f) + int(f.x)) % 320);
        }
        if (f.x < -4.f) f.x += 328.f;
        if (f.x > 324.f) f.x -= 328.f;
    }
    camBob_ = std::sin(t_ * 2.4f) * (mode_ == Mode::Run ? 0.04f + std::fabs(speed_) * 0.004f : 0.03f);
    if (mode_ == Mode::Title) lean_ = 0.42f * std::sin(t_ * 0.85f);

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(400.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            float steer = 0.f, thr = throttle_;
            bool set = false;
            if (bot_) pilot(steer, thr, set);
            else controls(steer, thr, set);
            throttle_ = thr;
            physics(steer, thr, set);
            if (mode_ == Mode::Run && race_ >= kClock) fail("missed the turns");
            if (mode_ == Mode::Run) {
                float ah = std::fabs(lean_);
                if (ah > kTip * 0.78f) sys.setLight(200, 40, 20);
                else if (ah > 0.45f) sys.setLight(180, 110, 30);
                else if (turns_ > 0) sys.setLight(40, 150, 70);
                else sys.setLight(40, 70, 120);
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
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
    if (rz < 1.2f) return false;
    scale = kFocal / rz;
    sx = 160.f + rx * scale;
    sy = horizon() - (wy - (eye() + camBob_)) * scale;
    fog = 0;
    if (rz > 52.f) fog = std::clamp(int((rz - 52.f) / 16.f), 0, 12);
    return true;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -8 || cy + h * 0.5f < -8 || cx - w * 0.5f > gs::SCREEN_W + 8 || cy - h * 0.5f > gs::SCREEN_H + 8)
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

void Game::drawTrail() {
    gs::VDP& v = sys_->vdp;
    const float hor = horizon();
    const float S = std::sin(heading_);
    const float C = std::cos(heading_);
    const float camX = x_ - S * back();
    const float camZ = z_ - C * back();
    const float camH = eye() + camBob_;
    const uint16_t zenith = gs::rgb4(4, 6, 11);
    const uint16_t mid = gs::rgb4(8, 10, 14);
    const uint16_t haze = gs::rgb4(14, 11, 8);
    const uint16_t deep = gs::rgb4(8, 10, 13);
    v.roadTime = int(t_ * 20.f);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (float(y) < hor) {
            float u = float(y) / std::max(hor, 1.f);
            v.lineBackdrop[y] = u < 0.55f ? lerpC(zenith, mid, u / 0.55f) : lerpC(mid, haze, (u - 0.55f) / 0.45f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = std::max(1.f, float(y) - hor);
        float dist = camH * kFocal / row;
        float x0 = camX + S * dist;
        float z0 = camZ + C * dist;
        float c0 = courseX(z0);
        float c1 = (courseX(z0 + 1.5f) - courseX(z0 - 1.5f)) / 3.f;
        float half = courseHalf(z0);
        float e0 = x0 - c0;
        float denom = C + c1 * S;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.pal = uint8_t(PAL_TRAIL);
        r.left = r.right = gs::GROUND_SNOWWALL;
        r.style = gs::ROAD_SNOW;
        r.v = z0 * 28.f;
        r.band = (int(std::floor(z0 * 0.24f)) & 1) ? 1 : 0;
        if (std::fabs(denom) < 0.045f) {
            if (std::fabs(e0) <= half) {
                r.cx = 160.f;
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
            r.cx = 160.f + midU * scl;
            r.hw = std::min(4000.f, halfU * scl);
        }
        int fog = 0;
        if (dist > 70.f) fog = std::clamp(int((dist - 70.f) / 28.f), 0, 11);
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
    items.reserve(180);
    auto push = [&](float wx, float wy, float wz, const gs::Mipped& img, float worldH, int pal, bool shadow, float rzBias) {
        float sx, sy, scale;
        int fog;
        float rz;
        if (!project(wx, wy, wz, sx, sy, scale, fog, rz)) return;
        float sh = worldH * scale;
        if (sh < 1.4f || rz > 230.f) return;
        items.push_back(Item{rz + rzBias, sx, sy, sh, &img, pal, fog, shadow});
    };

    for (const Prop& p : props_) {
        switch (p.kind) {
        case Kind::StakeR:
        case Kind::StakeL: push(p.x, p.h * 0.5f, p.z, art_.stake, p.h, p.pal, false, 0); break;
        case Kind::Tree: push(p.x, p.h * 0.5f, p.z, art_.spruce, p.h, p.pal, false, 0); break;
        case Kind::Cabin: push(p.x, p.h * 0.5f, p.z, art_.cabin, p.h, p.pal, false, 0); break;
        case Kind::Cache: push(p.x, p.h * 0.5f, p.z, art_.cache, p.h, p.pal, false, 0); break;
        case Kind::Sign: push(p.x, p.h * 0.5f, p.z, art_.sign[p.num], p.h, p.pal, false, 0); break;
        case Kind::Post: push(p.x, p.h * 0.5f, p.z, art_.post, p.h, p.pal, false, 0); break;
        }
    }

    int step = int(t_ * (2.f + std::fabs(speed_) * 0.55f)) & 1;
    float S = std::sin(heading_);
    float C = std::cos(heading_);
    for (int i = 0; i < 4; i++) {
        float lane = (float(i) - 1.5f) * 0.78f;
        float lead = 3.5f + float(i & 1) * 1.05f;
        float bob = std::sin(t_ * 10.f + float(i) * 1.3f) * 0.04f;
        float wx = x_ + S * lead + C * lane;
        float wz = z_ + C * lead - S * lane;
        push(wx, 0.62f + bob, wz, art_.dog[(step + i) & 1], 1.15f, PAL_DOG, false, 0);
    }

    int flap = int(t_ * 5.f) & 1;
    for (const Raven& g : ravens_) {
        float gx = g.x + std::sin(t_ * 0.45f + g.ph) * 5.f;
        float gz = g.z + std::cos(t_ * 0.3f + g.ph) * 4.f;
        float gy = g.y + std::sin(t_ * 1.6f + g.ph) * 0.3f;
        push(gx, gy, gz, art_.raven[flap], 0.7f, PAL_BIRD, false, 0);
    }

    for (const Puff& w : puffs_) {
        if (w.life <= 0.f) continue;
        float h = 0.45f + (1.f - w.life) * 0.9f;
        push(w.x, 0.15f, w.z, art_.spray, h, PAL_SNOW, false, 0);
    }

    float bob = std::sin(t_ * 2.6f) * 0.04f;
    float shake = shake_ * std::sin(t_ * 46.f) * 0.22f;
    bool tipped = mode_ == Mode::Fail && why_ && why_[0] == 't';
    push(x_ + shake, 0.08f, z_, art_.shadow, 1.5f, PAL_SLED, true, 0.9f);
    if (tipped) {
        push(x_, 0.85f, z_, art_.wreck, 2.4f, PAL_SLED, false, 0);
        push(x_ + 1.1f, 0.3f, z_, art_.spray, 1.1f, PAL_SNOW, false, -0.2f);
    } else {
        push(x_ + shake, 1.42f + bob, z_, art_.sled[leanFrame()], kSledH, PAL_SLED, false, 0);
        if (std::fabs(lean_) > 0.36f && (mode_ == Mode::Run || mode_ == Mode::Title)) {
            float side = lean_ > 0.f ? 1.f : -1.f;
            float sx = x_ + C * side * 1.05f;
            float sz = z_ - S * side * 1.05f;
            push(sx, 0.28f, sz, art_.spray, 0.8f, PAL_SNOW, false, -0.15f);
        }
    }

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.rz < b.rz; });
    for (const Item& it : items) spr(*it.img, it.sx, it.sy, it.sh, it.pal, it.fog, it.shadow);
}

void Game::drawSky() {
    float sunY = horizon() - 22.f;
    spr(art_.sun, 262.f, sunY, 22.f, PAL_SKY, 0, false);
    spr(art_.cloud, 54.f + std::sin(t_ * 0.12f) * 8.f, 26.f, 30.f, PAL_SKY, 0, false);
    spr(art_.cloud, 176.f + std::cos(t_ * 0.1f) * 10.f, 38.f, 20.f, PAL_SKY, 2, false);
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
        banner(art_.title, 160.f, 22.f, PAL_BANNER);
        hudC(6, "MAKE THE THREE TURNS", PAL_HUD);
        hudC(7, "WITHOUT TIPPING", PAL_BANNER);
        hudC(8, "EASE THE BAR IN THE BENDS", PAL_TAG);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "ARROWS STEER   UP KICK   Z SETS", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        banner(art_.paused, 160.f, 48.f, PAL_BANNER);
    } else if (mode_ == Mode::Fail) {
        bool tipped = why_ && why_[0] == 't';
        banner(tipped ? art_.tipped : art_.missed, 160.f, 40.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.upright, 160.f, 36.f, PAL_WIN);
    }

    for (const Flake& f : flakes_) spr(art_.flake, f.x, f.y, f.s + 1.f, PAL_SNOW, 0, false);

    if (mode_ == Mode::Title) return;

    hud(1, 0, "S3 SLED TURN", PAL_BANNER);
    if (mode_ == Mode::Win) std::snprintf(buf, sizeof buf, "MADE 3/3");
    else std::snprintf(buf, sizeof buf, "MADE %d/3", turns_);
    hud(31, 0, buf, PAL_TAG);

    if (mode_ == Mode::Pause) {
        hudC(16, "START CONTINUES", PAL_HUD);
        hudC(17, "MODE RETURNS", PAL_TAG);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "WITHOUT TIPPING  %.1fS", race_);
        hudC(8, buf, PAL_HUD);
        if (!bot_) hudC(26, "START RUNS IT AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(8, why_, PAL_ALERT);
        std::snprintf(buf, sizeof buf, "LEAN %.0f", lean_ * 57.2958f);
        hudC(9, buf, PAL_TAG);
        if (!bot_) hudC(26, "START TRIES AGAIN", PAL_HUD);
        return;
    }

    int show = std::min(turns_ + 1, 3);
    const char* line = "HOLD HER UPRIGHT";
    int pal = PAL_WIN;
    if (inBend(z_)) {
        std::snprintf(buf, sizeof buf, "TURN %d OF 3", show);
        line = buf;
        pal = PAL_BANNER;
    } else if (turns_ > 0) {
        std::snprintf(buf, sizeof buf, "TURN %d MADE", turns_);
        line = buf;
        pal = PAL_WIN;
    }
    if (std::fabs(lean_) > kTip * 0.72f) {
        line = setting_ ? "WEIGHT SET" : "EASE THE BAR";
        pal = PAL_ALERT;
    }
    hud(1, 1, line, pal);
    std::snprintf(buf, sizeof buf, "SPD %.0f", std::fabs(speed_));
    hud(33, 1, buf, PAL_TAG);

    char bar[12];
    for (int i = 0; i < 11; i++) bar[i] = '-';
    bar[11] = 0;
    float n = std::clamp(lean_ / kTip, -1.f, 1.f);
    int at = int(std::lround((n + 1.f) * 0.5f * 10.f));
    at = std::clamp(at, 0, 10);
    bar[at] = 'O';
    int leanPal = std::fabs(n) > 0.75f ? PAL_ALERT : PAL_HUD;
    std::snprintf(buf, sizeof buf, "LEAN L%sR %+0.0f", bar, lean_ * 57.2958f);
    hud(1, 2, buf, leanPal);
    hud(1, 26, "ARROWS STEER   UP KICK   Z SETS", PAL_HUD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    drawTrail();
    drawHud();
    drawWorld();
    drawSky();
}

}  // namespace sledturn
