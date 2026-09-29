#include "game/cliff.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cliffgrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kGrass0 = 138.f;
constexpr float kGrass1 = 176.f;
constexpr float kCar = 6.4f;
constexpr float kHalfU = 0.17f;
constexpr float kPadU = 0.58f;
constexpr float kStop = 0.55f;
constexpr float kHoldNeed = 0.42f;
constexpr float kStillNeed = 0.85f;
constexpr float kClock = 38.f;
constexpr float kHorizon = 78.f;
constexpr float kScale = 92.f;
constexpr float kHw = 1.02f;
constexpr float kSteer = 1.2f;
constexpr float kMaxSpd = 23.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float bend(float z) { return std::sin(z * 0.027f) * 0.48f + std::sin(z * 0.011f) * 0.16f; }

float roadRate(float z) {
    return std::cos(z * 0.027f) * 0.48f * 0.027f + std::cos(z * 0.011f) * 0.16f * 0.011f;
}

bool grassZ(float z) { return z >= kGrass0 && z <= kGrass1; }

}  // namespace

void Game::begin() {
    z_ = 8.f;
    u_ = 0.f;
    speed_ = 4.f;
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
    sys.vdp.setFogColor(gs::rgb4(6, 8, 10));
    sys.apu.setMaster(0.55f);
    sys.apu.setEcho(0.06f, 0.12f, 0.05f);
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
    const float park = kGrass0 + kCar + 8.f;
    const float dist = park - z_;
    const float drift = roadRate(z_ + 12.f) * speed_;
    steer = clampf((drift + u_ * 2.8f) / kSteer, -1.f, 1.f);
    if (onGrass() && speed_ < kStop + 2.f) {
        gas = 0.f;
        brake = speed_ > 0.12f ? 1.f : 0.f;
        steer = clampf(u_ * 4.f, -1.f, 1.f);
        return;
    }
    float want = dist > 34.f ? 19.f : std::max(0.f, dist * 0.46f);
    if (dist < 3.f) want = 0.f;
    if (speed_ > want + 0.3f) {
        gas = 0.f;
        brake = 1.f;
    } else if (speed_ < want - 0.45f) {
        gas = dist > 30.f ? 1.f : 0.55f;
        brake = 0.f;
    } else {
        gas = 0.15f;
        brake = 0.f;
    }
}

bool Game::onGrass() const {
    if (z_ - kCar < kGrass0 || z_ > kGrass1) return false;
    return std::fabs(u_) + kHalfU <= kPadU;
}

void Game::update(float dt) {
    float gas = 0.f, brake = 0.f, steer = 0.f;
    if (bot_) pilot(gas, brake, steer);
    else controls(gas, brake, steer);

    const bool turf = z_ >= kGrass0 - 1.f && z_ <= kGrass1 + 1.f;
    if (gas > 0.f) speed_ += gas * (turf ? 9.f : 14.f) * dt;
    if (brake > 0.f) speed_ -= brake * (turf ? 32.f : 22.f) * dt;
    speed_ -= speed_ * (turf ? 1.6f : 0.22f) * dt;
    speed_ = clampf(speed_, -4.f, kMaxSpd);

    float push = roadRate(z_) * std::max(speed_, 0.f);
    u_ += (push - steer * kSteer) * dt;
    z_ += speed_ * dt;
    if (z_ < 0.f) z_ = 0.f;

    raceT_ += dt;
    clock_ -= dt;
    const bool planted = onGrass();
    if (planted && std::fabs(speed_) < kStop) hold_ += dt;
    else hold_ = 0.f;
    if (!planted && std::fabs(speed_) < kStop * 0.7f && z_ < kGrass1) still_ += dt;
    else still_ = 0.f;

    if (hold_ >= kHoldNeed) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        why_ = "landed on the grass and stopped";
        sys_->apu.tone(0, 494.f, 0.12f);
        tone_ = 0.32f;
    } else if (z_ > kGrass1) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "overshot the grass";
    } else if (std::fabs(u_) > 1.04f) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "left the cliff";
    } else if (still_ >= kStillNeed) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "stopped short of the grass";
    } else if (clock_ <= 0.f) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "the grass got away";
        clock_ = 0.f;
    }

    if (tone_ > 0.f) {
        tone_ -= dt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0, 0);
    } else if (mode_ == Mode::Run && speed_ > 1.f) {
        sys_->apu.tone(1, 48.f + speed_ * 3.8f, 0.03f);
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

bool Game::project(float lat, float z, float& sx, float& sy, float& sh) const {
    float ahead = z - z_;
    if (ahead < 1.2f || ahead > 210.f) return false;
    float row = kScale / ahead;
    sy = kHorizon + row;
    if (sy < kHorizon - 2.f || sy > gs::SCREEN_H + 8.f) return false;
    float hw = kHw * row;
    float cx = 160.f + (bend(z) - bend(z_) - u_) * hw;
    sx = cx + lat * hw;
    sh = row * 0.38f;
    return true;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.roadTime = int(t_ * 60.f);

    const float viewZ = mode_ == Mode::Title ? 52.f : z_;
    const float viewU = mode_ == Mode::Title ? 0.f : u_;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& r = v.road[y];
        if (y <= int(kHorizon)) {
            float t = float(y) / kHorizon;
            v.lineBackdrop[y] = gs::rgb4(4 + int((1.f - t) * 3.f), 7 + int(t * 4.f), 11 + int((1.f - t) * 3.f));
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y) - kHorizon;
        float ahead = kScale / std::max(row, 1.f);
        float wz = viewZ + ahead;
        r.on = true;
        float wide = grassZ(wz) ? 1.18f : 1.f;
        float hw = kHw * row * wide;
        r.hw = hw;
        r.cx = 160.f + (bend(wz) - bend(viewZ) - viewU) * (kHw * row);
        r.v = wz * 36.f;
        bool pad = grassZ(wz);
        r.pal = pad ? PAL_GRASS : PAL_ROCKROAD;
        r.style = pad ? 0 : gs::ROAD_ROCKY;
        r.band = (int(std::floor(wz / 7.f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_DROP;
        v.lineFog[y] = uint8_t(std::clamp(int(8.f - row * 0.06f), 0, 8));
        float sea = std::clamp(row / 130.f, 0.f, 1.f);
        v.lineBackdrop[y] = gs::rgb4(1, 3 + int(sea * 4.f), 6 + int((1.f - sea) * 4.f));
    }

    auto prop = [&](float lat, float z, float mul, const gs::Mipped& img, int pal) {
        float sx, sy, sh;
        float savedZ = z_, savedU = u_;
        z_ = viewZ;
        u_ = viewU;
        bool ok = project(lat, z, sx, sy, sh);
        z_ = savedZ;
        u_ = savedU;
        if (!ok) return;
        spr(img, sx, sy, std::max(5.f, sh * mul), pal, lat < 0.f);
    };

    if (mode_ == Mode::Title) {
        prop(-1.2f, viewZ + 16.f, 2.2f, art_.rock, PAL_ROCK);
        prop(1.15f, viewZ + 28.f, 2.0f, art_.rock, PAL_ROCK);
        prop(0.72f, kGrass0 + 6.f, 1.8f, art_.flag, PAL_FLAG);
        prop(-0.7f, kGrass0 + 10.f, 1.4f, art_.tuft, PAL_TUFT);
    } else {
        for (int i = 0; i < 6; i++) {
            float zz = std::floor(z_ / 22.f) * 22.f + float(i) * 22.f + 6.f;
            if (grassZ(zz)) continue;
            float side = (i & 1) ? 1.22f : -1.24f;
            prop(side, zz, 2.1f, art_.rock, PAL_ROCK);
        }
        for (int i = 0; i < 5; i++) {
            float zz = kGrass0 + 4.f + float(i) * 7.f;
            prop(-0.78f, zz, 1.3f, art_.tuft, PAL_TUFT);
            prop(0.8f, zz + 3.f, 1.2f, art_.tuft, PAL_TUFT);
        }
        prop(0.62f, kGrass0 + 2.f, 1.7f, art_.flag, PAL_FLAG);
        prop(-0.64f, kGrass1 - 4.f, 1.5f, art_.flag, PAL_FLAG);
    }

    if (mode_ != Mode::Title) {
        float lean = (mode_ == Mode::Run ? u_ : 0.f) * 12.f;
        spr(art_.car, 160.f + lean, 208.f, 48.f, PAL_CAR, false);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(8, "S3 CLIFF GRASS", 3);
        hudC(12, "LAND ON THE GRASS", 1);
        hudC(14, "COME TO A FULL STOP", 5);
        hudC(18, "LEFT RIGHT  STEER", 1);
        hudC(20, "UP         GAS", 1);
        hudC(21, "DOWN       BRAKE", 1);
        if ((int(t_ * 2.f) & 1) == 0) hudC(24, "PRESS START", 3);
    } else if (mode_ == Mode::Win) {
        hudC(8, "FULL STOP", 5);
        hudC(10, "ON THE GRASS", 1);
    } else if (mode_ == Mode::Lose) {
        hudC(8, "OFF THE GRASS", 4);
        std::string up = why_;
        for (char& c : up)
            if (c >= 'a' && c <= 'z') c = char(c - 32);
        hudC(10, up, 1);
        hudC(14, "PRESS START", 3);
    } else {
        std::snprintf(line, sizeof line, "SPD %d", int(std::fabs(speed_) * 4.f));
        hud(1, 1, line, 1);
        int left = int(std::max(0.f, kGrass0 - z_));
        std::snprintf(line, sizeof line, "PAD %d", left);
        hud(30, 1, line, 3);
        std::snprintf(line, sizeof line, "CLK %d", int(std::ceil(std::max(clock_, 0.f))));
        hud(1, 26, line, clock_ < 8.f ? 4 : 1);
        if (onGrass()) hudC(3, std::fabs(speed_) < kStop ? "HOLD STILL" : "BRAKE ON THE GRASS", 5);
        else if (z_ > kGrass0 - 36.f) hudC(3, "GRASS AHEAD", 3);
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

}  // namespace cliffgrass
