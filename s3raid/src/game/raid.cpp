#include "game/raid.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace raid {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kHorizon = 96.f;
constexpr float kCamH = 1.08f;
constexpr float kFocal = 214.f;
constexpr float kYardAt = 470.f;
constexpr int kBlocks = 5;
constexpr int kMarks = 6;

struct BlockDef {
    float s, x;
};
struct MarkDef {
    float x, y;
    int kind;
};

const BlockDef kBlockDef[kBlocks] = {
    {96.f, -1.15f}, {172.f, 1.22f}, {248.f, -0.40f}, {324.f, 1.10f}, {400.f, -1.28f},
};
const MarkDef kMarkDef[kMarks] = {
    {-1.2f, 3.50f, 0}, {2.4f, 3.15f, 1}, {-4.2f, 2.55f, 0}, {4.3f, 2.55f, 1}, {-2.4f, -3.20f, 2}, {3.0f, -3.20f, 2},
};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float centerX(float s) {
    return 7.5f * std::sin(s * 0.0155f) + 2.4f * std::sin(s * 0.034f + 0.8f);
}

float centerDeriv(float s) {
    return 7.5f * 0.0155f * std::cos(s * 0.0155f) + 2.4f * 0.034f * std::cos(s * 0.034f + 0.8f);
}

float kappa(float s) {
    return -7.5f * 0.0155f * 0.0155f * std::sin(s * 0.0155f) - 2.4f * 0.034f * 0.034f * std::sin(s * 0.034f + 0.8f);
}

float roadHalf(float s) {
    if (s < kYardAt - 48.f) return 2.55f;
    if (s >= kYardAt) return 6.4f;
    float t = (s - (kYardAt - 48.f)) / 48.f;
    t = t * t * (3.f - 2.f * t);
    return 2.55f + t * 3.85f;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

int16_t q16(float v) {
    v = std::clamp(v, -4000.f, 4000.f);
    return int16_t(std::lround(v));
}

float shakeOf(float t, float mag) { return std::sin(t * 73.f) * mag; }

}  // namespace

int Game::markMask() const {
    int m = 0;
    for (int i = 0; i < kMarks; i++)
        if (marks_[i].live) m |= 1 << i;
    return m;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    stage_ = 0;
    demo_ = 24.f;
    speed_ = 0;
}

void Game::beginRide() {
    mode_ = Mode::Ride;
    stage_ = 0;
    over_ = false;
    won_ = false;
    score_ = 0;
    cleared_ = 0;
    hull_ = 4;
    fanStep_ = -1;
    playTime_ = 0;
    arriveT_ = 0;
    winT_ = 0;
    hurtT_ = 0;
    shake_ = 0;
    fireCd_ = 0;
    truckCd_ = 0;
    s_ = 12.f;
    x_ = 0;
    speed_ = 14.f;
    shownSteer_ = 0;
    px_ = 0;
    py_ = 0;
    heading_ = -kPi * 0.5f;
    for (int i = 0; i < kBlocks; i++) {
        blocks_[i].s = kBlockDef[i].s;
        blocks_[i].x = kBlockDef[i].x;
        blocks_[i].live = true;
    }
    for (auto& b : bullets_) b.life = 0;
    for (auto& p : puffs_) p.life = 0;
}

void Game::beginYard() {
    mode_ = Mode::Yard;
    stage_ = 1;
    px_ = 0;
    py_ = 4.15f;
    heading_ = -kPi * 0.5f;
    speed_ = 0;
    truckX_ = -5.2f;
    truckDir_ = 1.f;
    truckCd_ = 0;
    stuckT_ = 0;
    stuckX_ = px_;
    stuckY_ = py_;
    fireCd_ = 0.15f;
    for (int i = 0; i < kMarks; i++) {
        marks_[i].x = kMarkDef[i].x;
        marks_[i].y = kMarkDef[i].y;
        marks_[i].kind = kMarkDef[i].kind;
        marks_[i].live = true;
    }
    cleared_ = 0;
    for (auto& b : bullets_) b.life = 0;
}

void Game::victory() {
    if (mode_ == Mode::Win) return;
    mode_ = Mode::Win;
    int swift = std::max(0, 60 - int(playTime_));
    score_ += hull_ * 200 + swift * 12;
    winT_ = 0;
    fanStep_ = -1;
    speed_ = 0;
}

void Game::killRun() {
    mode_ = Mode::Dead;
    speed_ = 0;
    if (bot_) {
        over_ = true;
        won_ = false;
    }
}

bool Game::hurt() {
    if (hurtT_ > 0.f || mode_ == Mode::Dead || mode_ == Mode::Win) return false;
    hull_--;
    hurtT_ = 0.75f;
    shake_ = 6.5f;
    score_ = std::max(0, score_ - 100);
    if (sys_) {
        sys_->rumble(0.7f, 0.9f, 150);
        sys_->apu.noiseBurst(0.38f, 1100.f, 11.f);
        sys_->apu.keyOn(1, 96.f, 0.16f);
    }
    if (hull_ <= 0) {
        hull_ = 0;
        killRun();
    }
    return true;
}

void Game::spawnPuff(float a, float b, int space) {
    Puff& p = puffs_[puffWrite_ % 12];
    puffWrite_++;
    p.a = a;
    p.b = b;
    p.life = 0.42f;
    p.space = space;
}

void Game::clearMark(int i) {
    if (i < 0 || i >= kMarks || !marks_[i].live) return;
    marks_[i].live = false;
    cleared_++;
    score_ += marks_[i].kind == 2 ? 400 : 250;
    spawnPuff(marks_[i].x, marks_[i].y, 0);
    stuckT_ = 0;
    stuckX_ = px_;
    stuckY_ = py_;
    if (sys_) {
        static const float ping[] = {494.f, 587.f, 698.f, 830.f, 988.f, 1175.f};
        sys_->apu.keyOn(2, ping[std::min(cleared_ - 1, 5)], 0.15f);
        sys_->rumble(0.2f, 0.4f, 60);
    }
    if (cleared_ >= kMarks) victory();
}

void Game::shoot() {
    if (fireCd_ > 0.f) return;
    fireCd_ = 0.22f;
    float c = std::cos(heading_), s = std::sin(heading_);
    for (auto& b : bullets_) {
        if (b.life > 0.f) continue;
        b.x = px_ + c * 0.75f;
        b.y = py_ + s * 0.75f;
        b.vx = c * 17.f;
        b.vy = s * 17.f;
        b.life = 0.8f;
        if (sys_) sys_->apu.noiseBurst(0.12f, 3200.f, 28.f);
        return;
    }
}

int Game::pickMark() const {
    int best = -1;
    float bestD = 1e9f;
    for (int i = 0; i < kMarks; i++) {
        if (!marks_[i].live) continue;
        float gy = marks_[i].kind == 2 ? 1.72f : marks_[i].y;
        float dx = marks_[i].x - px_, dy = gy - py_;
        float d = std::sqrt(dx * dx + dy * dy);
        if (marks_[i].kind == 2) d += 4.f;
        if (d < bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

void Game::pilotRide(float& steer, float& drive, bool& brake) {
    drive = 1.f;
    brake = false;
    float look = 1e9f;
    float desired = 0.f;
    bool have = false;
    float lim = roadHalf(s_) - 0.55f;
    if (lim < 0.4f) lim = 0.4f;
    for (const auto& b : blocks_) {
        if (!b.live) continue;
        float ds = b.s - s_;
        if (ds < -1.4f || ds > 42.f) continue;
        if (ds >= look) continue;
        look = ds;
        float side = 1.48f;
        float leftX = std::clamp(b.x - side, -lim, lim);
        float rightX = std::clamp(b.x + side, -lim, lim);
        bool leftClear = std::fabs(leftX - b.x) > 1.08f;
        bool rightClear = std::fabs(rightX - b.x) > 1.08f;
        if (leftClear && rightClear) desired = (std::fabs(leftX - x_) <= std::fabs(rightX - x_)) ? leftX : rightX;
        else if (rightClear) desired = rightX;
        else desired = leftX;
        have = true;
    }
    if (!have) desired = 0.f;
    desired = std::clamp(desired, -lim, lim);
    steer = std::clamp((desired - x_) * 2.15f, -1.f, 1.f);
}

void Game::pilotYard(float& steer, float& drive, bool& brake, bool& fire) {
    fire = false;
    brake = false;
    drive = 0;
    steer = 0;
    int id = pickMark();
    if (id < 0) return;
    const Mark& m = marks_[id];
    bool far = m.kind == 2;
    float gx = m.x;
    float gy = far ? 1.72f : m.y;
    float dx = gx - px_, dy = gy - py_;
    float dd = std::sqrt(dx * dx + dy * dy);
    float aim = std::atan2(dy, dx);
    if (far && dd < 0.4f) aim = std::atan2(m.y - py_, m.x - px_);
    float err = wrap(aim - heading_);
    steer = std::clamp(err / 0.48f, -1.f, 1.f);
    if (far && dd < 0.26f) {
        brake = true;
        if (std::fabs(err) < 0.12f && std::fabs(speed_) < 1.4f && std::fabs(px_ - m.x) < 0.22f) fire = true;
        return;
    }
    if (std::fabs(err) > 0.8f) {
        brake = true;
        return;
    }
    if (!far && dd < 1.15f) {
        drive = 1.f;
        return;
    }
    drive = dd > 2.3f ? 1.f : (dd > 0.85f ? 0.6f : 0.4f);
}

void Game::controls(float& steer, float& drive, bool& brake, bool& fire) {
    steer = 0;
    drive = 0;
    brake = false;
    fire = false;
    if (bot_) {
        if (mode_ == Mode::Ride) pilotRide(steer, drive, brake);
        else if (mode_ == Mode::Yard) pilotYard(steer, drive, brake, fire);
        return;
    }
    const gs::Pad& pad = sys_->pad;
    steer = pad.axisX;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    steer = std::clamp(steer, -1.f, 1.f);
    bool gas = pad.down(gs::BTN_UP) || pad.down(gs::BTN_C) || pad.accel > 0.18f;
    brake = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B) || pad.brake > 0.18f;
    if (gas) drive = 1.f;
    if (mode_ == Mode::Yard) fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_TURBO);
    else fire = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO);
}

void Game::updateRide(float dt, float steer, float drive, bool brake, bool fire) {
    shownSteer_ = steer;
    if (fire && sys_) sys_->apu.noiseBurst(0.16f, 640.f, 16.f);
    float target = 14.f, resp = 2.1f;
    if (brake) {
        target = 0;
        resp = 5.2f;
    } else if (drive > 0.2f) {
        target = 25.f;
        resp = 2.7f;
    }
    speed_ += (target - speed_) * std::min(1.f, resp * dt);
    if (speed_ < 0.f) speed_ = 0.f;
    float push = -kappa(s_) * speed_ * speed_ * 0.32f;
    x_ += (steer * 9.2f + push) * dt;
    float lim = roadHalf(s_) - 0.30f;
    x_ = std::clamp(x_, -lim, lim);
    if (s_ > kYardAt - 20.f && s_ < kYardAt) {
        float gate = 1.85f;
        if (x_ > gate) x_ += (gate - x_) * std::min(1.f, dt * 5.f);
        if (x_ < -gate) x_ += (-gate - x_) * std::min(1.f, dt * 5.f);
    }
    s_ += speed_ * dt;
    if (s_ < 0.f) s_ = 0.f;
    for (auto& b : blocks_) {
        if (!b.live || speed_ < 2.f) continue;
        if (std::fabs(s_ - b.s) < 1.2f && std::fabs(x_ - b.x) < 0.95f) {
            b.live = false;
            spawnPuff(b.s, b.x, 1);
            if (hurt()) {
                speed_ = std::min(speed_, 9.f);
                s_ = std::max(0.f, b.s - 1.5f);
            }
            if (mode_ != Mode::Ride) return;
        }
    }
    if (s_ >= kYardAt - 5.f) {
        s_ = kYardAt - 6.f;
        mode_ = Mode::Arrive;
        arriveT_ = 0;
        speed_ = std::min(speed_, 16.f);
        if (sys_) sys_->apu.keyOn(0, 698.f, 0.14f);
    }
}

void Game::updateYard(float dt, float steer, float drive, bool brake, bool fire) {
    shownSteer_ = steer;
    heading_ = wrap(heading_ + steer * 2.9f * dt);
    float target = 0.f, resp = 3.2f;
    if (drive > 0.05f) {
        target = drive * 7.6f;
        resp = 6.2f;
    } else if (brake) {
        resp = 9.f;
        target = (!bot_ && std::fabs(speed_) < 1.1f) ? -2.6f : 0.f;
    }
    speed_ += (target - speed_) * std::min(1.f, resp * dt);
    px_ += std::cos(heading_) * speed_ * dt;
    py_ += std::sin(heading_) * speed_ * dt;
    const float bx = 6.25f, by = 4.15f;
    px_ = std::clamp(px_, -bx, bx);
    py_ = std::clamp(py_, -by, by);

    truckX_ += truckDir_ * 2.15f * dt;
    if (truckX_ > 5.3f) {
        truckX_ = 5.3f;
        truckDir_ = -1.f;
    } else if (truckX_ < -5.3f) {
        truckX_ = -5.3f;
        truckDir_ = 1.f;
    }
    if (truckCd_ > 0.f) truckCd_ -= dt;
    float nx = (px_ - truckX_) / 1.25f;
    float ny = (py_ - 0.f) / 0.72f;
    if (nx * nx + ny * ny < 1.f && truckCd_ <= 0.f) {
        truckCd_ = 1.15f;
        float ang = std::atan2(py_, px_ - truckX_);
        px_ += std::cos(ang) * 0.4f;
        py_ += std::sin(ang) * 0.4f;
        px_ = std::clamp(px_, -bx, bx);
        py_ = std::clamp(py_, -by, by);
        speed_ *= -0.25f;
        hurt();
        if (mode_ != Mode::Yard) return;
    }

    if (fire) shoot();
    for (auto& b : bullets_) {
        if (b.life <= 0.f) continue;
        b.life -= dt;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        if (std::fabs(b.x) > 8.f || std::fabs(b.y) > 6.f) b.life = 0;
        for (int i = 0; i < kMarks; i++) {
            if (!marks_[i].live) continue;
            float dx = b.x - marks_[i].x, dy = b.y - marks_[i].y;
            if (dx * dx + dy * dy < 0.72f * 0.72f) {
                b.life = 0;
                clearMark(i);
                break;
            }
        }
        if (mode_ != Mode::Yard) return;
    }
    for (int i = 0; i < kMarks; i++) {
        if (!marks_[i].live) continue;
        float dx = px_ - marks_[i].x, dy = py_ - marks_[i].y;
        if (dx * dx + dy * dy < 1.0f * 1.0f) {
            clearMark(i);
            speed_ *= 0.55f;
            if (mode_ != Mode::Yard) return;
        }
    }
    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 2.4f) {
            float moved = std::hypot(px_ - stuckX_, py_ - stuckY_);
            if (moved < 0.35f && cleared_ < kMarks) heading_ = wrap(heading_ + 1.25f);
            stuckT_ = 0;
            stuckX_ = px_;
            stuckY_ = py_;
        }
    }
}

void Game::audio() {
    if (!sys_) return;
    float spd = 0;
    float vol = 0;
    if (mode_ == Mode::Title) spd = 12.f;
    else if (mode_ == Mode::Ride || mode_ == Mode::Arrive || mode_ == Mode::Yard) spd = std::fabs(speed_);
    if (mode_ == Mode::Title || mode_ == Mode::Ride || mode_ == Mode::Arrive) vol = 0.04f + std::min(spd, 26.f) * 0.0021f;
    else if (mode_ == Mode::Yard && spd > 0.3f) vol = 0.028f + std::min(spd, 8.f) * 0.004f;
    float wob = 1.f + 0.028f * std::sin(t_ * 46.f);
    sys_->apu.tone(0, (44.f + spd * 3.4f) * wob, vol);
    sys_->apu.tone(1, (88.f + spd * 1.6f) * wob, vol * 0.32f);
    if ((mode_ == Mode::Ride || mode_ == Mode::Title) && spd > 8.f) sys_->apu.noise(0.011f, 4800.f + spd * 60.f, false);
    else sys_->apu.noise(0.f, 1000.f, false);
    if (mode_ == Mode::Win) {
        int step = int(winT_ * 7.f);
        if (step != fanStep_ && step >= 0 && step < 6) {
            fanStep_ = step;
            static const float n[] = {523.f, 659.f, 784.f, 1046.f, 784.f, 1318.f};
            sys_->apu.keyOn(0, n[step], 0.18f);
        }
    }
}

bool Game::project(float wx, float ws, float& sx, float& sy, float& scale, float& depth) const {
    float tx = centerDeriv(camS_);
    float fwdX = tx, fwdS = 1.f;
    float len = std::sqrt(fwdX * fwdX + fwdS * fwdS);
    fwdX /= len;
    fwdS /= len;
    float rightX = fwdS, rightS = -fwdX;
    float dx = wx - camX_, ds = ws - camS_;
    float lat = dx * rightX + ds * rightS;
    depth = dx * fwdX + ds * fwdS;
    if (depth < 0.85f || depth > 80.f) return false;
    scale = kFocal / depth;
    sx = 160.f + lat * scale + shakeOf(t_, shake_);
    sy = kHorizon + (kCamH * kFocal) / depth;
    return true;
}

void Game::addW(const gs::Mipped& m, float depth, float cx, float bottom, float h, int pal, int fog, bool flip, bool shadow) {
    if (h < 2.f || h > 230.f || m.h <= 0) return;
    if (bottom < kHorizon - 8.f) return;
    world_.push_back({depth, cx, bottom, h, &m, pal, fog, gs::SCREEN_H, flip, shadow});
}

void Game::blit(const gs::Mipped& m, float cx, float cy, float ph, int pal, bool flip, int fog, int clip, bool feet, bool shadow) {
    if (m.h <= 0 || m.w <= 0 || ph < 1.f || ph > 300.f) return;
    float w = ph * float(m.w) / float(m.h);
    if (w < 1.f || w > 420.f) return;
    float x = cx - w * 0.5f;
    float y = feet ? cy - ph : cy - ph * 0.5f;
    gs::Sprite s;
    s.x = q16(x);
    s.y = q16(y);
    s.w = q16(std::max(1.f, w));
    s.h = q16(std::max(1.f, ph));
    if (s.w < 1) s.w = 1;
    if (s.h < 1) s.h = 1;
    s.img = m.pick(ph);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    s.clipY = int16_t(std::clamp(clip, 0, gs::SCREEN_H));
    sys_->vdp.sprite(s);
}

int Game::topFrame() const {
    float a = heading_ + kPi * 0.5f;
    a = wrap(a);
    if (a < 0.f) a += kTau;
    int i = int(std::lround(a / kTau * 8.f)) & 7;
    return i;
}

void Game::drawRoad(float s, float x, float steer, float speed, bool title) {
    gs::VDP& v = sys_->vdp;
    camS_ = s;
    camX_ = centerX(s) + x;
    float tx = centerDeriv(s);
    float fwdX = tx, fwdS = 1.f;
    float len = std::sqrt(fwdX * fwdX + fwdS * fwdS);
    fwdX /= len;
    fwdS /= len;
    float rightX = fwdS, rightS = -fwdX;
    float sh = shakeOf(t_, shake_);
    const uint16_t skyTop = gs::rgb4(1, 1, 5);
    const uint16_t skyMid = gs::rgb4(7, 3, 8);
    const uint16_t skyHor = gs::rgb4(15, 8, 3);
    const int horizon = int(kHorizon);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.B.hscroll[y] = int16_t(std::lround(std::clamp(centerX(s) * 5.5f + x * 16.f, -70.f, 70.f)));
        v.B.vscroll[y] = 0;
        if (y < horizon) {
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = u < 0.55f ? lerpC(skyTop, skyMid, u / 0.55f) : lerpC(skyMid, skyHor, (u - 0.55f) / 0.45f);
            int haze = y > horizon - 10 ? (horizon - y) : 0;
            v.lineFog[y] = uint8_t(std::clamp(haze, 0, 6));
            continue;
        }
        float dy = float(y - horizon);
        if (dy < 1.f) dy = 1.f;
        float depth = (kCamH * kFocal) / dy;
        float ps = s + depth;
        float dx = centerX(ps) - camX_;
        float lat = dx * rightX + depth * rightS;
        float dep = dx * fwdX + depth * fwdS;
        if (dep < 0.35f) {
            v.lineBackdrop[y] = gs::rgb4(2, 3, 1);
            v.lineFog[y] = 0;
            continue;
        }
        float pix = kFocal / dep;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f + lat * pix + sh;
        r.hw = std::max(0.6f, roadHalf(ps) * pix);
        r.v = ps * 20.f;
        bool yard = ps > kYardAt - 50.f;
        r.pal = uint8_t(yard ? PAL_YARD : PAL_ROAD);
        r.style = uint8_t(yard ? gs::ROAD_RUTS : 0);
        r.band = uint8_t(int(std::floor(ps / 5.f)) & 1);
        r.left = 0;
        r.right = 0;
        int fog = depth > 20.f ? int((depth - 20.f) * 0.28f) : 0;
        v.lineFog[y] = uint8_t(std::clamp(fog, 0, 12));
        v.lineBackdrop[y] = gs::rgb4(2, 3, 1);
    }
    v.A.enabled = false;
    v.B.enabled = true;
    v.setFogColor(gs::rgb4(9, 5, 4));

    world_.clear();
    auto place = [&](const gs::Mipped& m, float ss, float xRel, float worldH, int pal, bool shadow) {
        float sx, sy, scale, depth;
        float wx = centerX(ss) + xRel;
        if (!project(wx, ss, sx, sy, scale, depth)) return;
        int fog = depth > 16.f ? std::min(13, int((depth - 16.f) * 0.4f)) : 0;
        addW(m, depth, sx, sy, worldH * scale, pal, fog, false, shadow);
    };
    for (int i = 0; i < 14; i++) {
        float ss = 40.f + i * 30.f;
        if (ss > kYardAt - 80.f) break;
        float side = (i & 1) ? 1.f : -1.f;
        place(art_.tree, ss, side * (roadHalf(ss) + 1.05f), 3.5f, PAL_TREE, true);
    }
    place(art_.shack, 210.f, roadHalf(210.f) + 2.5f, 2.3f, PAL_WOOD, true);
    place(art_.shack, 350.f, -(roadHalf(350.f) + 2.6f), 2.3f, PAL_WOOD, true);
    for (float ss = kYardAt - 62.f; ss <= kYardAt + 6.f; ss += 14.f) {
        place(art_.post, ss, roadHalf(ss) * 0.96f, 1.7f, PAL_STEEL, false);
        place(art_.post, ss, -roadHalf(ss) * 0.96f, 1.7f, PAL_STEEL, false);
    }
    place(art_.silo, kYardAt + 10.f, 4.6f, 5.4f, PAL_STEEL, true);
    for (const auto& b : blocks_) {
        if (!b.live) continue;
        place(art_.board, b.s, b.x, 1.15f, PAL_WOOD, true);
    }
    {
        float sx, sy, scale, depth;
        if (project(centerX(kYardAt), kYardAt, sx, sy, scale, depth) && depth > 3.4f) {
            float pixW = 5.7f * scale;
            float pixH = pixW * float(art_.arch.h) / float(std::max(1, int(art_.arch.w)));
            int fog = depth > 16.f ? std::min(13, int((depth - 16.f) * 0.4f)) : 0;
            addW(art_.arch, depth, sx, sy, pixH, PAL_STEEL, fog, false, false);
        }
    }
    for (const auto& p : puffs_) {
        if (p.life <= 0.f || p.space != 1) continue;
        float sx, sy, scale, depth;
        if (!project(centerX(p.a) + p.b, p.a, sx, sy, scale, depth)) continue;
        addW(art_.puff, depth - 0.2f, sx, sy - 8.f, 26.f * (p.life / 0.42f) + 8.f, PAL_FIRE, 0, false, false);
    }
    std::sort(world_.begin(), world_.end(), [](const Wspr& a, const Wspr& b) { return a.depth < b.depth; });

    if (title) blit(art_.logo, 160.f, 16.f, float(art_.logo.h), PAL_LOGO, false, 0, gs::SCREEN_H, false, false);
    int lean = 1;
    if (steer < -0.22f) lean = 0;
    else if (steer > 0.22f) lean = 2;
    float bob = std::sin(s * 0.72f) * (speed > 4.f ? 1.5f : 0.2f);
    float bikeX = 160.f + steer * 14.f + sh;
    float bikeY = 176.f + bob;
    if ((int(t_ * 9.f) & 1) == 0) blit(art_.lamp, bikeX, bikeY + 18.f, 8.f, PAL_FIRE, false, 0, gs::SCREEN_H, false, false);
    blit(art_.bike[lean], bikeX, bikeY, 94.f, PAL_BIKE, false, 0, gs::SCREEN_H, false, false);
    blit(art_.shadow, bikeX, 214.f, 16.f, PAL_BIKE, false, 0, gs::SCREEN_H, false, true);
    if (speed > 8.f) {
        float ph = t_ * 14.f;
        blit(art_.puff, bikeX - 18.f + std::sin(ph) * 3.f, 208.f, 14.f, PAL_WOOD, false, 0, gs::SCREEN_H, false, false);
        blit(art_.puff, bikeX + 16.f + std::cos(ph * 1.3f) * 3.f, 212.f, 11.f, PAL_WOOD, false, 0, gs::SCREEN_H, false, false);
    }
    for (const auto& w : world_) blit(*w.img, w.cx, w.cy, w.h, w.pal, w.flip, w.fog, w.clip, true, w.shadow);
    blit(art_.sun, 286.f - centerX(s) * 0.4f, 26.f, 26.f, PAL_FIRE, false, 0, horizon, false, false);
    blit(art_.stars, 160.f, 20.f, float(art_.stars.h), PAL_TEXT, false, 0, horizon, false, false);
}

void Game::drawYard() {
    gs::VDP& v = sys_->vdp;
    v.A.enabled = true;
    v.B.enabled = false;
    const uint16_t night = gs::rgb4(1, 2, 1);
    const uint16_t night2 = gs::rgb4(2, 3, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        v.lineBackdrop[y] = lerpC(night, night2, float(y) / float(gs::SCREEN_H - 1));
        v.A.hscroll[y] = 0;
        v.A.vscroll[y] = 0;
    }
    v.setFogColor(gs::rgb4(1, 1, 1));
    float sh = shakeOf(t_, shake_);
    auto at = [&](float wx, float wy, float& sx, float& sy) {
        sx = kYardCx + wx * kYardScale + sh;
        sy = kYardCy + wy * kYardScale;
    };
    // Earlier sprites sit on top, so the rider and shots go in first.
    for (const auto& b : bullets_) {
        if (b.life <= 0.f) continue;
        float sx, sy;
        at(b.x, b.y, sx, sy);
        blit(art_.bullet, sx, sy, 7.f, PAL_FIRE, false, 0, gs::SCREEN_H, false, false);
    }
    {
        float sx, sy;
        at(px_, py_, sx, sy);
        blit(art_.top[topFrame()], sx, sy, 32.f, PAL_BIKE, false, 0, gs::SCREEN_H, false, false);
        blit(art_.shadow, sx, sy + 8.f, 12.f, PAL_BIKE, false, 0, gs::SCREEN_H, false, true);
    }
    for (const auto& p : puffs_) {
        if (p.life <= 0.f || p.space != 0) continue;
        float sx, sy;
        at(p.a, p.b, sx, sy);
        blit(art_.puff, sx, sy, 18.f * (p.life / 0.42f) + 8.f, PAL_FIRE, false, 0, gs::SCREEN_H, false, false);
    }
    {
        float sx, sy;
        at(truckX_, 0.f, sx, sy);
        blit(art_.truck, sx, sy, 30.f, PAL_STEEL, truckDir_ < 0.f, 0, gs::SCREEN_H, false, false);
    }
    for (int i = 0; i < kMarks; i++) {
        if (!marks_[i].live) continue;
        float sx, sy;
        at(marks_[i].x, marks_[i].y, sx, sy);
        const gs::Mipped* m = &art_.crate;
        float h = 30.f;
        if (marks_[i].kind == 1) {
            m = &art_.drum;
            h = 28.f;
        } else if (marks_[i].kind == 2) {
            m = &art_.mast[(int(t_ * 5.f) & 1) ? 1 : 0];
            h = 40.f;
        }
        blit(*m, sx, sy, h, PAL_MARK, false, 0, gs::SCREEN_H, false, false);
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::drawHud() {
    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(4, "CLEAR THE YARD", PAL_AMBER);
        hudC(5, "ARROWS STEER", PAL_TEXT);
        hudC(6, "UP FAST   DOWN BRAKE", PAL_TEXT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(7, "ENTER STARTS", PAL_AMBER);
        else hudC(7, "Z FIRES IN THE YARD", PAL_GOOD);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(2, "HOLDING", PAL_AMBER);
        hudC(3, "ENTER CONTINUES", PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(2, "YARD CLEAR", PAL_GOOD);
        hudC(3, buf, PAL_AMBER);
        if (!bot_) hudC(5, "ENTER RIDES AGAIN", PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Dead) {
        hudC(2, "RIDE DOWN", PAL_ALERT);
        if (!bot_) hudC(4, "ENTER TRIES AGAIN", PAL_TEXT);
        return;
    }
    std::snprintf(buf, sizeof buf, "HULL %d", hull_);
    hud(1, 0, "S3 RAID", PAL_AMBER);
    hud(30, 0, buf, hull_ <= 1 ? PAL_ALERT : PAL_TEXT);
    if (mode_ == Mode::Arrive) {
        hudC(3, "THE YARD", PAL_ALERT);
        return;
    }
    if (stage_ == 0) {
        int dist = int(std::lround(std::max(0.f, kYardAt - s_)));
        std::snprintf(buf, sizeof buf, "SPD %d", int(std::lround(speed_)));
        hud(1, 1, buf, PAL_TEXT);
        std::snprintf(buf, sizeof buf, "YARD %dM", dist);
        hud(12, 1, buf, PAL_GOOD);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hud(28, 1, buf, PAL_AMBER);
        if (playTime_ < 5.f) hudC(3, "DODGE THE BOARDS", PAL_TEXT);
        return;
    }
    std::snprintf(buf, sizeof buf, "CLEAR %d/%d", cleared_, kMarks);
    hud(1, 1, buf, PAL_GOOD);
    std::snprintf(buf, sizeof buf, "SCORE %d", score_);
    hud(16, 1, buf, PAL_AMBER);
    int liveMast = 0;
    for (const auto& m : marks_)
        if (m.live && m.kind == 2) liveMast++;
    if (liveMast == 2 && cleared_ == 0) hudC(3, "RAM THE STACKS", PAL_TEXT);
    else if (liveMast > 0) hudC(3, "SHOOT THE MASTS", PAL_AMBER);
    else hudC(3, "SMASH THE REST", PAL_TEXT);
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    if (mode_ == Mode::Title) {
        float sway = std::sin(t_ * 0.75f) * 0.28f;
        drawRoad(demo_, sway, sway, 12.f, true);
    } else if (stage_ == 0) {
        drawRoad(s_, x_, shownSteer_, speed_, false);
    } else {
        drawYard();
    }
    drawHud();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    world_.reserve(96);
    buildArt(sys.vdp, art_);
    gs::FMPatch patch{};
    patch.alg = 7;
    patch.vol = 0.2f;
    patch.glide = 0.03f;
    patch.op[0].mul = 1.f;
    patch.op[0].level = 1.f;
    patch.op[0].ar = 0.004f;
    patch.op[0].dr = 0.11f;
    patch.op[0].sl = 0.12f;
    patch.op[0].rr = 0.16f;
    patch.op[1].level = 0;
    patch.op[2].level = 0;
    patch.op[3].level = 0;
    for (int i = 0; i < 3; i++) sys.apu.setPatch(i, patch);
    sys.apu.setEcho(0.12f, 0.22f, 0.1f);
    sys.apu.setMaster(0.85f);
    if (bot_) beginRide();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (hurtT_ > 0.f) hurtT_ -= kDt;
    if (fireCd_ > 0.f) fireCd_ -= kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 16.f);
    for (auto& p : puffs_)
        if (p.life > 0.f) p.life -= kDt;

    bool start = !bot_ && sys.pad.pressed(gs::BTN_START);
    bool began = false;
    if (mode_ == Mode::Title) {
        demo_ += 12.f * kDt;
        if (start) {
            beginRide();
            began = true;
        } else {
            draw();
            audio();
            return;
        }
    }
    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Dead) {
        if (start) showTitle();
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Win) {
        winT_ += kDt;
        if (winT_ > 0.45f) {
            won_ = true;
            over_ = true;
        }
        if (start) showTitle();
        draw();
        audio();
        return;
    }
    if (!began && start && (mode_ == Mode::Ride || mode_ == Mode::Arrive || mode_ == Mode::Yard)) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        audio();
        return;
    }

    if (mode_ == Mode::Arrive) {
        arriveT_ += kDt;
        playTime_ += kDt;
        x_ += (0.f - x_) * std::min(1.f, kDt * 3.f);
        speed_ += (0.f - speed_) * std::min(1.f, kDt * 2.f);
        if (arriveT_ > 1.05f) beginYard();
    } else if (mode_ == Mode::Ride || mode_ == Mode::Yard) {
        float steer = 0, drive = 0;
        bool brake = false, fire = false;
        controls(steer, drive, brake, fire);
        playTime_ += kDt;
        if (mode_ == Mode::Ride) updateRide(kDt, steer, drive, brake, fire);
        else updateYard(kDt, steer, drive, brake, fire);
    }
    draw();
    audio();
}

}  // namespace raid
