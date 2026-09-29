#include "game/cliff.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cliffbox {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kBox0 = 160.f;
constexpr float kBox1 = 196.f;
constexpr float kCar = 7.f;
constexpr float kHalfU = 0.15f;
constexpr float kBoxU = 0.46f;
constexpr float kStop = 0.7f;
constexpr float kHoldNeed = 0.40f;
constexpr float kStillNeed = 0.90f;
constexpr float kClock = 42.f;
constexpr float kHorizon = 76.f;
constexpr float kScale = 96.f;
constexpr float kHw = 1.08f;
constexpr float kSteer = 1.15f;
constexpr float kMaxSpd = 24.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float bend(float z) { return std::sin(z * 0.030f) * 0.62f + std::sin(z * 0.013f) * 0.22f; }

float roadRate(float z) {
    return std::cos(z * 0.030f) * 0.62f * 0.030f + std::cos(z * 0.013f) * 0.22f * 0.013f;
}

}  // namespace

void Game::begin() {
    z_ = 6.f;
    u_ = 0.f;
    speed_ = 0.f;
    hold_ = 0.f;
    still_ = 0.f;
    raceT_ = 0.f;
    clock_ = kClock;
    won_ = false;
    over_ = false;
    why_.clear();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(9, 6, 4));
    sys.apu.setMaster(0.6f);
    sys.apu.setEcho(0.08f, 0.14f, 0.06f);
    begin();
    mode_ = bot_ ? Mode::Run : Mode::Title;
}

void Game::controls(float& gas, float& brake, float& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    gas = 0.f;
    brake = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (p.axisX > 0.18f || p.axisX < -0.18f) steer = clampf(p.axisX, -1.f, 1.f);
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C)) gas = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.15f) brake = 1.f;
    if (p.accel > 0.15f) gas = std::max(gas, p.accel);
}

void Game::pilot(float& gas, float& brake, float& steer) {
    const float stopZ = kBox0 + kCar + 9.f;
    const float dist = stopZ - z_;
    const float drift = roadRate(z_ + 10.f) * speed_;
    steer = clampf((drift + u_ * 2.6f) / kSteer, -1.f, 1.f);
    float want = dist > 28.f ? 21.f : std::max(0.f, dist * 0.52f);
    if (dist < 2.5f) want = 0.f;
    if (carInside() && speed_ < kStop + 1.2f) {
        gas = 0.f;
        brake = speed_ > 0.15f ? 1.f : 0.f;
        steer = clampf(u_ * 3.f, -1.f, 1.f);
        return;
    }
    if (speed_ > want + 0.35f) {
        gas = 0.f;
        brake = 1.f;
    } else if (speed_ < want - 0.4f) {
        gas = dist > 24.f ? 1.f : 0.65f;
        brake = 0.f;
    } else {
        gas = 0.2f;
        brake = 0.f;
    }
}

bool Game::carInside() const {
    const float tail = z_ - kCar;
    if (tail < kBox0 || z_ > kBox1) return false;
    return std::fabs(u_) + kHalfU <= kBoxU;
}

void Game::update(float dt) {
    float gas = 0.f, brake = 0.f, steer = 0.f;
    if (bot_) pilot(gas, brake, steer);
    else controls(gas, brake, steer);

    if (gas > 0.f) speed_ += gas * 13.5f * dt;
    if (brake > 0.f) speed_ -= brake * 26.f * dt;
    speed_ -= speed_ * 0.28f * dt;
    speed_ = clampf(speed_, -7.f, kMaxSpd);

    float push = roadRate(z_) * std::max(speed_, 0.f);
    u_ += (push - steer * kSteer) * dt;
    z_ += speed_ * dt;
    if (z_ < 0.f) z_ = 0.f;

    raceT_ += dt;
    clock_ -= dt;
    bool in = carInside();
    if (in && std::fabs(speed_) < kStop) hold_ += dt;
    else hold_ = 0.f;
    if (!in && std::fabs(speed_) < kStop * 0.65f && z_ < kBox1) still_ += dt;
    else still_ = 0.f;

    if (hold_ >= kHoldNeed) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        why_ = "stopped inside the box";
        sys_->apu.tone(0, 523.f, 0.12f);
        tone_ = 0.28f;
    } else if (z_ > kBox1) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "missed the end";
    } else if (std::fabs(u_) > 1.02f) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "left the cliff";
    } else if (still_ >= kStillNeed) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = z_ + 1.f < kBox0 ? "stopped short of the box" : "stopped outside the box";
    } else if (clock_ <= 0.f) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "missed the end";
        clock_ = 0.f;
    }

    if (tone_ > 0.f) {
        tone_ -= dt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0, 0);
    } else if (mode_ == Mode::Run && speed_ > 1.f) {
        sys_->apu.tone(1, 55.f + speed_ * 4.5f, 0.035f);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 2.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

bool Game::project(float lat, float z, float& sx, float& sy, float& sh, int& fog) const {
    float ahead = z - z_;
    if (ahead < 1.1f || ahead > 220.f) return false;
    float row = kScale / ahead;
    sy = kHorizon + row;
    if (sy < kHorizon - 2.f || sy > gs::SCREEN_H + 8.f) return false;
    float hw = kHw * row;
    float cx = 160.f + (bend(z) - bend(z_) - u_) * hw;
    sx = cx + lat * hw;
    sh = row * 0.42f;
    fog = int(std::clamp(10.f - row * 0.09f, 0.f, 10.f));
    return true;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.roadTime = int(t_ * 60.f);

    const float viewZ = mode_ == Mode::Title ? 48.f : z_;
    const float viewU = mode_ == Mode::Title ? 0.f : u_;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& r = v.road[y];
        if (y <= int(kHorizon)) {
            float t = float(y) / kHorizon;
            int sky = int((1.f - t) * 4.f);
            v.lineBackdrop[y] = gs::rgb4(3 + sky, 4 + int(t * 6.f), 8 + int((1.f - t) * 4.f));
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y) - kHorizon;
        float ahead = kScale / std::max(row, 1.f);
        float wz = viewZ + ahead;
        r.on = true;
        float hw = kHw * row;
        r.hw = hw;
        r.cx = 160.f + (bend(wz) - bend(viewZ) - viewU) * hw;
        r.v = wz * 40.f;
        r.pal = PAL_ROAD;
        bool inBox = wz >= kBox0 && wz <= kBox1;
        r.style = inBox ? 1 : gs::ROAD_ROCKY;
        r.band = (int(std::floor(wz / 8.f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_DROP;
        v.lineFog[y] = uint8_t(std::clamp(int(9.f - row * 0.07f), 0, 8));
        float sea = std::clamp(row / 140.f, 0.f, 1.f);
        v.lineBackdrop[y] = gs::rgb4(1, 2 + int(sea * 3.f), 4 + int((1.f - sea) * 5.f));
    }

    auto prop = [&](float lat, float z, float mul, const gs::Mipped& img, int pal) {
        float sx, sy, sh;
        int fog;
        float savedZ = z_, savedU = u_;
        z_ = viewZ;
        u_ = viewU;
        bool ok = project(lat, z, sx, sy, sh, fog);
        z_ = savedZ;
        u_ = savedU;
        if (!ok) return;
        (void)fog;
        spr(img, sx, sy, std::max(6.f, sh * mul), pal, lat < 0.f);
    };

    if (mode_ == Mode::Title) {
        prop(-1.15f, viewZ + 18.f, 2.4f, art_.rock, PAL_ROCK);
        prop(1.18f, viewZ + 26.f, 2.1f, art_.rock, PAL_ROCK);
        prop(0.f, kBox0 + 4.f, 1.6f, art_.sign, PAL_SIGN);
    } else {
        for (int i = 0; i < 7; i++) {
            float zz = std::floor(z_ / 18.f) * 18.f + float(i) * 18.f + 8.f;
            float side = (i & 1) ? 1.2f : -1.22f;
            prop(side, zz, 2.2f, art_.rock, PAL_ROCK);
        }
        prop(-kBoxU, kBox0, 1.35f, art_.post, PAL_POST);
        prop(kBoxU, kBox0, 1.35f, art_.post, PAL_POST);
        prop(-kBoxU, kBox1, 1.35f, art_.post, PAL_POST);
        prop(kBoxU, kBox1, 1.35f, art_.post, PAL_POST);
        prop(0.f, kBox0 + 2.f, 1.5f, art_.sign, PAL_SIGN);
    }

    if (mode_ != Mode::Title) {
        float lean = (mode_ == Mode::Run ? u_ : 0.f) * 10.f;
        spr(art_.car, 160.f + lean, 210.f, 52.f, PAL_CAR, false);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(8, "S3 CLIFF BOX", 3);
        hudC(12, "STOP INSIDE THE BOX", 1);
        hudC(14, "MISSING THE END FAILS THE LEG", 4);
        hudC(18, "LEFT RIGHT  STEER", 1);
        hudC(20, "UP         GAS", 1);
        hudC(21, "DOWN       BRAKE", 1);
        if ((int(t_ * 2.f) & 1) == 0) hudC(24, "PRESS START", 3);
    } else if (mode_ == Mode::Win) {
        hudC(8, "LEG CLEAR", 5);
        hudC(10, "STOPPED INSIDE THE BOX", 1);
    } else if (mode_ == Mode::Lose) {
        hudC(8, "LEG FAILED", 4);
        std::string up = why_;
        for (char& c : up)
            if (c >= 'a' && c <= 'z') c = char(c - 32);
        hudC(10, up, 1);
        hudC(14, "PRESS START", 3);
    } else {
        std::snprintf(line, sizeof line, "SPD %d", int(std::fabs(speed_) * 4.f));
        hud(1, 1, line, 1);
        int left = int(std::max(0.f, kBox0 - z_));
        std::snprintf(line, sizeof line, "BOX %d", left);
        hud(30, 1, line, 3);
        std::snprintf(line, sizeof line, "CLK %d", int(std::ceil(std::max(clock_, 0.f))));
        hud(1, 26, line, clock_ < 8.f ? 4 : 1);
        if (carInside()) hudC(3, "IN THE BOX", 5);
        else if (z_ > kBox0 - 30.f) hudC(3, "BRAKE FOR THE BOX", 3);
    }
    hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, 6);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& p = sys.pad;
    t_ += kDt;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || bot_) {
            begin();
            mode_ = Mode::Run;
        }
    } else if (mode_ == Mode::Run) {
        update(kDt);
    } else if (!bot_ && p.pressed(gs::BTN_START)) {
        begin();
        mode_ = Mode::Run;
    }
    draw();
}

}  // namespace cliffbox
