#include "barge.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace bargeturn {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kTip = 1.02f;
constexpr float kLeanK = 7.4f;
constexpr float kLeanDamp = 5.6f;
constexpr float kCapFwd = 7.4f;
constexpr float kCurrent = 1.15f;
constexpr float kYawBase = 0.28f;
constexpr float kYawPer = 0.038f;
constexpr float kClock = 100.f;
constexpr float kFocal = 270.f;
constexpr float kHullH = 2.55f;

struct Bend {
    float z0, z1, x0, x1, exitZ;
    int dir;
};

constexpr Bend kBends[3] = {
    {52.f, 128.f, 0.f, 18.f, 154.f, 1},
    {184.f, 262.f, 18.f, -20.f, 288.f, -1},
    {318.f, 396.f, -20.f, 10.f, 422.f, 1},
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
    float dx = courseX(z + 3.2f) - courseX(z - 3.2f);
    return std::atan2(dx, 6.4f);
}

float courseHalf(float z) {
    float h = 10.4f;
    for (const Bend& b : kBends) {
        if (z > b.z0 && z < b.z1) {
            float t = (z - b.z0) / (b.z1 - b.z0);
            h += 3.1f * std::sin(t * kPi);
        }
    }
    if (z < 28.f) h += (28.f - z) * 0.12f;
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

int Game::leanFrame() const {
    float u = std::clamp(lean_ / 0.9f, -1.f, 1.f);
    int f = int(std::lround((u + 1.f) * 0.5f * float(kPoses - 1)));
    return std::clamp(f, 0, kPoses - 1);
}

const char* Game::tipWhy() const {
    int n = turns_;
    if (n <= 0) return "tipped before the first turn";
    if (n == 1) return "tipped on the second turn";
    if (n == 2) return "tipped on the third turn";
    return "tipped after the turns";
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (turns_ >= 2) return 3;
    if (turns_ >= 1 || inBend(z_)) return 2;
    return 1;
}

void Game::blip(float freq) { sys_->apu.tone(1, freq, 0.22f); }

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
    turns_ = 0;
    for (bool& m : made_) m = false;
    t_ = 0;
    race_ = 0;
    x_ = 0;
    z_ = 18.f;
    heading_ = 0.12f;
    speed_ = 2.2f;
    throttle_ = 0.3f;
    yaw_ = 0;
    lean_ = 0.28f;
    leanVel_ = 0;
    cargo_ = 0.22f;
    cargoVel_ = 0;
    shake_ = 0;
}

void Game::startRun() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    why_ = "";
    turns_ = 0;
    for (bool& m : made_) m = false;
    race_ = 0;
    x_ = 0;
    z_ = 6.f;
    heading_ = 0;
    speed_ = kCurrent;
    throttle_ = 0.35f;
    yaw_ = 0;
    lean_ = 0;
    leanVel_ = 0;
    cargo_ = 0;
    cargoVel_ = 0;
    shake_ = 0;
    blip(220.f);
}

void Game::win() {
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "";
    blip(660.f);
    sys_->rumble(0.2f, 0.12f, 140);
}

void Game::fail(const char* why) {
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    shake_ = 1.f;
    blip(90.f);
    sys_->rumble(0.55f, 0.3f, 220);
}

void Game::buildCourse() {
    props_.clear();
    props_.reserve(120);
    for (int i = 0; i < 28; i++) {
        float z = 10.f + float(i) * 16.f;
        float c = courseX(z);
        float h = courseHalf(z);
        props_.push_back(Prop{c + h + 0.6f, z, 2.1f, Kind::BuoyR, 0});
        props_.push_back(Prop{c - h - 0.6f, z + 8.f, 2.1f, Kind::BuoyL, 0});
    }
    for (int i = 0; i < 22; i++) {
        float z = 16.f + float(i) * 18.f;
        float c = courseX(z);
        float h = courseHalf(z);
        int n = hashN(i * 17 + 3);
        float j = float(n % 100) / 100.f;
        float side = (i & 1) ? 1.f : -1.f;
        props_.push_back(Prop{c + side * (h + 2.8f + j * 2.2f), z, 1.6f + j * 0.6f, Kind::Reed, 0});
    }
    for (int i = 0; i < 3; i++) {
        const Bend& b = kBends[i];
        float z = 0.5f * (b.z0 + b.z1);
        float side = float(-b.dir);
        props_.push_back(Prop{courseX(z) + side * (courseHalf(z) + 3.2f), z, 2.4f, Kind::Sign, i});
    }
    props_.push_back(Prop{courseX(36.f) + courseHalf(36.f) + 5.f, 36.f, 4.2f, Kind::Mill, 0});
    props_.push_back(Prop{courseX(250.f) - courseHalf(250.f) - 4.6f, 250.f, 5.2f, Kind::Crane, 0});
    gulls_[0] = Gull{8.f, 80.f, 6.4f, 0.2f};
    gulls_[1] = Gull{-12.f, 200.f, 7.1f, 1.4f};
    gulls_[2] = Gull{6.f, 360.f, 5.8f, 2.6f};
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildCourse();
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.16f, 0.28f, 0.14f);
    if (bot_) startRun();
    else showTitle();
}

void Game::controls(float& steer, float& throttle, bool& trim) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(p.axisX, -1.f, 1.f);
    const bool up = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_Y);
    const bool down = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    if (up) throttle_ = std::min(1.f, throttle_ + kDt * 0.7f);
    if (down) throttle_ = std::max(0.f, throttle_ - kDt * 1.1f);
    if (!up && !down) throttle_ += (0.28f - throttle_) * (1.f - std::exp(-0.6f * kDt));
    if (p.axisY > 0.25f) throttle_ = std::max(throttle_, p.axisY);
    if (p.accel > 0.08f) throttle_ = std::max(throttle_, p.accel);
    if (p.brake > 0.08f) throttle_ = std::min(throttle_, 1.f - p.brake);
    throttle = throttle_;
    trim = p.down(gs::BTN_A) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO);
}

void Game::pilot(float& steer, float& throttle, bool& trim) {
    float look = std::clamp(18.f + speed_ * 0.4f, 18.f, 26.f);
    float ahead = courseX(z_ + look);
    float off = x_ - courseX(z_);
    float half = courseHalf(z_);
    float hDes = std::atan2(ahead - x_, look);
    hDes -= std::clamp(off * 0.06f, -0.35f, 0.35f);
    float err = wrap(hDes - heading_);
    float cmd = std::clamp(err / 0.38f, -1.f, 1.f);

    float curv = std::fabs(wrap(courseHeading(z_ + 26.f) - courseHeading(z_)));
    float want = 6.2f;
    if (curv > 0.08f) want = 5.1f;
    if (curv > 0.18f) want = 4.4f;
    if (std::fabs(off) > half * 0.45f) want = std::min(want, 4.2f);
    if (std::fabs(lean_) > 0.42f || std::fabs(cargo_) > 0.35f) want = std::min(want, 4.0f);

    float cap = 0.72f;
    if (speed_ > 6.4f) cap = 0.4f;
    if (std::fabs(lean_) > 0.5f) {
        bool adding = (cmd > 0.f && lean_ < 0.f) || (cmd < 0.f && lean_ > 0.f);
        if (adding) cmd *= 0.3f;
        cap = std::min(cap, 0.42f);
    }
    if (std::fabs(cargo_) > 0.45f) {
        bool adding = (cmd > 0.f && cargo_ > 0.f) || (cmd < 0.f && cargo_ < 0.f);
        if (adding) cmd *= 0.45f;
    }
    steer = std::clamp(cmd, -cap, cap);
    throttle = std::clamp((want - speed_) * 0.55f + 0.35f, 0.05f, 0.9f);
    trim = std::fabs(cargo_) > 0.12f || std::fabs(lean_) > 0.48f;
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
        if (off > courseHalf(z_) + 1.2f || herr > 1.05f || speed_ < 1.6f) {
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
        blip(420.f + float(i) * 90.f);
        sys_->rumble(0.14f, 0.06f, 70);
    }
}

void Game::physics(float steer, float throttle, bool trim) {
    if (mode_ != Mode::Run) return;
    steer = std::clamp(steer, -1.f, 1.f);
    throttle = std::clamp(throttle, 0.f, 1.f);
    trimming_ = trim;
    float zPrev = z_;

    float rate = kYawBase + std::min(speed_, kCapFwd) * kYawPer;
    if (speed_ < 2.2f) rate *= 0.6f;
    if (trim) rate *= 0.82f;
    float yawCmd = steer * rate;
    yaw_ += (yawCmd - yaw_) * (1.f - std::exp(-5.5f * kDt));
    heading_ = wrap(heading_ + yaw_ * kDt);

    float target = kCurrent + throttle * (kCapFwd - kCurrent);
    speed_ += (target - speed_) * (1.f - std::exp(-1.35f * kDt));
    speed_ *= 1.f - std::fabs(steer) * speed_ * 0.004f;
    speed_ = std::clamp(speed_, 0.4f, kCapFwd);

    x_ += std::sin(heading_) * speed_ * kDt;
    z_ += std::cos(heading_) * speed_ * kDt;

    float out = -steer * speed_ * 0.085f - yaw_ * speed_ * 0.55f;
    float trimF = trim ? -cargo_ * 6.5f : 0.f;
    cargoVel_ += (out - cargo_ * 1.6f - cargoVel_ * 2.4f + trimF) * kDt;
    cargo_ += cargoVel_ * kDt;
    cargo_ = std::clamp(cargo_, -1.35f, 1.35f);

    float demand = cargo_ * 0.62f - steer * speed_ * 0.028f - yaw_ * speed_ * 0.05f;
    if (trim) demand *= 0.72f;

    float off = x_ - courseX(z_);
    float over = std::fabs(off) - courseHalf(z_);
    if (over > 0.f) {
        float sgn = off > 0.f ? 1.f : -1.f;
        x_ -= sgn * std::min(over, 0.35f);
        speed_ *= std::max(0.72f, 1.f - std::min(over, 2.f) * 0.06f);
        demand += sgn * (0.55f + std::min(over, 2.f) * 0.35f);
        cargo_ += sgn * std::min(over, 1.f) * 0.02f;
        if (over > 4.2f) {
            fail("grounded on the bank");
            return;
        }
    }

    float slide = cargo_ * speed_ * 0.035f;
    x_ += std::cos(heading_) * slide * kDt;
    z_ -= std::sin(heading_) * slide * kDt;

    leanVel_ += ((demand - lean_) * kLeanK - leanVel_ * kLeanDamp) * kDt;
    lean_ += leanVel_ * kDt;
    lean_ = std::clamp(lean_, -1.4f, 1.4f);
    if (std::fabs(lean_) > kTip || std::fabs(cargo_) > 1.15f) {
        fail(tipWhy());
        return;
    }

    markTurns(zPrev);
    if (mode_ != Mode::Run) return;
    if (z_ > kBends[2].exitZ + 36.f) {
        fail("missed the turns");
        return;
    }
    race_ += kDt;
    if (race_ > kClock) {
        fail("the tide ran out");
        return;
    }

    wakeT_ -= kDt;
    if (wakeT_ <= 0.f && speed_ > 2.f) {
        wakeT_ = 0.09f;
        Wake w;
        w.x = x_ - std::sin(heading_) * 2.1f;
        w.z = z_ - std::cos(heading_) * 2.1f;
        w.life = 1.f;
        wakes_[wakeN_] = w;
        wakeN_ = (wakeN_ + 1) % 8;
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= kDt * 0.7f;
    camBob_ = std::sin(t_ * 1.7f) * 0.03f + std::sin(t_ * 3.1f) * 0.015f;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt);
}

void Game::audio() {
    if (!sys_) return;
    float hum = 0.f;
    if (mode_ == Mode::Run) hum = 0.05f + speed_ * 0.012f;
    else if (mode_ == Mode::Title) hum = 0.04f;
    float f = 70.f + speed_ * 6.f;
    if (std::fabs(hum - tone0_) > 0.002f || mode_ == Mode::Run) sys_->apu.tone(0, f, hum);
    tone0_ = hum;
    if (mode_ != Mode::Run) sys_->apu.tone(1, 0.f, 0.f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        lean_ = 0.32f * std::sin(t_ * 0.7f);
        cargo_ = 0.25f * std::sin(t_ * 0.55f);
        heading_ = 0.08f * std::sin(t_ * 0.4f);
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) startRun();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float steer = 0, throttle = 0;
            bool trim = false;
            if (bot_) pilot(steer, throttle, trim);
            else controls(steer, throttle, trim);
            physics(steer, throttle, trim);
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
    const float back = mode_ == Mode::Title ? 14.f : 9.4f;
    const float eye = mode_ == Mode::Title ? 5.4f : 4.2f;
    const float hor = mode_ == Mode::Title ? 70.f : 82.f;
    float camX = x_ - std::sin(heading_) * back;
    float camZ = z_ - std::cos(heading_) * back;
    float dx = wx - camX;
    float dz = wz - camZ;
    float S = std::sin(heading_);
    float C = std::cos(heading_);
    rz = dx * S + dz * C;
    float rx = dx * C - dz * S;
    if (rz < 1.2f) return false;
    scale = kFocal / rz;
    sx = 160.f + rx * scale;
    sy = hor - (wy - (eye + camBob_)) * scale;
    fog = rz > 55.f ? std::clamp(int((rz - 55.f) / 18.f), 0, 12) : 0;
    return true;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -8 || cy + h * 0.5f < -8 || cx - w * 0.5f > gs::SCREEN_W + 8 || cy - h * 0.5f > gs::SCREEN_H + 8)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1600L);
    long sh = std::clamp(std::lround(h), 1L, 1600L);
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

void Game::drawRiver() {
    gs::VDP& v = sys_->vdp;
    const float back = mode_ == Mode::Title ? 14.f : 9.4f;
    const float eye = mode_ == Mode::Title ? 5.4f : 4.2f;
    const float hor = mode_ == Mode::Title ? 70.f : 82.f;
    const float S = std::sin(heading_);
    const float C = std::cos(heading_);
    const float camX = x_ - S * back;
    const float camZ = z_ - C * back;
    const float camH = eye + camBob_;
    const uint16_t zenith = gs::rgb4(3, 6, 10);
    const uint16_t mid = gs::rgb4(7, 10, 13);
    const uint16_t haze = gs::rgb4(12, 12, 10);
    const uint16_t deep = gs::rgb4(2, 5, 7);
    v.roadTime = int(t_ * 18.f);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (float(y) < hor) {
            float u = float(y) / std::max(hor, 1.f);
            v.lineBackdrop[y] = u < 0.6f ? lerpC(zenith, mid, u / 0.6f) : lerpC(mid, haze, (u - 0.6f) / 0.4f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
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
        r.pal = uint8_t(PAL_RIVER);
        r.left = r.right = gs::GROUND_LAND;
        r.style = 2;
        r.v = z0 * 22.f;
        r.band = (int(std::floor(z0 * 0.2f)) & 1) ? 1 : 0;
        if (std::fabs(denom) < 0.045f) {
            r.cx = std::fabs(e0) <= half ? 160.f : -4000.f;
            r.hw = std::fabs(e0) <= half ? 900.f : 2.f;
        } else {
            float uA = (half - e0) / denom;
            float uB = (-half - e0) / denom;
            float scl = kFocal / std::max(dist, 0.4f);
            r.cx = 160.f + 0.5f * (uA + uB) * scl;
            r.hw = std::min(4000.f, 0.5f * std::fabs(uA - uB) * scl);
        }
        int fog = dist > 80.f ? std::clamp(int((dist - 80.f) / 30.f), 0, 11) : 0;
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = deep;
    }
}

void Game::drawWorld() {
    struct Item {
        float rz, sx, sy, sh;
        const gs::Mipped* img;
        int pal, fog;
        bool shadow;
    };
    std::vector<Item> items;
    items.reserve(160);
    auto push = [&](float wx, float wy, float wz, const gs::Mipped& img, float worldH, int pal, bool shadow) {
        float sx, sy, scale;
        int fog;
        float rz;
        if (!project(wx, wy, wz, sx, sy, scale, fog, rz)) return;
        float sh = worldH * scale;
        if (sh < 1.3f || rz > 240.f) return;
        items.push_back(Item{rz, sx, sy, sh, &img, pal, fog, shadow});
    };

    for (const Prop& p : props_) {
        switch (p.kind) {
        case Kind::BuoyR:
        case Kind::BuoyL: push(p.x, p.h * 0.5f, p.z, art_.buoy, p.h, PAL_BUOY, false); break;
        case Kind::Reed: push(p.x, p.h * 0.5f, p.z, art_.reed, p.h, PAL_REED, false); break;
        case Kind::Mill: push(p.x, p.h * 0.5f, p.z, art_.mill, p.h, PAL_MILL, false); break;
        case Kind::Crane: push(p.x, p.h * 0.5f, p.z, art_.crane, p.h, PAL_CRANE, false); break;
        case Kind::Sign: push(p.x, p.h * 0.5f, p.z, art_.sign[p.num], p.h, PAL_SIGN, false); break;
        }
    }

    int flap = int(t_ * 4.f) & 1;
    for (const Gull& g : gulls_) {
        float gx = g.x + std::sin(t_ * 0.4f + g.ph) * 6.f;
        float gz = g.z + std::cos(t_ * 0.25f + g.ph) * 4.f;
        float gy = g.y + std::sin(t_ * 1.4f + g.ph) * 0.35f;
        push(gx, gy, gz, art_.gull[flap], 0.55f, PAL_BIRD, false);
    }
    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        push(w.x, 0.05f, w.z, art_.wake, 0.35f + (1.f - w.life) * 0.7f, PAL_FOAM, false);
    }

    float shake = shake_ * std::sin(t_ * 40.f) * 0.25f;
    bool tipped = mode_ == Mode::Fail && why_ && why_[0] == 't';
    push(x_ + shake, 0.05f, z_, art_.shadow, 1.3f, PAL_HULL, true);
    if (tipped) push(x_, 0.7f, z_, art_.wreck, 2.2f, PAL_HULL, false);
    else push(x_ + shake, 1.25f, z_, art_.barge[leanFrame()], kHullH, PAL_HULL, false);

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.rz < b.rz; });
    for (const Item& it : items) spr(*it.img, it.sx, it.sy, it.sh, it.pal, it.fog, it.shadow);
}

void Game::drawSky() {
    float hor = mode_ == Mode::Title ? 70.f : 82.f;
    spr(art_.sun, 248.f, hor - 28.f, 18.f, PAL_SKY, 0, false);
    spr(art_.cloud, 48.f + std::sin(t_ * 0.08f) * 6.f, 22.f, 26.f, PAL_SKY, 0, false);
    spr(art_.cloud, 168.f + std::cos(t_ * 0.07f) * 8.f, 34.f, 18.f, PAL_SKY, 2, false);
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
        banner(art_.title, 160.f, 24.f, PAL_BANNER);
        hudC(7, "MAKE THE THREE TURNS", PAL_HUD);
        hudC(8, "WITHOUT TIPPING", PAL_BANNER);
        hudC(9, "TRIM THE CARGO IN THE BENDS", PAL_TAG);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "ARROWS STEER   UP AHEAD   Z TRIMS", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        banner(art_.paused, 160.f, 48.f, PAL_BANNER);
    } else if (mode_ == Mode::Fail) {
        bool tipped = why_ && why_[0] == 't';
        banner(tipped ? art_.tipped : art_.missed, 160.f, 40.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.upright, 160.f, 36.f, PAL_WIN);
    }

    if (mode_ == Mode::Title) return;

    hud(1, 0, "S3 BARGE TURN", PAL_BANNER);
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

    const char* line = "HOLD HER UPRIGHT";
    int pal = PAL_WIN;
    if (inBend(z_)) {
        std::snprintf(buf, sizeof buf, "TURN %d OF 3", std::min(turns_ + 1, 3));
        line = buf;
        pal = PAL_BANNER;
    } else if (turns_ > 0) {
        std::snprintf(buf, sizeof buf, "TURN %d MADE", turns_);
        line = buf;
        pal = PAL_WIN;
    }
    if (std::fabs(lean_) > kTip * 0.7f || std::fabs(cargo_) > 0.7f) {
        line = trimming_ ? "CARGO TRIMMED" : "TRIM THE CARGO";
        pal = PAL_ALERT;
    }
    hud(1, 1, line, pal);
    std::snprintf(buf, sizeof buf, "SPD %.0f", speed_);
    hud(33, 1, buf, PAL_TAG);

    char bar[12];
    for (int i = 0; i < 11; i++) bar[i] = '-';
    bar[11] = 0;
    float n = std::clamp(lean_ / kTip, -1.f, 1.f);
    int at = std::clamp(int(std::lround((n + 1.f) * 5.f)), 0, 10);
    bar[at] = 'O';
    std::snprintf(buf, sizeof buf, "LEAN L%sR %+0.0f", bar, lean_ * 57.2958f);
    hud(1, 2, buf, std::fabs(n) > 0.72f ? PAL_ALERT : PAL_HUD);
    std::snprintf(buf, sizeof buf, "CARGO %+.0f", cargo_ * 100.f);
    hud(28, 2, buf, std::fabs(cargo_) > 0.55f ? PAL_ALERT : PAL_TAG);
    hud(1, 26, "ARROWS STEER   UP AHEAD   Z TRIMS", PAL_HUD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    drawRiver();
    drawWorld();
    drawSky();
    drawHud();
}

}  // namespace bargeturn
