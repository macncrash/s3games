#include "lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <vector>

namespace sledlane {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kLeg = 348.f;
constexpr float kClock = 52.f;
constexpr float kGateHalf = 3.25f;
constexpr float kHalfW = 0.8f;
constexpr float kHalfL = 1.95f;
constexpr float kCapFwd = 12.8f;
constexpr float kCapRev = 3.4f;
constexpr float kFocal = 268.f;
constexpr float kSledH = 2.55f;
constexpr float kAnchor = kLeg - 64.f;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float smooth(float t) {
    t = std::clamp(t, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

float waveAt(float z) {
    z = std::max(0.f, z);
    float w = 11.2f * std::sin(z * 0.0158f + 0.35f) + 7.4f * std::sin(z * 0.034f + 1.9f);
    return w * smooth(z / 34.f);
}

float courseCenter(float z) {
    z = std::max(0.f, z);
    if (z <= kAnchor) return waveAt(z);
    float t = smooth((z - kAnchor) / 52.f);
    return waveAt(z) * (1.f - t) + waveAt(kAnchor) * t;
}

float courseHalf(float z) {
    z = std::max(0.f, z);
    float h = 7.55f;
    auto pinch = [&](float at, float wid, float depth) {
        float d = (z - at) / wid;
        h -= depth * std::exp(-d * d);
    };
    pinch(124.f, 22.f, 1.45f);
    pinch(228.f, 18.f, 1.55f);
    if (z < 32.f) {
        float t = smooth(z / 32.f);
        h = 11.5f * (1.f - t) + h * t;
    }
    if (z > kLeg - 48.f) {
        float t = smooth((z - (kLeg - 48.f)) / 48.f);
        h = h * (1.f - t) + (kGateHalf + 2.15f) * t;
    }
    return h;
}

// Crosswind at full kick, in metres per second. It dies before the end.
float gust(float z) {
    float ramp = smooth((z - 24.f) / 40.f);
    float ease = 1.f - smooth((z - (kLeg - 70.f)) / 40.f);
    return ramp * ease * (1.15f * std::sin(z * 0.021f) + 0.45f * std::sin(z * 0.053f + 0.7f));
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

float Game::eye() const { return mode_ == Mode::Title ? 5.8f : 4.45f; }
float Game::back() const { return mode_ == Mode::Title ? 13.6f : 8.5f; }
float Game::horizon() const { return mode_ == Mode::Title ? 62.f : 76.f; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (kLeg - z_ < 62.f) return 3;
    if (std::fabs(lateral()) > courseHalf(z_) * 0.62f) return 2;
    return 1;
}

int Game::bankFrame() const {
    float b = std::clamp(yaw_ * 1.15f, -1.f, 1.f);
    if (b > 0.28f) return 2;
    if (b < -0.28f) return 0;
    return 1;
}

void Game::begin() {
    z_ = 18.f;
    x_ = courseCenter(z_);
    float ahead = courseCenter(z_ + 8.f);
    heading_ = std::atan2(ahead - x_, 8.f);
    speed_ = 0.f;
    throttle_ = 0.f;
    yaw_ = 0.f;
    race_ = 0.f;
    puffT_ = 0.f;
    puffN_ = 0;
    won_ = false;
    over_ = false;
    warned_ = false;
    sawEnd_ = false;
    why_ = "";
    chimeN_ = 0;
    chimeStep_ = 0;
    chimeT_ = 0.f;
    tone0_ = 0.f;
    tone1_ = 0.f;
    for (Puff& p : puffs_) p = {};
    for (int i = 0; i < 22; i++) {
        flakes_[i].x = float((i * 47) % gs::SCREEN_W);
        flakes_[i].y = float((i * 83) % gs::SCREEN_H);
        flakes_[i].s = 2.f + float(i % 3);
        flakes_[i].v = 18.f + float((i * 5) % 9) * 4.f;
    }
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    blip(520.f);
}

void Game::buildCourse() {
    props_.clear();
    auto add = [&](float x, float z, float h, Kind k, int pal) { props_.push_back(Prop{x, z, h, k, pal}); };
    for (int i = 0; i < 22; i++) {
        float z = 26.f + float(i) * 15.f;
        if (z > kLeg - 12.f) break;
        float c = courseCenter(z);
        float w = courseHalf(z);
        add(c - w, z, 1.85f, Kind::StakeL, PAL_STAKE);
        add(c + w, z, 1.85f, Kind::StakeR, PAL_STAKE);
    }
    for (int i = 0; i < 18; i++) {
        float z = 34.f + float(i) * 17.5f;
        if (z > kLeg - 16.f) break;
        int hsh = hashN(i * 19 + 5);
        float j = float(hsh % 9) * 0.16f;
        float jz = float((hsh / 9) % 7) * 0.35f;
        float th = 3.5f + float(hsh % 5) * 0.45f;
        float c = courseCenter(z + jz);
        float w = courseHalf(z + jz);
        add(c - w - 3.3f - j, z + jz, th, Kind::Tree, PAL_TREE);
        add(c + w + 3.6f + j * 0.6f, z + jz * 0.4f, th * 0.9f, Kind::Tree, PAL_TREE);
    }
    float zHut = 156.f;
    add(courseCenter(zHut) + courseHalf(zHut) + 4.6f, zHut, 3.4f, Kind::Hut, PAL_HUT);
    float zHut2 = 262.f;
    add(courseCenter(zHut2) - courseHalf(zHut2) - 4.8f, zHut2, 3.2f, Kind::Hut, PAL_HUT);
    float zBox = 98.f;
    add(courseCenter(zBox) - courseHalf(zBox) - 3.8f, zBox, 1.5f, Kind::Cache, PAL_HUT);
    float zBox2 = 206.f;
    add(courseCenter(zBox2) + courseHalf(zBox2) + 3.9f, zBox2, 1.45f, Kind::Cache, PAL_HUT);

    float gc = courseCenter(kLeg);
    add(gc - kGateHalf, kLeg, 5.1f, Kind::Post, PAL_END);
    add(gc + kGateHalf, kLeg, 5.1f, Kind::Post, PAL_END);
    add(gc, kLeg, 0.55f, Kind::Bar, PAL_END);
    add(gc - kGateHalf, kLeg, 0.9f, Kind::Flag, PAL_END);
    add(gc + kGateHalf, kLeg, 0.9f, Kind::Flag, PAL_END);

    for (int i = 0; i < 4; i++) {
        float z = 48.f + float(i) * 74.f;
        float side = (i & 1) ? 1.f : -1.f;
        ravens_[i].x = courseCenter(z) + side * (courseHalf(z) + 5.f);
        ravens_[i].z = z;
        ravens_[i].y = 6.2f + float(i) * 0.45f;
        ravens_[i].ph = float(i) * 1.4f;
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.74f);
    sys.apu.setEcho(0.16f, 0.22f, 0.1f);
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
    const bool up = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO);
    const bool down = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    if (up) throttle_ = std::min(1.f, throttle_ + kDt * 1.15f);
    if (down) throttle_ = std::max(-0.55f, throttle_ - kDt * 1.5f);
    if (!up && !down) throttle_ *= std::exp(-0.4f * kDt);
    if (p.axisY > 0.25f) throttle_ = std::max(throttle_, p.axisY);
    if (p.axisY < -0.25f) throttle_ = std::min(throttle_, p.axisY * 0.55f);
    if (p.accel > 0.08f) throttle_ = std::max(throttle_, p.accel);
    if (p.brake > 0.08f) throttle_ = std::min(throttle_, -p.brake * 0.55f);
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    const float distEnd = kLeg - z_;
    const float off = x_ - courseCenter(z_);
    const float half = courseHalf(z_);
    const float margin = half - std::fabs(off);

    if (distEnd < 56.f) {
        float aim = courseCenter(kLeg) - x_;
        float hDes = std::atan2(aim, std::max(8.f, distEnd));
        steer = std::clamp(wrap(hDes - heading_) / 0.26f, -1.f, 1.f);
        float want = std::fabs(aim) > 0.8f ? 6.2f : 10.f;
        if (distEnd < 16.f) want = std::fabs(aim) > 0.35f ? 5.f : 8.4f;
        throttle = std::clamp((want - speed_) * 0.42f, -0.55f, 0.9f);
        return;
    }
    if (margin < 2.35f) {
        float hDes = std::atan2(-off, margin < 1.1f ? 8.f : 13.f);
        steer = std::clamp(wrap(hDes - heading_) / 0.2f, -1.f, 1.f);
        throttle = margin < 1.1f ? 0.12f : 0.4f;
        return;
    }

    float look = std::clamp(18.f + speed_ * 0.4f, 18.f, 30.f);
    float tx = courseCenter(z_ + look);
    tx -= gust(z_) * look / kCapFwd;
    float hDes = std::atan2(tx - x_, look);
    float herr = wrap(hDes - heading_);
    steer = std::clamp(herr / 0.36f, -1.f, 1.f);
    float want = std::fabs(herr) > 0.38f ? 8.6f : 12.2f;
    throttle = std::clamp((want - speed_) * 0.34f, 0.f, 1.f);
}

void Game::judge() {
    if (z_ < -4.f) {
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
        if (wz < kLeg - 0.02f) {
            allThrough = false;
            if (std::fabs(wx - courseCenter(wz)) > courseHalf(wz) + 0.05f) left = true;
        } else if (std::fabs(wx - gateC) > kGateHalf + 0.04f) {
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
    float rate = 1.7f - std::min(sp, 12.f) * 0.05f;
    if (sp < 3.5f) rate += 0.45f;
    float yaw = steer * rate;
    heading_ = wrap(heading_ + yaw * kDt);
    yaw_ += (yaw - yaw_) * (1.f - std::exp(-8.f * kDt));

    float cap = throttle >= 0.f ? kCapFwd : kCapRev;
    float target = throttle * cap;
    speed_ += (target - speed_) * (1.f - std::exp(-2.15f * kDt));
    speed_ = std::clamp(speed_, -kCapRev, kCapFwd);

    float drift = gust(z_) * (speed_ / kCapFwd);
    float vx = std::sin(heading_) * speed_ + drift;
    float vz = std::cos(heading_) * speed_;
    x_ += vx * kDt;
    z_ += vz * kDt;

    judge();
    if (mode_ != Mode::Run) return;

    float margin = courseHalf(z_) - std::fabs(lateral());
    if (margin < 1.9f && !warned_) {
        warned_ = true;
        blip(160.f);
    } else if (margin > 2.7f) {
        warned_ = false;
    }
    if (!sawEnd_ && kLeg - z_ < 64.f) {
        sawEnd_ = true;
        blip(680.f);
    }

    puffT_ -= kDt;
    if (puffT_ <= 0.f && sp > 2.6f) {
        puffT_ = 0.06f;
        Puff w;
        w.x = x_ - std::sin(heading_) * (kHalfL + 0.25f);
        w.z = z_ - std::cos(heading_) * (kHalfL + 0.25f);
        w.life = 1.f;
        puffs_[puffN_] = w;
        puffN_ = (puffN_ + 1) % 12;
    }
    for (Puff& w : puffs_)
        if (w.life > 0.f) w.life -= kDt * 0.9f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    why_ = "held";
    chime();
    sys_->rumble(0.3f, 0.16f, 180);
    sys_->setLight(40, 170, 90);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    speed_ = 0.f;
    why_ = why;
    sys_->apu.noiseBurst(0.4f, 70.f, 0.38f);
    sys_->apu.tone(0, 64.f, 0.07f);
    tone0_ = 0.4f;
    sys_->rumble(0.55f, 0.1f, 170);
    sys_->setLight(170, 30, 24);
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
    float wind = mode_ == Mode::Run ? 0.012f + std::fabs(speed_) * 0.001f : 0.008f;
    sys_->apu.noise(wind, 820.f, false);
    if (mode_ == Mode::Run && std::fabs(speed_) > 1.2f) {
        float wob = 0.75f + 0.25f * std::sin(t_ * (9.f + std::fabs(speed_)));
        float vol = (0.008f + std::fabs(speed_) * 0.0016f) * wob;
        sys_->apu.tone(2, 150.f + std::fabs(speed_) * 16.f, vol);
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
            static const float notes[] = {392.f, 494.f, 587.f, 784.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 3)], 0.06f);
            tone0_ = 0.16f;
            chimeT_ = 0.15f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    camBob_ = std::sin(t_ * 1.8f) * (mode_ == Mode::Run ? 0.05f + std::fabs(speed_) * 0.004f : 0.04f);
    for (Flake& f : flakes_) {
        f.y += f.v * kDt;
        f.x += std::sin(t_ * 0.7f + f.y * 0.02f) * 12.f * kDt;
        if (f.y > gs::SCREEN_H + 4.f) {
            f.y = -4.f;
            f.x = std::fmod(f.x + 80.f, float(gs::SCREEN_W));
        }
        if (f.x < -4.f) f.x += gs::SCREEN_W;
        if (f.x > gs::SCREEN_W + 4.f) f.x -= gs::SCREEN_W;
    }

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
            if (bot_) pilot(steer, thr);
            else controls(steer, thr);
            throttle_ = thr;
            physics(steer, thr);
            if (mode_ == Mode::Run) {
                float margin = courseHalf(z_) - std::fabs(lateral());
                if (kLeg - z_ < 48.f && margin > 1.6f) sys.setLight(40, 160, 90);
                else if (margin < 2.f) sys.setLight(170, 110, 30);
                else sys.setLight(30, 70, 120);
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
    if (rz < 1.3f) return false;
    scale = kFocal / rz;
    sx = 160.f + rx * scale;
    sy = horizon() - (wy - (eye() + camBob_)) * scale;
    fog = 0;
    if (rz > 58.f) fog = std::clamp(int((rz - 58.f) / 18.f), 0, 12);
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
    const uint16_t zenith = gs::rgb4(4, 6, 11);
    const uint16_t mid = gs::rgb4(8, 11, 14);
    const uint16_t haze = gs::rgb4(14, 12, 10);
    const uint16_t deep = gs::rgb4(7, 9, 12);
    v.roadTime = int(t_ * 24.f);

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
        float c1 = (courseCenter(z0 + 1.5f) - courseCenter(z0 - 1.5f)) / 3.f;
        float half = courseHalf(z0);
        float e0 = x0 - c0;
        // Lane edges are not parallel to the camera. denom folds that slant into screen x.
        float denom = C + c1 * S;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.pal = uint8_t(PAL_LANE);
        r.left = r.right = gs::GROUND_SNOWWALL;
        r.style = gs::ROAD_SNOW;
        r.v = z0 * 26.f;
        r.band = (int(std::floor(z0 * 0.22f)) & 1) ? 1 : 0;
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
        if (dist > 72.f) fog = std::clamp(int((dist - 72.f) / 30.f), 0, 10);
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
        if (sh < 1.5f || rz > 220.f) return;
        items.push_back(Item{rz + rzBias, sx, sy, sh, &img, pal, fog, shadow});
    };
    const float S = std::sin(heading_);
    const float C = std::cos(heading_);
    auto body = [&](float lx, float ly, float lz, const gs::Mipped& img, float worldH, int pal, bool shadow, float bias) {
        float wx = x_ + S * lz + C * lx;
        float wz = z_ + C * lz - S * lx;
        push(wx, ly, wz, img, worldH, pal, shadow, bias);
    };

    for (const Prop& p : props_) {
        switch (p.kind) {
        case Kind::StakeL: push(p.x, p.h * 0.5f, p.z, art_.stakeRed, p.h, p.pal, false, 0); break;
        case Kind::StakeR: push(p.x, p.h * 0.5f, p.z, art_.stakeBlue, p.h, p.pal, false, 0); break;
        case Kind::Tree: push(p.x, p.h * 0.5f, p.z, art_.spruce, p.h, p.pal, false, 0); break;
        case Kind::Hut: push(p.x, p.h * 0.5f, p.z, art_.hut, p.h, p.pal, false, 0); break;
        case Kind::Cache: push(p.x, p.h * 0.5f, p.z, art_.cache, p.h, p.pal, false, 0); break;
        case Kind::Post: push(p.x, p.h * 0.5f, p.z, art_.post, p.h, p.pal, false, 0); break;
        case Kind::Flag: {
            float flutter = 4.85f + std::sin(t_ * 3.2f + p.x) * 0.08f;
            push(p.x, flutter, p.z, art_.flag, p.h, p.pal, false, 0);
            break;
        }
        case Kind::Bar: {
            float span = kGateHalf * 2.f;
            float worldH = span * float(art_.bar.h) / float(std::max(art_.bar.w, 1));
            push(p.x, 4.15f, p.z, art_.bar, worldH, p.pal, false, 0);
            float signH = span * 0.62f * float(art_.end.h) / float(std::max(art_.end.w, 1));
            push(p.x, 4.85f, p.z - 0.35f, art_.end, signH, PAL_BANNER, false, -0.05f);
            break;
        }
        }
    }

    int wing = int(t_ * 4.f) & 1;
    for (const Raven& rv : ravens_) {
        float gx = rv.x + std::sin(t_ * 0.4f + rv.ph) * 4.f;
        float gz = rv.z + std::cos(t_ * 0.28f + rv.ph) * 3.f;
        float gy = rv.y + std::sin(t_ * 1.2f + rv.ph) * 0.3f;
        push(gx, gy, gz, art_.raven[wing], 0.7f, PAL_BIRD, false, 0);
    }

    for (const Puff& w : puffs_) {
        if (w.life <= 0.f) continue;
        float h = 0.45f + (1.f - w.life) * 0.9f;
        push(w.x, 0.2f, w.z, art_.spray, h, PAL_SNOW, false, 0);
    }

    for (int i = 0; i < 5; i++) {
        float lz = 1.7f + float(i) * 0.72f;
        body(0.f, 0.35f, lz, art_.bead, 0.16f, PAL_SLED, false, 0.f);
    }
    int step = int(t_ * 8.f) & 1;
    for (int i = 0; i < 4; i++) {
        float lz = 3.5f + float(i) * 1.45f;
        float side = (i & 1) ? 1.f : -1.f;
        float lx = side * (0.55f + float(i) * 0.12f);
        float bob = std::sin(t_ * 11.f + float(i) * 1.3f) * 0.05f;
        int frame = (step + i) & 1;
        body(lx, 0.5f + bob, lz, art_.dog[frame], 0.92f, PAL_DOG, false, 0.f);
    }

    float bob = std::sin(t_ * 2.4f) * 0.04f;
    body(0.f, 0.15f, 0.f, art_.shadow, 1.05f, PAL_SLED, true, 0.6f);
    body(0.f, kSledH * 0.42f + bob, 0.f, art_.sled[bankFrame()], kSledH, PAL_SLED, false, 0.f);
    if (std::fabs(speed_) > 2.2f) {
        body(-0.35f, 0.18f, -1.3f, art_.spray, 0.55f, PAL_SNOW, false, -0.15f);
        body(0.35f, 0.18f, -1.3f, art_.spray, 0.55f, PAL_SNOW, false, -0.15f);
    }

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.rz < b.rz; });
    for (const Item& it : items) spr(*it.img, it.sx, it.sy, it.sh, it.pal, it.fog, it.shadow);
}

void Game::drawSky() {
    const float hor = horizon();
    spr(art_.sun, 248.f, hor - 16.f, 20.f, PAL_SKY, 0, false);
    spr(art_.moon, 46.f, 22.f, 12.f, PAL_SKY, 0, false);
    spr(art_.cloud, 78.f + std::sin(t_ * 0.12f) * 8.f, 30.f, 28.f, PAL_SKY, 0, false);
    spr(art_.cloud, 188.f + std::cos(t_ * 0.1f) * 10.f, 42.f, 20.f, PAL_SKY, 2, false);
    for (const Flake& f : flakes_) spr(art_.flake, f.x, f.y, f.s, PAL_SNOW, 0, false);
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
        banner(art_.title, 160.f, 18.f, PAL_BANNER);
        hudC(5, "STAY IN THE LANE", PAL_HUD);
        hudC(6, "THE WHOLE LEG, THROUGH THE END", PAL_TAG);
        hudC(7, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "START", PAL_WIN);
        else hudC(27, "ARROWS STEER   UP KICKS", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 40.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        bool missed = why_ && why_[0] == 'm';
        banner(missed ? art_.missed : art_.left, 160.f, 36.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.held, 160.f, 32.f, PAL_WIN);
        banner(art_.whole, 160.f, 54.f, PAL_WIN);
    }

    hud(1, 0, "S3 SLED LANE", PAL_BANNER);
    int left = std::max(0, int(std::ceil(kClock - race_ - 1e-3f)));
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
        bool missed = why_ && why_[0] == 'm';
        hudC(8, missed ? "MISSED THE END" : "LEFT THE LANE", PAL_ALERT);
        hudC(9, missed ? "MISSING THE END FAILS THE LEG" : "THE LANE IS THE WHOLE LEG", PAL_HUD);
        if (!bot_) hudC(27, "START TRIES AGAIN", PAL_HUD);
        return;
    }

    float half = courseHalf(z_);
    float off = lateral();
    float distEnd = std::max(0.f, kLeg - z_);
    const char* line = "IN THE LANE";
    int pal = PAL_WIN;
    if (distEnd < 70.f) {
        line = "LINE UP THE END";
        pal = PAL_BANNER;
    } else if (half - std::fabs(off) < 2.f) {
        line = "NEAR THE WALL";
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
    int sled = int(std::lround((std::clamp(off / half, -1.f, 1.f) + 1.f) * 0.5f * 14.f));
    sled = std::clamp(sled, 0, 14);
    g[sled] = 'O';
    std::snprintf(buf, sizeof buf, "LANE <%s>", g);
    hud(1, 2, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "OFF %+.1f  SPD %.0f", off, speed_);
    hud(1, 3, buf, PAL_TAG);
    hud(1, 27, "ARROWS STEER   UP KICKS   DOWN BRAKES", PAL_HUD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    drawLane();
    // Banners first so they sit above the trail. Sky last, so it sits behind the team.
    drawHud();
    drawWorld();
    drawSky();
}

}  // namespace sledlane
