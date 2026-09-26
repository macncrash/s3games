#include "turn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace skiffturn {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kTip = 0.96f;
constexpr float kHeelGain = 0.064f;
constexpr float kHeelK = 8.5f;
constexpr float kHeelDamp = 4.6f;
constexpr float kCapFwd = 12.8f;
constexpr float kCapRev = 4.2f;
constexpr float kYawBase = 0.48f;
constexpr float kYawPer = 0.088f;
constexpr float kClock = 96.f;
constexpr float kFocal = 268.f;
constexpr float kBoatH = 3.9f;

// Three bends. Each opens straight, hooks, and closes straight. exitZ is on the straight after.
struct Bend {
    float z0, z1, x0, x1, exitZ;
    int dir;
};

constexpr Bend kBends[3] = {
    {40.f, 104.f, 0.f, -28.f, 126.f, -1},
    {142.f, 222.f, -28.f, 26.f, 246.f, 1},
    {258.f, 322.f, 26.f, -18.f, 350.f, -1},
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
    float dx = courseX(z + 2.5f) - courseX(z - 2.5f);
    return std::atan2(dx, 5.f);
}

float courseHalf(float z) {
    float h = 11.6f;
    for (const Bend& b : kBends) {
        if (z > b.z0 && z < b.z1) {
            float t = (z - b.z0) / (b.z1 - b.z0);
            h += 2.3f * std::sin(t * kPi);
        }
    }
    if (z < 22.f) h += (22.f - z) * 0.12f;
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

float Game::eye() const { return mode_ == Mode::Title ? 5.4f : 4.15f; }
float Game::back() const { return mode_ == Mode::Title ? 12.4f : 8.1f; }
float Game::horizon() const { return mode_ == Mode::Title ? 74.f : 80.f; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (turns_ >= 2) return 3;
    if (turns_ >= 1 || inBend(z_)) return 2;
    return 1;
}

int Game::heelFrame() const {
    float u = std::clamp(heel_ / 0.72f, -1.f, 1.f);
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
    wakeT_ = 0.f;
    wakeN_ = 0;
    turns_ = 0;
    won_ = false;
    over_ = false;
    warned_ = false;
    why_ = "";
    chimeN_ = 0;
    chimeStep_ = 0;
    chimeT_ = 0.f;
    tone0_ = 0.f;
    tone1_ = 0.f;
    for (int i = 0; i < 3; i++) made_[i] = false;
    for (Wake& w : wakes_) w = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    z_ = 72.f;
    x_ = courseX(z_);
    heading_ = courseHeading(z_);
    heel_ = 0.42f;
    speed_ = 7.f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    blip(640.f);
}

void Game::buildCourse() {
    props_.clear();
    props_.reserve(120);
    for (int i = 0; i < 22; i++) {
        float z = 10.f + float(i) * 16.5f;
        float c = courseX(z);
        float h = courseHalf(z);
        props_.push_back(Prop{c + h + 0.2f, z, 1.65f, Kind::BuoyR, PAL_RED, 0});
        props_.push_back(Prop{c - h - 0.2f, z + 8.f, 1.65f, Kind::BuoyG, PAL_GREEN, 0});
    }
    for (int i = 0; i < 36; i++) {
        float z = 6.f + float(i) * 10.5f;
        float c = courseX(z);
        float h = courseHalf(z);
        int n = hashN(i * 17 + 3);
        float j = float(n % 100) / 100.f;
        float side = (i & 1) ? 1.f : -1.f;
        props_.push_back(Prop{c + side * (h + 1.5f + j * 1.8f), z + float(n % 7) * 0.2f, 1.35f + j * 0.45f, Kind::Reed,
                               PAL_REED, 0});
    }
    for (int i = 0; i < 3; i++) {
        const Bend& b = kBends[i];
        float z = 0.5f * (b.z0 + b.z1);
        float side = float(-b.dir);
        props_.push_back(Prop{courseX(z) + side * (courseHalf(z) + 3.1f), z, 2.5f, Kind::Board, PAL_MARK, i});
    }
    props_.push_back(Prop{courseHalf(22.f) + 3.6f, 22.f, 1.3f, Kind::Dock, PAL_WOOD, 0});
    props_.push_back(Prop{-(courseHalf(38.f) + 6.f), 38.f, 2.6f, Kind::Shack, PAL_WOOD, 0});
    props_.push_back(Prop{courseX(54.f) - courseHalf(54.f) - 2.4f, 54.f, 1.7f, Kind::Heron, PAL_BIRD, 0});
    props_.push_back(Prop{courseX(200.f) + courseHalf(200.f) + 2.8f, 200.f, 1.7f, Kind::Heron, PAL_BIRD, 0});
    props_.push_back(Prop{courseX(300.f) - courseHalf(300.f) - 2.6f, 300.f, 1.7f, Kind::Heron, PAL_BIRD, 0});

    gulls_[0] = Gull{8.f, 60.f, 7.5f, 0.4f};
    gulls_[1] = Gull{-18.f, 150.f, 8.2f, 1.7f};
    gulls_[2] = Gull{20.f, 230.f, 6.8f, 2.8f};
    gulls_[3] = Gull{-6.f, 310.f, 9.f, 4.1f};
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildCourse();
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.14f, 0.24f, 0.16f);
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
    if (up) throttle_ = std::min(1.f, throttle_ + kDt * 0.85f);
    if (down) throttle_ = std::max(-0.45f, throttle_ - kDt * 1.15f);
    if (p.accel > 0.08f) throttle_ = std::max(throttle_, p.accel);
    if (p.brake > 0.08f) throttle_ = std::min(throttle_, -p.brake * 0.45f);
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    float look = std::clamp(14.f + speed_ * 0.45f, 14.f, 24.f);
    float ahead = courseX(z_ + look);
    float off = x_ - courseX(z_);
    float hDes = std::atan2(ahead - x_, look);
    hDes -= std::clamp(off * 0.055f, -0.40f, 0.40f);
    float err = wrap(hDes - heading_);
    float cmd = std::clamp(err / 0.38f, -1.f, 1.f);

    float curv = std::fabs(wrap(courseHeading(z_ + 16.f) - courseHeading(z_)));
    float half = courseHalf(z_);
    float wide = std::fabs(off) / std::max(half, 1.f);
    float want = curv > 0.16f ? 8.6f : 11.0f;
    float cap = 0.75f;
    if (wide > 0.42f) {
        want = 6.6f;
        cap = 0.9f;
    }
    // Positive steer heels to port. Ease off only the input that is still adding heel.
    if (std::fabs(heel_) > 0.58f) {
        want = std::min(want, 6.4f);
        bool adding = (cmd > 0.f && heel_ < 0.f) || (cmd < 0.f && heel_ > 0.f);
        if (adding) cmd *= 0.45f;
    }
    steer = std::clamp(cmd, -cap, cap);
    throttle = std::clamp(want / kCapFwd + (want - speed_) * 0.06f, 0.2f, 0.95f);
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
        bool inside = off <= courseHalf(z_) + 0.6f;
        bool aligned = herr <= 0.85f;
        bool moving = speed_ > 2.f;
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
        blip(520.f + float(i) * 110.f);
        sys_->rumble(0.18f, 0.08f, 90);
    }
}

void Game::physics(float steer, float throttle) {
    if (mode_ != Mode::Run) return;
    steer = std::clamp(steer, -1.f, 1.f);
    throttle = std::clamp(throttle, -1.f, 1.f);
    float zPrev = z_;

    float rate = kYawBase + std::min(std::fabs(speed_), kCapFwd) * kYawPer;
    float yawCmd = steer * rate;
    yaw_ += (yawCmd - yaw_) * (1.f - std::exp(-7.f * kDt));
    heading_ = wrap(heading_ + yaw_ * kDt);

    float cap = throttle >= 0.f ? kCapFwd : kCapRev;
    float target = throttle * cap;
    speed_ += (target - speed_) * (1.f - std::exp(-2.6f * kDt));
    speed_ = std::clamp(speed_, -kCapRev, kCapFwd);

    x_ += std::sin(heading_) * speed_ * kDt;
    z_ += std::cos(heading_) * speed_ * kDt;
    if (z_ < 4.f) {
        z_ = 4.f;
        if (speed_ < 0.f) speed_ = 0.f;
    }

    // Heel is outward. A bank catch shoves the target past the tip.
    float demand = -yaw_ * speed_ * kHeelGain;
    float off = x_ - courseX(z_);
    float over = std::fabs(off) - courseHalf(z_);
    if (over > 0.f) {
        float sgn = off > 0.f ? 1.f : -1.f;
        x_ -= sgn * std::min(over, 0.35f);
        speed_ *= std::max(0.62f, 1.f - std::min(over, 2.f) * 0.06f);
        demand += sgn * (1.25f + std::min(over, 2.f) * 0.7f);
    }
    heelVel_ += ((demand - heel_) * kHeelK - heelVel_ * kHeelDamp) * kDt;
    heel_ += heelVel_ * kDt;
    heel_ = std::clamp(heel_, -1.35f, 1.35f);
    if (std::fabs(heel_) > kTip) {
        fail(tipWhy());
        return;
    }

    if (std::fabs(heel_) > 0.62f && !warned_) {
        warned_ = true;
        blip(160.f);
    } else if (std::fabs(heel_) < 0.40f) {
        warned_ = false;
    }

    markTurns(zPrev);
    if (mode_ != Mode::Run) return;

    wakeT_ -= kDt;
    if (wakeT_ <= 0.f && std::fabs(speed_) > 2.2f) {
        wakeT_ = 0.08f;
        Wake w;
        w.x = x_ - std::sin(heading_) * 2.4f;
        w.z = z_ - std::cos(heading_) * 2.4f;
        w.life = 1.f;
        wakes_[wakeN_] = w;
        wakeN_ = (wakeN_ + 1) % 12;
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= kDt * 0.85f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "steady";
    chime();
    sys_->rumble(0.32f, 0.16f, 200);
    sys_->setLight(50, 190, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.48f, 70.f, 0.45f);
    sys_->apu.tone(0, 64.f, 0.08f);
    tone0_ = 0.48f;
    sys_->rumble(0.7f, 0.2f, 220);
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
    float water = mode_ == Mode::Run ? 0.014f + std::fabs(speed_) * 0.0006f : 0.01f;
    sys_->apu.noise(water, 420.f, false);
    if (mode_ == Mode::Run && (std::fabs(throttle_) > 0.04f || std::fabs(speed_) > 1.5f)) {
        float wob = 0.72f + 0.28f * std::sin(t_ * (11.f + std::fabs(throttle_) * 16.f));
        float vol = (0.012f + std::fabs(throttle_) * 0.028f) * wob;
        sys_->apu.tone(2, 52.f + std::fabs(throttle_) * 30.f + std::fabs(speed_) * 0.45f, vol);
    } else if (tone0_ <= 0.f) {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && tone0_ <= 0.f) {
        if (std::fabs(heel_) > 0.55f) {
            float v = (std::fabs(heel_) - 0.55f) * 0.16f;
            sys_->apu.tone(0, 96.f + std::fabs(heel_) * 50.f, v);
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
            static const float notes[] = {349.f, 440.f, 523.25f, 698.46f};
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
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.4f);
    camBob_ = std::sin(t_ * 2.1f) * (mode_ == Mode::Run ? 0.045f + std::fabs(speed_) * 0.004f : 0.03f);
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
            if (mode_ == Mode::Run && race_ >= kClock) fail("missed the turns");
            if (mode_ == Mode::Run) {
                float ah = std::fabs(heel_);
                if (ah > kTip * 0.78f) sys.setLight(200, 40, 20);
                else if (ah > 0.45f) sys.setLight(180, 120, 30);
                else if (turns_ > 0) sys.setLight(40, 150, 70);
                else sys.setLight(30, 70, 110);
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
    if (rz > 48.f) fog = std::clamp(int((rz - 48.f) / 18.f), 0, 13);
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

void Game::drawCreek() {
    gs::VDP& v = sys_->vdp;
    const float hor = horizon();
    const float S = std::sin(heading_);
    const float C = std::cos(heading_);
    const float camX = x_ - S * back();
    const float camZ = z_ - C * back();
    const float camH = eye() + camBob_;
    const uint16_t zenith = gs::rgb4(3, 5, 10);
    const uint16_t mid = gs::rgb4(11, 8, 7);
    const uint16_t haze = gs::rgb4(15, 12, 8);
    const uint16_t deep = gs::rgb4(1, 4, 5);
    v.roadTime = int(t_ * 28.f);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (float(y) < hor) {
            float u = float(y) / std::max(hor, 1.f);
            v.lineBackdrop[y] = u < 0.5f ? lerpC(zenith, mid, u / 0.5f) : lerpC(mid, haze, (u - 0.5f) / 0.5f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = std::max(1.f, float(y) - hor);
        float dist = camH * kFocal / row;
        float x0 = camX + S * dist;
        float z0 = camZ + C * dist;
        float c0 = courseX(z0);
        float c1 = (courseX(z0 + 1.4f) - courseX(z0 - 1.4f)) / 2.8f;
        float half = courseHalf(z0);
        float e0 = x0 - c0;
        float denom = C + c1 * S;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.pal = uint8_t(PAL_CREEK);
        r.left = r.right = gs::GROUND_LAND;
        r.style = 2;
        r.v = z0 * 42.f + t_ * 18.f;
        r.band = (int(std::floor(z0 * 0.38f - t_ * 4.f)) & 1) ? 1 : 0;
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
        if (dist > 42.f) fog = std::clamp(int((dist - 42.f) / 24.f), 0, 12);
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

    for (const Prop& p : props_) {
        switch (p.kind) {
        case Kind::BuoyR:
        case Kind::BuoyG: push(p.x, p.h * 0.5f, p.z, art_.buoy, p.h, p.pal, false, 0); break;
        case Kind::Board: push(p.x, p.h * 0.5f, p.z, art_.board[p.num], p.h, p.pal, false, 0); break;
        case Kind::Reed: {
            float sway = std::sin(t_ * 1.5f + p.z * 0.17f) * 0.18f;
            push(p.x + sway, p.h * 0.5f, p.z, art_.reed, p.h, p.pal, false, 0);
            break;
        }
        case Kind::Heron: push(p.x, p.h * 0.5f, p.z, art_.heron, p.h, p.pal, false, 0); break;
        case Kind::Shack: push(p.x, p.h * 0.5f, p.z, art_.shack, p.h, p.pal, false, 0); break;
        case Kind::Dock: push(p.x, p.h * 0.45f, p.z, art_.dock, p.h, p.pal, false, 0); break;
        }
    }

    int flap = int(t_ * 4.5f) & 1;
    for (const Gull& g : gulls_) {
        float gx = g.x + std::sin(t_ * 0.4f + g.ph) * 6.f;
        float gz = g.z + std::cos(t_ * 0.28f + g.ph) * 4.f;
        float gy = g.y + std::sin(t_ * 1.4f + g.ph) * 0.35f;
        push(gx, gy, gz, art_.gull[flap], 0.8f, PAL_BIRD, false, 0);
    }

    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        float h = 0.5f + (1.f - w.life) * 1.15f;
        push(w.x, 0.12f, w.z, art_.foam, h, PAL_FOAM, false, 0);
    }
    if (mode_ == Mode::Title) {
        push(x_ - std::sin(heading_) * 2.2f, 0.1f, z_ - std::cos(heading_) * 2.2f, art_.foam, 0.9f, PAL_FOAM, false, 0);
    }

    float bob = std::sin(t_ * 2.4f) * 0.05f;
    float shake = shake_ * std::sin(t_ * 48.f) * 0.18f;
    bool tipped = mode_ == Mode::Fail && why_ && why_[0] == 't';
    push(x_ + shake, 0.08f, z_, art_.shadow, 1.35f, PAL_HULL, true, 0.8f);
    if (tipped) {
        push(x_, 0.7f + bob, z_, art_.wreck, 2.5f, PAL_HULL, false, 0);
        push(x_ + 0.8f, 0.25f, z_ - 0.4f, art_.spray, 1.1f, PAL_FOAM, false, -0.2f);
    } else {
        push(x_, 1.75f + bob, z_, art_.stern[heelFrame()], kBoatH, PAL_HULL, false, 0);
        if (std::fabs(heel_) > 0.34f && (mode_ == Mode::Run || mode_ == Mode::Title)) {
            float side = heel_ > 0.f ? 1.f : -1.f;
            float sx = x_ + std::cos(heading_) * side * 1.15f;
            float sz = z_ - std::sin(heading_) * side * 1.15f;
            push(sx, 0.25f, sz, art_.spray, 0.85f, PAL_FOAM, false, -0.15f);
        }
        if (std::fabs(speed_) > 2.f) {
            float sx = x_ - std::sin(heading_) * 1.6f;
            float sz = z_ - std::cos(heading_) * 1.6f;
            push(sx, 0.15f, sz, art_.foam, 0.7f, PAL_FOAM, false, -0.1f);
        }
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
    char buf[80];
    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, 0, false); };

    if (mode_ == Mode::Title) {
        banner(art_.title, 160.f, 18.f, PAL_BANNER);
        hudC(5, "MAKE THE THREE TURNS", PAL_HUD);
        hudC(6, "WITHOUT TIPPING", PAL_BANNER);
        hudC(7, "EASE THE TILLER IN THE BENDS", PAL_TAG);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "ARROWS STEER   UP DRIVES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 48.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        bool tipped = why_ && why_[0] == 't';
        banner(tipped ? art_.tipped : art_.missed, 160.f, 40.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.steady, 160.f, 36.f, PAL_WIN);
    }

    hud(1, 0, "S3 SKIFF TURN", PAL_BANNER);
    if (mode_ == Mode::Win) std::snprintf(buf, sizeof buf, "MADE 3/3");
    else std::snprintf(buf, sizeof buf, "MADE %d/3", turns_);
    hud(30, 0, buf, PAL_TAG);

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
        std::snprintf(buf, sizeof buf, "HEEL %.0f", heel_ * 57.2958f);
        hudC(9, buf, PAL_TAG);
        if (!bot_) hudC(26, "START TRIES AGAIN", PAL_HUD);
        return;
    }

    int show = std::min(turns_ + 1, 3);
    const char* line = "HOLD HER STEADY";
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
    if (std::fabs(heel_) > kTip * 0.72f) {
        line = "EASE OFF";
        pal = PAL_ALERT;
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
    int heelPal = std::fabs(n) > 0.75f ? PAL_ALERT : PAL_HUD;
    std::snprintf(buf, sizeof buf, "HEEL P%sS %+0.0f", bar, heel_ * 57.2958f);
    hud(1, 2, buf, heelPal);
    hud(1, 26, "ARROWS STEER   UP DRIVES   DOWN BACKS", PAL_HUD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    drawCreek();
    drawHud();
    drawWorld();
    float sunY = horizon() - 18.f;
    spr(art_.sun, 274.f, sunY, 20.f, PAL_SKY, 0, false);
    spr(art_.cloud, 58.f + std::sin(t_ * 0.16f) * 8.f, 28.f, 28.f, PAL_SKY, 0, false);
    spr(art_.cloud, 188.f + std::cos(t_ * 0.13f) * 10.f, 40.f, 20.f, PAL_SKY, 2, false);
}

}  // namespace skiffturn
