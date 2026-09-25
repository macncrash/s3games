#include "game/luge.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace luge {
namespace {

// Lateral position is in chute half-widths. 0 is the center, ±1 is the lip.
constexpr int HORIZON = 80;
constexpr float FOCAL = 210.f;
constexpr float CAM_H = 1.62f;
constexpr float HALF_W = 2.05f;
constexpr float FINISH = 1420.f;
constexpr float WALL = 0.97f;
constexpr float STEER_A = 9.f;
constexpr float CENT = 0.40f;
constexpr float ICE_GRIP = 0.60f;
constexpr float GROOVE = 1.15f;
constexpr float DAMP = 3.6f;
constexpr float V_RUN = 33.f;
constexpr float V_TUCK = 46.f;
constexpr float V_SLOW = 22.f;
constexpr float VIS = 1.35f;
constexpr float DT = 1.f / 60.f;
constexpr int SHIFT_N = 280;
constexpr float SHIFT_DZ = 1.7f;
constexpr float NEAR_Z = 6.f;
constexpr float FAR_Z = 240.f;

struct Bend {
    float a, b, c, d, k;
};

// Positive kappa bends the chute right and pushes the sled toward the left wall.
const Bend kBends[] = {
    {50.f, 90.f, 180.f, 240.f, 0.0056f},
    {300.f, 370.f, 560.f, 650.f, -0.0082f},
    {740.f, 810.f, 980.f, 1070.f, 0.0074f},
    {1140.f, 1190.f, 1260.f, 1310.f, -0.0052f},
};

float piece(float z, const Bend& r) {
    if (z <= r.a || z >= r.d) return 0.f;
    if (z < r.b) {
        float den = std::max(1.f, r.b - r.a);
        float u = (z - r.a) / den;
        float s = u * u * (3.f - 2.f * u);
        return r.k * s;
    }
    if (z > r.c) {
        float den = std::max(1.f, r.d - r.c);
        float u = (r.d - z) / den;
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

}  // namespace

float Game::kappa(float z) const {
    float k = 0.f;
    for (const Bend& r : kBends) k += piece(z, r);
    return k;
}

float Game::feed(float k) const {
    return (k * speed_ * speed_ * CENT) / (STEER_A * ICE_GRIP);
}

float Game::camZ() const { return playerZ_; }

float Game::lookX() const {
    if (mode_ == Mode::Title) return std::sin(modeTime_ * 0.65f) * 0.18f;
    return playerX_;
}

void Game::tune() {
    gs::FMPatch glass;
    glass.alg = 5;
    glass.fb = 0.12f;
    glass.vol = 0.2f;
    glass.echo = 0.42f;
    glass.glide = 0.02f;
    const float mul[4] = {1.f, 2.f, 3.4f, 5.1f};
    const float level[4] = {0.75f, 0.55f, 0.35f, 0.22f};
    for (int i = 0; i < 4; i++) {
        glass.op[i].mul = mul[i];
        glass.op[i].level = level[i];
        glass.op[i].ar = 0.004f;
        glass.op[i].dr = 0.18f;
        glass.op[i].sl = 0.12f;
        glass.op[i].rr = 0.38f;
    }
    for (int ch = 0; ch < 4; ch++) sys_->apu.setPatch(ch, glass);
    sys_->apu.setEcho(0.2f, 0.34f, 0.18f);
    sys_->apu.setMaster(0.82f);
}

void Game::buildChute() {
    props_.clear();
    props_.push_back({28.f, Kind::Drop});
    for (float z = 120.f; z < FINISH - 48.f; z += 72.f) props_.push_back({z, Kind::Gantry});
    props_.push_back({FINISH - 2.f, Kind::Finish});
}

void Game::clearRun() {
    over_ = false;
    won_ = false;
    verdict_ = Verdict::None;
    playerZ_ = 0;
    playerX_ = 0;
    latV_ = 0;
    speed_ = 0;
    yaw_ = 0;
    steerSm_ = 0;
    shake_ = 0;
    raceTime_ = 0;
    tucking_ = false;
    braking_ = false;
    chime_ = 0;
    fanT_ = 0;
    report_[0] = 0;
    for (auto& p : puff_) p.life = 0;
}

void Game::beginDrop() {
    clearRun();
    mode_ = Mode::Drop;
    modeTime_ = 0;
}

void Game::launch() {
    mode_ = Mode::Run;
    modeTime_ = 0;
    raceTime_ = 0;
    speed_ = 22.f;
    playerZ_ = 0;
    playerX_ = 0;
    latV_ = 0;
    chime_ = 0.16f;
    sys_->apu.keyOn(3, 988.f, 0.11f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildChute();
    tune();
    draws_.reserve(96);
    shiftU_.assign(SHIFT_N + 1, 0.f);
    for (int i = 0; i < 18; i++) {
        mote_[i].x = hash01(i) * 320.f;
        mote_[i].y = hash01(i + 20) * 220.f;
        mote_[i].sp = 28.f + hash01(i + 40) * 70.f;
        mote_[i].sc = 3.f + hash01(i + 60) * 4.f;
    }
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.hudEnabled = true;
    clearRun();
    mode_ = Mode::Title;
    modeTime_ = 0;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Drop) return 1;
    if (mode_ == Mode::Result) return 4;
    return std::fabs(playerX_) > 0.72f ? 3 : 2;
}

void Game::human(float& steer, bool& tuck, bool& brake) {
    const gs::Pad& p = sys_->pad;
    steer = p.axisX;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    steer = std::clamp(steer, -1.f, 1.f);
    tuck = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO) || p.accel > 0.3f;
    brake = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.3f;
}

void Game::bot(float& steer, bool& tuck, bool& brake) {
    const float look = 28.f;
    const float ff = feed(kappa(playerZ_)) * 0.48f + feed(kappa(playerZ_ + look)) * 0.52f;
    steer = ff - playerX_ * 1.15f - latV_ * 1.55f;
    if (std::fabs(playerX_) > 0.4f) steer -= playerX_ * 0.7f;
    steer = std::clamp(steer, -1.f, 1.f);
    tuck = false;
    brake = std::fabs(playerX_) > 0.7f;
}

void Game::end(bool win, Verdict v) {
    won_ = win;
    verdict_ = v;
    mode_ = Mode::Result;
    modeTime_ = 0;
    over_ = true;
    fanT_ = 1.25f;
    if (win) {
        std::snprintf(report_, sizeof report_, "S3 LUGE  PASS  stayed off the walls  %.1f s", raceTime_);
        sys_->apu.keyOn(0, 523.25f, 0.2f);
        sys_->apu.keyOn(1, 659.25f, 0.16f);
        sys_->apu.keyOn(2, 783.99f, 0.14f);
        sys_->rumble(0.25f, 0.45f, 160);
    } else if (v == Verdict::Slow) {
        std::snprintf(report_, sizeof report_, "S3 LUGE  FAIL  the chute did not end");
        sys_->apu.keyOn(0, 196.f, 0.16f);
        sys_->apu.keyOn(1, 233.08f, 0.12f);
        sys_->rumble(0.4f, 0.2f, 180);
    } else {
        std::snprintf(report_, sizeof report_, "S3 LUGE  FAIL  hit the wall at %d m", int(playerZ_ + 0.5f));
        shake_ = 0.45f;
        sys_->apu.noiseBurst(0.5f, 700.f, 0.16f);
        sys_->apu.keyOn(0, 146.83f, 0.2f);
        sys_->apu.keyOn(1, 174.61f, 0.14f);
        sys_->rumble(0.85f, 0.4f, 220);
    }
}

void Game::spawnPuff(float x, float y, float vx, float vy) {
    Puff& p = puff_[puffN_++ % 20];
    p.x = x;
    p.y = y;
    p.vx = vx;
    p.vy = vy;
    p.life = 0.28f;
    p.sc = 3.f + hash01(puffN_) * 4.f;
}

void Game::physics(float dt) {
    float steer = 0.f;
    bool tuck = false, brake = false;
    if (bot_) bot(steer, tuck, brake);
    else human(steer, tuck, brake);
    tucking_ = tuck && !brake;
    braking_ = brake;
    const float follow = bot_ ? 0.7f : std::min(1.f, dt * 18.f);
    steerSm_ += (steer - steerSm_) * follow;

    const float scale = STEER_A * ICE_GRIP / HALF_W;
    const float steerU = steerSm_ * scale;
    const float pushU = -kappa(playerZ_) * speed_ * speed_ * CENT / HALF_W;
    latV_ += (steerU + pushU - playerX_ * GROOVE) * dt;
    latV_ *= std::max(0.f, 1.f - DAMP * dt);
    latV_ = std::clamp(latV_, -3.5f, 3.5f);
    playerX_ += latV_ * dt;

    if (std::fabs(playerX_) >= WALL) {
        end(false, Verdict::Wall);
        return;
    }

    float target = V_RUN;
    if (tucking_) target = V_TUCK;
    if (braking_) target = V_SLOW;
    const float rate = braking_ ? 11.f : 5.4f;
    if (speed_ < target) speed_ += rate * dt;
    else speed_ -= (braking_ ? rate : 4.f) * dt;
    speed_ = std::clamp(speed_, 16.f, 52.f);

    playerZ_ += speed_ * dt;
    yaw_ += kappa(playerZ_) * speed_ * dt;
    raceTime_ += dt;

    if (playerZ_ >= FINISH) {
        end(true, Verdict::Clean);
        return;
    }
    if (raceTime_ > 100.f) {
        end(false, Verdict::Slow);
        return;
    }

    if ((sys_->frame & 1u) && speed_ > 24.f) {
        float side = (puffN_ & 1) ? 8.f : -8.f;
        spawnPuff(160.f + steerSm_ * 12.f + side, 198.f, side * 2.2f, -(24.f + hash01(puffN_ + 4) * 36.f));
    }
    if (std::fabs(playerX_) > 0.78f) {
        float side = playerX_ > 0 ? 18.f : -18.f;
        spawnPuff(160.f + side, 188.f, side * 3.f, -20.f);
    }
}

void Game::audio(float dt) {
    if (chime_ > 0) {
        chime_ -= dt;
        if (chime_ <= 0) sys_->apu.keyOff(3);
    }
    if (mode_ == Mode::Result) {
        sys_->apu.noise(0.f, 1000.f, false);
        sys_->apu.tone(0, 0.f, 0.f);
        sys_->apu.tone(1, 0.f, 0.f);
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

    const float rush = mode_ == Mode::Run ? speed_ : 8.f;
    sys_->apu.noise(0.012f + rush * 0.0011f, 1600.f + rush * 36.f, false);
    sys_->apu.tone(0, 48.f + rush * 1.7f, mode_ == Mode::Run ? 0.012f : 0.f);

    float near = 0.f;
    if (mode_ == Mode::Run) near = std::clamp((std::fabs(playerX_) - 0.75f) / 0.22f, 0.f, 1.f);
    if (near > 0.02f) sys_->apu.tone(1, 220.f + near * 640.f, 0.015f + near * 0.05f);
    else sys_->apu.tone(1, 0.f, 0.f);
}

void Game::motes(float dt) {
    const float rush = mode_ == Mode::Run ? std::max(0.45f, speed_ / V_RUN) : 0.32f;
    for (int i = 0; i < 18; i++) {
        Mote& m = mote_[i];
        m.y += m.sp * rush * dt;
        m.x += (m.x - 160.f) * 0.22f * rush * dt;
        if (m.y > 232.f || m.x < -12.f || m.x > 332.f) {
            m.y = -4.f;
            m.x = 30.f + hash01(i + int(modeTime_ * 8.f) + puffN_) * 260.f;
        }
    }
}

void Game::fadePuffs(float dt) {
    for (auto& p : puff_) {
        if (p.life <= 0) continue;
        p.life -= dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy += 30.f * dt;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = DT;
    modeTime_ += dt;
    if (shake_ > 0) shake_ -= dt;
    shakeX_ = 0;
    shakeY_ = 0;
    if (shake_ > 0) {
        int f = int(sys.frame);
        shakeX_ = (f * 17 % 5) - 2;
        shakeY_ = (f * 13 % 3) - 1;
    }
    motes(dt);
    fadePuffs(dt);

    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
        if (sys.pad.pressed(gs::BTN_START) || (bot_ && modeTime_ > 0.5f)) beginDrop();
    } else if (mode_ == Mode::Drop) {
        if (modeTime_ > 1.5f) launch();
    } else if (mode_ == Mode::Run) {
        physics(dt);
    } else if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
        beginDrop();
    }

    audio(dt);
    draw();
}

float Game::shiftAt(float z) const {
    if (z <= 0 || shiftU_.size() < 2) return 0.f;
    float i = z / SHIFT_DZ;
    int n = int(i);
    if (n >= SHIFT_N) return shiftU_[SHIFT_N];
    float f = i - float(n);
    return shiftU_[size_t(n)] * (1.f - f) + shiftU_[size_t(n + 1)] * f;
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

void Game::ui(const gs::Image& img, float x, float y) {
    if (img.w <= 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int(std::lround(x));
    s.y = int(std::lround(y));
    s.pal = PAL_TITLE;
    queue(-8.f, s);
}

Game::Proj Game::project(float worldZ, float roadX) const {
    Proj p{};
    float dz = worldZ - camZ();
    p.z = dz;
    if (dz < NEAR_Z || dz > FAR_Z) return p;
    p.y = float(hor_) + FOCAL * CAM_H / dz;
    p.hw = FOCAL * HALF_W / dz;
    float su = std::clamp(shiftAt(dz), -12.f, 12.f);
    p.x = 160.f + float(shakeX_) + (su - lookX()) * p.hw + roadX * p.hw;
    p.fog = std::clamp((dz - 24.f) / 18.f, 0.f, 14.f);
    p.ok = p.y > -20.f && p.y < float(gs::SCREEN_H + 40);
    return p;
}

void Game::sky() {
    int bob = 0;
    if (mode_ == Mode::Run) bob = int(std::sin(raceTime_ * 7.f) * 1.1f);
    hor_ = std::clamp(HORIZON + bob + shakeY_, 64, 110);
    gs::VDP& v = sys_->vdp;
    v.setFogColor(gs::rgb4(10, 13, 15));
    const float yawShow = mode_ == Mode::Title ? std::sin(modeTime_ * 0.4f) * 0.35f : yaw_;
    const int16_t hs = int16_t(std::lround(-yawShow * 32.f + float(shakeX_)));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= hor_) {
            float u = std::clamp(float(y) / float(hor_), 0.f, 1.f);
            float s = u * u * (3.f - 2.f * u);
            int r = int(std::lround(2.f + 11.f * s));
            int g = int(std::lround(3.f + 7.f * s));
            int b = int(std::lround(8.f + 4.f * s));
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = uint8_t(std::clamp((1.f - u) * 3.f, 0.f, 3.f));
        } else {
            v.lineBackdrop[y] = gs::rgb4(8, 11, 14);
            float dz = FOCAL * CAM_H / float(y - hor_);
            v.lineFog[y] = uint8_t(std::clamp((dz - 36.f) / 24.f, 0.f, 12.f));
        }
        v.B.hscroll[y] = y < hor_ + 12 ? hs : 0;
        v.B.vscroll[y] = 0;
    }
    v.A.enabled = false;
    v.B.enabled = true;
}

void Game::road() {
    if ((int)shiftU_.size() != SHIFT_N + 1) shiftU_.assign(SHIFT_N + 1, 0.f);
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
        if (dz > 460.f) continue;
        float hw = FOCAL * HALF_W / dz;
        float su = std::clamp(shiftAt(dz), -12.f, 12.f);
        L.on = true;
        L.cx = 160.f + float(shakeX_) + (su - lx) * hw;
        L.hw = hw;
        L.v = origin + scroll_ + dz;
        L.pal = uint8_t(PAL_ICE);
        L.band = (int(std::floor((origin + dz) * 0.16f)) & 1) ? 1 : 0;
        L.style = gs::ROAD_ICE;
        L.left = gs::GROUND_SNOWWALL;
        L.right = gs::GROUND_SNOWWALL;
    }
}

void Game::gantry(float z) {
    Proj p = project(z, 0.f);
    if (!p.ok) return;
    const float postH = FOCAL * 3.35f / p.z;
    const int fog = int(p.fog);
    blit(art_.post, p.x - p.hw * 1.04f, p.y, postH, PAL_POST, false, fog, p.z);
    blit(art_.post, p.x + p.hw * 1.04f, p.y, postH, PAL_POST, true, fog, p.z + 0.01f);
    const float bw = std::min(p.hw * 2.2f, 400.f);
    const float bh = std::max(2.f, postH * 0.065f);
    stretch(art_.beam, p.x - bw * 0.5f, p.y - postH, bw, bh, PAL_POST, fog, p.z - 0.15f);
}

void Game::gate(float z, const gs::Mipped& banner, int postPal) {
    Proj p = project(z, 0.f);
    if (!p.ok) return;
    const float postH = FOCAL * 3.7f / p.z;
    const float banH = FOCAL * 1.15f / p.z;
    const int fog = int(p.fog);
    blit(art_.post, p.x - p.hw * 0.98f, p.y, postH, postPal, false, fog, p.z);
    blit(art_.post, p.x + p.hw * 0.98f, p.y, postH, postPal, true, fog, p.z + 0.01f);
    blit(banner, p.x, p.y - postH + banH, banH, PAL_BANNER, false, fog, p.z - 0.2f);
}

void Game::world() {
    for (const Prop& prop : props_) {
        if (prop.kind == Kind::Gantry) gantry(prop.z);
        else if (prop.kind == Kind::Drop) gate(prop.z, art_.dropBan, PAL_POST);
        else gate(prop.z, art_.finishBan, PAL_CHECK);
    }

    for (const Mote& m : mote_) blit(art_.spark, m.x, m.y, m.sc, PAL_FX, false, 0, 0.35f);

    for (const Puff& p : puff_) {
        if (p.life <= 0) continue;
        blit(art_.spark, p.x, p.y, p.sc, PAL_FX, false, 0, 0.12f);
    }

    const float foot = 206.f + float(hor_ - HORIZON);
    const float lean = steerSm_;
    const bool hard = std::fabs(lean) > 0.28f;
    const gs::Mipped* me = &art_.flat;
    float ph = 62.f;
    if (mode_ == Mode::Result && verdict_ == Verdict::Wall) {
        me = &art_.lean;
        ph = 64.f;
    } else if (hard) {
        me = &art_.lean;
    } else if (tucking_) {
        me = &art_.tuck;
        ph = 54.f;
    }
    const float px = 160.f + lean * 10.f + float(shakeX_);
    blit(art_.shadow, px, foot + 3.f, 15.f, PAL_FX, false, 0, 0.45f, true);
    blit(*me, px, foot, ph, PAL_RIDER, hard && lean > 0.f, 0, 0.06f);

    if (mode_ == Mode::Title) {
        ui(art_.title, (320.f - art_.title.w) * 0.5f, 8.f);
        ui(art_.sub, (320.f - art_.sub.w) * 0.5f, 46.f);
    } else if (mode_ == Mode::Drop) {
        int n = modeTime_ < 0.5f ? 0 : modeTime_ < 1.0f ? 1 : 2;
        const gs::Image& im = art_.num[n];
        ui(im, (320.f - im.w) * 0.5f, 28.f);
    } else if (mode_ == Mode::Result) {
        const gs::Image& im = verdict_ == Verdict::Clean ? art_.clean : verdict_ == Verdict::Slow ? art_.slow : art_.hit;
        ui(im, (320.f - im.w) * 0.5f, 12.f);
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

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) h.set(x, y, 0);

    if (mode_ == Mode::Title) {
        hudC(24, "ONE ICE CHUTE", PAL_GOLD);
        hudC(26, "ARROWS STEER   Z TUCKS   X SLOWS", PAL_HUD);
        hudC(27, "ENTER TO DROP", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Drop) {
        hudC(26, "STAY OFF THE WALLS", PAL_ALERT);
        return;
    }

    if (mode_ == Mode::Result) {
        char line[48];
        if (verdict_ == Verdict::Clean) std::snprintf(line, sizeof line, "CLEAN  %.1f S", raceTime_);
        else if (verdict_ == Verdict::Slow) std::snprintf(line, sizeof line, "THE CHUTE DID NOT END");
        else std::snprintf(line, sizeof line, "WALLED AT %d M", int(std::max(0.f, playerZ_) + 0.5f));
        hudC(25, line, verdict_ == Verdict::Clean ? PAL_GOLD : PAL_ALERT);
        if (!bot_) hudC(27, "ENTER FOR ANOTHER DROP", PAL_HUD);
        return;
    }

    char left[16], right[16], bar[41];
    int leftM = int(std::max(0.f, FINISH - playerZ_) + 0.5f);
    int kmh = int(speed_ * 3.6f + 0.5f);
    std::snprintf(left, sizeof left, "%4d M", leftM);
    std::snprintf(right, sizeof right, "%3d KM/H", kmh);
    hudText(1, 0, left, PAL_HUD);
    hudText(30, 0, right, tucking_ ? PAL_GOLD : PAL_HUD);

    for (int i = 0; i < 40; i++) bar[i] = '-';
    bar[0] = '#';
    bar[39] = '#';
    bar[40] = 0;
    int slot = std::clamp(20 + int(std::lround(playerX_ * 18.f)), 1, 38);
    bar[slot] = 'O';
    int barPal = std::fabs(playerX_) > 0.85f ? PAL_ALERT : PAL_HUD;
    hudText(0, 1, bar, barPal);

    const float need = feed(kappa(playerZ_ + 32.f));
    const char* hint = "HOLD THE CENTER";
    int hintPal = PAL_GOLD;
    if (playerX_ > 0.9f) {
        hint = "STEER LEFT";
        hintPal = PAL_ALERT;
    } else if (playerX_ < -0.9f) {
        hint = "STEER RIGHT";
        hintPal = PAL_ALERT;
    } else if (need > 0.18f) {
        hint = "STEER RIGHT";
        hintPal = PAL_HUD;
    } else if (need < -0.18f) {
        hint = "STEER LEFT";
        hintPal = PAL_HUD;
    } else if (playerX_ > 0.32f) {
        hint = "STEER LEFT";
        hintPal = PAL_GOLD;
    } else if (playerX_ < -0.32f) {
        hint = "STEER RIGHT";
        hintPal = PAL_GOLD;
    }
    hudC(26, hint, hintPal);
    if (std::fabs(playerX_) > 0.55f) hudC(27, "STAY OFF THE WALLS", PAL_ALERT);
    else if (braking_) hudC(27, "SLOWING", PAL_GOLD);
    else if (tucking_) hudC(27, "TUCKED", PAL_GOLD);
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

}  // namespace luge
