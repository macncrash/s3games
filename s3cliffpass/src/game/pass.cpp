#include "game/pass.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace cliffpass {

struct Rock {
    float z;
    float lat;
};

namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kFinish = 420.f;
constexpr float kCrew0 = 62.f;
constexpr float kCrewSpd = 15.2f;
constexpr float kHorizon = 78.f;
constexpr float kScale = 100.f;
constexpr float kHw = 1.05f;
constexpr float kSteer = 1.25f;
constexpr float kMaxSpd = 26.f;
constexpr int kRocks = 7;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float bend(float z) { return std::sin(z * 0.021f) * 0.72f + std::sin(z * 0.009f) * 0.28f; }

float roadRate(float z) {
    return std::cos(z * 0.021f) * 0.72f * 0.021f + std::cos(z * 0.009f) * 0.28f * 0.009f;
}

Rock rockAt(int i) {
    // Off the racing line so a centred car clears them. Sides alternate.
    static const Rock kTab[kRocks] = {
        {78.f, 0.62f},  {128.f, -0.64f}, {176.f, 0.58f}, {224.f, -0.70f},
        {268.f, 0.66f}, {312.f, -0.60f}, {358.f, 0.55f},
    };
    return kTab[i];
}

}  // namespace

void Game::begin() {
    z_ = 4.f;
    u_ = 0.f;
    speed_ = 10.f;
    crewZ_ = kCrew0;
    raceT_ = 0.f;
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
    sys.vdp.setFogColor(gs::rgb4(6, 7, 9));
    sys.apu.setMaster(0.55f);
    sys.apu.setEcho(0.1f, 0.16f, 0.05f);
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
    float want = 0.f;
    float soon = 1e9f;
    for (int i = 0; i < kRocks; i++) {
        Rock r = rockAt(i);
        float dz = r.z - z_;
        if (dz > 1.5f && dz < soon && dz < 42.f) {
            soon = dz;
            want = r.lat > 0.f ? -0.28f : 0.28f;
        }
    }
    if (soon > 28.f) want = 0.f;
    float drift = roadRate(z_ + 12.f) * speed_;
    steer = clampf((drift + (u_ - want) * 3.1f) / kSteer, -1.f, 1.f);
    gas = 1.f;
    brake = 0.f;
    if (std::fabs(u_) > 0.72f) {
        gas = 0.45f;
        brake = 0.35f;
    }
}

void Game::update(float dt) {
    float gas = 0.f, brake = 0.f, steer = 0.f;
    if (bot_) pilot(gas, brake, steer);
    else controls(gas, brake, steer);

    if (gas > 0.f) speed_ += gas * 16.f * dt;
    if (brake > 0.f) speed_ -= brake * 28.f * dt;
    speed_ -= speed_ * 0.22f * dt;
    speed_ = clampf(speed_, -4.f, kMaxSpd);

    float push = roadRate(z_) * std::max(speed_, 0.f);
    u_ += (push - steer * kSteer) * dt;
    z_ += speed_ * dt;
    if (z_ < 0.f) z_ = 0.f;

    crewZ_ += kCrewSpd * dt;
    raceT_ += dt;

    for (int i = 0; i < kRocks; i++) {
        Rock r = rockAt(i);
        if (std::fabs(z_ - r.z) < 2.4f && std::fabs(u_ - r.lat) < 0.26f) {
            speed_ *= 0.42f;
            u_ += (u_ - r.lat) * 0.4f;
            sys_->apu.tone(2, 90.f, 0.12f);
        }
    }

    if (z_ >= kFinish && crewZ_ < kFinish) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        why_ = "cleared the pass ahead of the other crew";
        sys_->apu.tone(0, 523.f, 0.14f);
        tone_ = 0.35f;
    } else if (crewZ_ >= kFinish) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "the other crew took the pass";
        crewZ_ = kFinish;
    } else if (std::fabs(u_) > 1.05f) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "left the cliff";
    }

    if (tone_ > 0.f) {
        tone_ -= dt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0, 0);
    } else if (mode_ == Mode::Run && speed_ > 1.f) {
        sys_->apu.tone(1, 48.f + speed_ * 5.f, 0.03f);
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog, bool flip) {
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
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

bool Game::project(float lat, float z, float& sx, float& sy, float& sh, int& fog) const {
    float ahead = z - z_;
    if (ahead < 1.2f || ahead > 240.f) return false;
    float row = kScale / ahead;
    sy = kHorizon + row;
    if (sy < kHorizon - 2.f || sy > gs::SCREEN_H + 12.f) return false;
    float hw = kHw * row;
    float cx = 160.f + (bend(z) - bend(z_) - u_) * hw;
    sx = cx + lat * hw;
    sh = row * 0.46f;
    fog = int(std::clamp(11.f - row * 0.08f, 0.f, 12.f));
    return true;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.roadTime = int(t_ * 60.f);

    const bool show = mode_ != Mode::Title;
    const float viewZ = show ? z_ : 36.f;
    const float viewU = show ? u_ : 0.f;
    const float crewView = show ? crewZ_ : kCrew0;
    float storm = clampf((crewView - kCrew0) / (kFinish - kCrew0), 0.f, 1.f);
    if (mode_ == Mode::Title) storm = 0.15f;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& r = v.road[y];
        if (y <= int(kHorizon)) {
            float t = float(y) / kHorizon;
            int gloom = int(storm * 6.f);
            v.lineBackdrop[y] = gs::rgb4(std::max(1, 4 + int((1.f - t) * 3.f) - gloom),
                                         std::max(1, 5 + int(t * 4.f) - gloom / 2),
                                         std::max(2, 9 + int((1.f - t) * 3.f) - gloom / 3));
            v.lineFog[y] = uint8_t(std::clamp(int(storm * 8.f * (1.f - t)), 0, 10));
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
        r.v = wz * 36.f;
        r.pal = PAL_ROAD;
        r.style = gs::ROAD_ROCKY;
        r.band = (int(std::floor(wz / 7.f)) & 1) ? 1 : 0;
        r.left = gs::GROUND_DROP;
        r.right = gs::GROUND_LAND;
        int fog = int(8.f - row * 0.055f + storm * 7.f);
        v.lineFog[y] = uint8_t(std::clamp(fog, 0, 14));
        v.lineBackdrop[y] = gs::rgb4(1, 2, 3 + int((1.f - storm) * 3.f));
    }

    auto prop = [&](float lat, float z, float mul, const gs::Mipped& img, int pal) {
        float sx, sy, sh;
        int fog;
        float vz = show ? z_ : viewZ;
        float vu = show ? u_ : viewU;
        float ahead = z - vz;
        if (ahead < 1.2f || ahead > 240.f) return;
        float row = kScale / ahead;
        sy = kHorizon + row;
        float hw = kHw * row;
        float cx = 160.f + (bend(z) - bend(vz) - vu) * hw;
        sx = cx + lat * hw;
        sh = row * mul;
        fog = int(std::clamp(11.f - row * 0.08f + storm * 4.f, 0.f, 14.f));
        spr(img, sx, sy, sh, pal, fog);
    };

    prop(-0.95f, kFinish, 0.95f, art_.gate, PAL_GATE);
    prop(0.95f, kFinish, 0.95f, art_.gate, PAL_GATE);
    for (int i = kRocks - 1; i >= 0; i--) {
        Rock r = rockAt(i);
        prop(r.lat, r.z, 0.7f, art_.rock, PAL_ROCK);
    }
    prop(0.f, crewView, 0.55f, art_.crew, PAL_CREW);

    if (show && mode_ == Mode::Run) {
        spr(art_.car, 160.f, 196.f, 46.f, PAL_CAR, 0);
    } else if (mode_ == Mode::Title) {
        spr(art_.car, 160.f, 188.f, 52.f, PAL_CAR, 0);
    }

    if (mode_ == Mode::Title) {
        hudC(3, "S3 CLIFF PASS", 3);
        hudC(5, "THE OTHER CREW IS THE STORM CLOCK", 1);
        hudC(7, S3_VERSION_STRING, 2);
        hudC(18, "CLEAR THE PASS BEFORE THEY DO", 1);
        hudC(20, "LEFT DROP  RIGHT WALL  DODGE ROCK", 6);
        hudC(23, "A OR START", 5);
    } else if (mode_ == Mode::Run) {
        float gap = crewZ_ - z_;
        char line[48];
        std::snprintf(line, sizeof(line), "CREW %s %d", gap >= 0.f ? "AHEAD" : "BEHIND", int(std::fabs(gap)));
        hud(1, 1, line, gap >= 0.f ? 4 : 5);
        float left = std::max(0.f, kFinish - crewZ_);
        std::snprintf(line, sizeof(line), "CLOCK %d", int(left));
        hud(28, 1, line, storm > 0.72f ? 4 : 3);
        std::snprintf(line, sizeof(line), "PASS %d", int(std::max(0.f, kFinish - z_)));
        hud(1, 26, line, 1);
        hud(30, 26, "STORM", 2);
    } else if (mode_ == Mode::Win) {
        hudC(10, "PASS CLEAR", 5);
        hudC(12, why_, 1);
        char line[40];
        std::snprintf(line, sizeof(line), "%.1f S", raceT_);
        hudC(14, line, 3);
    } else if (mode_ == Mode::Lose) {
        hudC(10, "STORM SEALED", 4);
        hudC(12, why_, 1);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (mode_ == Mode::Title) {
        const gs::Pad& p = sys.pad;
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) {
            begin();
            mode_ = Mode::Run;
        }
    } else if (mode_ == Mode::Run) {
        update(kDt);
    } else if (!bot_) {
        const gs::Pad& p = sys.pad;
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) {
            begin();
            mode_ = Mode::Run;
            over_ = false;
            won_ = false;
        }
    }
    draw();
}

}  // namespace cliffpass
