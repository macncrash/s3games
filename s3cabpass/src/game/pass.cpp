#include "pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cabpass {
namespace {

constexpr int HORIZON = 78;
constexpr float FOCAL = 220.f;
constexpr float CAM_H = 1.45f;
constexpr float HALF_W = 2.7f;
constexpr float FINISH = 780.f;
constexpr float RIVAL0 = 48.f;
constexpr float RIVAL_V = 28.4f;
constexpr float WALL = 0.97f;
constexpr float STEER_A = 12.f;
constexpr float CENT = 0.34f;
constexpr float GRIP = 0.82f;
constexpr float GROOVE = 1.35f;
constexpr float DAMP = 4.4f;
constexpr float V_GAS = 41.f;
constexpr float V_COAST = 22.f;
constexpr float V_BRAKE = 14.f;
constexpr float VIS = 1.15f;
constexpr float DT = 1.f / 60.f;
constexpr int SHIFT_N = 220;
constexpr float SHIFT_DZ = 1.45f;
constexpr float NEAR_Z = 3.2f;
constexpr float FAR_Z = 260.f;

struct Bend {
    float a, b, c, d, k;
};

const Bend kBends[] = {
    {70.f, 130.f, 210.f, 290.f, 0.0042f},
    {320.f, 380.f, 470.f, 540.f, -0.0048f},
    {580.f, 630.f, 700.f, 750.f, 0.0036f},
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

float Game::feed(float k) const { return (k * speed_ * speed_ * CENT) / (STEER_A * GRIP); }

float Game::lookX() const {
    if (mode_ == Mode::Title || mode_ == Mode::Go) return std::sin(modeTime_ * 0.45f) * 0.08f;
    return playerX_;
}

float Game::crewLeft() const {
    float left = (FINISH - rivalZ_) / RIVAL_V;
    return std::max(0.f, left);
}

float Game::stormAmt() const {
    if (mode_ == Mode::Title || mode_ == Mode::Go) return 0.06f;
    if (end_ == End::Storm) return 1.f;
    float full = (FINISH - RIVAL0) / RIVAL_V;
    return std::clamp(1.f - crewLeft() / std::max(1.f, full), 0.f, 1.f);
}

const char* Game::why() const {
    switch (end_) {
        case End::Storm: return "the other crew closed the pass";
        case End::Ditch: return "the cab left the road";
        case End::Clear: return "clear";
        default: return "";
    }
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Go) return 1;
    if (mode_ == Mode::Result) return 4;
    if (crewLeft() < 8.f) return 3;
    return 2;
}

void Game::tune() {
    sys_->apu.setMaster(0.85f);
    sys_->apu.setEcho(0.12f, 0.25f, 0.18f);
}

void Game::buildCourse() {
    props_.clear();
    haz_.clear();
    props_.push_back({36.f, 0.f, 0.f, Kind::FareGate});
    props_.push_back({FINISH, 0.f, 0.f, Kind::PassGate});
    for (float z = 60.f; z < FINISH - 24.f; z += 38.f) {
        int n = int(z);
        float side = (n / 38) & 1 ? 1.f : -1.f;
        float x = side * (1.35f + hash01(n) * 0.35f);
        Kind k = Kind::Pine;
        float h = 3.4f + hash01(n + 3) * 1.6f;
        int pick = n % 5;
        if (pick == 0) {
            k = Kind::Lamp;
            h = 2.6f;
            x = side * 1.18f;
        } else if (pick == 2) {
            k = Kind::Rock;
            h = 1.4f;
            x = side * 1.55f;
        }
        props_.push_back({z, x, h, k});
    }
    const float rocks[] = {160.f, 280.f, 410.f, 530.f, 650.f};
    for (int i = 0; i < 5; i++) {
        float x = (i & 1) ? 0.58f : -0.58f;
        haz_.push_back({rocks[i], x, 0.16f, false});
    }
}

void Game::resetRun() {
    won_ = false;
    over_ = false;
    end_ = End::None;
    gassing_ = false;
    braking_ = false;
    playerZ_ = 0;
    playerX_ = 0;
    rivalZ_ = RIVAL0;
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
    rivalZ_ = RIVAL0;
    chime_ = 2;
    chimeT_ = 0.02f;
    sys_->apu.keyOn(0, 392.f, 0.07f);
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
        sys_->setLight(40, 140, 50);
    } else if (e == End::Storm) {
        shake_ = 0.4f;
        sys_->apu.noiseBurst(0.42f, 480.f, 0.26f);
        sys_->rumble(0.5f, 0.2f, 200);
        sys_->setLight(90, 100, 140);
    } else {
        shake_ = 0.55f;
        sys_->apu.noiseBurst(0.5f, 220.f, 0.2f);
        sys_->rumble(0.75f, 0.3f, 200);
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
        flake_[i].sp = 30.f + hash01(i + 80) * 80.f;
        flake_[i].sc = 2.2f + hash01(i + 120) * 2.8f;
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
    rivalZ_ = RIVAL0 + 18.f;
    speed_ = 0;
}

void Game::human(float& steer, bool& gas, bool& brake) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.12f) steer = std::clamp(p.axisX, -1.f, 1.f);
    brake = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.22f;
    gas = !brake && (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) ||
                     p.accel > 0.22f);
}

void Game::bot(float& steer, bool& gas, bool& brake) const {
    const float look = 34.f;
    const float here = feed(kappa(playerZ_));
    const float ahead = feed(kappa(playerZ_ + look));
    float s = here * 0.35f + ahead * 0.65f - playerX_ * 1.4f - latV_ * 1.8f;
    if (playerX_ > 0.42f) s -= (playerX_ - 0.42f) * 3.0f;
    if (playerX_ < -0.42f) s += (-0.42f - playerX_) * 3.0f;
    if (std::fabs(playerX_) > 0.76f) s = -playerX_ * 2.5f - latV_ * 2.2f;
    for (const Hazard& h : haz_) {
        if (h.hit) continue;
        float dz = h.z - playerZ_;
        if (dz < 4.f || dz > 42.f) continue;
        float gate = h.r + 0.22f;
        float side = playerX_ - h.x;
        if (std::fabs(side) >= gate) continue;
        float away = side >= 0.f ? 1.f : -1.f;
        float urge = (gate - std::fabs(side)) / gate;
        s += away * urge * (0.8f + (1.f - dz / 42.f));
    }
    steer = std::clamp(s, -1.f, 1.f);
    brake = std::fabs(playerX_) > 0.82f;
    gas = !brake;
}

void Game::hazards() {
    for (Hazard& h : haz_) {
        if (h.hit) continue;
        float dz = h.z - playerZ_;
        if (dz > 8.f || dz < -1.f) continue;
        if (std::fabs(playerX_ - h.x) > h.r + 0.06f) continue;
        h.hit = true;
        speed_ = std::max(V_BRAKE, speed_ * 0.78f);
        shake_ = std::max(shake_, 0.35f);
        sys_->apu.noiseBurst(0.26f, 320.f, 0.1f);
        sys_->rumble(0.3f, 0.12f, 70);
    }
}

void Game::spawnPuff(float x, float y, float vx, float vy) {
    Puff& p = puff_[puffN_++ % 16];
    p.x = x;
    p.y = y;
    p.vx = vx;
    p.vy = vy;
    p.life = 0.28f;
    p.sc = 3.f + hash01(puffN_) * 3.f;
}

void Game::physics(float dt) {
    float steer = 0.f;
    bool gas = false, brake = false;
    if (bot_) bot(steer, gas, brake);
    else human(steer, gas, brake);
    gassing_ = gas;
    braking_ = brake;
    const float follow = bot_ ? 0.7f : std::min(1.f, dt * 16.f);
    steerSm_ += (steer - steerSm_) * follow;

    const float scale = STEER_A * GRIP / HALF_W;
    const float steerU = steerSm_ * scale;
    const float pushU = -kappa(playerZ_) * speed_ * speed_ * CENT / HALF_W;
    latV_ += (steerU + pushU - playerX_ * GROOVE) * dt;
    latV_ *= std::max(0.f, 1.f - DAMP * dt);
    latV_ = std::clamp(latV_, -3.2f, 3.2f);
    playerX_ += latV_ * dt;

    if (std::fabs(playerX_) > 0.84f) {
        float s = std::copysign(1.f, playerX_);
        latV_ -= s * 4.2f * dt;
        speed_ -= 8.f * dt;
        shake_ = std::max(shake_, 0.16f);
    }
    if (std::fabs(playerX_) >= WALL) {
        end(End::Ditch);
        return;
    }

    float target = gas ? V_GAS : V_COAST;
    if (brake) target = V_BRAKE;
    const float rate = brake ? 14.f : gas ? 9.f : 3.f;
    if (speed_ < target) speed_ += rate * dt;
    else speed_ -= rate * dt;
    speed_ = std::clamp(speed_, 10.f, 48.f);

    hazards();

    playerZ_ += speed_ * dt;
    rivalZ_ += RIVAL_V * dt;
    raceTime_ += dt;
    yaw_ += kappa(playerZ_) * speed_ * dt;

    // The other crew is the clock. Crossing after they take the arch still fails.
    if (rivalZ_ >= FINISH && playerZ_ < FINISH) {
        end(End::Storm);
        return;
    }
    if (playerZ_ >= FINISH) {
        if (crewLeft() <= 0.f) end(End::Storm);
        else end(End::Clear);
        return;
    }

    if ((sys_->frame & 1u) && speed_ > 20.f) {
        float side = (puffN_ & 1) ? 18.f : -18.f;
        spawnPuff(160.f + steerSm_ * 10.f + side, 206.f, side * 0.6f, -12.f);
    }
}

void Game::flakes(float dt) {
    const float rush = mode_ == Mode::Run ? std::max(0.35f, speed_ / V_GAS) : 0.22f;
    const float blow = 0.35f + stormAmt() * 1.8f;
    for (int i = 0; i < 22; i++) {
        Flake& f = flake_[i];
        f.y += f.sp * rush * blow * dt;
        f.x += (f.x - 160.f) * 0.16f * rush * dt + std::sin(modeTime_ + float(i)) * 6.f * dt;
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
        p.vy += 20.f * dt;
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
        if (sys.pad.pressed(gs::BTN_START) || (bot_ && modeTime_ > 0.35f)) resetRun();
    } else if (mode_ == Mode::Go) {
        if (modeTime_ > 0.55f) launch();
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
            if (mode_ == Mode::Result && won_) {
                int i = 4 - chime_;
                if (i >= 0 && i < 4) sys_->apu.keyOn(i % 3, winN[i], 0.15f);
            } else if (!won_ && mode_ == Mode::Result) {
                sys_->apu.keyOn(0, 180.f, 0.12f);
            }
            chime_--;
            chimeT_ = 0.13f;
        }
    }

    if (mode_ != Mode::Run) {
        sys_->apu.noise(mode_ == Mode::Title ? 0.01f : 0.018f, 700.f, false);
        sys_->apu.tone(0, 0.f, 0.f);
        if (mode_ != Mode::Result) sys_->apu.tone(1, 0.f, 0.f);
        return;
    }

    sys_->apu.noise(0.018f + speed_ * 0.001f + stormAmt() * 0.02f, 900.f + speed_ * 22.f, false);
    sys_->apu.tone(0, 55.f + speed_ * 1.4f, gassing_ ? 0.016f : 0.008f);

    int whole = int(crewLeft());
    if (whole <= 8 && whole >= 0 && whole != beepSec_ && crewLeft() > 0.05f) {
        beepSec_ = whole;
        sys_->apu.tone(1, whole <= 3 ? 880.f : 620.f, 0.045f);
    } else if (std::fmod(raceTime_, 1.f) > 0.08f) {
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
    if (mode_ == Mode::Run) bob = int(std::sin(raceTime_ * 8.f) * (gassing_ ? 1.2f : 0.4f));
    hor_ = std::clamp(HORIZON + bob + shakeY_, 64, 98);
    const float st = stormAmt();
    gs::VDP& v = sys_->vdp;
    int fr = ilerp(6, 12, st);
    int fg = ilerp(8, 13, st);
    int fb = ilerp(12, 15, st);
    v.setFogColor(gs::rgb4(fr, fg, fb));
    const float yawShow = (mode_ == Mode::Title || mode_ == Mode::Go) ? std::sin(modeTime_ * 0.3f) * 0.3f : yaw_;
    const int16_t hs = int16_t(std::lround(-yawShow * 60.f + float(shakeX_)));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= hor_) {
            float u = std::clamp(float(y) / float(std::max(hor_, 1)), 0.f, 1.f);
            float s = u * u * (3.f - 2.f * u);
            int r = ilerp(ilerp(2, 5, st), ilerp(8, 12, st), s);
            int g = ilerp(ilerp(3, 6, st), ilerp(10, 13, st), s);
            int b = ilerp(ilerp(8, 10, st), ilerp(14, 15, st), s);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = uint8_t(std::clamp((1.f - u) * 2.f + st * 5.f, 0.f, 14.f));
        } else {
            v.lineBackdrop[y] = gs::rgb4(ilerp(5, 10, st), ilerp(6, 11, st), ilerp(8, 13, st));
            float dz = FOCAL * CAM_H / float(y - hor_);
            float fog = std::clamp((dz - 40.f) / 26.f, 0.f, 10.f) + st * st * 6.f;
            v.lineFog[y] = uint8_t(std::clamp(fog, 0.f, 15.f));
        }
        v.B.hscroll[y] = y < hor_ + 8 ? hs : 0;
        v.B.vscroll[y] = 0;
    }
    v.A.enabled = false;
    v.B.enabled = true;
    v.roadTime = int(raceTime_ * 60.f);
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
        L.pal = uint8_t(PAL_ROAD);
        L.band = (int(std::floor((origin + dz) * 0.08f)) & 1) ? 1 : 0;
        L.style = 1;
        L.left = gs::GROUND_SNOWWALL;
        L.right = gs::GROUND_SNOWWALL;
    }
}

void Game::gate(float z, const gs::Mipped& banner) {
    Proj p = project(z, 0.f);
    if (!p.ok) return;
    const float postH = FOCAL * 3.8f / p.z;
    const float banH = FOCAL * 1.15f / p.z;
    const int fog = int(p.fog);
    blit(art_.post, p.x - p.hw * 1.05f, p.y, postH, PAL_BANNER, false, fog, p.z);
    blit(art_.post, p.x + p.hw * 1.05f, p.y, postH, PAL_BANNER, true, fog, p.z + 0.02f);
    blit(banner, p.x, p.y - postH + banH * 0.9f, banH, PAL_BANNER, false, fog, p.z - 0.15f);
}

void Game::rival() {
    float dz = rivalZ_ - camZ();
    if (dz < 4.f) return;
    float rx = std::sin(rivalZ_ * 0.02f) * 0.22f;
    Proj p = project(rivalZ_, rx);
    if (!p.ok) return;
    const float h = FOCAL * 1.55f / p.z;
    blit(art_.rival, p.x, p.y, h, PAL_RIVAL, false, int(p.fog), p.z);
}

void Game::cab() {
    const float lean = steerSm_;
    const bool hard = std::fabs(lean) > 0.22f;
    const gs::Mipped& me = hard ? art_.cab[1] : art_.cab[0];
    const float bob = (mode_ == Mode::Run) ? std::sin(raceTime_ * (gassing_ ? 14.f : 8.f)) * 1.1f : 0.f;
    const float foot = 218.f + bob + float(shakeY_);
    const float px = 160.f + lean * 14.f + float(shakeX_);
    int fog = int(stormAmt() * stormAmt() * 5.f);
    blit(art_.shadow, px, foot + 2.f, 14.f, PAL_FX, false, 0, 0.35f, true);
    blit(me, px, foot, 108.f, PAL_CAB, hard && lean > 0.f, fog, 0.1f);
}

void Game::world() {
    const float st = stormAmt();
    for (const Prop& prop : props_) {
        if (prop.kind == Kind::FareGate) {
            if (mode_ == Mode::Title || mode_ == Mode::Go || playerZ_ < prop.z + 8.f) gate(prop.z, art_.fareBan);
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
        else if (prop.kind == Kind::Lamp) blit(art_.lamp, p.x, p.y, h, PAL_GOLD, false, fog, p.z);
        else blit(art_.rock, p.x, p.y, h, PAL_ROCK, false, fog, p.z);
    }
    for (const Hazard& h : haz_) {
        Proj p = project(h.z, h.x);
        if (!p.ok) continue;
        float worldH = h.hit ? 0.55f : 1.05f;
        blit(art_.rock, p.x, p.y, FOCAL * worldH / p.z, PAL_ROCK, false, int(p.fog), p.z);
    }
    rival();
    int flakesOn = 6 + int(st * 16.f);
    for (int i = 0; i < flakesOn && i < 22; i++) {
        const Flake& f = flake_[i];
        blit(art_.flake, f.x, f.y, f.sc * (1.f + st), PAL_FX, false, int(st * 4.f), (i & 1) ? 0.08f : 0.5f);
    }
    for (const Puff& p : puff_) {
        if (p.life <= 0.f) continue;
        blit(art_.flake, p.x, p.y, p.sc * (0.5f + p.life), PAL_FX, false, 0, 0.18f);
    }
    cab();

    if (mode_ == Mode::Title) {
        ui(art_.title, (320.f - art_.title.w) * 0.5f, 8.f, PAL_GOLD);
        ui(art_.sub, (320.f - art_.sub.w) * 0.5f, 40.f, PAL_HUD);
    } else if (mode_ == Mode::Go) {
        ui(art_.go, (320.f - art_.go.w) * 0.5f, 16.f, PAL_GOLD);
    } else if (mode_ == Mode::Result) {
        const gs::Image& im = end_ == End::Clear ? art_.clear : end_ == End::Storm ? art_.closed : art_.ditch;
        int pal = end_ == End::Clear ? PAL_GOLD : PAL_ALERT;
        ui(im, (320.f - im.w) * 0.5f, 12.f, pal);
    }
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
        if (end_ == End::Storm) return "THE OTHER CREW TOOK THE PASS";
        return "THE CAB LEFT THE ROAD";
    }
    if (crewLeft() < 8.f) return "THE OTHER CREW IS CLOSING";
    if (playerX_ > 0.7f) return "STEER LEFT";
    if (playerX_ < -0.7f) return "STEER RIGHT";
    const float need = feed(kappa(playerZ_ + 36.f));
    if (need > 0.15f) return "BEND RIGHT";
    if (need < -0.15f) return "BEND LEFT";
    if (playerZ_ > FINISH - 120.f) return "THE ARCH. TAKE IT";
    return "BEAT THEIR CLOCK";
}

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    h.clear();
    if (mode_ == Mode::Title) {
        hudC(8, "ARROWS STEER   Z GAS   X BRAKE", PAL_GOLD);
        hudC(9, "ENTER TO TAKE THE CAB", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Go) {
        hudC(8, "CLEAR THE PASS BEFORE THEIR CLOCK", PAL_ALERT);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(2, "PAUSED", PAL_GOLD);
        hudC(4, "ENTER TO DRIVE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Result) {
        char line[48];
        if (end_ == End::Clear)
            std::snprintf(line, sizeof line, "CLEAR  %.1f S   CREW %.1f", raceTime_, crewLeft());
        else if (end_ == End::Storm)
            std::snprintf(line, sizeof line, "CLOSED AT %d M", int(std::max(0.f, playerZ_) + 0.5f));
        else
            std::snprintf(line, sizeof line, "DITCHED AT %d M", int(std::max(0.f, playerZ_) + 0.5f));
        hudC(6, line, end_ == End::Clear ? PAL_GOLD : PAL_ALERT);
        hudC(8, hint(), end_ == End::Clear ? PAL_HUD : PAL_ALERT);
        if (!bot_) hudC(10, "ENTER FOR ANOTHER FARE", PAL_HUD);
        return;
    }

    char left[24], right[16];
    int leftM = int(std::max(0.f, FINISH - playerZ_) + 0.5f);
    std::snprintf(left, sizeof left, "CREW %4.1f", crewLeft());
    std::snprintf(right, sizeof right, "%4d M", leftM);
    hudText(1, 0, left, crewLeft() < 8.f ? PAL_ALERT : PAL_HUD);
    hudText(33, 0, right, PAL_HUD);
    if (gassing_) hudText(16, 0, "GAS", PAL_GOLD);
    else if (braking_) hudText(16, 0, "BRAKE", PAL_ALERT);
    hudC(3, hint(), crewLeft() < 8.f || std::fabs(playerX_) > 0.72f ? PAL_ALERT : PAL_GOLD);
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

}  // namespace cabpass
