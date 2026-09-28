#include "pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace trampass {
namespace {

constexpr int HORIZON = 78;
constexpr float FOCAL = 220.f;
constexpr float CAM_H = 1.35f;
constexpr float HALF_W = 2.35f;
constexpr float FINISH = 760.f;
constexpr float RIVAL0 = 42.f;
constexpr float RIVAL_V = 27.2f;
constexpr float V_GAS = 40.f;
constexpr float V_COAST = 22.f;
constexpr float V_BRAKE = 28.f;
constexpr float POINT_FAST = 36.6f;
constexpr float VIS = 1.05f;
constexpr float DT = 1.f / 60.f;
constexpr int SHIFT_N = 220;
constexpr float SHIFT_DZ = 1.45f;
constexpr float NEAR_Z = 3.2f;
constexpr float FAR_Z = 260.f;

struct Bend {
    float a, b, c, d, k;
};

const Bend kBends[] = {
    {80.f, 140.f, 200.f, 270.f, 0.0034f},
    {300.f, 360.f, 430.f, 500.f, -0.0038f},
    {540.f, 590.f, 660.f, 730.f, 0.0028f},
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

float Game::crewLeft() const {
    float left = (FINISH - rivalZ_) / RIVAL_V;
    return std::max(0.f, left);
}

float Game::stormAmt() const {
    if (mode_ == Mode::Title || mode_ == Mode::Go) return 0.05f;
    if (end_ == End::Storm) return 1.f;
    float full = (FINISH - RIVAL0) / RIVAL_V;
    return std::clamp(1.f - crewLeft() / std::max(1.f, full), 0.f, 1.f);
}

const trampass::Game::Point* Game::nextPoint() const {
    for (const Point& pt : points_) {
        if (!pt.done && pt.z + 2.f >= playerZ_) return &pt;
    }
    return nullptr;
}

const char* Game::why() const {
    switch (end_) {
        case End::Storm: return "the other crew closed the pass";
        case End::Derail: return "the tram left the rail";
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
    sys_->apu.setEcho(0.1f, 0.22f, 0.16f);
}

void Game::buildCourse() {
    points_.clear();
    props_.clear();
    points_.push_back({190.f, -1, false});
    points_.push_back({390.f, 1, false});
    points_.push_back({580.f, -1, false});
    props_.push_back({28.f, 0.f, 0.f, 3});
    props_.push_back({FINISH, 0.f, 0.f, 4});
    for (const Point& pt : points_) props_.push_back({pt.z - 18.f, float(pt.side) * 0.72f, 1.6f, 5});
    for (float z = 70.f; z < FINISH - 30.f; z += 34.f) {
        int n = int(z);
        float side = (n / 34) & 1 ? 1.f : -1.f;
        int pick = n % 4;
        if (pick == 0) props_.push_back({z, side * 1.25f, 3.6f, 1});
        else if (pick == 1) props_.push_back({z, side * 1.45f, 3.2f + hash01(n) * 1.4f, 0});
        else props_.push_back({z, side * 1.6f, 1.2f, 2});
    }
}

void Game::resetRun() {
    won_ = false;
    over_ = false;
    end_ = End::None;
    gassing_ = false;
    braking_ = false;
    playerZ_ = 0;
    rivalZ_ = RIVAL0;
    speed_ = 0;
    yaw_ = 0;
    throwSm_ = 0;
    shake_ = 0;
    raceTime_ = 0;
    beepSec_ = -1;
    chime_ = 0;
    chimeT_ = 0;
    for (Point& pt : points_) pt.done = false;
    mode_ = Mode::Go;
    modeTime_ = 0;
}

void Game::launch() {
    mode_ = Mode::Run;
    modeTime_ = 0;
    raceTime_ = 0;
    speed_ = 26.f;
    playerZ_ = 0;
    rivalZ_ = RIVAL0;
    chime_ = 2;
    chimeT_ = 0.02f;
    sys_->apu.keyOn(0, 330.f, 0.07f);
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
        sys_->rumble(0.22f, 0.35f, 150);
        sys_->setLight(40, 140, 50);
    } else if (e == End::Storm) {
        shake_ = 0.4f;
        sys_->apu.noiseBurst(0.4f, 460.f, 0.24f);
        sys_->rumble(0.45f, 0.18f, 180);
        sys_->setLight(90, 100, 140);
    } else {
        shake_ = 0.6f;
        sys_->apu.noiseBurst(0.55f, 180.f, 0.22f);
        sys_->rumble(0.8f, 0.35f, 220);
        sys_->setLight(160, 30, 20);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildCourse();
    tune();
    draws_.reserve(96);
    shiftU_.assign(SHIFT_N + 1, 0.f);
    for (int i = 0; i < 20; i++) {
        flake_[i].x = hash01(i) * 320.f;
        flake_[i].y = hash01(i + 40) * 224.f;
        flake_[i].sp = 28.f + hash01(i + 80) * 70.f;
        flake_[i].sc = 2.f + hash01(i + 120) * 2.4f;
    }
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.hudEnabled = true;
    over_ = false;
    won_ = false;
    end_ = End::None;
    mode_ = Mode::Title;
    modeTime_ = 0;
    playerZ_ = 40.f;
    rivalZ_ = RIVAL0 + 30.f;
    speed_ = 0;
}

void Game::human(float& lever, bool& gas, bool& brake) {
    const gs::Pad& p = sys_->pad;
    lever = 0.f;
    if (p.down(gs::BTN_LEFT)) lever -= 1.f;
    if (p.down(gs::BTN_RIGHT)) lever += 1.f;
    if (std::fabs(p.axisX) > 0.25f) lever = p.axisX > 0.f ? 1.f : -1.f;
    brake = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.22f;
    gas = !brake && (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) ||
                     p.accel > 0.22f);
}

void Game::bot(float& lever, bool& gas, bool& brake) const {
    lever = 0.f;
    gas = true;
    brake = false;
    const Point* pt = nextPoint();
    if (!pt) return;
    float d = pt->z - playerZ_;
    if (d < 48.f) lever = float(pt->side);
    if (d < 30.f && speed_ > 33.f) {
        brake = true;
        gas = false;
    }
}

void Game::physics(float dt) {
    float lever = 0.f;
    bool gas = false, brake = false;
    if (bot_) bot(lever, gas, brake);
    else human(lever, gas, brake);
    gassing_ = gas;
    braking_ = brake;
    const float follow = bot_ ? 0.55f : std::min(1.f, dt * 14.f);
    throwSm_ += (lever - throwSm_) * follow;

    float target = gas ? V_GAS : V_COAST;
    if (brake) target = V_BRAKE;
    const float rate = brake ? 18.f : gas ? 8.f : 4.f;
    if (speed_ < target) speed_ += rate * dt;
    else speed_ -= rate * dt;
    speed_ = std::clamp(speed_, 12.f, 46.f);

    const float prevZ = playerZ_;
    const float prevR = rivalZ_;
    playerZ_ += speed_ * dt;
    rivalZ_ += RIVAL_V * dt;
    raceTime_ += dt;
    yaw_ += kappa(playerZ_) * speed_ * dt;

    for (Point& pt : points_) {
        if (pt.done) continue;
        if (playerZ_ < pt.z) continue;
        pt.done = true;
        int held = 0;
        if (throwSm_ > 0.4f) held = 1;
        else if (throwSm_ < -0.4f) held = -1;
        if (held != pt.side || speed_ > POINT_FAST) {
            end(End::Derail);
            return;
        }
        sys_->apu.keyOn(2, 520.f, 0.06f);
        sys_->rumble(0.15f, 0.08f, 40);
    }

    const bool pCross = playerZ_ >= FINISH && prevZ < FINISH;
    const bool rCross = rivalZ_ >= FINISH && prevR < FINISH;
    if (rCross && !pCross) {
        end(End::Storm);
        return;
    }
    if (pCross) {
        if (!rCross) {
            end(End::Clear);
            return;
        }
        float pt = (FINISH - prevZ) / std::max(0.01f, speed_);
        float rt = (FINISH - prevR) / RIVAL_V;
        end(pt <= rt ? End::Clear : End::Storm);
    }
}

void Game::flakes(float dt) {
    const float rush = mode_ == Mode::Run ? std::max(0.3f, speed_ / V_GAS) : 0.2f;
    const float blow = 0.3f + stormAmt() * 1.7f;
    for (int i = 0; i < 20; i++) {
        Flake& f = flake_[i];
        f.y += f.sp * rush * blow * dt;
        f.x += std::sin(modeTime_ + float(i)) * 8.f * dt;
        if (f.y > 236.f) {
            f.y = -6.f;
            f.x = 12.f + hash01(i + int(modeTime_ * 7.f)) * 296.f;
        }
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

    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
        if (sys.pad.pressed(gs::BTN_START) || (bot_ && modeTime_ > 0.35f)) resetRun();
    } else if (mode_ == Mode::Go) {
        if (modeTime_ > 0.5f) launch();
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
            static const float winN[] = {349.f, 440.f, 523.25f, 698.f};
            if (mode_ == Mode::Result && won_) {
                int i = 4 - chime_;
                if (i >= 0 && i < 4) sys_->apu.keyOn(i % 3, winN[i], 0.14f);
            } else if (!won_ && mode_ == Mode::Result) {
                sys_->apu.keyOn(0, 160.f, 0.12f);
            }
            chime_--;
            chimeT_ = 0.13f;
        }
    }
    if (mode_ != Mode::Run) {
        sys_->apu.noise(mode_ == Mode::Title ? 0.012f : 0.02f, 640.f, false);
        sys_->apu.tone(0, 0.f, 0.f);
        return;
    }
    sys_->apu.noise(0.02f + speed_ * 0.0008f + stormAmt() * 0.02f, 700.f + speed_ * 16.f, false);
    sys_->apu.tone(0, 48.f + speed_ * 1.1f, gassing_ ? 0.018f : 0.008f);
    int whole = int(crewLeft());
    if (whole <= 8 && whole >= 0 && whole != beepSec_ && crewLeft() > 0.05f) {
        beepSec_ = whole;
        sys_->apu.tone(1, whole <= 3 ? 880.f : 640.f, 0.04f);
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
    if (mode_ == Mode::Run) bob = int(std::sin(raceTime_ * 7.f) * (gassing_ ? 1.f : 0.3f));
    hor_ = std::clamp(HORIZON + bob + shakeY_, 64, 98);
    const float st = stormAmt();
    gs::VDP& v = sys_->vdp;
    v.setFogColor(gs::rgb4(ilerp(6, 12, st), ilerp(8, 13, st), ilerp(12, 15, st)));
    const float yawShow = (mode_ == Mode::Title || mode_ == Mode::Go) ? std::sin(modeTime_ * 0.25f) * 0.25f : yaw_;
    const int16_t hs = int16_t(std::lround(-yawShow * 50.f + float(shakeX_)));
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
        L.cx = 160.f + float(shakeX_) + su * hw;
        L.hw = hw;
        L.v = origin + dz;
        L.pal = uint8_t(PAL_ROAD);
        L.band = (int(std::floor((origin + dz) * 0.12f)) & 1) ? 1 : 0;
        L.style = gs::ROAD_ROCKY;
        L.left = gs::GROUND_SNOWWALL;
        L.right = gs::GROUND_SNOWWALL;
    }
}

void Game::gate(float z, const gs::Mipped& banner) {
    Proj p = project(z, 0.f);
    if (!p.ok) return;
    const float postH = FOCAL * 3.6f / p.z;
    const float banH = FOCAL * 0.9f / p.z;
    const int fog = int(p.fog);
    blit(art_.mast, p.x - p.hw * 1.02f, p.y, postH, PAL_MAST, false, fog, p.z);
    blit(art_.mast, p.x + p.hw * 1.02f, p.y, postH, PAL_MAST, true, fog, p.z + 0.02f);
    blit(banner, p.x, p.y - postH + banH, banH, PAL_BANNER, false, fog, p.z - 0.1f);
}

void Game::rival() {
    float dz = rivalZ_ - camZ();
    if (dz < 5.f || dz > FAR_Z) return;
    Proj p = project(rivalZ_, 0.55f);
    if (!p.ok) return;
    blit(art_.rival, p.x, p.y, FOCAL * 1.7f / p.z, PAL_RIVAL, false, int(p.fog), p.z);
}

void Game::tram() {
    const float bob = (mode_ == Mode::Run) ? std::sin(raceTime_ * 11.f) * 0.8f : 0.f;
    const float foot = 214.f + bob + float(shakeY_);
    const float px = 160.f + throwSm_ * 10.f + float(shakeX_);
    int fog = int(stormAmt() * stormAmt() * 4.f);
    blit(art_.shadow, px, foot + 2.f, 12.f, PAL_FX, false, 0, 0.3f, true);
    blit(art_.tram, px, foot, 96.f, PAL_TRAM, false, fog, 0.08f);
}

void Game::world() {
    const float st = stormAmt();
    for (const Prop& prop : props_) {
        if (prop.kind == 3) {
            if (mode_ == Mode::Title || mode_ == Mode::Go || playerZ_ < prop.z + 6.f) gate(prop.z, art_.yardBan);
            continue;
        }
        if (prop.kind == 4) {
            gate(prop.z, art_.passBan);
            continue;
        }
        if (prop.kind == 5) {
            Proj p = project(prop.z, prop.x);
            if (!p.ok) continue;
            const gs::Mipped& ch = prop.x < 0.f ? art_.chevL : art_.chevR;
            blit(ch, p.x, p.y - FOCAL * 1.3f / p.z, FOCAL * 0.7f / p.z, PAL_BANNER, false, int(p.fog), p.z);
            continue;
        }
        Proj p = project(prop.z, prop.x);
        if (!p.ok) continue;
        const float h = FOCAL * prop.h / p.z;
        if (prop.kind == 0) blit(art_.pine, p.x, p.y, h, PAL_PINE, prop.x < 0.f, int(p.fog), p.z);
        else if (prop.kind == 1) blit(art_.mast, p.x, p.y, h, PAL_MAST, false, int(p.fog), p.z);
        else blit(art_.rock, p.x, p.y, h, PAL_MAST, false, int(p.fog), p.z);
    }
    rival();
    int flakesOn = 5 + int(st * 15.f);
    for (int i = 0; i < flakesOn && i < 20; i++) {
        const Flake& f = flake_[i];
        blit(art_.flake, f.x, f.y, f.sc * (1.f + st), PAL_FX, false, int(st * 3.f), 0.2f);
    }
    tram();
    if (mode_ == Mode::Title) {
        ui(art_.title, (320.f - art_.title.w) * 0.5f, 8.f, PAL_GOLD);
        ui(art_.sub, (320.f - art_.sub.w) * 0.5f, 42.f, PAL_HUD);
    } else if (mode_ == Mode::Go) {
        ui(art_.go, (320.f - art_.go.w) * 0.5f, 16.f, PAL_GOLD);
    } else if (mode_ == Mode::Result) {
        const gs::Image& im = end_ == End::Clear ? art_.clear : end_ == End::Storm ? art_.closed : art_.derail;
        ui(im, (320.f - im.w) * 0.5f, 12.f, end_ == End::Clear ? PAL_GOLD : PAL_ALERT);
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
        return "THE TRAM LEFT THE RAIL";
    }
    const Point* pt = nextPoint();
    if (pt) {
        float d = pt->z - playerZ_;
        if (d < 55.f && d > -2.f) {
            if (speed_ > 34.f && d < 34.f) return "EASE THE POINTS";
            return pt->side < 0 ? "THROW LEFT" : "THROW RIGHT";
        }
    }
    if (crewLeft() < 8.f) return "THE OTHER CREW IS CLOSING";
    if (playerZ_ > FINISH - 110.f) return "THE ARCH. TAKE IT";
    return "BEAT THEIR CLOCK";
}

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    h.clear();
    if (mode_ == Mode::Title) {
        hudC(8, "ARROWS THROW THE POINTS", PAL_GOLD);
        hudC(9, "Z GAS   X BRAKE   ENTER TO ROLL", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Go) {
        hudC(8, "CLEAR THE PASS BEFORE THEIR CLOCK", PAL_ALERT);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(2, "PAUSED", PAL_GOLD);
        hudC(4, "ENTER TO ROLL", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Result) {
        char line[48];
        if (end_ == End::Clear)
            std::snprintf(line, sizeof line, "CLEAR  %.1f S   CREW %.1f", raceTime_, crewLeft());
        else if (end_ == End::Storm)
            std::snprintf(line, sizeof line, "CLOSED AT %d M", int(std::max(0.f, playerZ_) + 0.5f));
        else
            std::snprintf(line, sizeof line, "DERAILED AT %d M", int(std::max(0.f, playerZ_) + 0.5f));
        hudC(6, line, end_ == End::Clear ? PAL_GOLD : PAL_ALERT);
        hudC(8, hint(), end_ == End::Clear ? PAL_HUD : PAL_ALERT);
        if (!bot_) hudC(10, "ENTER FOR ANOTHER RUN", PAL_HUD);
        return;
    }
    char left[24], right[16];
    int leftM = int(std::max(0.f, FINISH - playerZ_) + 0.5f);
    std::snprintf(left, sizeof left, "CREW %4.1f", crewLeft());
    std::snprintf(right, sizeof right, "%4d M", leftM);
    hudText(1, 0, left, crewLeft() < 8.f ? PAL_ALERT : PAL_HUD);
    hudText(33, 0, right, PAL_HUD);
    if (gassing_) hudText(16, 0, "GAS", PAL_GOLD);
    else if (braking_) hudText(15, 0, "BRAKE", PAL_ALERT);
    const Point* pt = nextPoint();
    bool hot = pt && (pt->z - playerZ_) < 40.f;
    hudC(3, hint(), (crewLeft() < 8.f || hot) ? PAL_ALERT : PAL_GOLD);
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

}  // namespace trampass
