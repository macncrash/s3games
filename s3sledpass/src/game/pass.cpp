#include "pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace sledpass {
namespace {

constexpr int HORIZON = 74;
constexpr float FOCAL = 220.f;
constexpr float CAM_H = 1.52f;
constexpr float HALF_W = 2.55f;
constexpr float FINISH = 1080.f;
constexpr float CLOCK = 38.f;
constexpr float WALL = 0.97f;
constexpr float STEER_A = 12.f;
constexpr float CENT = 0.36f;
constexpr float GRIP = 0.78f;
constexpr float GROOVE = 1.25f;
constexpr float DAMP = 4.2f;
constexpr float V_MUSH = 39.f;
constexpr float V_COAST = 20.f;
constexpr float V_BRAKE = 14.f;
constexpr float VIS = 1.22f;
constexpr float DT = 1.f / 60.f;
constexpr int SHIFT_N = 220;
constexpr float SHIFT_DZ = 1.45f;
constexpr float NEAR_Z = 3.2f;
constexpr float FAR_Z = 260.f;

// Positive kappa bends the pass right and pushes the sled toward the left bank.
struct Bend {
    float a, b, c, d, k;
};

const Bend kBends[] = {
    {90.f, 150.f, 260.f, 340.f, 0.0064f},
    {400.f, 470.f, 600.f, 690.f, -0.0072f},
    {740.f, 800.f, 880.f, 950.f, 0.0060f},
};

float piece(float z, const Bend& r) {
    if (z <= r.a || z >= r.d) return 0.f;
    if (z < r.b) {
        float u = (z - r.a) / std::max(1.f, r.b - r.a);
        float s = u * u * (3.f - 2.f * u);
        return r.k * s;
    }
    if (z > r.c) {
        float u = (r.d - z) / std::max(1.f, r.d - r.c);
        float s = u * u * (3.f - 2.f * u);
        return r.k * s;
    }
    return r.k;
}

float hash01(int i) {
    uint32_t h = uint32_t(i) * 747796405u + 2891336453u;
    h ^= h >> 16;
    return float(h & 0xffffff) / float(0x1000000);
}

float lerp(float a, float b, float t) { return a + (b - a) * t; }

int ilerp(int a, int b, float t) { return int(std::lround(lerp(float(a), float(b), t))); }

}  // namespace

float Game::kappa(float z) const {
    float k = 0.f;
    for (const Bend& r : kBends) k += piece(z, r);
    return k;
}

float Game::feed(float k) const {
    return (k * speed_ * speed_ * CENT) / (STEER_A * GRIP);
}

float Game::lookX() const {
    if (mode_ == Mode::Title || mode_ == Mode::Go) return std::sin(modeTime_ * 0.55f) * 0.12f;
    return playerX_;
}

float Game::clockLeft() const { return std::max(0.f, CLOCK - raceTime_); }

float Game::stormAmt() const {
    if (mode_ == Mode::Title || mode_ == Mode::Go) return 0.08f;
    if (end_ == End::Storm) return 1.f;
    return std::clamp(1.f - clockLeft() / CLOCK, 0.f, 1.f);
}

const char* Game::why() const {
    switch (end_) {
        case End::Storm: return "the storm closed the pass";
        case End::Buried: return "buried in the bank";
        case End::Clear: return "clear";
        default: return "";
    }
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Go) return 1;
    if (mode_ == Mode::Result) return 4;
    if (clockLeft() < 12.f) return 3;
    return 2;
}

void Game::tune() {
    gs::FMPatch bell;
    bell.alg = 4;
    bell.fb = 0.18f;
    bell.vol = 0.18f;
    bell.echo = 0.28f;
    bell.glide = 0.03f;
    const float mul[4] = {1.f, 2.f, 3.2f, 4.5f};
    const float level[4] = {0.8f, 0.45f, 0.28f, 0.16f};
    for (int i = 0; i < 4; i++) {
        bell.op[i].mul = mul[i];
        bell.op[i].level = level[i];
        bell.op[i].ar = 0.004f;
        bell.op[i].dr = 0.22f;
        bell.op[i].sl = 0.15f;
        bell.op[i].rr = 0.4f;
    }
    for (int ch = 0; ch < 4; ch++) sys_->apu.setPatch(ch, bell);
    sys_->apu.setEcho(0.18f, 0.28f, 0.14f);
    sys_->apu.setMaster(0.8f);
    sys_->apu.silence();
}

void Game::buildCourse() {
    props_.clear();
    haz_.clear();
    for (float z = 48.f; z < FINISH - 30.f; z += 44.f) {
        int i = int(z);
        float side = ((i / 44) & 1) ? 1.f : -1.f;
        props_.push_back({z, side * (1.30f + hash01(i) * 0.16f), 4.4f + hash01(i + 3) * 2.6f, Kind::Pine});
        props_.push_back({z + 22.f, -side * 1.05f, 2.05f, Kind::Stake});
    }
    props_.push_back({230.f, -1.58f, 4.3f, Kind::Cabin});
    props_.push_back({610.f, 1.62f, 3.7f, Kind::Cabin});
    props_.push_back({1010.f, -1.52f, 4.5f, Kind::Cabin});
    props_.push_back({150.f, 1.46f, 1.7f, Kind::Boulder});
    props_.push_back({530.f, -1.50f, 2.2f, Kind::Boulder});
    props_.push_back({860.f, 1.40f, 1.6f, Kind::Boulder});
    props_.push_back({64.f, 0.f, 0.f, Kind::MushGate});
    props_.push_back({FINISH, 0.f, 0.f, Kind::PassGate});

    haz_.push_back({72.f, 0.62f, 0.16f, 0, false});
    haz_.push_back({366.f, -0.60f, 0.15f, 1, false});
    haz_.push_back({712.f, 0.58f, 0.16f, 0, false});
    haz_.push_back({990.f, -0.55f, 0.14f, 1, false});
}

void Game::resetRun() {
    over_ = false;
    won_ = false;
    end_ = End::None;
    mushing_ = false;
    braking_ = false;
    playerZ_ = 0;
    playerX_ = 0;
    latV_ = 0;
    speed_ = 0;
    yaw_ = 0;
    steerSm_ = 0;
    shake_ = 0;
    raceTime_ = 0;
    beepSec_ = -1;
    chime_ = 0;
    chimeT_ = 0;
    puffN_ = 0;
    for (Hazard& h : haz_) h.hit = false;
    for (Puff& p : puff_) p.life = 0;
    mode_ = Mode::Go;
    modeTime_ = 0;
}

void Game::launch() {
    mode_ = Mode::Run;
    modeTime_ = 0;
    raceTime_ = 0;
    speed_ = 24.f;
    playerZ_ = 0;
    chime_ = 2;
    chimeT_ = 0.02f;
    sys_->apu.keyOn(3, 392.f, 0.08f);
}

void Game::end(End e) {
    if (mode_ != Mode::Run) return;
    end_ = e;
    won_ = e == End::Clear;
    over_ = true;
    mode_ = Mode::Result;
    modeTime_ = 0;
    chime_ = won_ ? 4 : 2;
    chimeT_ = 0.02f;
    if (won_) {
        sys_->rumble(0.25f, 0.4f, 160);
        sys_->setLight(40, 150, 70);
    } else if (e == End::Storm) {
        shake_ = 0.35f;
        sys_->apu.noiseBurst(0.45f, 500.f, 0.28f);
        sys_->rumble(0.55f, 0.2f, 200);
        sys_->setLight(140, 150, 170);
    } else {
        shake_ = 0.55f;
        sys_->apu.noiseBurst(0.55f, 280.f, 0.22f);
        sys_->rumble(0.8f, 0.35f, 220);
        sys_->setLight(160, 30, 20);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildCourse();
    tune();
    draws_.reserve(128);
    shiftU_.assign(SHIFT_N + 1, 0.f);
    for (int i = 0; i < 22; i++) {
        flake_[i].x = hash01(i) * 320.f;
        flake_[i].y = hash01(i + 40) * 224.f;
        flake_[i].sp = 26.f + hash01(i + 80) * 90.f;
        flake_[i].sc = 2.4f + hash01(i + 120) * 3.2f;
    }
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.hudEnabled = true;
    over_ = false;
    won_ = false;
    end_ = End::None;
    mode_ = Mode::Title;
    modeTime_ = 0;
    playerZ_ = 0;
    playerX_ = 0;
    speed_ = 0;
}

void Game::human(float& steer, bool& mush, bool& brake) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.12f) steer = std::clamp(p.axisX, -1.f, 1.f);
    brake = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.22f;
    mush = !brake && (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) ||
                      p.accel > 0.22f);
}

void Game::bot(float& steer, bool& mush, bool& brake) const {
    const float look = 36.f;
    const float here = feed(kappa(playerZ_));
    const float ahead = feed(kappa(playerZ_ + look));
    float s = here * 0.40f + ahead * 0.60f - playerX_ * 1.35f - latV_ * 1.75f;
    if (playerX_ > 0.46f) s -= (playerX_ - 0.46f) * 2.8f;
    if (playerX_ < -0.46f) s += (-0.46f - playerX_) * 2.8f;
    if (std::fabs(playerX_) > 0.78f) s = -playerX_ * 2.4f - latV_ * 2.1f;

    for (const Hazard& h : haz_) {
        if (h.hit) continue;
        float dz = h.z - playerZ_;
        if (dz < 4.f || dz > 46.f) continue;
        float gate = h.r + 0.24f;
        float side = playerX_ - h.x;
        if (std::fabs(side) >= gate) continue;
        float away = side >= 0.f ? 1.f : -1.f;
        float urge = (gate - std::fabs(side)) / gate;
        float dest = playerX_ + away * 0.28f;
        if (std::fabs(dest) < 0.88f) s += away * urge * (0.7f + (1.f - dz / 46.f));
        else s -= std::copysign(urge * 1.2f, playerX_);
    }
    steer = std::clamp(s, -1.f, 1.f);
    brake = std::fabs(playerX_) > 0.80f;
    mush = !brake;
}

void Game::hazards() {
    for (Hazard& h : haz_) {
        if (h.hit) continue;
        float dz = h.z - playerZ_;
        if (dz > 11.f || dz < -1.2f) continue;
        if (std::fabs(playerX_ - h.x) > h.r + 0.08f) continue;
        h.hit = true;
        speed_ = std::max(V_BRAKE, speed_ * 0.72f);
        shake_ = std::max(shake_, 0.4f);
        sys_->apu.noiseBurst(0.28f, h.rock ? 340.f : 900.f, 0.12f);
        sys_->rumble(0.35f, 0.15f, 80);
        float side = playerX_ >= h.x ? 14.f : -14.f;
        spawnPuff(160.f + side, 168.f, side * 2.f, -30.f);
    }
}

void Game::spawnPuff(float x, float y, float vx, float vy) {
    Puff& p = puff_[puffN_++ % 16];
    p.x = x;
    p.y = y;
    p.vx = vx;
    p.vy = vy;
    p.life = 0.32f;
    p.sc = 3.f + hash01(puffN_) * 3.5f;
}

void Game::physics(float dt) {
    float steer = 0.f;
    bool mush = false, brake = false;
    if (bot_) bot(steer, mush, brake);
    else human(steer, mush, brake);
    mushing_ = mush;
    braking_ = brake;
    const float follow = bot_ ? 0.72f : std::min(1.f, dt * 16.f);
    steerSm_ += (steer - steerSm_) * follow;

    const float scale = STEER_A * GRIP / HALF_W;
    const float steerU = steerSm_ * scale;
    const float pushU = -kappa(playerZ_) * speed_ * speed_ * CENT / HALF_W;
    latV_ += (steerU + pushU - playerX_ * GROOVE) * dt;
    latV_ *= std::max(0.f, 1.f - DAMP * dt);
    latV_ = std::clamp(latV_, -3.2f, 3.2f);
    playerX_ += latV_ * dt;

    if (std::fabs(playerX_) > 0.86f) {
        float s = std::copysign(1.f, playerX_);
        latV_ -= s * 4.f * dt;
        speed_ -= 9.f * dt;
        shake_ = std::max(shake_, 0.18f);
    }
    if (std::fabs(playerX_) >= WALL) {
        end(End::Buried);
        return;
    }

    float target = mush ? V_MUSH : V_COAST;
    if (brake) target = V_BRAKE;
    const float rate = brake ? 14.f : mush ? 8.5f : 3.2f;
    if (speed_ < target) speed_ += rate * dt;
    else speed_ -= rate * dt;
    speed_ = std::clamp(speed_, 12.f, 46.f);

    hazards();

    playerZ_ += speed_ * dt;
    raceTime_ += dt;
    yaw_ += kappa(playerZ_) * speed_ * dt;

    // The clock is checked before the arch. Crossing as it hits zero still fails.
    if (raceTime_ >= CLOCK) {
        end(End::Storm);
        return;
    }
    if (playerZ_ >= FINISH) {
        end(End::Clear);
        return;
    }

    if ((sys_->frame & 1u) && speed_ > 22.f) {
        float side = (puffN_ & 1) ? 16.f : -16.f;
        spawnPuff(160.f + steerSm_ * 10.f + side, 200.f, side * 1.4f, -(18.f + hash01(puffN_ + 5) * 28.f));
    }
}

void Game::flakes(float dt) {
    const float rush = mode_ == Mode::Run ? std::max(0.4f, speed_ / V_MUSH) : 0.28f;
    const float blow = 0.4f + stormAmt() * 1.6f;
    for (int i = 0; i < 22; i++) {
        Flake& f = flake_[i];
        f.y += f.sp * rush * blow * dt;
        f.x += (f.x - 160.f) * 0.18f * rush * dt + std::sin(modeTime_ + float(i)) * 8.f * dt;
        if (f.y > 236.f || f.x < -16.f || f.x > 336.f) {
            f.y = -6.f;
            f.x = 16.f + hash01(i + int(modeTime_ * 9.f) + puffN_) * 288.f;
        }
    }
}

void Game::fadePuffs(float dt) {
    for (Puff& p : puff_) {
        if (p.life <= 0.f) continue;
        p.life -= dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy += 24.f * dt;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    modeTime_ += DT;
    if (shake_ > 0.f) shake_ -= DT;
    shakeX_ = 0;
    shakeY_ = 0;
    if (shake_ > 0.f) {
        int f = int(sys.frame);
        shakeX_ = (f * 17 % 5) - 2;
        shakeY_ = (f * 13 % 3) - 1;
    }
    flakes(DT);
    fadePuffs(DT);

    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
        if (sys.pad.pressed(gs::BTN_START) || (bot_ && modeTime_ > 0.4f)) resetRun();
    } else if (mode_ == Mode::Go) {
        if (modeTime_ > 0.65f) launch();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else physics(DT);
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
        resetRun();
    }

    audio(DT);
    draw();
}

void Game::audio(float dt) {
    if (chime_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float winN[] = {392.f, 523.25f, 659.25f, 784.f};
            static const float bark[] = {330.f, 494.f};
            if (mode_ == Mode::Result && won_) {
                int i = 4 - chime_;
                if (i >= 0 && i < 4) sys_->apu.keyOn(i % 3, winN[i], 0.16f);
            } else if (chime_ > 0 && mode_ != Mode::Result) {
                sys_->apu.tone(2, bark[2 - chime_], 0.04f);
            } else if (!won_) {
                sys_->apu.keyOn(0, 196.f, 0.14f);
                sys_->apu.keyOn(1, 155.f, 0.1f);
            }
            chime_--;
            chimeT_ = 0.14f;
            if (chime_ <= 0) {
                sys_->apu.tone(2, 0.f, 0.f);
            }
        }
    }

    if (mode_ == Mode::Result || mode_ == Mode::Pause || mode_ == Mode::Title || mode_ == Mode::Go) {
        if (mode_ != Mode::Result) {
            sys_->apu.noise(mode_ == Mode::Title ? 0.012f : 0.02f, 900.f, false);
            sys_->apu.tone(0, 0.f, 0.f);
            sys_->apu.tone(1, 0.f, 0.f);
        }
        return;
    }

    const float rush = speed_;
    sys_->apu.noise(0.016f + rush * 0.0011f + stormAmt() * 0.02f, 1400.f + rush * 28.f, false);
    sys_->apu.tone(0, 42.f + rush * 1.5f, mushing_ ? 0.014f : 0.008f);

    if (beepSec_ >= 0 && mode_ == Mode::Run) {
        // tone 1 is the clock. Held only for a short slice, cleared below.
    }
    int whole = int(clockLeft());
    if (whole <= 8 && whole >= 0 && whole != beepSec_ && clockLeft() > 0.05f) {
        beepSec_ = whole;
        sys_->apu.tone(1, whole <= 4 ? 880.f : 660.f, 0.05f);
    } else if (beepSec_ == whole) {
        // leave the beep to decay by rewriting a quieter tone after a few frames
        if (std::fmod(raceTime_, 1.f) > 0.08f) sys_->apu.tone(1, 0.f, 0.f);
    } else {
        sys_->apu.tone(1, 0.f, 0.f);
    }
}

float Game::shiftAt(float z) const {
    if (z <= 0.f || int(shiftU_.size()) < 2) return 0.f;
    float i = z / SHIFT_DZ;
    int n = int(i);
    if (n >= SHIFT_N) return shiftU_[size_t(SHIFT_N)];
    if (n < 0) return 0.f;
    float f = i - float(n);
    return shiftU_[size_t(n)] * (1.f - f) + shiftU_[size_t(n + 1)] * f;
}

void Game::queue(float z, const gs::Sprite& s) { draws_.push_back({z, s}); }

void Game::blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog, float z, bool shadow) {
    if (h < 2.f || cx < -180.f || cx > 500.f) return;
    const gs::Image& img = m.pick(h);
    if (img.h <= 0 || img.w <= 0) return;
    float sc = h / float(img.h);
    float w = float(img.w) * sc;
    gs::Sprite s;
    s.img = img;
    s.w = std::max(1, int(std::lround(w)));
    s.h = std::max(1, int(std::lround(h)));
    s.x = int(std::lround(cx - w * 0.5f));
    s.y = int(std::lround(foot - h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    queue(z, s);
}

void Game::stretch(const gs::Mipped& m, float x, float y, float w, float h, int pal, int fog, float z) {
    if (w < 1.f || h < 1.f) return;
    const gs::Image& img = m.pick(h);
    if (img.w <= 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = std::max(1, int(std::lround(std::min(w, 420.f))));
    s.h = std::max(1, int(std::lround(h)));
    s.x = int(std::lround(x));
    s.y = int(std::lround(y));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    queue(z, s);
}

void Game::ui(const gs::Image& img, float x, float y, int pal) {
    if (img.w <= 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int(std::lround(x));
    s.y = int(std::lround(y));
    s.pal = uint8_t(pal);
    queue(-8.f, s);
}

Game::Proj Game::project(float worldZ, float roadX) const {
    Proj p{};
    float dz = worldZ - camZ();
    p.z = dz;
    if (dz < NEAR_Z || dz > FAR_Z) return p;
    p.y = float(hor_) + FOCAL * CAM_H / dz;
    p.hw = FOCAL * HALF_W / dz;
    float su = std::clamp(shiftAt(dz), -14.f, 14.f);
    p.x = 160.f + float(shakeX_) + (su - lookX()) * p.hw + roadX * p.hw;
    float base = std::clamp((dz - 28.f) / 20.f, 0.f, 12.f);
    p.fog = std::clamp(base + stormAmt() * stormAmt() * 7.f, 0.f, 15.f);
    p.ok = p.y > -30.f && p.y < float(gs::SCREEN_H + 48);
    return p;
}

void Game::sky() {
    int bob = 0;
    if (mode_ == Mode::Run) bob = int(std::sin(raceTime_ * 6.5f) * 1.0f);
    hor_ = std::clamp(HORIZON + bob + shakeY_, 62, 96);
    const float st = stormAmt();
    gs::VDP& v = sys_->vdp;
    int fr = ilerp(7, 13, st);
    int fg = ilerp(9, 14, st);
    int fb = ilerp(12, 15, st);
    v.setFogColor(gs::rgb4(fr, fg, fb));
    const float yawShow = (mode_ == Mode::Title || mode_ == Mode::Go) ? std::sin(modeTime_ * 0.35f) * 0.4f : yaw_;
    const int16_t hs = int16_t(std::lround(-yawShow * 70.f + float(shakeX_)));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= hor_) {
            float u = std::clamp(float(y) / float(std::max(hor_, 1)), 0.f, 1.f);
            float s = u * u * (3.f - 2.f * u);
            int r = ilerp(ilerp(2, 6, st), ilerp(9, 13, st), s);
            int g = ilerp(ilerp(3, 7, st), ilerp(11, 14, st), s);
            int b = ilerp(ilerp(8, 10, st), ilerp(14, 15, st), s);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = uint8_t(std::clamp((1.f - u) * 2.f + st * 5.f, 0.f, 14.f));
        } else {
            v.lineBackdrop[y] = gs::rgb4(ilerp(7, 12, st), ilerp(9, 13, st), ilerp(12, 15, st));
            float dz = FOCAL * CAM_H / float(y - hor_);
            float fog = std::clamp((dz - 40.f) / 26.f, 0.f, 10.f) + st * st * 6.f;
            v.lineFog[y] = uint8_t(std::clamp(fog, 0.f, 15.f));
        }
        v.B.hscroll[y] = y < hor_ + 8 ? hs : 0;
        v.B.vscroll[y] = 0;
    }
    v.A.enabled = false;
    v.B.enabled = true;
}

void Game::road() {
    if (int(shiftU_.size()) != SHIFT_N + 1) shiftU_.assign(SHIFT_N + 1, 0.f);
    float heading = 0.f, x = 0.f;
    const float origin = camZ();
    shiftU_[0] = 0.f;
    for (int i = 1; i <= SHIFT_N; i++) {
        float z = (float(i) - 0.5f) * SHIFT_DZ;
        heading += kappa(origin + z) * VIS * SHIFT_DZ;
        x += heading * SHIFT_DZ;
        shiftU_[size_t(i)] = x / HALF_W;
    }

    const float lx = lookX();
    const float st = stormAmt();
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& L = v.road[y];
        L.on = false;
        if (y <= hor_) continue;
        float dz = FOCAL * CAM_H / float(y - hor_);
        if (dz > 480.f) continue;
        float hw = FOCAL * HALF_W / dz;
        float su = std::clamp(shiftAt(dz), -14.f, 14.f);
        L.on = true;
        L.cx = 160.f + float(shakeX_) + (su - lx) * hw;
        L.hw = hw;
        L.v = origin + dz;
        L.pal = uint8_t(PAL_SNOW);
        L.band = (int(std::floor((origin + dz) * 0.09f)) & 1) ? 1 : 0;
        L.style = gs::ROAD_SNOW;
        L.left = gs::GROUND_SNOWWALL;
        L.right = gs::GROUND_SNOWWALL;
        (void)st;
    }
}

void Game::gate(float z, const gs::Mipped& banner) {
    Proj p = project(z, 0.f);
    if (!p.ok) return;
    const float postH = FOCAL * 4.4f / p.z;
    const float banH = FOCAL * 2.05f / p.z;
    const int fog = int(p.fog);
    blit(art_.post, p.x - p.hw * 1.02f, p.y, postH, PAL_TIMBER, false, fog, p.z);
    blit(art_.post, p.x + p.hw * 1.02f, p.y, postH, PAL_TIMBER, true, fog, p.z + 0.02f);
    const float bw = std::min(p.hw * 1.85f, 400.f);
    blit(banner, p.x, p.y - postH + banH * 0.95f, banH, PAL_BANNER, false, fog, p.z - 0.2f);
    (void)bw;
}

void Game::world() {
    const float st = stormAmt();
    for (const Prop& prop : props_) {
        if (prop.kind == Kind::MushGate) {
            if (mode_ != Mode::Title && mode_ != Mode::Go) gate(prop.z, art_.mushBan);
            continue;
        }
        if (prop.kind == Kind::PassGate) {
            gate(prop.z, art_.passBan);
            continue;
        }
        Proj p = project(prop.z, prop.x);
        if (!p.ok) continue;
        const int fog = int(p.fog);
        const float h = FOCAL * prop.h / p.z;
        if (prop.kind == Kind::Pine) blit(art_.pine, p.x, p.y, h, PAL_PINE, prop.x < 0.f, fog, p.z);
        else if (prop.kind == Kind::Stake) blit(art_.stake, p.x, p.y, h, PAL_TIMBER, false, fog, p.z);
        else if (prop.kind == Kind::Cabin) blit(art_.cabin, p.x, p.y, h, PAL_TIMBER, prop.x > 0.f, fog, p.z);
        else blit(art_.boulder, p.x, p.y, h, PAL_ROCK, false, fog, p.z);
    }

    for (const Hazard& h : haz_) {
        Proj p = project(h.z, h.x);
        if (!p.ok) continue;
        float worldH = h.rock ? 1.35f : 1.05f;
        if (h.hit) worldH *= 0.55f;
        const float px = FOCAL * worldH / p.z;
        blit(h.rock ? art_.boulder : art_.drift, p.x, p.y, px, h.rock ? PAL_ROCK : PAL_FX, false, int(p.fog), p.z);
    }

    int flakesOn = 7 + int(st * 15.f);
    for (int i = 0; i < flakesOn && i < 22; i++) {
        const Flake& f = flake_[i];
        float sc = f.sc * (1.f + st * 0.8f);
        float z = (i & 1) ? 0.08f : 0.55f;
        blit(art_.flake, f.x, f.y, sc, PAL_FX, false, int(st * 4.f), z);
    }
    for (const Puff& p : puff_) {
        if (p.life <= 0.f) continue;
        blit(art_.flake, p.x, p.y, p.sc * (0.6f + p.life), PAL_FX, false, 0, 0.2f);
    }

    team();

    if (mode_ == Mode::Title) {
        ui(art_.title, (320.f - art_.title.w) * 0.5f, 6.f, PAL_GOLD);
        ui(art_.sub, (320.f - art_.sub.w) * 0.5f, 34.f, PAL_HUD);
    } else if (mode_ == Mode::Go) {
        ui(art_.go, (320.f - art_.go.w) * 0.5f, 18.f, PAL_GOLD);
    } else if (mode_ == Mode::Result) {
        const gs::Image& im = end_ == End::Clear ? art_.clear : end_ == End::Storm ? art_.storm : art_.buried;
        int pal = end_ == End::Clear ? PAL_GOLD : PAL_ALERT;
        ui(im, (320.f - im.w) * 0.5f, 10.f, pal);
    }
}

void Game::team() {
    const float lean = steerSm_;
    const bool hard = std::fabs(lean) > 0.22f;
    const gs::Mipped& me = hard ? art_.team[1] : art_.team[0];
    const float bob = (mode_ == Mode::Run) ? std::sin(raceTime_ * (mushing_ ? 11.f : 7.f)) * (mushing_ ? 1.6f : 0.7f) : 0.f;
    const float foot = 214.f + bob + float(shakeY_);
    const float px = 160.f + lean * 12.f + float(shakeX_);
    const float ph = mushing_ ? 96.f : 92.f;
    int fog = int(stormAmt() * stormAmt() * 6.f);
    blit(art_.shadow, px, foot + 2.f, 16.f, PAL_FX, false, 0, 0.4f, true);
    blit(me, px, foot, ph, PAL_TEAM, hard && lean > 0.f, fog, 0.12f);
}

void Game::hudText(int col, int row, const char* s, int pal) {
    gs::Plane& h = sys_->vdp.HUD;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        if (x < 0 || x > 39 || row < 0 || row > 27) continue;
        unsigned char ch = (unsigned char)s[i];
        if (ch < 32 || ch > 126) ch = '?';
        int t = art_.font[ch];
        if (!t) continue;
        h.set(x, row, gs::entry(t, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hudText(std::max(0, (40 - n) / 2), row, s, pal);
}

const char* Game::hint() const {
    if (mode_ == Mode::Pause) return "PAUSED";
    if (mode_ == Mode::Result) {
        if (end_ == End::Clear) return "THE PASS IS CLEAR";
        if (end_ == End::Storm) return "THE STORM CLOSED THE PASS";
        return "BURIED IN THE BANK";
    }
    if (clockLeft() < 10.f) return "THE STORM IS CLOSING";
    const float need = feed(kappa(playerZ_ + 40.f));
    if (playerX_ > 0.72f) return "STEER LEFT";
    if (playerX_ < -0.72f) return "STEER RIGHT";
    if (need > 0.16f) return "STEER RIGHT";
    if (need < -0.16f) return "STEER LEFT";
    for (const Hazard& h : haz_) {
        float dz = h.z - playerZ_;
        if (dz > 2.f && dz < 28.f && std::fabs(playerX_ - h.x) < h.r + 0.35f) return h.rock ? "ROCK IN THE PASS" : "SNOW DRIFT";
    }
    if (playerZ_ > FINISH - 140.f) return "THE ARCH. TAKE IT";
    if (mushing_) return "MUSH";
    return "MUSH OR THE STORM WINS";
}

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) h.set(x, y, 0);

    if (mode_ == Mode::Title) {
        hudC(7, "ARROWS STEER   Z/C MUSH   X BRAKE", PAL_GOLD);
        hudC(8, "ENTER TO MUSH", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Go) {
        hudC(8, "BEAT THE STORM TO THE ARCH", PAL_ALERT);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(2, "PAUSED", PAL_GOLD);
        hudC(4, "ENTER TO MUSH", PAL_HUD);
        return;
    }

    if (mode_ == Mode::Result) {
        char line[48];
        if (end_ == End::Clear)
            std::snprintf(line, sizeof line, "CLEAR  %.1f S   %.1f LEFT", raceTime_, clockLeft());
        else if (end_ == End::Storm)
            std::snprintf(line, sizeof line, "CLOSED AT %d M", int(std::max(0.f, playerZ_) + 0.5f));
        else
            std::snprintf(line, sizeof line, "BANKED AT %d M", int(std::max(0.f, playerZ_) + 0.5f));
        hudC(5, line, end_ == End::Clear ? PAL_GOLD : PAL_ALERT);
        hudC(7, hint(), end_ == End::Clear ? PAL_HUD : PAL_ALERT);
        if (!bot_) hudC(9, "ENTER FOR ANOTHER RUN", PAL_HUD);
        return;
    }

    char left[20], right[16], bar[41];
    int leftM = int(std::max(0.f, FINISH - playerZ_) + 0.5f);
    std::snprintf(left, sizeof left, "STORM %4.1f", clockLeft());
    std::snprintf(right, sizeof right, "%4d M", leftM);
    int clockPal = clockLeft() < 10.f ? PAL_ALERT : PAL_HUD;
    hudText(1, 0, left, clockPal);
    if (mushing_) hudText(16, 0, "MUSH", PAL_GOLD);
    else if (braking_) hudText(16, 0, "DRAG", PAL_ALERT);
    hudText(32, 0, right, PAL_HUD);

    for (int i = 0; i < 40; i++) bar[i] = '.';
    bar[0] = '[';
    bar[39] = ']';
    bar[40] = 0;
    for (const Hazard& hz : haz_) {
        float dz = hz.z - playerZ_;
        if (dz <= 0.f || dz > 48.f || hz.hit) continue;
        int hs = std::clamp(20 + int(std::lround(hz.x * 18.f)), 1, 38);
        bar[hs] = hz.rock ? '^' : '*';
    }
    int slot = std::clamp(20 + int(std::lround(playerX_ * 18.f)), 1, 38);
    bar[slot] = 'O';
    int barPal = std::fabs(playerX_) > 0.8f ? PAL_ALERT : PAL_HUD;
    hudText(0, 1, bar, barPal);
    hudC(3, hint(), clockLeft() < 10.f || std::fabs(playerX_) > 0.72f ? PAL_ALERT : PAL_GOLD);

    int lights = std::clamp(int(std::lround(clockLeft() / CLOCK * 8.f)), 0, 8);
    sys_->setLight(8 + (8 - lights) * 14, 12 + lights * 8, 18);
}

void Game::draw() {
    sky();
    road();
    sys_->vdp.clearSprites();
    draws_.clear();
    world();
    std::sort(draws_.begin(), draws_.end(), [](const Draw& a, const Draw& b) { return a.z < b.z; });
    for (const Draw& d : draws_) sys_->vdp.sprite(d.s);
    hud();
}

}  // namespace sledpass
