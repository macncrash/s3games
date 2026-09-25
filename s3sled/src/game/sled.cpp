#include "game/sled.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace sled {
namespace {

constexpr int HORIZON = 106;
constexpr float FOCAL = 188.f;
constexpr float CAM_H = 3.15f;
constexpr float HALF_W = 4.f;
constexpr float FINISH = 2100.f;
constexpr float HEAD = 88.f;
constexpr float CLOCK = 33.f;
constexpr float V_TUCK = 40.f;
constexpr float V_SIT = 28.f;
constexpr float STEER_A = 10.f;
constexpr float CENT = 0.40f;
constexpr float ICE_GRIP = 0.40f;
constexpr float VIS = 1.65f;
constexpr float DT = 1.f / 60.f;
constexpr int SHIFT_N = 420;
constexpr float SHIFT_DZ = 1.5f;
constexpr float RIVAL_X = 0.20f;

struct Bend {
    float a, b, c, d, k;
    bool ice;
};

// Ramps in meters. Positive kappa bends the run to the right.
const Bend kBends[] = {
    {180, 250, 430, 510, 0.0086f, false},
    {600, 680, 920, 1010, -0.0100f, false},
    {1120, 1200, 1420, 1520, 0.0090f, true},
    {1620, 1680, 1840, 1920, -0.0088f, false},
    {1980, 2020, 2060, 2110, 0.0065f, true},
};

float piece(float z, const Bend& r) {
    if (z <= r.a || z >= r.d) return 0;
    if (z < r.b) return r.k * (z - r.a) / (r.b - r.a);
    if (z > r.c) return r.k * (r.d - z) / (r.d - r.c);
    return r.k;
}

float hash01(int i) {
    uint32_t h = uint32_t(i) * 747796405u + 2891336453u;
    h ^= h >> 16;
    return float(h & 0xffffff) / float(0x1000000);
}

}  // namespace

float Game::kappa(float z) const {
    float k = 0;
    for (const Bend& r : kBends) k += piece(z, r);
    return k;
}

bool Game::ice(float z) const {
    for (const Bend& r : kBends)
        if (r.ice && z >= r.a && z <= r.d) return true;
    return false;
}

void Game::tune() {
    gs::FMPatch bell;
    bell.alg = 5;
    bell.fb = 0.15f;
    bell.vol = 0.22f;
    bell.echo = 0.35f;
    bell.op[0].mul = 1.f;
    bell.op[0].level = 0.7f;
    bell.op[1].mul = 1.f;
    bell.op[1].level = 1.f;
    bell.op[2].mul = 2.8f;
    bell.op[2].level = 0.45f;
    bell.op[3].mul = 4.2f;
    bell.op[3].level = 0.28f;
    for (int i = 0; i < 4; i++) {
        bell.op[i].ar = 0.004f;
        bell.op[i].dr = 0.22f;
        bell.op[i].sl = 0.12f;
        bell.op[i].rr = 0.35f;
    }
    for (int ch = 0; ch < 4; ch++) sys_->apu.setPatch(ch, bell);
    sys_->apu.setEcho(0.16f, 0.32f, 0.16f);
    sys_->apu.setMaster(0.85f);
}

void Game::buildCourse() {
    things_.clear();
    auto add = [&](float z, float x, float r, Kind k, int var, bool solid) {
        things_.push_back({z, x, r, k, var, solid, false});
    };
    add(36.f, 0.f, 0.f, Kind::Drop, 0, false);
    add(FINISH - 12.f, 0.f, 0.f, Kind::Finish, 0, false);
    const float rocks[][2] = {{340, -0.88f}, {430, -0.84f}, {540, 0.66f},   {780, 0.88f},
                               {900, 0.84f},  {1060, -0.64f}, {1300, -0.88f}, {1410, -0.84f},
                               {1560, 0.66f}, {1760, 0.86f},  {1860, -0.62f}, {1960, 0.84f}};
    for (auto& rk : rocks) add(rk[0], rk[1], 0.15f, Kind::Rock, int(rk[0]) & 1, true);
    for (float z = 48.f; z < FINISH - 40.f; z += 26.f) {
        int n = int(z);
        float side = (n / 26) % 2 ? 1.f : -1.f;
        float j = float((n / 7) % 4) * 0.05f;
        add(z, side * (1.38f + j), 0.20f, Kind::Tree, n % 3, true);
        add(z + 12.f, -side * (1.78f + j * 0.5f), 0.16f, Kind::Tree, (n + 1) % 3, true);
        if ((n / 26) % 5 == 0) {
            add(z, 0.96f, 0.f, Kind::Flag, 0, false);
            add(z, -0.96f, 0.f, Kind::Flag, 1, false);
        }
    }
}

void Game::resetRun() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    lead_ = false;
    banner_ = Banner::None;
    crashes_ = 0;
    margin_ = 0;
    puffN_ = 0;
    report_[0] = 0;
    modeTime_ = 0;
    raceTime_ = 0;
    playerZ_ = 0;
    playerX_ = 0;
    latV_ = 0;
    speed_ = 0;
    rivalZ_ = HEAD;
    rivalSpd_ = CLOCK;
    yaw_ = 0;
    steerSm_ = 0;
    stun_ = 0;
    shake_ = 0;
    bankCd_ = 0;
    bannerT_ = 0;
    tickAcc_ = 0;
    tickVol_ = 0;
    fanT_ = 0;
    tucking_ = false;
    braking_ = false;
    for (auto& t : things_) t.hit = false;
    for (auto& p : puffs_) p.life = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    draws_.reserve(256);
    buildArt(sys.vdp, art_);
    buildCourse();
    for (int i = 0; i < 20; i++) {
        flakes_[i].x = hash01(i) * 320.f;
        flakes_[i].y = hash01(i + 30) * 224.f;
        flakes_[i].sp = 16.f + hash01(i + 60) * 36.f;
        flakes_[i].sc = 5.f + hash01(i + 90) * 6.f;
    }
    tune();
    resetRun();
    sys.vdp.A.enabled = false;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Count) return 1;
    if (mode_ == Mode::Result) return 4;
    return lead_ ? 3 : 2;
}

void Game::beginCount() {
    mode_ = Mode::Count;
    modeTime_ = 0;
    playerZ_ = 0;
    playerX_ = 0;
    latV_ = 0;
    speed_ = 0;
    rivalZ_ = HEAD;
    rivalSpd_ = CLOCK;
}

void Game::launch() {
    mode_ = Mode::Run;
    modeTime_ = 0;
    raceTime_ = 0;
    speed_ = 18.f;
    rivalSpd_ = CLOCK;
    playerZ_ = 0;
    rivalZ_ = HEAD;
}

void Game::human(float& steer, bool& tuck, bool& brake) {
    const gs::Pad& p = sys_->pad;
    steer = p.axisX;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    steer = std::clamp(steer, -1.f, 1.f);
    tuck = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO) || p.accel > 0.25f;
    brake = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.25f;
}

void Game::bot(float& steer, bool& tuck, bool& brake) {
    const bool onIce = ice(playerZ_);
    const float grip = onIce ? ICE_GRIP : 1.f;
    const float look = std::clamp(speed_ * 0.85f, 22.f, 40.f);
    const float k0 = kappa(playerZ_);
    const float k1 = kappa(playerZ_ + look * 0.45f);
    auto ffOf = [&](float k) { return (k * speed_ * speed_ * CENT) / (STEER_A * std::max(grip, 0.35f)); };
    const float ff = ffOf(k0) * 0.72f + ffOf(k1) * 0.28f;

    float want = 0.f;
    for (const Thing& t : things_) {
        if (!t.solid || t.hit) continue;
        float dz = t.z - playerZ_;
        if (dz < 1.5f || dz > look) continue;
        float pred = playerX_ + latV_ * (dz / std::max(speed_, 10.f));
        float rad = t.r + 0.24f;
        if (std::fabs(pred - t.x) > rad && std::fabs(playerX_ - t.x) > rad) continue;
        float side = pred >= t.x ? 1.f : -1.f;
        float clear = std::clamp(t.x + side * (rad + 0.06f), -0.78f, 0.78f);
        float urg = std::clamp(1.f - dz / look, 0.f, 1.f);
        urg *= urg;
        want += (clear - want) * urg;
    }
    want = std::clamp(want, -0.78f, 0.78f);
    steer = ff + (want - playerX_) * 2.35f - latV_ * 3.1f;
    if (std::fabs(playerX_) > 0.88f) steer += (0.f - playerX_) * 1.6f;
    steer = std::clamp(steer, -1.f, 1.f);

    brake = false;
    float worst = 0.f;
    bool soon = false;
    for (float d = 0; d <= 42.f; d += 3.f) {
        float z = playerZ_ + d;
        if (!ice(z)) continue;
        soon = true;
        worst = std::max(worst, std::fabs(kappa(z)));
    }
    if (soon && worst > 0.002f) {
        float v2 = (STEER_A * ICE_GRIP * 0.82f) / (worst * CENT);
        if (speed_ > std::sqrt(std::max(v2, 16.f)) + 0.3f) brake = true;
    }
    if (std::fabs(playerX_) > 0.82f) brake = true;
    tuck = !brake && std::fabs(playerX_) < 0.62f && std::fabs(ff) < 0.82f;
}

void Game::bump(float keep) {
    if (bankCd_ > 0) return;
    bankCd_ = 0.4f;
    speed_ = std::max(12.f, speed_ * keep);
    stun_ = std::max(stun_, 0.22f);
    shake_ = 0.28f;
    crashes_++;
    sys_->apu.noiseBurst(0.42f, 2600.f, 0.1f);
    sys_->rumble(0.7f, 0.35f, 110);
}

void Game::collide(float prevZ) {
    for (Thing& t : things_) {
        if (!t.solid || t.hit) continue;
        if (playerZ_ < t.z - 1.6f || prevZ > t.z + 1.6f) continue;
        if (std::fabs(playerX_ - t.x) >= t.r + 0.12f) continue;
        t.hit = true;
        float side = playerX_ >= t.x ? 1.f : -1.f;
        playerX_ = t.x + side * (t.r + 0.16f);
        latV_ = side * 0.9f;
        bump(t.kind == Kind::Tree ? 0.62f : 0.7f);
    }
}

void Game::finishRace() {
    float pCross, rCross;
    if (playerZ_ >= FINISH) pCross = raceTime_ - (playerZ_ - FINISH) / std::max(speed_, 1.f);
    else pCross = raceTime_ + (FINISH - playerZ_) / std::max(speed_, 1.f);
    if (rivalZ_ >= FINISH) rCross = raceTime_ - (rivalZ_ - FINISH) / std::max(rivalSpd_, 1.f);
    else rCross = raceTime_ + (FINISH - rivalZ_) / std::max(rivalSpd_, 1.f);
    won_ = pCross <= rCross;
    margin_ = std::fabs(rCross - pCross);
    std::snprintf(report_, sizeof report_, "S3 SLED  %s  %s %.1f s", won_ ? "PASS" : "FAIL",
                  won_ ? "beat the clock sled by" : "the clock sled won by", margin_);
    mode_ = Mode::Result;
    modeTime_ = 0;
    over_ = true;
    fanT_ = 0.85f;
    sys_->apu.keyOn(0, won_ ? 523.25f : 196.f, 0.2f);
    sys_->apu.keyOn(1, won_ ? 659.25f : 246.94f, 0.16f);
    sys_->apu.keyOn(2, won_ ? 783.99f : 311.13f, 0.14f);
    sys_->rumble(won_ ? 0.3f : 0.6f, won_ ? 0.5f : 0.2f, won_ ? 180 : 220);
}

void Game::race(float dt) {
    float steer = 0;
    bool tuck = false, brake = false;
    if (bot_) bot(steer, tuck, brake);
    else human(steer, tuck, brake);
    if (stun_ > 0) {
        stun_ -= dt;
        steer *= 0.4f;
        tuck = false;
    }
    float follow = bot_ ? 1.f : std::min(1.f, dt * 10.f);
    steerSm_ += (steer - steerSm_) * follow;
    braking_ = brake;
    tucking_ = tuck && !brake;

    const bool onIce = ice(playerZ_);
    float grip = onIce ? ICE_GRIP : (std::fabs(playerX_) > 1.04f ? 0.62f : 1.f);
    float steerU = steerSm_ * STEER_A * grip / HALF_W;
    float pushU = -kappa(playerZ_) * speed_ * speed_ * CENT / HALF_W;
    float damp = onIce ? 1.7f : 5.6f;
    latV_ += (steerU + pushU) * dt;
    latV_ *= std::max(0.f, 1.f - damp * dt);
    playerX_ += latV_ * dt;

    float vmax = tucking_ ? V_TUCK : V_SIT;
    if (brake) speed_ -= 16.f * dt;
    else if (speed_ < vmax) speed_ += (tucking_ ? 9.2f : 4.2f) * dt;
    else speed_ -= 9.f * dt;
    if (std::fabs(playerX_) > 1.04f) speed_ -= (10.f + (std::fabs(playerX_) - 1.04f) * 24.f) * dt;
    speed_ = std::clamp(speed_, 11.f, 46.f);

    if (playerX_ > 1.58f) {
        playerX_ = 1.46f;
        latV_ = -0.85f;
        bump(0.8f);
    } else if (playerX_ < -1.58f) {
        playerX_ = -1.46f;
        latV_ = 0.85f;
        bump(0.8f);
    }
    if (bankCd_ > 0) bankCd_ -= dt;

    float prevZ = playerZ_;
    playerZ_ += speed_ * dt;
    rivalZ_ += rivalSpd_ * dt;
    yaw_ += kappa(playerZ_) * speed_ * dt;
    raceTime_ += dt;
    collide(prevZ);

    bool ahead = playerZ_ > rivalZ_;
    if (ahead && !lead_) {
        lead_ = true;
        banner_ = Banner::Passed;
        bannerT_ = 1.35f;
        sys_->apu.keyOn(3, 880.f, 0.16f);
    } else if (!ahead && lead_) {
        lead_ = false;
        banner_ = Banner::Lost;
        bannerT_ = 1.35f;
    }
    if (bannerT_ > 0) bannerT_ -= dt;

    if (speed_ > 20.f && (int(raceTime_ * 60.f) % 2) == 0) {
        Puff& p = puffs_[puffN_++ % 28];
        p.x = 160.f + steerSm_ * 10.f + (hash01(puffN_ + int(playerZ_)) - 0.5f) * 18.f;
        p.y = 198.f;
        p.vx = -steerSm_ * 30.f + (hash01(puffN_ * 3) - 0.5f) * 40.f;
        p.vy = -(16.f + hash01(puffN_ * 5) * 28.f);
        p.life = 0.32f;
        p.sc = 4.f + hash01(puffN_) * 5.f;
    }
    for (Puff& p : puffs_) {
        if (p.life <= 0) continue;
        p.life -= dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy += 40.f * dt;
    }

    if (playerZ_ >= FINISH || rivalZ_ >= FINISH) finishRace();
    if (raceTime_ > 150.f && mode_ == Mode::Run) {
        won_ = false;
        std::snprintf(report_, sizeof report_, "S3 SLED  FAIL  the run did not finish");
        mode_ = Mode::Result;
        over_ = true;
    }
}

void Game::audio(float dt) {
    if (mode_ == Mode::Result) {
        sys_->apu.noise(0, 1000, false);
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        if (fanT_ > 0) {
            fanT_ -= dt;
            if (fanT_ <= 0) {
                sys_->apu.keyOff(0);
                sys_->apu.keyOff(1);
                sys_->apu.keyOff(2);
            }
        }
        return;
    }
    float period = mode_ == Mode::Run ? 0.5f : 1.f;
    tickAcc_ += dt;
    if (tickAcc_ >= period) {
        tickAcc_ -= period;
        tock_ = !tock_;
        tickFreq_ = tock_ ? 392.f : 294.f;
        float dist = std::fabs(rivalZ_ - playerZ_);
        tickVol_ = (mode_ == Mode::Run ? 0.035f : 0.02f) + 0.07f * std::clamp(1.f - dist / 100.f, 0.f, 1.f);
    }
    tickVol_ *= std::max(0.f, 1.f - dt * 7.f);
    sys_->apu.tone(0, tickFreq_, tickVol_);
    float wind = mode_ == Mode::Run ? std::min(0.05f, speed_ * 0.0011f) : 0.012f;
    sys_->apu.noise(wind, 4200.f + speed_ * 28.f, false);
    float scrape = mode_ == Mode::Run ? 0.02f * (speed_ / 40.f) : 0.f;
    sys_->apu.tone(1, 70.f + speed_ * 2.5f, scrape);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = DT;
    if (shake_ > 0) shake_ -= dt;
    shakeX_ = shake_ > 0 ? int(sys.frame * 17 % 5) - 2 : 0;
    shakeY_ = shake_ > 0 ? int(sys.frame * 13 % 3) - 1 : 0;
    for (int i = 0; i < 20; i++) {
        Flake& f = flakes_[i];
        f.y += f.sp * dt;
        f.x += std::sin(f.y * 0.05f + i) * 10.f * dt;
        if (f.y > 230.f) {
            f.y = -6.f;
            f.x = hash01(i + int(sys.frame)) * 320.f;
        }
        if (f.x < -8) f.x += 320.f;
        if (f.x > 328) f.x -= 320.f;
    }

    modeTime_ += dt;
    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
        if (sys.pad.pressed(gs::BTN_START) || (bot_ && modeTime_ > 0.3f)) beginCount();
    } else if (mode_ == Mode::Count) {
        if (modeTime_ > 2.05f) launch();
    } else if (mode_ == Mode::Run) {
        race(dt);
    } else if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
        resetRun();
    }
    audio(dt);
    draw();
}

float Game::shiftAt(float z) const {
    if (z <= 0) return 0;
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
    queue(-20.f, s);
}

Game::Proj Game::project(float worldZ, float roadX) const {
    Proj p{};
    float dz = worldZ - playerZ_;
    p.z = dz;
    if (dz < 2.5f || dz > 340.f) return p;
    float dy = FOCAL * CAM_H / dz;
    p.y = float(hor_) + dy + float(shakeY_);
    p.hw = FOCAL * HALF_W / dz;
    float su = std::clamp(shiftAt(dz), -9.f, 9.f);
    p.x = 160.f + float(shakeX_) + (su - playerX_) * p.hw + roadX * p.hw;
    p.fog = std::clamp((dz - 18.f) / 16.f, 0.f, 14.f);
    p.ok = true;
    return p;
}

void Game::sky() {
    int bob = mode_ == Mode::Run ? int(std::sin(raceTime_ * 2.1f) * (speed_ / 38.f)) : 0;
    hor_ = HORIZON + bob + (shake_ > 0 ? shakeY_ : 0);
    hor_ = std::clamp(hor_, 96, 116);
    gs::VDP& v = sys_->vdp;
    v.setFogColor(gs::rgb4(12, 14, 15));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(float(y) / float(hor_), 0.f, 1.f);
        float s = u * u * (3.f - 2.f * u);
        int r = int(std::lround(3 + 11 * s));
        int g = int(std::lround(6 + 8 * s));
        int b = int(std::lround(12 + 3 * s));
        if (y > hor_) {
            r = 12;
            g = 14;
            b = 15;
        }
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        if (y < hor_) v.lineFog[y] = uint8_t(u * u * 7.f);
        else v.lineFog[y] = uint8_t(std::min(12.f, (FOCAL * CAM_H / std::max(1, y - hor_)) / 28.f));
        float depth = y < hor_ ? 0.2f + 0.8f * float(y) / float(hor_) : 1.f;
        float showYaw = yaw_ + (mode_ == Mode::Run ? 0.f : std::sin(modeTime_ * 0.6f) * 0.12f);
        v.B.hscroll[y] = int16_t(showYaw * 60.f * depth + shakeX_);
        v.B.vscroll[y] = 0;
    }
    v.A.enabled = false;
    v.B.enabled = true;
}

void Game::road() {
    float heading = 0, x = 0;
    shiftU_[0] = 0;
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
        if (dz > 520.f) continue;
        float hw = FOCAL * HALF_W / dz;
        float su = std::clamp(shiftAt(dz), -9.f, 9.f);
        bool icy = ice(playerZ_ + dz);
        bool wall = icy || std::fabs(kappa(playerZ_ + dz)) > 0.0048f;
        L.on = true;
        L.cx = 160.f + float(shakeX_) + (su - playerX_) * hw;
        L.hw = hw;
        L.v = playerZ_ + dz;
        L.pal = uint8_t(icy ? PAL_ICE : PAL_SNOW);
        L.band = (int(std::floor((playerZ_ + dz) * 0.09f)) & 1) ? 1 : 0;
        L.style = icy ? gs::ROAD_ICE : gs::ROAD_SNOW;
        L.left = L.right = wall ? gs::GROUND_SNOWWALL : gs::GROUND_LAND;
    }
}

void Game::world() {
    auto poleBanner = [&](const gs::Mipped& img, float zGate, float hangM) {
        Proj p = project(zGate, 0);
        if (!p.ok) return;
        float banH = FOCAL * 1.55f / p.z;
        float pole = FOCAL * hangM / p.z;
        blit(img, p.x, p.y - (pole - banH), banH, PAL_BANNER, false, int(p.fog), p.z - 0.2f);
        blit(art_.flag, p.x + p.hw * 0.92f, p.y, pole, PAL_BANNER, false, int(p.fog), p.z);
        blit(art_.flag, p.x - p.hw * 0.92f, p.y, pole, PAL_BANNER, true, int(p.fog), p.z);
    };
    for (const Thing& t : things_) {
        if (t.kind == Kind::Drop) {
            poleBanner(art_.drop, t.z, 3.3f);
            continue;
        }
        if (t.kind == Kind::Finish) {
            poleBanner(art_.banner, t.z, 3.5f);
            continue;
        }
        Proj p = project(t.z, t.x);
        if (!p.ok || p.x < -70.f || p.x > 390.f) continue;
        if (t.kind == Kind::Tree) {
            float mh = t.var == 0 ? 8.2f : t.var == 1 ? 6.6f : 9.4f;
            float h = FOCAL * mh / p.z;
            if (p.z < 48.f) blit(art_.shadow, p.x, p.y, h * 0.18f, PAL_FX, false, int(p.fog), p.z + 0.3f, true);
            blit(art_.tree[t.var], p.x, p.y, h, PAL_TREE, t.x < 0, int(p.fog), p.z);
        } else if (t.kind == Kind::Rock) {
            float h = FOCAL * (t.var ? 1.35f : 1.15f) / p.z;
            blit(art_.rock[t.var], p.x, p.y, h, PAL_ROCK, false, int(p.fog), p.z);
        } else if (t.kind == Kind::Flag) {
            float h = FOCAL * 2.7f / p.z;
            blit(art_.flag, p.x, p.y, h, PAL_BANNER, t.var != 0, int(p.fog), p.z);
        }
    }

    float dz = rivalZ_ - playerZ_;
    int hand = int(raceTime_ * 4.f + modeTime_ * 2.f) & 3;
    const gs::Mipped& rival = art_.rival[hand];
    if (dz >= 2.5f && dz < 300.f) {
        Proj p = project(rivalZ_, RIVAL_X);
        if (p.ok) {
            float h = FOCAL * 1.55f / p.z;
            blit(art_.shadow, p.x, p.y, h * 0.28f, PAL_FX, false, int(p.fog), p.z + 0.2f, true);
            blit(rival, p.x, p.y, h, PAL_RIVAL, kappa(rivalZ_) < 0, int(p.fog), p.z);
        }
    } else if (dz > -6.f && dz < 2.5f) {
        Proj p = project(playerZ_ + 2.5f, RIVAL_X);
        float t = std::clamp((dz + 6.f) / 8.5f, 0.f, 1.f);
        if (p.ok && t > 0.2f) {
            float x = 236.f + (p.x - 236.f) * t;
            float y = 198.f + (p.y - 198.f) * t;
            float h = 34.f + (FOCAL * 1.55f / 2.5f - 34.f) * t;
            blit(rival, x, y, h * t + 8.f * (1.f - t), PAL_RIVAL, false, int((1.f - t) * 8), 1.2f);
        }
    }

    float foot = 202.f + float(hor_ - HORIZON);
    float lean = steerSm_;
    bool hard = std::fabs(lean) > 0.42f;
    const gs::Mipped& me = hard ? art_.playerLean : (tucking_ ? art_.playerTuck : art_.playerSit);
    float ph = tucking_ && !hard ? 50.f : 56.f;
    blit(art_.shadow, 160.f + lean * 8.f, foot + 2.f, 16.f, PAL_FX, false, 0, 0.55f, true);
    blit(me, 160.f + lean * 9.f, foot, ph, PAL_PLAYER, hard && lean > 0, 0, 0.25f);

    for (const Puff& p : puffs_) {
        if (p.life <= 0) continue;
        blit(art_.flake, p.x, p.y, p.sc, PAL_FX, false, 0, 0.15f);
    }
    for (const Flake& f : flakes_) blit(art_.flake, f.x, f.y, f.sc, PAL_FX, false, 0, -1.f);

    if (mode_ == Mode::Title) {
        ui(art_.titleMain, (320 - art_.titleMain.w) * 0.5f, 10);
        ui(art_.titleA, (320 - art_.titleA.w) * 0.5f, 48);
        ui(art_.titleB, (320 - art_.titleB.w) * 0.5f, 72);
    } else if (mode_ == Mode::Count) {
        int n = modeTime_ < 0.55f ? 0 : modeTime_ < 1.10f ? 1 : modeTime_ < 1.65f ? 2 : 3;
        const gs::Image& im = art_.count[n];
        ui(im, (320 - im.w) * 0.5f, 36);
    } else if (bannerT_ > 0 && banner_ != Banner::None) {
        const gs::Image& im = banner_ == Banner::Passed ? art_.passed : art_.lostLead;
        ui(im, (320 - im.w) * 0.5f, 36);
    } else if (mode_ == Mode::Result) {
        const gs::Image& im = won_ ? art_.winText : art_.loseText;
        ui(im, (320 - im.w) * 0.5f, 40);
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
        hudText(1, 26, "ARROWS STEER   Z TUCK   X BRAKE", PAL_WHITE);
        hudText(12, 27, "ENTER TO DROP", PAL_GOLD);
        return;
    }

    float gap = playerZ_ - rivalZ_;
    char line[48];
    int pal = gap >= 0 ? PAL_GOLD : PAL_ALERT;
    if (mode_ == Mode::Result) {
        std::snprintf(line, sizeof line, won_ ? "BEAT THE CLOCK BY %.1f S" : "CLOCK WON BY %.1f S", margin_);
    } else if (std::fabs(gap) < 2.f) {
        std::snprintf(line, sizeof line, "LEVEL WITH THE CLOCK");
        pal = PAL_GOLD;
    } else if (gap > 0) {
        std::snprintf(line, sizeof line, "AHEAD OF THE CLOCK  %d M", int(gap + 0.5f));
    } else {
        std::snprintf(line, sizeof line, "THE CLOCK LEADS  %d M", int(-gap + 0.5f));
    }
    int col = std::max(0, (40 - int(std::strlen(line))) / 2);
    hudText(col, 0, line, pal);

    char bar[41];
    for (int i = 0; i < 40; i++) bar[i] = '-';
    bar[40] = 0;
    int ycol = std::clamp(int(playerZ_ / FINISH * 37.f) + 1, 1, 38);
    int ccol = std::clamp(int(rivalZ_ / FINISH * 37.f) + 1, 1, 38);
    bar[0] = '|';
    bar[39] = '|';
    bar[ycol] = 'Y';
    bar[ccol] = (ccol == ycol) ? 'X' : 'C';
    hudText(0, 1, bar, PAL_WHITE);

    if (mode_ == Mode::Result) {
        std::snprintf(line, sizeof line, "HITS %d", crashes_);
        hudText(1, 26, line, crashes_ ? PAL_ALERT : PAL_WHITE);
        if (!bot_) hudText(9, 27, "ENTER FOR ANOTHER RUN", PAL_WHITE);
        return;
    }
    if (mode_ == Mode::Count) return;

    int kmh = int(speed_ * 3.6f + 0.5f);
    const char* state = "COAST";
    int sp = PAL_WHITE;
    if (stun_ > 0) {
        state = "HIT";
        sp = PAL_ALERT;
    } else if (std::fabs(playerX_) > 1.04f) {
        state = "POWDER";
        sp = PAL_ALERT;
    } else if (ice(playerZ_)) {
        state = "ICE";
        sp = PAL_ALERT;
    } else if (braking_) {
        state = "BRAKE";
        sp = PAL_GOLD;
    } else if (tucking_) {
        state = "TUCK";
        sp = PAL_GOLD;
    }
    std::snprintf(line, sizeof line, "%3d KM/H", kmh);
    hudText(1, 26, line, PAL_WHITE);
    hudText(32, 26, state, sp);
    if (FINISH - playerZ_ < 120.f && playerZ_ < FINISH) hudText(15, 27, "FINISH", PAL_GOLD);
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

}  // namespace sled
