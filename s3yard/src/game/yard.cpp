#include "game/yard.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "version.h"

namespace yard {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr float TAU = 6.2831853f;
constexpr float ARENA_W = 72.f;
constexpr float ARENA_H = 48.f;
constexpr float RAD = 3.05f;
constexpr float PURSE_X = 36.f;
constexpr float PURSE_Y = 24.f;
constexpr float ORIGIN_X = 16.f;
constexpr float ORIGIN_Y = 16.f;
constexpr float SCALE = 4.f;

struct Prop {
    float x, y, r;
    int kind;
};

constexpr Prop PROPS[] = {
    {22.f, 7.4f, 2.7f, 0}, {50.f, 7.2f, 2.9f, 1}, {24.f, 41.2f, 2.6f, 1}, {51.f, 40.6f, 2.8f, 0},
    {8.4f, 24.f, 2.4f, 0}, {64.2f, 24.f, 2.5f, 1}, {28.f, 16.f, 0.f, 2},  {46.f, 18.f, 0.f, 2},
    {44.f, 33.f, 0.f, 2},  {27.f, 34.f, 0.f, 2},  {48.f, 26.f, 0.f, 2},  {10.f, 9.f, 0.f, 3},
};

struct Spawn {
    float x, y, hp;
    int pal;
    bool player;
};

constexpr Spawn SPAWNS[] = {
    {15.f, 24.f, 180.f, PAL_YOU, true},
    {33.f, 31.f, 60.f, PAL_CREAM, false},
    {60.f, 14.f, 66.f, PAL_BLUE, false},
    {60.f, 34.f, 70.f, PAL_OLIVE, false},
};

float wrap(float a) {
    while (a > PI) a -= TAU;
    while (a < -PI) a += TAU;
    return a;
}

float mixAngle(float a, float b, float w) {
    float s = std::sin(a) * (1.f - w) + std::sin(b) * w;
    float c = std::cos(a) * (1.f - w) + std::cos(b) * w;
    return std::atan2(s, c);
}

void toScreen(float x, float y, float& sx, float& sy) {
    sx = ORIGIN_X + x * SCALE;
    sy = ORIGIN_Y + y * SCALE;
}

gs::FMPatch enginePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.5f;
    p.op[0] = {0.5f, 0.85f, 0.05f, 0.4f, 0.8f, 0.3f};
    p.op[1] = {1.f, 0.5f, 0.04f, 0.45f, 0.65f, 0.28f, 3.f};
    p.op[2] = {2.f, 0.2f, 0.03f, 0.35f, 0.4f, 0.24f};
    p.op[3] = {0.25f, 0.4f, 0.08f, 0.55f, 0.75f, 0.32f};
    p.vol = 0.14f;
    p.drive = 0.55f;
    p.tone = 720.f;
    p.vibRate = 6.5f;
    p.vibDepth = 0.02f;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.25f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.16f, 0.7f, 0.12f};
    p.op[1] = {2.f, 0.4f, 0.01f, 0.18f, 0.5f, 0.14f};
    p.op[2] = {3.f, 0.22f, 0.02f, 0.2f, 0.35f, 0.16f};
    p.op[3] = {1.f, 0.3f, 0.01f, 0.18f, 0.5f, 0.14f};
    p.vol = 0.18f;
    p.drive = 0.1f;
    return p;
}

gs::FMPatch clangPatch() {
    gs::FMPatch p;
    p.alg = 2;
    p.fb = 0.4f;
    p.op[0] = {2.f, 1.f, 0.005f, 0.08f, 0.2f, 0.12f};
    p.op[1] = {3.5f, 0.6f, 0.005f, 0.1f, 0.15f, 0.1f};
    p.op[2] = {5.f, 0.3f, 0.004f, 0.08f, 0.1f, 0.1f};
    p.op[3] = {1.f, 0.4f, 0.01f, 0.12f, 0.2f, 0.14f};
    p.vol = 0.2f;
    p.drive = 0.35f;
    p.tone = 1800.f;
    return p;
}

}  // namespace

int Game::live() const {
    int n = 0;
    for (const Rig& r : rigs_)
        if (r.alive) n++;
    return n;
}

int Game::youHp() const {
    if (rigs_.empty()) return 0;
    return int(std::lround(std::max(0.f, rigs_[0].hp)));
}

float Game::px() const { return rigs_.empty() ? 0.f : rigs_[0].x; }
float Game::py() const { return rigs_.empty() ? 0.f : rigs_[0].y; }

int Game::marker() const {
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    if (mode_ == Mode::Bout || mode_ == Mode::Pause) return 1;
    return 0;
}

int Game::rivalsAlive() const {
    int n = 0;
    for (size_t i = 1; i < rigs_.size(); i++)
        if (rigs_[i].alive) n++;
    return n;
}

int Game::frameOf(float hdg) const {
    float h = std::fmod(hdg, TAU);
    if (h < 0) h += TAU;
    return int(std::floor((h + PI / 8.f) / (PI / 4.f))) & 7;
}

void Game::place() {
    rigs_.clear();
    bits_.clear();
    for (const Spawn& s : SPAWNS) {
        Rig r;
        r.x = s.x;
        r.y = s.y;
        r.hp = r.maxHp = s.hp;
        r.pal = s.pal;
        r.player = s.player;
        r.alive = true;
        rigs_.push_back(r);
    }
    rigs_[0].hdg = 0.f;
    for (size_t i = 1; i < rigs_.size(); i++)
        rigs_[i].hdg = std::atan2(rigs_[0].y - rigs_[i].y, rigs_[0].x - rigs_[i].x);
    purseX_ = PURSE_X;
    purseY_ = PURSE_Y;
    purseLift_ = 0;
    banner_ = 0;
    shake_ = 0;
    over_ = false;
    won_ = false;
    why_ = "running";
}

void Game::begin() {
    place();
    mode_ = Mode::Bout;
    why_ = "running";
    t_ = 0;
    sys_->apu.tone(0, 520.f, 0.05f);
    beep_ = 0.06f;
}

void Game::puff(float x, float y, bool spark) {
    if (bits_.size() > 48) bits_.erase(bits_.begin());
    Bit b;
    b.x = x;
    b.y = y;
    b.life = spark ? 0.22f : 0.7f;
    b.h = spark ? 12.f : 18.f;
    b.spark = spark;
    bits_.push_back(b);
}

void Game::burst(float x, float y) {
    for (int i = 0; i < 5; i++) {
        float a = i * 1.256f;
        puff(x + std::cos(a) * 1.1f, y + std::sin(a) * 1.1f, true);
    }
    puff(x, y, false);
}

void Game::fanfare() {
    fanStep_ = 0;
    fanT_ = 0;
}

void Game::clampWall(Rig& r) {
    const float m = RAD;
    float vx = std::cos(r.hdg) * r.spd;
    float vy = std::sin(r.hdg) * r.spd;
    if (r.x < m) {
        r.x = m;
        if (vx < 0) vx = -vx * 0.22f;
    } else if (r.x > ARENA_W - m) {
        r.x = ARENA_W - m;
        if (vx > 0) vx = -vx * 0.22f;
    }
    if (r.y < m) {
        r.y = m;
        if (vy < 0) vy = -vy * 0.22f;
    } else if (r.y > ARENA_H - m) {
        r.y = ARENA_H - m;
        if (vy > 0) vy = -vy * 0.22f;
    }
    float fwd = std::cos(r.hdg) * vx + std::sin(r.hdg) * vy;
    float mag = std::hypot(vx, vy);
    if (mag < 0.12f) r.spd = 0;
    else r.spd = fwd < 0 ? -mag : mag;
}

void Game::shovePile(Rig& r) {
    if (!r.alive) return;
    for (const Prop& p : PROPS) {
        if (p.r <= 0) continue;
        float dx = r.x - p.x;
        float dy = r.y - p.y;
        float d = std::hypot(dx, dy);
        float need = RAD + p.r;
        if (d >= need) continue;
        if (d < 1e-3f) {
            dx = 1;
            dy = 0;
            d = 1;
        }
        float nx = dx / d, ny = dy / d;
        r.x = p.x + nx * (need + 0.05f);
        r.y = p.y + ny * (need + 0.05f);
        float vn = (std::cos(r.hdg) * nx + std::sin(r.hdg) * ny) * r.spd;
        if (vn < 0) r.spd *= 0.4f;
    }
}

template <class RigT>
float pileAvoid(const RigT& r, float distScale) {
    float steer = 0;
    float fx = std::cos(r.hdg), fy = std::sin(r.hdg);
    int n = 0;
    for (const Prop& p : PROPS) {
        if (p.r <= 0) continue;
        float dx = p.x - r.x, dy = p.y - r.y;
        float d = std::hypot(dx, dy);
        float reach = p.r + RAD + 4.4f;
        n++;
        if (d < 1e-3f || d > reach) continue;
        float dot = (dx * fx + dy * fy) / d;
        if (dot < 0.12f) continue;
        float cross = fx * dy - fy * dx;
        if (std::fabs(cross) < 0.18f * d) cross = (n & 1) ? d : -d;
        float urgency = (reach - d) / reach;
        steer += (cross > 0 ? -1.f : 1.f) * (0.35f + 1.5f * urgency * urgency);
    }
    return steer * distScale;
}

template <class RigT>
float wallAim(const RigT& r, float want) {
    const float margin = 6.2f;
    float ax = 0, ay = 0;
    if (r.x < margin) ax += margin - r.x;
    if (r.x > ARENA_W - margin) ax -= r.x - (ARENA_W - margin);
    if (r.y < margin) ay += margin - r.y;
    if (r.y > ARENA_H - margin) ay -= r.y - (ARENA_H - margin);
    float mag = std::hypot(ax, ay);
    if (mag < 0.25f) return want;
    return mixAngle(want, std::atan2(ay, ax), std::clamp(mag / 5.f, 0.f, 1.f));
}

void Game::rivalDrive(Rig& r, int index) {
    r.boost = false;
    if (!r.alive || rigs_.empty() || !rigs_[0].alive) {
        r.throttle = 0;
        r.steer = 0;
        return;
    }
    float side = (index - 2) * 5.5f;
    float dx = rigs_[0].x - r.x;
    float dy = rigs_[0].y - r.y;
    float d = std::hypot(dx, dy) + 0.001f;
    float tx = rigs_[0].x + (-dy / d) * side;
    float ty = rigs_[0].y + (dx / d) * side;
    float want = wallAim(r, std::atan2(ty - r.y, tx - r.x));
    float diff = wrap(want - r.hdg);
    float avoid = pileAvoid(r, d < 7.f ? 0.25f : 1.f);
    r.steer = std::clamp(diff / 0.7f + avoid, -1.f, 1.f);
    r.throttle = std::fabs(diff) > 1.5f ? 0.35f : 0.85f;
}

void Game::botDrive() {
    Rig& r = rigs_[0];
    if (!r.alive) return;
    int tgt = -1;
    float best = 1e9f;
    for (int i = 1; i < 4; i++) {
        if (!rigs_[i].alive) continue;
        float d = std::hypot(rigs_[i].x - r.x, rigs_[i].y - r.y);
        if (d < best) {
            best = d;
            tgt = i;
        }
    }
    float tx = PURSE_X, ty = PURSE_Y;
    if (tgt >= 0) {
        const Rig& o = rigs_[tgt];
        float lead = std::clamp(best / 16.f, 0.f, 0.8f);
        tx = o.x + std::cos(o.hdg) * o.spd * lead;
        ty = o.y + std::sin(o.hdg) * o.spd * lead;
    }
    tx = std::clamp(tx, 5.f, ARENA_W - 5.f);
    ty = std::clamp(ty, 5.f, ARENA_H - 5.f);
    float want = wallAim(r, std::atan2(ty - r.y, tx - r.x));
    float diff = wrap(want - r.hdg);
    float avoid = pileAvoid(r, (tgt < 0 || best < 6.5f) ? 0.2f : 1.f);
    if (std::fabs(r.spd) < 0.75f) r.stuck += DT;
    else r.stuck = 0;
    if (r.stuck > 0.26f) {
        r.throttle = -1.f;
        r.steer = r.stuck > 0.55f ? -1.f : 1.f;
        r.boost = false;
        if (r.stuck > 0.85f) r.stuck = 0;
        return;
    }
    r.steer = std::clamp(diff / 0.42f + avoid, -1.f, 1.f);
    r.throttle = std::fabs(diff) > 1.35f ? 0.2f : 1.f;
    r.boost = std::fabs(diff) < 0.95f;
}

void Game::driveHuman(const gs::Pad& pad) {
    Rig& r = rigs_[0];
    float steer = 0, throttle = 0;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(pad.axisX) > 0.15f) steer = pad.axisX;
    if (pad.down(gs::BTN_UP)) throttle += 1.f;
    if (pad.down(gs::BTN_DOWN)) throttle -= 1.f;
    if (pad.accel > 0.1f) throttle = std::max(throttle, pad.accel);
    if (pad.brake > 0.1f) throttle = std::min(throttle, -pad.brake);
    r.steer = std::clamp(steer, -1.f, 1.f);
    r.throttle = std::clamp(throttle, -1.f, 1.f);
    r.boost = pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.down(gs::BTN_TURBO);
}

void Game::collide(int i, int j, bool hurt) {
    Rig& a = rigs_[i];
    Rig& b = rigs_[j];
    if (!a.alive || !b.alive) return;
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float d = std::hypot(dx, dy);
    const float need = RAD * 2.f;
    if (d >= need) return;
    if (d < 1e-3f) {
        dx = 1;
        dy = 0;
        d = 1;
    }
    float nx = dx / d, ny = dy / d;
    float push = (need - d) * 0.5f + 0.05f;
    a.x -= nx * push;
    a.y -= ny * push;
    b.x += nx * push;
    b.y += ny * push;
    float avx = std::cos(a.hdg) * a.spd, avy = std::sin(a.hdg) * a.spd;
    float bvx = std::cos(b.hdg) * b.spd, bvy = std::sin(b.hdg) * b.spd;
    float aInto = avx * nx + avy * ny;
    float bInto = -(bvx * nx + bvy * ny);
    if (!hurt || a.cool[j] > 0.f) return;
    if (aInto < 0.7f && bInto < 0.7f && std::fabs(a.spd) < 3.5f && std::fabs(b.spd) < 3.5f) return;
    auto mult = [](const Rig& r) { return r.player ? (r.boost ? 8.f : 6.f) : 2.2f; };
    float dmgB = std::max(0.f, aInto) * mult(a);
    float dmgA = std::max(0.f, bInto) * mult(b);
    if (a.player && std::fabs(a.spd) > 8.f) dmgB = std::max(dmgB, 36.f);
    else if (a.player && std::fabs(a.spd) > 3.5f) dmgB = std::max(dmgB, 14.f);
    if (b.player && std::fabs(b.spd) > 8.f) dmgA = std::max(dmgA, 36.f);
    else if (b.player && std::fabs(b.spd) > 3.5f) dmgA = std::max(dmgA, 14.f);
    if (dmgA <= 0.f && dmgB <= 0.f) return;
    a.hp -= dmgA;
    b.hp -= dmgB;
    a.cool[j] = b.cool[i] = 0.32f;
    a.flash = b.flash = 0.16f;
    if (aInto > 0.f) a.spd *= 0.45f;
    if (bInto > 0.f) b.spd *= 0.45f;
    puff((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, true);
    shake_ = 0.9f;
    sys_->apu.keyOn(2, 90.f + std::fabs(aInto + bInto) * 6.f, 0.16f);
    sys_->apu.noiseBurst(0.48f, 1100.f, 0.14f);
    if (a.player || b.player) sys_->rumble(0.35f, 0.7f, 80);
}

void Game::physics(float dt) {
    for (Rig& r : rigs_) {
        for (float& c : r.cool) c = std::max(0.f, c - dt);
        if (!r.alive) {
            r.spd *= std::max(0.f, 1.f - 3.f * dt);
            r.x += std::cos(r.hdg) * r.spd * dt;
            r.y += std::sin(r.hdg) * r.spd * dt;
            clampWall(r);
            continue;
        }
        float turn = r.steer * (r.player ? 4.6f : 1.15f) * dt;
        if (r.spd < -0.35f) turn = -turn;
        r.hdg = wrap(r.hdg + turn);
        float maxF = r.player ? (r.boost ? 15.f : 12.f) : 5.4f;
        float acc = r.player ? (r.boost ? 34.f : 18.f) : 7.f;
        if (r.throttle > 0.05f) r.spd = std::min(maxF, r.spd + acc * r.throttle * dt);
        else if (r.throttle < -0.05f) r.spd = std::max(-6.f, r.spd + acc * 0.7f * r.throttle * dt);
        else {
            float d = (r.player ? 7.f : 4.f) * dt;
            if (std::fabs(r.spd) <= d) r.spd = 0;
            else r.spd -= std::copysign(d, r.spd);
        }
        r.x += std::cos(r.hdg) * r.spd * dt;
        r.y += std::sin(r.hdg) * r.spd * dt;
    }
    for (int pass = 0; pass < 2; pass++) {
        for (Rig& r : rigs_) clampWall(r);
        for (Rig& r : rigs_) shovePile(r);
        for (int i = 0; i < 4; i++)
            for (int j = i + 1; j < 4; j++) collide(i, j, pass == 0);
    }
    for (Rig& r : rigs_) clampWall(r);
}

void Game::reap() {
    bool playerDied = false;
    for (Rig& r : rigs_) {
        if (!r.alive || r.hp > 0.f) continue;
        r.alive = false;
        r.hp = 0;
        r.spd *= 0.3f;
        r.boost = false;
        if (r.player) playerDied = true;
        burst(r.x, r.y);
        sys_->apu.noiseBurst(r.player ? 0.55f : 0.4f, r.player ? 320.f : 780.f, 0.26f);
        shake_ = 1.f;
    }
    if (mode_ != Mode::Bout) return;
    if (playerDied || !rigs_[0].alive) {
        mode_ = Mode::Lost;
        won_ = false;
        why_ = "engine dead";
        banner_ = 0;
        sys_->apu.setVol(0, 0);
        return;
    }
    float dx = rigs_[0].x - purseX_;
    float dy = rigs_[0].y - purseY_;
    if (rivalsAlive() == 0 && dx * dx + dy * dy < 5.6f * 5.6f) {
        mode_ = Mode::Won;
        won_ = true;
        why_ = "purse";
        banner_ = 0;
        fanfare();
        sys_->apu.noiseBurst(0.3f, 500.f, 0.2f);
    }
}

void Game::celebrate(float dt) {
    banner_ += dt;
    for (Rig& r : rigs_) {
        r.spd *= std::max(0.f, 1.f - 2.4f * dt);
        r.x += std::cos(r.hdg) * r.spd * dt;
        r.y += std::sin(r.hdg) * r.spd * dt;
        clampWall(r);
        r.flash = std::max(0.f, r.flash - dt);
    }
    if (won_) {
        float dx = rigs_[0].x - purseX_;
        float dy = rigs_[0].y - purseY_;
        purseX_ += dx * std::min(1.f, 2.8f * dt);
        purseY_ += dy * std::min(1.f, 2.8f * dt);
        purseLift_ = std::min(1.f, purseLift_ + dt * 1.6f);
    }
    if (banner_ > 0.45f) over_ = true;
}

void Game::update(float dt) {
    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        celebrate(dt);
        return;
    }
    if (bot_) botDrive();
    for (int i = 1; i < 4; i++) rivalDrive(rigs_[i], i);
    physics(dt);
    reap();
}

void Game::tickBits(float dt) {
    for (int i = int(bits_.size()) - 1; i >= 0; i--) {
        bits_[i].t += dt;
        if (bits_[i].t >= bits_[i].life) bits_.erase(bits_.begin() + i);
    }
    shake_ = std::max(0.f, shake_ - dt * 1.7f);
    for (Rig& r : rigs_) r.flash = std::max(0.f, r.flash - dt);
    int tick = int(t_ * 3.f);
    int prev = int((t_ - dt) * 3.f);
    if (tick != prev) {
        const Prop& p = PROPS[tick % int(sizeof PROPS / sizeof PROPS[0])];
        if (p.kind <= 1) puff(p.x, p.y - 0.6f, false);
    }
    if (mode_ == Mode::Bout && !rigs_.empty() && rigs_[0].alive && std::fabs(rigs_[0].spd) > 2.f) {
        int f = int(t_ * 60.f);
        if (f % 5 == 0) {
            const Rig& r = rigs_[0];
            puff(r.x - std::cos(r.hdg) * 2.4f, r.y - std::sin(r.hdg) * 2.4f, false);
        }
    }
}

void Game::audio() {
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    bool rolling = engineOn_ && mode_ != Mode::Pause && mode_ != Mode::Lost && !rigs_.empty() && rigs_[0].alive;
    if (rolling) {
        float spd = mode_ == Mode::Title ? 2.2f : std::fabs(rigs_[0].spd);
        bool boost = mode_ == Mode::Bout && rigs_[0].boost;
        float bur = 1.f + 0.035f * std::sin(t_ * 42.f);
        sys_->apu.setFreq(0, (40.f + spd * 3.6f + (boost ? 20.f : 0.f)) * bur);
        sys_->apu.setVol(0, mode_ == Mode::Bout ? 0.08f + spd * 0.005f : 0.045f);
    } else if (mode_ == Mode::Lost || mode_ == Mode::Pause) {
        sys_->apu.setVol(0, mode_ == Mode::Pause ? 0.03f : 0.f);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {349.2f, 440.f, 523.2f, 698.4f};
        fanT_ += DT;
        if (fanT_ > 0.11f) {
            if (fanStep_ < 4) sys_->apu.keyOn(1, notes[fanStep_], 0.2f);
            else sys_->apu.keyOff(1);
            fanStep_++;
            fanT_ = 0;
            if (fanStep_ > 7) fanStep_ = -1;
        }
    }
    if (mode_ == Mode::Won) sys_->setLight(48, 36, 8);
    else if (mode_ == Mode::Lost) sys_->setLight(48, 8, 4);
    else sys_->setLight(28, 14, 4);
}

void Game::readInput(gs::System& sys) {
    if (bot_) return;
    const gs::Pad& pad = sys.pad;
    const bool start = pad.pressed(gs::BTN_START);
    const bool back = pad.pressed(gs::BTN_MODE);
    if (mode_ == Mode::Title) {
        if (start) begin();
        else if (back) sys.quit();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Bout;
        else if (back) {
            mode_ = Mode::Title;
            place();
            why_ = "ready";
        }
        return;
    }
    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (start || back) {
            mode_ = Mode::Title;
            place();
            why_ = "ready";
        }
        return;
    }
    if (start) {
        mode_ = Mode::Pause;
        return;
    }
    if (pad.pressed(gs::BTN_B)) sys_->apu.keyOn(3, 174.f, 0.18f);
    driveHuman(pad);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.11f, 0.24f, 0.14f);
    sys.apu.setPatch(0, enginePatch());
    sys.apu.setPatch(1, hornPatch());
    sys.apu.setPatch(2, clangPatch());
    sys.apu.setPatch(3, hornPatch());
    sys.apu.keyOn(0, 46.f, 0.06f);
    engineOn_ = true;
    place();
    if (bot_) begin();
    else {
        mode_ = Mode::Title;
        why_ = "ready";
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += DT;
    readInput(sys);
    if (mode_ == Mode::Bout || mode_ == Mode::Won || mode_ == Mode::Lost) update(DT);
    if (mode_ != Mode::Pause) tickBits(DT);
    audio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0;
    for (unsigned char c : s) {
        if (c < 32 || c >= 128) {
            width += 12.f * scale;
            continue;
        }
        width += art_.glyph[c - 32].w * scale;
    }
    float cursor = x - width * 0.5f;
    for (unsigned char c : s) {
        if (c < 32 || c >= 128) {
            cursor += 12.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float w = g.w * scale;
        float h = g.h * scale;
        if (c > 32) spr(g, cursor + w * 0.5f, y, h, pal, false);
        cursor += w;
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

void Game::bar(int col, int row, float hp, float maxHp) {
    int n = std::clamp(int(std::lround(hp / std::max(1.f, maxHp) * 8.f)), 0, 8);
    int pal = n <= 2 ? PAL_ALERT : (n <= 4 ? PAL_PRIZE : PAL_OK);
    for (int i = 0; i < 8; i++) hud(col + i, row, i < n ? "=" : "-", i < n ? pal : PAL_HUD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float ox = 0, oy = 0;
    if (shake_ > 0.02f) {
        ox = std::sin(t_ * 95.f) * shake_ * 4.f;
        oy = std::cos(t_ * 77.f) * shake_ * 3.f;
    }

    if (mode_ == Mode::Title) {
        text("JUNKYARD", 160, 46, 1.15f, PAL_PRIZE);
    } else if (mode_ == Mode::Won) {
        text("PURSE IS YOURS", 160, 46, 0.85f, PAL_PRIZE);
    } else if (mode_ == Mode::Lost) {
        text("ENGINE DEAD", 160, 46, 1.f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 78, 1.f, PAL_HUD);
    }

    for (const Bit& b : bits_) {
        float k = b.life > 0 ? b.t / b.life : 1.f;
        float sx, sy;
        toScreen(b.x, b.y, sx, sy);
        spr(b.spark ? art_.spark : art_.smoke, sx + ox, sy + oy - k * 8.f, b.h * (0.7f + k), PAL_FX, false);
    }

    int order[4] = {0, 1, 2, 3};
    if (rigs_.size() == 4) {
        std::sort(order, order + 4, [&](int a, int b) { return rigs_[a].y > rigs_[b].y; });
        for (int id : order) {
            const Rig& r = rigs_[id];
            float sx, sy;
            toScreen(r.x, r.y, sx, sy);
            sx += ox;
            sy += oy;
            if (r.alive && r.flash > 0.f) spr(art_.spark, sx, sy, 18, PAL_FX, false);
            if (r.player && mode_ == Mode::Bout && t_ < 1.5f) spr(art_.spark, sx, sy - 18, 12, PAL_FX, false);
            if (r.alive && r.boost && r.spd > 1.f) {
                float bx = sx - std::cos(r.hdg) * 16.f;
                float by = sy - std::sin(r.hdg) * 16.f;
                spr(art_.flame, bx, by, 14, PAL_FX, false);
            }
            spr(art_.shadow, sx, sy + 8, 11, 0, true);
            if (!r.alive) {
                spr(art_.wreck, sx, sy, art_.wreck.h * 0.72f, PAL_SCRAP, false);
            } else {
                float bob = (mode_ == Mode::Title) ? std::sin(t_ * 1.6f + id) * 0.04f : 0.f;
                const gs::Mipped& car = art_.car[frameOf(r.hdg + bob)];
                spr(car, sx, sy, car.h * 0.82f, r.pal, false);
            }
        }
    }

    if (mode_ == Mode::Won) {
        float sx, sy;
        toScreen(purseX_, purseY_, sx, sy);
        float h = art_.purse.h * (0.85f + purseLift_ * 0.45f);
        spr(art_.purse, sx + ox, sy + oy - purseLift_ * 16.f, h, PAL_PURSE, false);
    }

    float ssx, ssy;
    toScreen(36.f, 5.4f, ssx, ssy);
    spr(art_.sign, ssx + ox, ssy + oy, art_.sign.h * 0.8f, PAL_SIGN, false);

    int propOrder[int(sizeof PROPS / sizeof PROPS[0])];
    const int nprop = int(sizeof PROPS / sizeof PROPS[0]);
    for (int i = 0; i < nprop; i++) propOrder[i] = i;
    std::sort(propOrder, propOrder + nprop, [](int a, int b) { return PROPS[a].y > PROPS[b].y; });
    for (int i = 0; i < nprop; i++) {
        const Prop& p = PROPS[propOrder[i]];
        float sx, sy;
        toScreen(p.x, p.y, sx, sy);
        sx += ox;
        sy += oy;
        if (p.kind <= 1) spr(art_.pile[p.kind], sx, sy, art_.pile[p.kind].h * 0.52f, PAL_SCRAP, false);
        else if (p.kind == 2) spr(art_.drum, sx, sy, art_.drum.h * 0.7f, PAL_SCRAP, false);
        else spr(art_.crane, sx, sy, art_.crane.h * 0.62f, PAL_SCRAP, false);
    }

    if (mode_ != Mode::Won) {
        float sx, sy;
        float bob = std::sin(t_ * 3.f) * 1.6f;
        toScreen(purseX_, purseY_, sx, sy);
        spr(art_.purse, sx + ox, sy + oy + bob, art_.purse.h * 0.85f, PAL_PURSE, false);
    }

    if (mode_ == Mode::Title) {
        hud(1, 0, "S3 YARD", PAL_HUD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
        hudC(24, "LAST MACHINE TAKES THE PURSE", PAL_PRIZE);
        hudC(25, "HIT THEM FAST", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "PRESS START", PAL_HUD);
        hudC(27, "ARROWS STEER  UP GAS  C BOOST", PAL_HUD);
    } else if (mode_ == Mode::Bout || mode_ == Mode::Pause) {
        hud(1, 0, "YOU", PAL_HUD);
        if (!rigs_.empty()) bar(5, 0, rigs_[0].hp, rigs_[0].maxHp);
        char buf[24];
        int left = rivalsAlive();
        std::snprintf(buf, sizeof buf, "LEFT %d", left);
        hud(31, 0, buf, left ? PAL_ALERT : PAL_OK);
        if (mode_ == Mode::Pause) {
            hudC(16, "START  RESUME", PAL_HUD);
            hudC(18, "ESC    TITLE", PAL_HUD);
        } else if (left == 0 && (int(t_ * 3.f) & 1)) {
            hudC(27, "GET THE PURSE", PAL_PRIZE);
        } else {
            hudC(27, "LAST MACHINE TAKES THE PURSE", PAL_HUD);
        }
    } else if (mode_ == Mode::Won) {
        hudC(0, "LAST MACHINE", PAL_OK);
        hudC(27, "START", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudC(0, "THE YARD KEEPS IT", PAL_ALERT);
        hudC(27, "START", PAL_HUD);
    }
}

}  // namespace yard
