#include "lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace skifflane {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kLeg = 390.f;
constexpr float kClock = 54.f;
constexpr float kGateHalf = 3.55f;
constexpr float kHalfW = 1.15f;
constexpr float kHalfL = 2.45f;
constexpr float kCapFwd = 15.5f;
constexpr float kCapRev = 4.5f;
constexpr float kFocal = 270.f;
constexpr float kBoatH = 2.85f;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float smooth(float t) {
    t = std::clamp(t, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

float sineCenter(float z) {
    float fade = smooth(z / 28.f);
    return fade * (16.5f * std::sin(z * 0.011f) + 6.5f * std::sin(z * 0.0265f + 0.8f));
}

// Centre of the buoyed channel. The last stretch is straightened onto the gate.
float courseCenter(float z) {
    const float straighten = kLeg - 52.f;
    float w = sineCenter(z);
    if (z > straighten) {
        float t = smooth((z - straighten) / 44.f);
        w = w * (1.f - t) + sineCenter(straighten) * t;
    }
    return w;
}

float courseHalf(float z) {
    float h = 8.15f;
    auto pinch = [&](float at, float wid, float depth) {
        float d = (z - at) / wid;
        h -= depth * std::exp(-d * d);
    };
    pinch(150.f, 26.f, 1.7f);
    pinch(255.f, 22.f, 1.9f);
    if (z < 22.f) h = 11.f * (1.f - std::clamp(z / 22.f, 0.f, 1.f)) + h * std::clamp(z / 22.f, 0.f, 1.f);
    return h;
}

// Side set, ramped in after the start and out before the gate.
float setX(float z) {
    float ramp = smooth((z - 18.f) / 36.f);
    float ease = 1.f - smooth((z - (kLeg - 48.f)) / 36.f);
    return ramp * ease * (1.65f * std::sin(z * 0.018f) + 0.5f * std::sin(z * 0.041f + 1.1f));
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

float Game::lateral() const { return x_ - courseCenter(z_); }

float Game::eye() const { return mode_ == Mode::Title ? 6.1f : 4.7f; }
float Game::back() const { return mode_ == Mode::Title ? 13.2f : 9.2f; }
float Game::horizon() const { return mode_ == Mode::Title ? 64.f : 70.f; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (kLeg - z_ < 58.f) return 3;
    if (std::fabs(lateral()) > courseHalf(z_) * 0.62f) return 2;
    return 1;
}

int Game::bankFrame() const {
    float b = std::clamp(yaw_ * 1.4f + lateral() * 0.04f, -1.f, 1.f);
    if (b > 0.28f) return 2;
    if (b < -0.28f) return 0;
    return 1;
}

void Game::begin() {
    z_ = 12.f;
    x_ = courseCenter(z_);
    float ahead = courseCenter(z_ + 6.f);
    heading_ = std::atan2(ahead - x_, 6.f);
    speed_ = 0.f;
    throttle_ = 0.f;
    yaw_ = 0.f;
    race_ = 0.f;
    wakeT_ = 0.f;
    wakeN_ = 0;
    won_ = false;
    over_ = false;
    warned_ = false;
    sawGate_ = false;
    why_ = "";
    chimeN_ = 0;
    chimeStep_ = 0;
    chimeT_ = 0.f;
    tone0_ = 0.f;
    tone1_ = 0.f;
    for (Wake& w : wakes_) w = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    blip(620.f);
}

void Game::buildCourse() {
    props_.clear();
    auto add = [&](float x, float z, float h, Kind k, int pal) { props_.push_back(Prop{x, z, h, k, pal}); };
    for (int i = 0; i < 20; i++) {
        float z = 26.f + float(i) * 18.f;
        if (z > kLeg - 14.f) break;
        float c = courseCenter(z);
        float w = courseHalf(z);
        add(c - w, z, 2.15f, Kind::Buoy, PAL_RED);
        add(c + w, z, 2.15f, Kind::Buoy, PAL_GREEN);
    }
    for (int i = 0; i < 14; i++) {
        float z = 34.f + float(i) * 24.f;
        if (z > kLeg - 18.f) break;
        add(courseCenter(z), z, 0.62f, Kind::Can, PAL_MARK);
    }
    for (int i = 0; i < 26; i++) {
        float z = 16.f + float(i) * 14.5f;
        if (z > kLeg - 6.f) break;
        int hsh = hashN(i * 17 + 3);
        float jx = float((hsh % 17) - 8) * 0.16f;
        float jz = float((hsh / 17) % 9) * 0.28f;
        float rh = 1.55f + float(hsh % 5) * 0.28f;
        float c = courseCenter(z + jz);
        float w = courseHalf(z + jz);
        add(c - w - 2.4f + jx, z + jz, rh, Kind::Reed, PAL_REED);
        add(c + w + 2.6f - jx * 0.6f, z + jz * 0.5f, rh * 0.92f, Kind::Reed, PAL_REED);
    }
    float z0 = 14.f;
    add(courseCenter(z0) - courseHalf(z0) - 3.6f, z0, 2.3f, Kind::Dock, PAL_WOOD);
    float z1 = 168.f;
    add(courseCenter(z1) + courseHalf(z1) + 4.2f, z1, 3.5f, Kind::Shack, PAL_WOOD);
    float z2 = 286.f;
    add(courseCenter(z2) - courseHalf(z2) - 4.4f, z2, 3.3f, Kind::Shack, PAL_WOOD);
    float gc = courseCenter(kLeg);
    add(gc - kGateHalf, kLeg, 4.6f, Kind::Post, PAL_RED);
    add(gc + kGateHalf, kLeg, 4.6f, Kind::Post, PAL_GREEN);
    add(gc, kLeg, 0.5f, Kind::Bar, PAL_WOOD);
    add(gc - kGateHalf - 0.15f, kLeg, 1.35f, Kind::Flag, PAL_FLAG);
    add(gc + kGateHalf + 0.15f, kLeg, 1.35f, Kind::Flag, PAL_FLAG);

    for (int i = 0; i < 5; i++) {
        float z = 40.f + float(i) * 70.f;
        float side = (i & 1) ? 1.f : -1.f;
        gulls_[i].x = courseCenter(z) + side * (courseHalf(z) + 6.f);
        gulls_[i].z = z;
        gulls_[i].y = 5.2f + float(i) * 0.35f;
        gulls_[i].ph = float(i) * 1.3f;
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.13f, 0.22f, 0.15f);
    buildCourse();
    if (bot_) startRun();
    else showTitle();
}

void Game::controls(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(p.axisX, -1.f, 1.f);
    const bool up = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A);
    const bool down = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    if (up) throttle_ = std::min(1.f, throttle_ + kDt * 0.9f);
    if (down) throttle_ = std::max(-0.55f, throttle_ - kDt * 1.15f);
    if (p.accel > 0.08f) throttle_ = std::max(throttle_, p.accel);
    if (p.brake > 0.08f) throttle_ = std::min(throttle_, -p.brake * 0.55f);
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    const float distEnd = kLeg - z_;
    const float off = x_ - courseCenter(z_);
    const float half = courseHalf(z_);
    const float margin = half - std::fabs(off);

    if (distEnd < 34.f) {
        float aim = courseCenter(kLeg) - x_;
        float hDes = std::atan2(aim, std::max(5.f, distEnd));
        steer = std::clamp(wrap(hDes - heading_) / 0.28f, -1.f, 1.f);
        float want = std::fabs(aim) > 1.05f ? 6.2f : 9.f;
        if (distEnd < 12.f) want = std::fabs(aim) > 0.4f ? 5.f : 8.f;
        throttle = std::clamp((want - speed_) * 0.35f, -0.55f, 0.85f);
        return;
    }
    if (margin < 2.15f) {
        float hDes = std::atan2(-off, 14.f);
        steer = std::clamp(wrap(hDes - heading_) / 0.24f, -1.f, 1.f);
        throttle = 0.4f;
        return;
    }

    float look = std::clamp(18.f + speed_ * 0.5f, 18.f, 28.f);
    float tx = courseCenter(z_ + look);
    tx -= setX(z_) * (look / std::max(speed_, 8.f));
    float hDes = std::atan2(tx - x_, look);
    float herr = wrap(hDes - heading_);
    steer = std::clamp(herr / 0.32f, -1.f, 1.f);
    float want = std::fabs(herr) > 0.42f ? 10.f : 13.2f;
    throttle = std::clamp((want - speed_) * 0.3f, 0.f, 1.f);
}

void Game::judge() {
    if (z_ < -3.f) {
        fail("missed the end");
        return;
    }
    const float s = std::sin(heading_);
    const float c = std::cos(heading_);
    const float gateC = courseCenter(kLeg);
    const float lxS[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const float lzS[8] = {1, 1, 1, 0, 0, -1, -1, -1};
    bool left = false;
    bool bad = false;
    bool allThrough = true;
    for (int i = 0; i < 8; i++) {
        float lx = lxS[i] * kHalfW;
        float lz = lzS[i] * kHalfL;
        float wx = x_ + s * lz + c * lx;
        float wz = z_ + c * lz - s * lx;
        if (wz < kLeg - 0.05f) {
            allThrough = false;
            if (std::fabs(wx - courseCenter(wz)) > courseHalf(wz) + 0.06f) left = true;
        } else if (std::fabs(wx - gateC) > kGateHalf + 0.05f) {
            bad = true;
        }
    }
    if (left) fail("left the lane");
    else if (bad) fail("missed the end");
    else if (allThrough) win();
    else if (race_ >= kClock) fail("missed the end");
}

void Game::physics(float steer, float throttle) {
    steer = std::clamp(steer, -1.f, 1.f);
    throttle = std::clamp(throttle, -1.f, 1.f);
    float sp = std::fabs(speed_);
    float rate = 2.05f - std::min(sp, 14.f) * 0.06f;
    float yaw = steer * rate;
    heading_ = wrap(heading_ + yaw * kDt);
    yaw_ += (yaw - yaw_) * (1.f - std::exp(-8.f * kDt));

    float cap = throttle >= 0.f ? kCapFwd : kCapRev;
    float target = throttle * cap;
    speed_ += (target - speed_) * (1.f - std::exp(-2.6f * kDt));
    speed_ = std::clamp(speed_, -kCapRev, kCapFwd);

    float vx = std::sin(heading_) * speed_ + setX(z_);
    float vz = std::cos(heading_) * speed_;
    x_ += vx * kDt;
    z_ += vz * kDt;

    judge();
    if (mode_ != Mode::Run) return;

    float margin = courseHalf(z_) - std::fabs(lateral());
    if (margin < 2.0f && !warned_) {
        warned_ = true;
        blip(180.f);
    } else if (margin > 2.8f) {
        warned_ = false;
    }
    if (!sawGate_ && kLeg - z_ < 62.f) {
        sawGate_ = true;
        blip(740.f);
    }

    wakeT_ -= kDt;
    if (wakeT_ <= 0.f && sp > 2.4f) {
        wakeT_ = 0.07f;
        Wake w;
        w.x = x_ - std::sin(heading_) * (kHalfL + 0.3f);
        w.z = z_ - std::cos(heading_) * (kHalfL + 0.3f);
        w.life = 1.f;
        wakes_[wakeN_] = w;
        wakeN_ = (wakeN_ + 1) % 14;
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= kDt * 0.85f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "held";
    chime();
    sys_->rumble(0.32f, 0.16f, 180);
    sys_->setLight(50, 190, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    sys_->apu.noiseBurst(0.42f, 80.f, 0.4f);
    sys_->apu.tone(0, 70.f, 0.07f);
    tone0_ = 0.42f;
    sys_->rumble(0.55f, 0.1f, 170);
    sys_->setLight(180, 30, 24);
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
    float water = mode_ == Mode::Run ? 0.015f + std::fabs(speed_) * 0.00055f : 0.01f;
    sys_->apu.noise(water, 460.f, false);
    if (mode_ == Mode::Run && (std::fabs(throttle_) > 0.04f || std::fabs(speed_) > 1.6f)) {
        float wob = 0.7f + 0.3f * std::sin(t_ * (12.f + std::fabs(throttle_) * 18.f));
        float vol = (0.012f + std::fabs(throttle_) * 0.03f) * wob;
        sys_->apu.tone(2, 48.f + std::fabs(throttle_) * 36.f + std::fabs(speed_) * 0.4f, vol);
    } else if (tone0_ <= 0.f) {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= kDt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {294.f, 349.f, 440.f, 587.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 3)], 0.06f);
            tone0_ = 0.16f;
            chimeT_ = 0.16f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    camBob_ = std::sin(t_ * 1.7f) * 0.06f;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(420.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            float steer = 0.f, thr = throttle_;
            if (bot_) pilot(steer, thr);
            else controls(steer, thr);
            throttle_ = thr;
            physics(steer, thr);
            if (mode_ == Mode::Run) {
                float margin = courseHalf(z_) - std::fabs(lateral());
                if (kLeg - z_ < 40.f && margin > 2.f) sys.setLight(40, 160, 80);
                else if (margin < 2.1f) sys.setLight(170, 120, 30);
                else sys.setLight(20, 60, 90);
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
    if (rz < 1.4f) return false;
    scale = kFocal / rz;
    sx = 160.f + rx * scale;
    sy = horizon() - (wy - (eye() + camBob_)) * scale;
    fog = 0;
    if (rz > 52.f) fog = std::clamp(int((rz - 52.f) / 16.f), 0, 14);
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

void Game::drawLane() {
    gs::VDP& v = sys_->vdp;
    const float hor = horizon();
    const float S = std::sin(heading_);
    const float C = std::cos(heading_);
    const float camX = x_ - S * back();
    const float camZ = z_ - C * back();
    const float camH = eye() + camBob_;
    const uint16_t zenith = gs::rgb4(5, 8, 13);
    const uint16_t mid = gs::rgb4(10, 14, 15);
    const uint16_t haze = gs::rgb4(15, 13, 10);
    const uint16_t deep = gs::rgb4(2, 6, 7);
    v.roadTime = int(t_ * 30.f);

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
        float c0 = courseCenter(z0);
        float c1 = (courseCenter(z0 + 1.2f) - courseCenter(z0 - 1.2f)) / 2.4f;
        float half = courseHalf(z0);
        float e0 = x0 - c0;
        float denom = C + c1 * S;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.pal = uint8_t(PAL_LANE);
        r.left = r.right = gs::GROUND_LAND;
        r.style = 2;
        r.v = z0 * 34.f;
        r.band = (int(std::floor(z0 * 0.32f + t_ * 5.f)) & 1) ? 1 : 0;
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
        if (dist > 46.f) fog = std::clamp(int((dist - 46.f) / 22.f), 0, 12);
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
        if (sh < 1.6f || rz > 240.f) return;
        items.push_back(Item{rz + rzBias, sx, sy, sh, &img, pal, fog, shadow});
    };

    for (const Prop& p : props_) {
        switch (p.kind) {
        case Kind::Buoy: push(p.x, p.h * 0.5f, p.z, art_.buoy, p.h, p.pal, false, 0); break;
        case Kind::Can: push(p.x, 0.2f, p.z, art_.can, p.h, p.pal, false, 0); break;
        case Kind::Reed: {
            float sway = std::sin(t_ * 1.4f + p.z * 0.2f) * 0.15f;
            push(p.x + sway, p.h * 0.5f, p.z, art_.reed, p.h, p.pal, false, 0);
            break;
        }
        case Kind::Post: push(p.x, p.h * 0.5f, p.z, art_.post, p.h, p.pal, false, 0); break;
        case Kind::Flag: {
            float flutter = 3.15f + std::sin(t_ * 3.f + p.x) * 0.08f;
            push(p.x, flutter, p.z, art_.flag, p.h, p.pal, false, 0);
            break;
        }
        case Kind::Bar: {
            float span = kGateHalf * 2.f;
            float worldH = span * float(art_.bar.h) / float(std::max(art_.bar.w, 1));
            push(p.x, 3.15f, p.z, art_.bar, worldH, p.pal, false, 0);
            if (mode_ == Mode::Run || mode_ == Mode::Title) {
                float endH = span * 0.42f * float(art_.end.h) / float(std::max(art_.end.w, 1));
                push(p.x, 3.85f, p.z - 0.2f, art_.end, endH, PAL_BANNER, false, -0.05f);
            }
            break;
        }
        case Kind::Shack: push(p.x, p.h * 0.5f, p.z, art_.shack, p.h, p.pal, false, 0); break;
        case Kind::Dock: push(p.x, p.h * 0.5f, p.z, art_.dock, p.h, p.pal, false, 0); break;
        }
    }

    int flap = int(t_ * 5.f) & 1;
    for (const Gull& g : gulls_) {
        float gx = g.x + std::sin(t_ * 0.45f + g.ph) * 5.f;
        float gz = g.z + std::cos(t_ * 0.32f + g.ph) * 4.f;
        float gy = g.y + std::sin(t_ * 1.3f + g.ph) * 0.35f;
        push(gx, gy, gz, art_.gull[flap], 0.85f, PAL_GULL, false, 0);
    }

    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        float h = 0.55f + (1.f - w.life) * 1.1f;
        push(w.x, 0.15f, w.z, art_.foam, h, PAL_FOAM, false, 0);
    }

    float bob = std::sin(t_ * 2.2f) * 0.05f;
    push(x_, bob, z_, art_.shadow, 1.15f, PAL_HULL, true, 0.7f);
    push(x_, kBoatH * 0.42f + bob, z_, art_.stern[bankFrame()], kBoatH, PAL_HULL, false, 0.f);
    if (std::fabs(speed_) > 2.f) {
        float sx = x_ - std::sin(heading_) * (kHalfL * 0.2f);
        float sz = z_ - std::cos(heading_) * (kHalfL * 0.2f);
        push(sx, 0.2f, sz, art_.foam, 0.7f, PAL_FOAM, false, -0.2f);
    }

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.rz < b.rz; });
    for (const Item& it : items) spr(*it.img, it.sx, it.sy, it.sh, it.pal, it.fog, it.shadow);
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
    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, 0, false); };

    if (mode_ == Mode::Title) {
        banner(art_.title, 160.f, 16.f, PAL_BANNER);
        hudC(5, "STAY IN THE LANE", PAL_HUD);
        hudC(6, "BETWEEN THE BUOYS, THROUGH THE GATE", PAL_TAG);
        hudC(7, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "START", PAL_WIN);
        else hudC(27, "ARROWS STEER   UP DRIVES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 46.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        bool missed = why_ && why_[0] == 'm';
        banner(missed ? art_.missed : art_.left, 160.f, 42.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.held, 160.f, 36.f, PAL_WIN);
        banner(art_.whole, 160.f, 54.f, PAL_WIN);
    }

    hud(1, 0, "S3 SKIFF LANE", PAL_BANNER);
    int left = std::max(0, int(std::ceil(kClock - race_ - 1e-3f)));
    if (mode_ == Mode::Win) left = 0;
    std::snprintf(buf, sizeof buf, "TIME %d", mode_ == Mode::Win ? int(race_ + 0.5f) : left);
    hud(31, 0, buf, (mode_ == Mode::Run && left <= 10) ? PAL_ALERT : PAL_TAG);

    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "THE WHOLE LEG  %.1fS", race_);
        hudC(9, buf, PAL_HUD);
        if (!bot_) hudC(27, "START RUNS IT AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(8, why_, PAL_ALERT);
        if (!bot_) hudC(27, "START TRIES AGAIN", PAL_HUD);
        return;
    }

    float half = courseHalf(z_);
    float off = lateral();
    float distEnd = std::max(0.f, kLeg - z_);
    const char* line = "IN THE LANE";
    int pal = PAL_WIN;
    if (distEnd < 70.f) {
        line = "LINE UP THE GATE";
        pal = PAL_BANNER;
    } else if (half - std::fabs(off) < 2.1f) {
        line = "NEAR THE MARKS";
        pal = PAL_ALERT;
    }
    hud(1, 1, line, pal);
    std::snprintf(buf, sizeof buf, "END %.0f", distEnd);
    hud(32, 1, buf, PAL_TAG);

    char g[16];
    for (int i = 0; i < 15; i++) g[i] = '-';
    g[15] = 0;
    if (distEnd < 110.f) {
        auto tick = [&](float worldOff) {
            float u = std::clamp(worldOff / half, -1.f, 1.f);
            int i = int(std::lround((u + 1.f) * 0.5f * 14.f));
            if (i >= 0 && i < 15) g[i] = '+';
        };
        tick(-kGateHalf);
        tick(kGateHalf);
    }
    int boat = int(std::lround((std::clamp(off / half, -1.f, 1.f) + 1.f) * 0.5f * 14.f));
    boat = std::clamp(boat, 0, 14);
    g[boat] = 'O';
    std::snprintf(buf, sizeof buf, "LANE <%s>", g);
    hud(1, 2, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "OFF %+.1f  SPD %.0f", off, speed_);
    hud(1, 3, buf, PAL_TAG);
    hud(1, 27, "ARROWS STEER   UP DRIVES   DOWN BACKS", PAL_HUD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    drawLane();
    // Banners first so they sit above the world. Sky sprites last so they sit behind it.
    drawHud();
    drawWorld();
    spr(art_.sun, 268.f, 28.f, 18.f, PAL_BANNER, 0, false);
    spr(art_.cloud, 64.f + std::sin(t_ * 0.18f) * 8.f, 26.f, 30.f, PAL_SKY, 0, false);
    spr(art_.cloud, 196.f + std::cos(t_ * 0.15f) * 10.f, 40.f, 22.f, PAL_SKY, 2, false);
}

}  // namespace skifflane
