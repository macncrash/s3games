#include "game/plow.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace plow {
namespace {

constexpr int HORIZON = 102;
constexpr float FOCAL = 180.f;
constexpr float CAM_H = 3.8f;
constexpr float HALF_W = 5.2f;
constexpr float FINISH = 2160.f;
constexpr float GATE = 2168.f;
constexpr float STORM = 118.f;
constexpr float STEER_A = 11.f;
constexpr float CENT = 0.28f;
constexpr float VIS = 1.45f;
constexpr float DT = 1.f / 60.f;
constexpr int SHIFT_N = 480;
constexpr float SHIFT_DZ = 1.4f;
constexpr int DRIFT_N = 5;

struct Bend {
    float a, b, c, d, k;
};

// Positive kappa bends the pass to the right. Drifts sit on the straights.
const Bend kBends[] = {
    {140.f, 220.f, 400.f, 500.f, 0.0056f},
    {620.f, 700.f, 920.f, 1020.f, -0.0060f},
    {1180.f, 1260.f, 1460.f, 1560.f, 0.0052f},
};

const float kDrifts[] = {560.f, 1100.f, 1660.f, 1840.f, 2000.f};

float piece(float z, const Bend& r) {
    if (z <= r.a || z >= r.d) return 0.f;
    if (z < r.b) return r.k * (z - r.a) / (r.b - r.a);
    if (z > r.c) return r.k * (r.d - z) / (r.d - r.c);
    return r.k;
}

float hash01(int i) {
    uint32_t h = uint32_t(i) * 747796405u + 2891336453u;
    h ^= h >> 16;
    return float(h & 0xffffff) / float(0x1000000);
}

bool inCut(float z) { return z > 980.f && z < 1420.f; }

}  // namespace

float Game::kappa(float z) const {
    float k = 0.f;
    for (const Bend& r : kBends) k += piece(z, r);
    return k;
}

void Game::tune() {
    gs::FMPatch horn;
    horn.alg = 4;
    horn.fb = 0.12f;
    horn.vol = 0.22f;
    horn.echo = 0.22f;
    horn.drive = 0.08f;
    for (int i = 0; i < 4; i++) {
        gs::FMOp& o = horn.op[i];
        o.mul = i == 0 ? 1.f : i == 1 ? 2.f : i == 2 ? 3.f : 4.5f;
        o.level = i == 0 ? 1.f : 0.45f;
        o.ar = 0.006f;
        o.dr = 0.2f;
        o.sl = 0.2f;
        o.rr = 0.3f;
    }
    for (int ch = 0; ch < 4; ch++) sys_->apu.setPatch(ch, horn);
    sys_->apu.setEcho(0.18f, 0.28f, 0.14f);
    sys_->apu.setMaster(0.85f);
}

void Game::buildCourse() {
    things_.clear();
    driftN_ = DRIFT_N;
    auto add = [&](float z, float x, float r, Kind k, int var) {
        things_.push_back({z, x, r, k, var, false});
    };
    for (int i = 0; i < DRIFT_N; i++) add(kDrifts[i], 0.f, 0.70f, Kind::Drift, i & 1);
    const float rocks[][2] = {{240.f, 0.88f},  {340.f, -0.84f}, {470.f, 0.90f},  {680.f, -0.86f},
                               {800.f, 0.86f},  {1000.f, -0.88f}, {1220.f, 0.84f}, {1380.f, -0.90f},
                               {1520.f, 0.88f}, {1740.f, -0.86f}, {1920.f, 0.90f}, {2070.f, -0.84f}};
    for (auto& rk : rocks) add(rk[0], rk[1], 0.14f, Kind::Rock, int(rk[0]) & 1);
    for (float z = 70.f; z < FINISH - 24.f; z += 24.f) {
        int n = int(z);
        float side = ((n / 24) & 1) ? 1.f : -1.f;
        float j = float((n / 9) % 4) * 0.04f;
        add(z, side * (1.55f + j), 0.f, Kind::Tree, n % 3);
        add(z + 11.f, -side * (1.85f + j * 0.4f), 0.f, Kind::Tree, (n + 1) % 3);
    }
    for (float z = 400.f; z < FINISH; z += 400.f) {
        add(z, 1.18f, 0.f, Kind::Post, 0);
        add(z, -1.18f, 0.f, Kind::Post, 1);
    }
}

void Game::resetRun() {
    mode_ = Mode::Title;
    end_ = End::None;
    over_ = false;
    won_ = false;
    cleared_ = 0;
    puffN_ = 0;
    report_[0] = 0;
    modeTime_ = 0;
    raceTime_ = 0;
    clock_ = STORM;
    playerZ_ = 0;
    playerX_ = 0;
    latV_ = 0;
    speed_ = 0;
    yaw_ = 0;
    steerSm_ = 0;
    stun_ = 0;
    shake_ = 0;
    bury_ = 0;
    bannerT_ = 0;
    sting_ = 0;
    tickAcc_ = 0;
    gas_ = false;
    brake_ = false;
    lift_ = false;
    for (auto& t : things_) t.cleared = false;
    for (auto& p : puffs_) p.life = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    draws_.reserve(320);
    buildArt(sys.vdp, art_);
    buildCourse();
    for (int i = 0; i < 32; i++) {
        flakes_[i].x = hash01(i) * 320.f;
        flakes_[i].y = hash01(i + 40) * 224.f;
        flakes_[i].sp = 18.f + hash01(i + 80) * 40.f;
        flakes_[i].sc = 4.f + hash01(i + 120) * 6.f;
    }
    tune();
    resetRun();
    sys.vdp.A.enabled = false;
    sys.setLight(255, 150, 30);
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Count) return 1;
    if (mode_ == Mode::Result) return 4;
    if (clock_ < 18.f) return 3;
    return 2;
}

void Game::beginCount() {
    mode_ = Mode::Count;
    modeTime_ = 0;
    playerZ_ = 0;
    playerX_ = 0;
    latV_ = 0;
    speed_ = 0;
    clock_ = STORM;
    cleared_ = 0;
    bury_ = 0;
    for (auto& t : things_) t.cleared = false;
}

void Game::launch() {
    mode_ = Mode::Run;
    modeTime_ = 0;
    raceTime_ = 0;
    clock_ = STORM;
    speed_ = 14.f;
    playerZ_ = 0;
    playerX_ = 0;
    latV_ = 0;
    yaw_ = 0;
    stun_ = 0;
    bury_ = 0;
    cleared_ = 0;
    for (auto& t : things_) t.cleared = false;
}

void Game::driver(float& steer, bool& gas, bool& brake, bool& lift) {
    const gs::Pad& p = sys_->pad;
    steer = p.axisX;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    steer = std::clamp(steer, -1.f, 1.f);
    gas = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) || p.accel > 0.22f;
    brake = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.22f;
    lift = p.down(gs::BTN_Y) || p.down(gs::BTN_Z) || p.down(gs::BTN_X);
}

void Game::autopilot(float& steer, bool& gas, bool& brake, bool& lift) {
    gas = true;
    brake = false;
    lift = false;
    float look = std::clamp(speed_ * 0.75f, 16.f, 34.f);
    float k0 = kappa(playerZ_);
    float k1 = kappa(playerZ_ + look * 0.5f);
    auto ff = [&](float k) { return (k * speed_ * speed_ * CENT) / STEER_A; };
    float feed = ff(k0) * 0.65f + ff(k1) * 0.35f;

    float want = 0.f;
    float pull = 0.f;
    for (const Thing& t : things_) {
        if (t.kind != Kind::Rock || t.cleared) continue;
        float dz = t.z - playerZ_;
        if (dz < 3.f || dz > look) continue;
        float rad = t.r + 0.26f;
        float pred = playerX_ + latV_ * (dz / std::max(speed_, 8.f));
        if (std::fabs(pred - t.x) > rad + 0.15f) continue;
        float clear = t.x > 0.f ? t.x - (rad + 0.12f) : t.x + (rad + 0.12f);
        clear = std::clamp(clear, -0.45f, 0.45f);
        float urg = 1.f - dz / look;
        urg *= urg;
        if (urg > pull) {
            pull = urg;
            want = clear;
        }
    }
    for (const Thing& t : things_) {
        if (t.kind != Kind::Drift || t.cleared) continue;
        float dz = t.z - playerZ_;
        if (dz < 0.f || dz > 36.f) continue;
        float urg = 1.f - dz / 36.f;
        if (urg > pull) {
            pull = urg;
            want = 0.f;
        }
    }
    steer = feed + (want - playerX_) * 2.5f - latV_ * 3.5f;
    if (std::fabs(playerX_) > 0.55f) steer += -playerX_ * 2.2f;
    steer = std::clamp(steer, -1.f, 1.f);
    if (std::fabs(playerX_) > 0.9f) brake = true;
}

void Game::bump(float keep) {
    speed_ = std::max(6.f, speed_ * keep);
    stun_ = std::max(stun_, 0.38f);
    shake_ = 0.32f;
    sys_->apu.noiseBurst(0.4f, 1800.f, 0.1f);
    sys_->rumble(0.65f, 0.3f, 90);
}

void Game::collide(float prevZ) {
    for (Thing& t : things_) {
        if (t.cleared) continue;
        if (t.kind != Kind::Drift && t.kind != Kind::Rock) continue;
        if (prevZ > t.z + 0.9f || playerZ_ < t.z - 0.9f) continue;
        if (t.kind == Kind::Drift) {
            if (std::fabs(playerX_) > t.r) continue;
            if (!lift_ && speed_ >= 9.f) {
                t.cleared = true;
                cleared_++;
                speed_ = std::max(10.f, speed_ * 0.64f);
                shake_ = 0.22f;
                bannerT_ = 0.7f;
                sys_->apu.noiseBurst(0.38f, 2400.f, 0.12f);
                sys_->apu.keyOn(3, 196.f, 0.12f);
                sys_->rumble(0.45f, 0.25f, 70);
                for (int n = 0; n < 8; n++) {
                    Puff& p = puffs_[puffN_++ % 24];
                    p.x = 120.f + hash01(puffN_ + n) * 80.f;
                    p.y = 150.f;
                    p.vx = (hash01(puffN_ * 3 + n) - 0.5f) * 90.f;
                    p.vy = -(30.f + hash01(puffN_ * 5 + n) * 50.f);
                    p.life = 0.4f;
                    p.sc = 6.f + hash01(n + puffN_) * 6.f;
                }
            } else {
                playerZ_ = t.z - 2.4f;
                speed_ = std::min(speed_, 7.f);
                bury_ += 0.35f;
                stun_ = std::max(stun_, 0.3f);
                shake_ = 0.28f;
                sys_->apu.noiseBurst(0.5f, 900.f, 0.14f);
                sys_->rumble(0.8f, 0.4f, 120);
            }
            continue;
        }
        float hit = t.r + 0.18f;
        if (std::fabs(playerX_ - t.x) >= hit) continue;
        t.cleared = true;
        float side = playerX_ >= t.x ? 1.f : -1.f;
        playerX_ = t.x + side * (hit + 0.08f);
        latV_ = side * 0.85f;
        bump(0.5f);
    }
}

void Game::reachGate() {
    speed_ = 0.f;
    won_ = cleared_ == driftN_ && clock_ > 0.f;
    if (won_) {
        end_ = End::Clear;
        std::snprintf(report_, sizeof report_, "S3 PLOW  PASS  pass clear with %.1f s before the storm", clock_);
        mode_ = Mode::Result;
        modeTime_ = 0;
        over_ = true;
        sting_ = 1.2f;
        sys_->apu.keyOn(0, 523.25f, 0.22f);
        sys_->apu.keyOn(1, 659.25f, 0.18f);
        sys_->apu.keyOn(2, 783.99f, 0.16f);
        sys_->rumble(0.25f, 0.55f, 200);
        sys_->setLight(40, 180, 60);
    } else if (cleared_ != driftN_) {
        failRun(End::Snow);
    } else {
        failRun(End::Storm);
    }
}

void Game::failRun(End why) {
    won_ = false;
    end_ = why;
    speed_ = 0.f;
    if (why == End::Snow) std::snprintf(report_, sizeof report_, "S3 PLOW  FAIL  snow still blocks the pass");
    else if (why == End::Buried) std::snprintf(report_, sizeof report_, "S3 PLOW  FAIL  the plow was buried");
    else std::snprintf(report_, sizeof report_, "S3 PLOW  FAIL  the storm closed the pass");
    mode_ = Mode::Result;
    modeTime_ = 0;
    over_ = true;
    sting_ = 1.1f;
    sys_->apu.noiseBurst(0.45f, 400.f, 0.2f);
    sys_->apu.keyOn(0, 196.f, 0.2f);
    sys_->apu.keyOn(1, 155.56f, 0.16f);
    sys_->apu.keyOn(2, 130.81f, 0.14f);
    sys_->rumble(0.7f, 0.2f, 240);
    sys_->setLight(255, 30, 20);
}

void Game::drive(float dt) {
    float steer = 0.f;
    bool gas = false, brake = false, lift = false;
    if (bot_) autopilot(steer, gas, brake, lift);
    else driver(steer, gas, brake, lift);
    if (stun_ > 0.f) {
        stun_ -= dt;
        steer *= 0.5f;
        gas = false;
    }
    float follow = bot_ ? 1.f : std::min(1.f, dt * 9.f);
    steerSm_ += (steer - steerSm_) * follow;
    gas_ = gas;
    brake_ = brake;
    lift_ = lift;

    float steerU = steerSm_ * STEER_A / HALF_W;
    float pushU = -kappa(playerZ_) * speed_ * speed_ * CENT / HALF_W;
    latV_ += (steerU + pushU) * dt;
    latV_ *= std::max(0.f, 1.f - 7.2f * dt);
    playerX_ += latV_ * dt;

    float vmax = lift_ ? 28.5f : 22.8f;
    if (stun_ > 0.f) speed_ = std::min(speed_, 8.f);
    else if (brake_) speed_ -= 18.f * dt;
    else if (gas_) {
        if (speed_ < vmax) speed_ += 5.4f * dt;
        else speed_ -= 7.f * dt;
    } else if (speed_ > 9.f) speed_ -= 6.5f * dt;
    if (std::fabs(playerX_) > 1.02f) speed_ -= (12.f + (std::fabs(playerX_) - 1.02f) * 22.f) * dt;
    speed_ = std::clamp(speed_, 3.5f, 32.f);

    if (playerX_ > 1.28f) {
        playerX_ = 1.12f;
        latV_ = -1.1f;
        bump(0.55f);
    } else if (playerX_ < -1.28f) {
        playerX_ = -1.12f;
        latV_ = 1.1f;
        bump(0.55f);
    }

    float prevZ = playerZ_;
    float nextClock = clock_ - dt;
    playerZ_ += speed_ * dt;
    yaw_ += kappa(playerZ_) * speed_ * dt;
    raceTime_ += dt;
    collide(prevZ);

    if (playerZ_ >= FINISH) {
        playerZ_ = FINISH;
        clock_ = std::max(0.f, nextClock);
        reachGate();
        return;
    }
    clock_ = nextClock;
    if (clock_ <= 0.f) {
        clock_ = 0.f;
        failRun(End::Storm);
        return;
    }
    if (speed_ < 6.f) bury_ += dt;
    else bury_ = std::max(0.f, bury_ - dt);
    if (bury_ > 2.4f) {
        failRun(End::Buried);
        return;
    }
    if (bannerT_ > 0.f) bannerT_ -= dt;

    if (!lift_ && gas_ && speed_ > 12.f && (int(raceTime_ * 60.f) & 1) == 0) {
        Puff& p = puffs_[puffN_++ % 24];
        p.x = 150.f + steerSm_ * 16.f + (hash01(puffN_) - 0.5f) * 36.f;
        p.y = 158.f;
        p.vx = -steerSm_ * 20.f + (hash01(puffN_ * 3) - 0.5f) * 50.f;
        p.vy = -(18.f + hash01(puffN_ * 5) * 28.f);
        p.life = 0.28f;
        p.sc = 4.f + hash01(puffN_) * 4.f;
    }
    for (Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        p.life -= dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy += 36.f * dt;
    }
}

void Game::audio(float dt) {
    if (mode_ == Mode::Result) {
        sys_->apu.noise(0.f, 1000.f, false);
        sys_->apu.tone(0, 0.f, 0.f);
        sys_->apu.tone(1, 0.f, 0.f);
        if (sting_ > 0.f) {
            sting_ -= dt;
            if (sting_ <= 0.f) {
                sys_->apu.keyOff(0);
                sys_->apu.keyOff(1);
                sys_->apu.keyOff(2);
            }
        }
        return;
    }
    float storm = (mode_ == Mode::Run) ? 1.f - std::clamp(clock_ / STORM, 0.f, 1.f) : 0.12f;
    float wind = 0.018f + storm * 0.07f + (mode_ == Mode::Run ? speed_ * 0.0009f : 0.f);
    sys_->apu.noise(wind, 1600.f + storm * 4200.f + speed_ * 28.f, false);
    float hz = 46.f + speed_ * 2.1f;
    float vol = mode_ == Mode::Run ? 0.035f + speed_ * 0.0016f : 0.018f;
    if (lift_) vol *= 0.7f;
    sys_->apu.tone(0, hz, vol);
    sys_->apu.tone(1, hz * 1.5f, vol * 0.4f);
    if (mode_ == Mode::Run && clock_ < 12.f && clock_ > 0.f) {
        tickAcc_ += dt;
        if (tickAcc_ >= 0.5f) {
            tickAcc_ = 0.f;
            sys_->apu.keyOn(3, clock_ < 5.f ? 880.f : 660.f, 0.07f);
        }
    }
    if ((int(sys_->frame) % 12) == 0) {
        if (mode_ == Mode::Run && clock_ < 15.f) sys_->setLight(255, 40, 20);
        else sys_->setLight(255, 150, 30);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (shake_ > 0.f) shake_ -= DT;
    shakeX_ = shake_ > 0.f ? int(sys.frame * 17 % 5) - 2 : 0;
    shakeY_ = shake_ > 0.f ? int(sys.frame * 13 % 3) - 1 : 0;
    float storm = (mode_ == Mode::Run || mode_ == Mode::Result) ? 1.f - std::clamp(clock_ / STORM, 0.f, 1.f) : 0.1f;
    for (int i = 0; i < 32; i++) {
        Flake& f = flakes_[i];
        f.y += f.sp * (1.f + storm * 2.4f) * DT;
        f.x += std::sin(f.y * 0.04f + i) * (12.f + storm * 30.f) * DT;
        if (f.y > 230.f) {
            f.y = -6.f;
            f.x = hash01(i + int(sys.frame)) * 320.f;
        }
        if (f.x < -8.f) f.x += 320.f;
        if (f.x > 328.f) f.x -= 320.f;
    }

    modeTime_ += DT;
    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
        if (sys.pad.pressed(gs::BTN_START) || (bot_ && modeTime_ > 0.35f)) beginCount();
    } else if (mode_ == Mode::Count) {
        if (modeTime_ > 2.05f) launch();
    } else if (mode_ == Mode::Run) {
        drive(DT);
    } else if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
        resetRun();
    }
    audio(DT);
    draw();
}

float Game::shiftAt(float z) const {
    if (z <= 0.f) return 0.f;
    float i = z / SHIFT_DZ;
    int n = int(i);
    if (n >= SHIFT_N) return shiftU_[SHIFT_N];
    float f = i - float(n);
    return shiftU_[n] * (1.f - f) + shiftU_[n + 1] * f;
}

void Game::queue(float z, const gs::Sprite& s) { draws_.push_back({z, s}); }

void Game::blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog, float z, bool shadow) {
    if (h < 2.f) return;
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

void Game::ui(const gs::Image& img, float x, float y) {
    if (img.w <= 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int(std::lround(x));
    s.y = int(std::lround(y));
    s.pal = PAL_TITLE;
    queue(-30.f, s);
}

Game::Proj Game::project(float worldZ, float roadX) const {
    Proj p{};
    float dz = worldZ - playerZ_;
    p.z = dz;
    if (dz < 2.2f || dz > 360.f) return p;
    p.y = float(hor_) + FOCAL * CAM_H / dz + float(shakeY_);
    p.hw = FOCAL * HALF_W / dz;
    float su = std::clamp(shiftAt(dz), -9.f, 9.f);
    p.x = 160.f + float(shakeX_) + (su - playerX_) * p.hw + roadX * p.hw;
    p.fog = std::clamp((dz - 28.f) / 18.f, 0.f, 14.f);
    p.ok = true;
    return p;
}

void Game::sky() {
    int bob = mode_ == Mode::Run ? int(std::sin(raceTime_ * 2.4f) * (speed_ / 48.f)) : int(std::sin(modeTime_ * 1.3f) * 1.5f);
    hor_ = std::clamp(HORIZON + bob + (shake_ > 0.f ? shakeY_ : 0), 94, 114);
    float storm = (mode_ == Mode::Run || mode_ == Mode::Result) ? 1.f - std::clamp(clock_ / STORM, 0.f, 1.f) : 0.08f;
    gs::VDP& v = sys_->vdp;
    int fr = std::clamp(int(std::lround(11.f - storm * 6.f)), 0, 15);
    int fg = std::clamp(int(std::lround(13.f - storm * 6.f)), 0, 15);
    int fb = std::clamp(int(std::lround(15.f - storm * 4.f)), 0, 15);
    v.setFogColor(gs::rgb4(fr, fg, fb));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(float(y) / float(hor_), 0.f, 1.f);
        float s = u * u * (3.f - 2.f * u);
        int r = std::clamp(int(std::lround((3.f - storm * 2.f) + (9.f - storm * 5.f) * s)), 0, 15);
        int g = std::clamp(int(std::lround((5.f - storm * 3.f) + (8.f - storm * 4.f) * s)), 0, 15);
        int b = std::clamp(int(std::lround((11.f - storm * 4.f) + (4.f - storm * 2.f) * s)), 0, 15);
        if (y > hor_) {
            r = std::clamp(12 - int(storm * 5.f), 0, 15);
            g = std::clamp(14 - int(storm * 5.f), 0, 15);
            b = std::clamp(15 - int(storm * 3.f), 0, 15);
        }
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        if (y < hor_) v.lineFog[y] = uint8_t(std::min(12.f, u * u * 6.f + storm * 5.f));
        else {
            float dz = FOCAL * CAM_H / std::max(1, y - hor_);
            v.lineFog[y] = uint8_t(std::min(14.f, dz / 30.f + storm * 3.f));
        }
        float depth = y < hor_ ? 0.25f + 0.75f * float(y) / float(std::max(hor_, 1)) : 1.f;
        float show = yaw_;
        if (mode_ == Mode::Title) show += std::sin(modeTime_ * 0.4f) * 0.08f;
        v.B.hscroll[y] = int16_t(show * 48.f * depth + shakeX_);
        v.B.vscroll[y] = 0;
    }
    v.A.enabled = false;
    v.B.enabled = true;
}

void Game::road() {
    float heading = 0.f, x = 0.f;
    shiftU_[0] = 0.f;
    for (int i = 1; i <= SHIFT_N; i++) {
        float z = (float(i) - 0.5f) * SHIFT_DZ;
        heading += kappa(playerZ_ + z) * VIS * SHIFT_DZ;
        x += heading * SHIFT_DZ;
        shiftU_[i] = x / HALF_W;
    }
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& L = v.road[y];
        L.on = false;
        if (y <= hor_) continue;
        float dz = FOCAL * CAM_H / float(y - hor_);
        if (dz > 500.f) continue;
        float hw = FOCAL * HALF_W / dz;
        float su = std::clamp(shiftAt(dz), -9.f, 9.f);
        float z = playerZ_ + dz;
        bool cut = inCut(z);
        L.on = true;
        L.cx = 160.f + float(shakeX_) + (su - playerX_) * hw;
        L.hw = hw;
        L.v = z;
        L.pal = uint8_t(cut ? PAL_CUT : PAL_ROAD);
        L.band = (int(std::floor(z * 0.08f)) & 1) ? 1 : 0;
        L.style = cut ? gs::ROAD_ROCKY : gs::ROAD_SNOW;
        L.left = L.right = gs::GROUND_SNOWWALL;
    }
}

void Game::world() {
    auto gantry = [&](const gs::Mipped& img, float zGate) {
        Proj p = project(zGate, 0.f);
        if (!p.ok) return;
        float banH = FOCAL * 2.4f / p.z;
        float pole = FOCAL * 6.5f / p.z;
        blit(img, p.x, p.y - (pole - banH), banH, PAL_SIGN, false, int(p.fog), p.z - 0.2f);
        blit(art_.post, p.x + p.hw * 0.92f, p.y, pole, PAL_SIGN, false, int(p.fog), p.z);
        blit(art_.post, p.x - p.hw * 0.92f, p.y, pole, PAL_SIGN, true, int(p.fog), p.z);
    };
    gantry(art_.gateRoll, 48.f);
    gantry(art_.gateSummit, GATE);

    for (const Thing& t : things_) {
        if (t.kind == Kind::Drift && t.cleared) continue;
        Proj p = project(t.z, t.x);
        if (!p.ok || p.x < -80.f || p.x > 400.f) continue;
        if (t.kind == Kind::Tree) {
            float mh = t.var == 0 ? 8.4f : t.var == 1 ? 6.8f : 9.6f;
            float h = FOCAL * mh / p.z;
            blit(art_.spruce[t.var], p.x, p.y, h, PAL_TREE, t.x < 0.f, int(p.fog), p.z);
        } else if (t.kind == Kind::Rock) {
            float h = FOCAL * (t.var ? 1.45f : 1.15f) / p.z;
            blit(art_.rock[t.var], p.x, p.y, h, PAL_ROCK, false, int(p.fog), p.z);
        } else if (t.kind == Kind::Post) {
            float h = FOCAL * 3.1f / p.z;
            blit(art_.post, p.x, p.y, h, PAL_SIGN, t.var != 0, int(p.fog), p.z);
        } else if (t.kind == Kind::Drift) {
            float wantW = t.r * 2.f * p.hw;
            float h = wantW * float(art_.drift[t.var].h) / float(std::max(1, art_.drift[t.var].w));
            blit(art_.drift[t.var], p.x, p.y, h, PAL_DRIFT, false, int(p.fog), p.z);
        }
    }

    float foot = 198.f + float(hor_ - HORIZON);
    float lean = steerSm_;
    const gs::Mipped& me = lift_ ? art_.truckUp : art_.truckDown;
    blit(art_.shadow, 160.f + lean * 10.f, foot + 4.f, 18.f, PAL_FX, false, 0, 0.6f, true);
    blit(me, 160.f + lean * 14.f, foot, lift_ ? 70.f : 78.f, PAL_TRUCK, lean > 0.35f, 0, 0.25f);

    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        blit(art_.flake, p.x, p.y, p.sc, PAL_FX, false, 0, 0.12f);
    }
    int flakes = 32;
    if (mode_ != Mode::Run && mode_ != Mode::Result) flakes = 16;
    for (int i = 0; i < flakes; i++) blit(art_.flake, flakes_[i].x, flakes_[i].y, flakes_[i].sc, PAL_FX, false, 0, -1.f);

    if (mode_ == Mode::Title) {
        ui(art_.titleCard, (320 - art_.titleCard.w) * 0.5f, 8);
        ui(art_.titleSub, (320 - art_.titleSub.w) * 0.5f, 8.f + art_.titleCard.h + 4.f);
    } else if (mode_ == Mode::Count) {
        int n = modeTime_ < 0.5f ? 0 : modeTime_ < 1.0f ? 1 : modeTime_ < 1.5f ? 2 : 3;
        const gs::Image& im = art_.count[n];
        ui(im, (320 - im.w) * 0.5f, 28);
    } else if (bannerT_ > 0.f && mode_ == Mode::Run) {
        ui(art_.plowed, (320 - art_.plowed.w) * 0.5f, 28);
    } else if (mode_ == Mode::Result) {
        const gs::Image& im = end_ == End::Clear ? art_.winText : end_ == End::Snow ? art_.loseSnow : end_ == End::Buried ? art_.loseBuried : art_.loseStorm;
        ui(im, (320 - im.w) * 0.5f, 32);
    }
}

void Game::hudText(int col, int row, const std::string& s, int pal) {
    gs::Plane& h = sys_->vdp.HUD;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        if (x < 0 || x > 39) continue;
        unsigned char ch = (unsigned char)s[i];
        if (ch < 32 || ch > 126) ch = '?';
        int t = art_.font[ch];
        if (!t) continue;
        h.set(x, row, gs::entry(t, pal));
    }
}

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) h.set(x, y, 0);

    if (mode_ == Mode::Title) {
        hudText(4, 25, "CLEAR THE PASS BEFORE THE STORM", PAL_AMBER);
        hudText(4, 26, "ARROWS STEER   Z GAS   X BRAKE", PAL_INK);
        hudText(3, 27, "W LIFTS THE BLADE    ENTER ROLLS", PAL_INK);
        return;
    }

    char line[48];
    int sec = std::max(0, int(std::ceil(clock_ - 1e-3f)));
    std::snprintf(line, sizeof line, "STORM %d:%02d", sec / 60, sec % 60);
    int pal = (mode_ == Mode::Run && clock_ < 15.f) ? PAL_ALARM : PAL_AMBER;
    hudText(1, 0, line, pal);
    std::snprintf(line, sizeof line, "CLEARED %d/%d", cleared_, driftN_);
    hudText(28, 0, line, cleared_ == driftN_ ? PAL_AMBER : PAL_INK);

    char bar[41];
    for (int i = 0; i < 40; i++) bar[i] = '-';
    bar[40] = 0;
    bar[0] = '|';
    bar[39] = '|';
    int mark = std::clamp(int(playerZ_ / FINISH * 37.f) + 1, 1, 38);
    bar[mark] = 'P';
    hudText(0, 1, bar, PAL_INK);

    if (mode_ == Mode::Result) {
        if (end_ == End::Clear) std::snprintf(line, sizeof line, "PASS CLEAR  %.1f S LEFT", clock_);
        else if (end_ == End::Snow) std::snprintf(line, sizeof line, "SNOW STILL IN THE LANE");
        else if (end_ == End::Buried) std::snprintf(line, sizeof line, "THE PLOW IS BURIED");
        else std::snprintf(line, sizeof line, "THE STORM CLOSED THE PASS");
        int col = std::max(0, (40 - int(std::strlen(line))) / 2);
        hudText(col, 26, line, end_ == End::Clear ? PAL_AMBER : PAL_ALARM);
        if (!bot_) hudText(8, 27, "ENTER FOR ANOTHER PASS", PAL_INK);
        return;
    }
    if (mode_ == Mode::Count) return;

    int kmh = int(speed_ * 3.6f + 0.5f);
    int left = std::max(0, int(FINISH - playerZ_ + 0.5f));
    std::snprintf(line, sizeof line, "%3d KM/H", kmh);
    hudText(1, 26, line, PAL_INK);
    hudText(30, 26, lift_ ? "BLADE UP" : "BLADE DN", lift_ ? PAL_ALARM : PAL_AMBER);
    std::snprintf(line, sizeof line, "PASS %d M", left);
    int col = std::max(0, (40 - int(std::strlen(line))) / 2);
    hudText(col, 27, line, left < 180 ? PAL_AMBER : PAL_INK);
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

}  // namespace plow
