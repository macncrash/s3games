#include "lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>

namespace lugelane {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kLeg = 340.f;
constexpr float kClock = 48.f;
constexpr float kGateHalf = 3.1f;
constexpr float kHalfW = 0.72f;
constexpr float kHalfL = 1.7f;
constexpr float kCap = 16.5f;
constexpr float kFocal = 270.f;
constexpr float kRiderH = 2.15f;
constexpr float kAnchor = kLeg - 58.f;

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
    float w = 9.6f * std::sin(z * 0.0172f + 0.4f) + 5.8f * std::sin(z * 0.038f + 1.6f);
    return w * smooth(z / 30.f);
}

float courseCenter(float z) {
    z = std::max(0.f, z);
    if (z <= kAnchor) return waveAt(z);
    float t = smooth((z - kAnchor) / 48.f);
    return waveAt(z) * (1.f - t) + waveAt(kAnchor) * t;
}

float courseHalf(float z) {
    z = std::max(0.f, z);
    float h = 6.9f;
    auto pinch = [&](float at, float wid, float depth) {
        float d = (z - at) / wid;
        h -= depth * std::exp(-d * d);
    };
    pinch(118.f, 20.f, 1.25f);
    pinch(214.f, 16.f, 1.35f);
    if (z < 28.f) {
        float t = smooth(z / 28.f);
        h = 10.5f * (1.f - t) + h * t;
    }
    if (z > kLeg - 44.f) {
        float t = smooth((z - (kLeg - 44.f)) / 44.f);
        h = h * (1.f - t) + (kGateHalf + 1.9f) * t;
    }
    return h;
}

float camber(float z) {
    float ramp = smooth((z - 20.f) / 36.f);
    float ease = 1.f - smooth((z - (kLeg - 64.f)) / 36.f);
    return ramp * ease * (0.85f * std::sin(z * 0.023f) + 0.32f * std::sin(z * 0.051f + 0.6f));
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
float Game::eye() const { return mode_ == Mode::Title ? 5.4f : 3.9f; }
float Game::back() const { return mode_ == Mode::Title ? 12.4f : 7.6f; }
float Game::horizon() const { return mode_ == Mode::Title ? 64.f : 78.f; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (kLeg - z_ < 58.f) return 3;
    if (std::fabs(lateral()) > courseHalf(z_) * 0.62f) return 2;
    return 1;
}

int Game::bankFrame() const {
    float b = std::clamp(yaw_ * 1.2f, -1.f, 1.f);
    if (b > 0.26f) return 2;
    if (b < -0.26f) return 0;
    return 1;
}

void Game::begin() {
    z_ = 16.f;
    x_ = courseCenter(z_);
    float ahead = courseCenter(z_ + 8.f);
    heading_ = std::atan2(ahead - x_, 8.f);
    speed_ = 0.f;
    tuck_ = 0.f;
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
    for (int i = 0; i < 20; i++) {
        flakes_[i].x = float((i * 53) % gs::SCREEN_W);
        flakes_[i].y = float((i * 71) % gs::SCREEN_H);
        flakes_[i].s = 2.f + float(i % 3);
        flakes_[i].v = 22.f + float((i * 5) % 8) * 5.f;
    }
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    blip(480.f);
}

void Game::buildCourse() {
    props_.clear();
    auto add = [&](float x, float z, float h, Kind k, int pal) { props_.push_back(Prop{x, z, h, k, pal}); };
    for (int i = 0; i < 20; i++) {
        float z = 24.f + float(i) * 16.f;
        if (z > kLeg - 10.f) break;
        float c = courseCenter(z);
        float w = courseHalf(z);
        add(c - w, z, 1.7f, Kind::Stake, PAL_WALL);
        add(c + w, z, 1.7f, Kind::Stake, PAL_WALL);
    }
    for (int i = 0; i < 16; i++) {
        float z = 40.f + float(i) * 18.f;
        if (z > kLeg - 14.f) break;
        int hsh = hashN(i * 17 + 3);
        float j = float(hsh % 7) * 0.18f;
        float th = 2.6f + float(hsh % 4) * 0.35f;
        float c = courseCenter(z);
        float w = courseHalf(z);
        add(c - w - 2.8f - j, z, th, Kind::Rock, PAL_ROCK);
        add(c + w + 3.1f + j * 0.5f, z + 2.f, th * 0.85f, Kind::Rock, PAL_ROCK);
    }
    for (int i = 0; i < 5; i++) {
        float z = 70.f + float(i) * 52.f;
        if (z > kLeg - 20.f) break;
        float side = (i & 1) ? 1.f : -1.f;
        add(courseCenter(z) + side * (courseHalf(z) + 2.2f), z, 3.4f, Kind::Lamp, PAL_ICE);
    }
    float gc = courseCenter(kLeg);
    add(gc - kGateHalf, kLeg, 4.8f, Kind::Post, PAL_GATE);
    add(gc + kGateHalf, kLeg, 4.8f, Kind::Post, PAL_GATE);
    add(gc, kLeg, 0.5f, Kind::Bar, PAL_GATE);
    add(gc - kGateHalf, kLeg, 0.85f, Kind::Flag, PAL_END);
    add(gc + kGateHalf, kLeg, 0.85f, Kind::Flag, PAL_END);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.14f, 0.2f, 0.08f);
    buildCourse();
    if (bot_) startRun();
    else showTitle();
}

void Game::controls(float& steer, float& tuck) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(p.axisX, -1.f, 1.f);
    const bool up = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO);
    const bool down = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    if (up) tuck_ = std::min(1.f, tuck_ + kDt * 1.4f);
    if (down) tuck_ = std::max(-0.7f, tuck_ - kDt * 1.8f);
    if (!up && !down) tuck_ += (0.35f - tuck_) * (1.f - std::exp(-1.2f * kDt));
    if (p.accel > 0.08f) tuck_ = std::max(tuck_, p.accel);
    if (p.brake > 0.08f) tuck_ = std::min(tuck_, -p.brake);
    tuck = tuck_;
}

void Game::pilot(float& steer, float& tuck) {
    const float distEnd = kLeg - z_;
    const float off = x_ - courseCenter(z_);
    const float half = courseHalf(z_);
    const float margin = half - std::fabs(off);

    if (distEnd < 52.f) {
        float aim = courseCenter(kLeg) - x_;
        float hDes = std::atan2(aim, std::max(7.f, distEnd));
        steer = std::clamp(wrap(hDes - heading_) / 0.24f, -1.f, 1.f);
        float want = std::fabs(aim) > 0.7f ? 7.4f : 11.5f;
        if (distEnd < 14.f) want = std::fabs(aim) > 0.3f ? 6.2f : 9.6f;
        tuck = std::clamp((want - speed_) * 0.28f, -0.6f, 1.f);
        return;
    }
    if (margin < 2.05f) {
        float hDes = std::atan2(-off, margin < 1.f ? 7.f : 12.f);
        steer = std::clamp(wrap(hDes - heading_) / 0.18f, -1.f, 1.f);
        tuck = margin < 1.f ? -0.15f : 0.25f;
        return;
    }
    float look = std::clamp(16.f + speed_ * 0.35f, 16.f, 28.f);
    float tx = courseCenter(z_ + look);
    tx -= camber(z_) * look / 10.f;
    float hDes = std::atan2(tx - x_, look);
    float herr = wrap(hDes - heading_);
    steer = std::clamp(herr / 0.32f, -1.f, 1.f);
    float want = std::fabs(herr) > 0.34f ? 10.f : 14.2f;
    tuck = std::clamp((want - speed_) * 0.22f, 0.15f, 1.f);
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
            if (std::fabs(wx - courseCenter(wz)) > courseHalf(wz) + 0.04f) left = true;
        } else if (std::fabs(wx - gateC) > kGateHalf + 0.05f) {
            bad = true;
        }
    }
    if (left) fail("left the lane");
    else if (bad) fail("missed the end");
    else if (allThrough) win();
    else if (race_ >= kClock) fail("missed the end");
}

void Game::physics(float steer, float tuck) {
    steer = std::clamp(steer, -1.f, 1.f);
    tuck = std::clamp(tuck, -1.f, 1.f);
    float sp = std::fabs(speed_);
    float rate = 2.05f - std::min(sp, 14.f) * 0.055f;
    if (sp < 4.f) rate += 0.5f;
    float yaw = steer * rate;
    heading_ = wrap(heading_ + yaw * kDt);
    yaw_ += (yaw - yaw_) * (1.f - std::exp(-9.f * kDt));

    float grade = 7.4f + tuck * 6.8f;
    if (tuck < 0.f) grade += tuck * 4.f;
    float drag = 0.42f + sp * 0.055f;
    speed_ += (grade - drag * sp) * kDt;
    speed_ = std::clamp(speed_, 0.f, kCap);

    float drift = camber(z_) * (speed_ / kCap);
    x_ += (std::sin(heading_) * speed_ + drift) * kDt;
    z_ += std::cos(heading_) * speed_ * kDt;

    judge();
    if (mode_ != Mode::Run) return;

    float margin = courseHalf(z_) - std::fabs(lateral());
    if (margin < 1.7f && !warned_) {
        warned_ = true;
        blip(150.f);
    } else if (margin > 2.5f) {
        warned_ = false;
    }
    if (!sawEnd_ && kLeg - z_ < 60.f) {
        sawEnd_ = true;
        blip(640.f);
    }

    puffT_ -= kDt;
    if (puffT_ <= 0.f && sp > 4.f) {
        puffT_ = 0.05f;
        Puff w;
        w.x = x_ - std::sin(heading_) * (kHalfL + 0.2f);
        w.z = z_ - std::cos(heading_) * (kHalfL + 0.2f);
        w.life = 1.f;
        puffs_[puffN_] = w;
        puffN_ = (puffN_ + 1) % 10;
    }
    for (Puff& w : puffs_)
        if (w.life > 0.f) w.life -= kDt * 1.1f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    why_ = "held";
    chime();
    sys_->rumble(0.28f, 0.14f, 160);
    sys_->setLight(40, 170, 110);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    speed_ = 0.f;
    why_ = why;
    sys_->apu.noiseBurst(0.38f, 80.f, 0.36f);
    sys_->apu.tone(0, 58.f, 0.07f);
    tone0_ = 0.4f;
    sys_->rumble(0.5f, 0.1f, 160);
    sys_->setLight(170, 30, 30);
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
    float wind = mode_ == Mode::Run ? 0.01f + speed_ * 0.0012f : 0.006f;
    sys_->apu.noise(wind, 900.f, false);
    if (mode_ == Mode::Run && speed_ > 2.f) {
        float wob = 0.8f + 0.2f * std::sin(t_ * (8.f + speed_));
        sys_->apu.tone(2, 180.f + speed_ * 14.f, (0.006f + speed_ * 0.0014f) * wob);
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
            static const float notes[] = {440.f, 554.f, 659.f, 880.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 3)], 0.06f);
            tone0_ = 0.16f;
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    camBob_ = std::sin(t_ * 2.1f) * (mode_ == Mode::Run ? 0.04f + speed_ * 0.003f : 0.03f);
    for (Flake& f : flakes_) {
        f.y += f.v * kDt;
        f.x += std::sin(t_ * 0.8f + f.y * 0.02f) * 14.f * kDt;
        if (f.y > gs::SCREEN_H + 4.f) {
            f.y = -4.f;
            f.x = std::fmod(f.x + 90.f, float(gs::SCREEN_W));
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
            blip(380.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            float steer = 0.f, tuck = tuck_;
            if (bot_) pilot(steer, tuck);
            else controls(steer, tuck);
            tuck_ = tuck;
            physics(steer, tuck);
            if (mode_ == Mode::Run) {
                float margin = courseHalf(z_) - std::fabs(lateral());
                if (kLeg - z_ < 46.f && margin > 1.4f) sys.setLight(40, 160, 100);
                else if (margin < 1.8f) sys.setLight(170, 90, 30);
                else sys.setLight(40, 80, 140);
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
    if (rz > 54.f) fog = std::clamp(int((rz - 54.f) / 16.f), 0, 12);
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
    const uint16_t zenith = gs::rgb4(3, 5, 10);
    const uint16_t mid = gs::rgb4(7, 10, 14);
    const uint16_t haze = gs::rgb4(13, 14, 15);
    const uint16_t deep = gs::rgb4(5, 8, 12);
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
        float c1 = (courseCenter(z0 + 1.5f) - courseCenter(z0 - 1.5f)) / 3.f;
        float half = courseHalf(z0);
        float e0 = x0 - c0;
        float denom = C + c1 * S;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.pal = uint8_t(PAL_LANE);
        r.left = r.right = gs::GROUND_SNOWWALL;
        r.style = gs::ROAD_ICE;
        r.v = z0 * 28.f;
        r.band = (int(std::floor(z0 * 0.18f)) & 1) ? 1 : 0;
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
        if (dist > 68.f) fog = std::clamp(int((dist - 68.f) / 28.f), 0, 10);
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
    items.reserve(120);
    auto push = [&](float wx, float wy, float wz, const gs::Mipped& img, float worldH, int pal, bool shadow, float rzBias) {
        float sx, sy, scale;
        int fog;
        float rz;
        if (!project(wx, wy, wz, sx, sy, scale, fog, rz)) return;
        float sh = worldH * scale;
        if (sh < 1.5f || rz > 210.f) return;
        items.push_back(Item{rz + rzBias, sx, sy, sh, &img, pal, fog, shadow});
    };
    const float S = std::sin(heading_);
    const float C = std::cos(heading_);
    auto body = [&](float lx, float ly, float lz, const gs::Mipped& img, float worldH, int pal, bool shadow, float bias) {
        push(x_ + S * lz + C * lx, ly, z_ + C * lz - S * lx, img, worldH, pal, shadow, bias);
    };

    for (const Prop& p : props_) {
        switch (p.kind) {
        case Kind::Stake: push(p.x, p.h * 0.5f, p.z, art_.stake, p.h, p.pal, false, 0); break;
        case Kind::Rock: push(p.x, p.h * 0.5f, p.z, art_.rock, p.h, p.pal, false, 0); break;
        case Kind::Lamp: push(p.x, p.h * 0.5f, p.z, art_.lamp, p.h, p.pal, false, 0); break;
        case Kind::Post: push(p.x, p.h * 0.5f, p.z, art_.post, p.h, p.pal, false, 0); break;
        case Kind::Flag: {
            float flutter = 4.6f + std::sin(t_ * 3.4f + p.x) * 0.08f;
            push(p.x, flutter, p.z, art_.flag, p.h, p.pal, false, 0);
            break;
        }
        case Kind::Bar: {
            float span = kGateHalf * 2.f;
            float worldH = span * float(art_.bar.h) / float(std::max(art_.bar.w, 1));
            push(p.x, 3.9f, p.z, art_.bar, worldH, p.pal, false, 0);
            float signH = span * 0.58f * float(art_.end.h) / float(std::max(art_.end.w, 1));
            push(p.x, 4.55f, p.z - 0.3f, art_.end, signH, PAL_BANNER, false, -0.05f);
            break;
        }
        }
    }

    for (const Puff& w : puffs_) {
        if (w.life <= 0.f) continue;
        push(w.x, 0.15f, w.z, art_.spray, 0.4f + (1.f - w.life) * 0.8f, PAL_SPRAY, false, 0);
    }

    float bob = std::sin(t_ * 2.6f) * 0.03f;
    body(0.f, 0.12f, 0.2f, art_.shadow, 0.9f, PAL_RIDER, true, 0.5f);
    body(0.f, 0.28f, 0.55f, art_.pod, 0.55f, PAL_RIDER, false, 0.1f);
    body(0.f, kRiderH * 0.42f + bob, -0.15f, art_.rider[bankFrame()], kRiderH, PAL_RIDER, false, 0.f);
    if (speed_ > 3.f) {
        body(-0.28f, 0.12f, -1.15f, art_.spray, 0.48f, PAL_SPRAY, false, -0.12f);
        body(0.28f, 0.12f, -1.15f, art_.spray, 0.48f, PAL_SPRAY, false, -0.12f);
    }

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.rz < b.rz; });
    for (const Item& it : items) spr(*it.img, it.sx, it.sy, it.sh, it.pal, it.fog, it.shadow);
}

void Game::drawSky() {
    const float hor = horizon();
    spr(art_.sun, 250.f, hor - 18.f, 18.f, PAL_SKY, 0, false);
    spr(art_.cloud, 70.f + std::sin(t_ * 0.1f) * 6.f, 28.f, 26.f, PAL_SKY, 0, false);
    spr(art_.cloud, 190.f + std::cos(t_ * 0.08f) * 8.f, 40.f, 18.f, PAL_SKY, 2, false);
    for (const Flake& f : flakes_) spr(art_.flake, f.x, f.y, f.s, PAL_SPRAY, 0, false);
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
        else hudC(27, "ARROWS STEER   UP TUCKS", PAL_HUD);
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

    hud(1, 0, "S3 LUGE LANE", PAL_BANNER);
    int left = std::max(0, int(std::ceil(kClock - race_ - 1e-3f)));
    std::snprintf(buf, sizeof buf, "TIME %d", mode_ == Mode::Win ? int(race_ + 0.5f) : left);
    hud(31, 0, buf, (mode_ == Mode::Run && left <= 8) ? PAL_ALERT : PAL_TAG);

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
    if (distEnd < 64.f) {
        line = "LINE UP THE END";
        pal = PAL_BANNER;
    } else if (half - std::fabs(off) < 1.8f) {
        line = "NEAR THE WALL";
        pal = PAL_ALERT;
    }
    hud(1, 1, line, pal);
    std::snprintf(buf, sizeof buf, "END %.0f", distEnd);
    hud(32, 1, buf, PAL_TAG);

    char g[16];
    for (int i = 0; i < 15; i++) g[i] = '-';
    g[15] = 0;
    if (distEnd < 100.f) {
        auto tick = [&](float worldOff) {
            float u = std::clamp(worldOff / half, -1.f, 1.f);
            int i = int(std::lround((u + 1.f) * 0.5f * 14.f));
            if (i >= 0 && i < 15) g[i] = '+';
        };
        tick(-kGateHalf);
        tick(kGateHalf);
    }
    int pod = int(std::lround((std::clamp(off / half, -1.f, 1.f) + 1.f) * 0.5f * 14.f));
    pod = std::clamp(pod, 0, 14);
    g[pod] = 'O';
    std::snprintf(buf, sizeof buf, "LANE <%s>", g);
    hud(1, 2, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "OFF %+.1f  SPD %.0f", off, speed_);
    hud(1, 3, buf, PAL_TAG);
    hud(1, 27, "ARROWS STEER   UP TUCKS   DOWN BRAKES", PAL_HUD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    drawLane();
    drawHud();
    drawWorld();
    drawSky();
}

}  // namespace lugelane
