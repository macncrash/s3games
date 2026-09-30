#include "game/lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace lane {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FOCAL = 168.f;
constexpr float CAM_H = 1.15f;
constexpr float CLOCK0 = 38.f;
constexpr float END_Z = 430.f;

float smooth(float u) { return u * u * (3.f - 2.f * u); }

}  // namespace

float Game::laneAt(float z) const {
    float bend = std::sin(z * 0.020f) * 1.7f + std::sin(z * 0.047f) * 0.42f;
    float shift = 0.f;
    if (z > 210.f && z < 290.f) {
        float up = smooth(std::clamp((z - 210.f) / 22.f, 0.f, 1.f));
        float down = smooth(std::clamp((z - 262.f) / 22.f, 0.f, 1.f));
        shift = 1.45f * (up - down);
    }
    if (z > 330.f) {
        float u = smooth(std::clamp((z - 330.f) / 40.f, 0.f, 1.f));
        bend *= (1.f - u);
        shift *= (1.f - u);
    }
    return bend + shift;
}

float Game::halfAt(float z) const {
    float h = 1.28f;
    if (z > 140.f && z < 250.f) {
        float u = smooth(std::clamp((z - 140.f) / 28.f, 0.f, 1.f));
        float v = smooth(std::clamp((z - 210.f) / 28.f, 0.f, 1.f));
        h = 1.28f + (0.82f - 1.28f) * (u - v);
    }
    if (z > END_Z - 36.f) {
        float u = smooth(std::clamp((z - (END_Z - 36.f)) / 24.f, 0.f, 1.f));
        h = h + (1.05f - h) * u;
    }
    return h;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.75f);
    over_ = false;
    won_ = false;
    result_[0] = 0;
    if (bot_) resetRide();
    else mode_ = Mode::Title;
}

void Game::resetRide() {
    mode_ = Mode::Ride;
    over_ = false;
    won_ = false;
    why_ = 0;
    rideT_ = 0;
    clock_ = CLOCK0;
    z_ = 0;
    x_ = laneAt(0);
    vx_ = 0;
    speed_ = 7.f;
    steer_ = 0;
    result_[0] = 0;
}

Game::In Game::controls() const {
    In in{};
    if (bot_) {
        float look = z_ + std::clamp(speed_, 4.f, 16.f) * 0.48f;
        float err = x_ - laneAt(look);
        float want = std::clamp(-err * 4.4f, -6.5f, 6.5f);
        in.steer = std::clamp((want - vx_) * 0.72f, -1.f, 1.f);
        float target = z_ < END_Z - 50.f ? 15.2f : 11.5f;
        in.pedal = speed_ < target - 0.3f;
        in.brake = speed_ > target + 0.8f;
        return in;
    }
    const gs::Pad& p = sys_->pad;
    in.steer = p.axisX;
    if (p.down(gs::BTN_LEFT)) in.steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) in.steer += 1.f;
    in.steer = std::clamp(in.steer, -1.f, 1.f);
    in.pedal = p.down(gs::BTN_A) || p.accel > 0.2f;
    in.brake = p.down(gs::BTN_B) || p.brake > 0.2f;
    in.start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_C);
    return in;
}

void Game::finishWin() {
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = 1;
    std::snprintf(result_, sizeof result_,
                  "S3 BIKE LANE  WIN  stayed in the lane to the end  %.0f m  clock %.1f s left  (%.1f s)", z_, clock_,
                  rideT_);
    sys_->apu.tone(0, 523.f, 0.18f);
    sys_->apu.tone(1, 659.f, 0.15f);
    sys_->apu.tone(2, 784.f, 0.12f);
}

void Game::finishFail(const char* why) {
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(result_, sizeof result_, "S3 BIKE LANE  FAIL  %s  %.0f m  spd %.1f  clock %.1f  (%.1f s)", why, z_,
                  speed_, clock_, rideT_);
    sys_->apu.noiseBurst(0.38f, 640.f, 0.28f);
    if (!sys_->headless) sys_->rumble(0.65f, 0.4f, 140);
}

void Game::logic(float dt) {
    In in = controls();
    rideT_ += dt;
    clock_ -= dt;
    steer_ += (in.steer - steer_) * std::min(1.f, dt * 9.f);

    float acc = -speed_ * 0.28f;
    if (in.pedal) acc += 8.6f;
    if (in.brake) acc -= 14.f;
    if (!in.pedal && !in.brake) acc -= 1.1f;
    speed_ = std::clamp(speed_ + acc * dt, 0.f, 18.5f);

    float grip = 13.5f * (0.4f + 0.6f * std::clamp(speed_ / 12.f, 0.f, 1.f));
    vx_ += steer_ * grip * dt;
    vx_ *= std::exp(-3.1f * dt);
    x_ += vx_ * dt;
    z_ += speed_ * dt;

    float err = x_ - laneAt(z_);
    float half = halfAt(z_);
    if (std::fabs(err) > half) {
        why_ = 3;
        finishFail("left the lane");
        return;
    }
    if (z_ >= END_Z) {
        finishWin();
        return;
    }
    if (clock_ <= 0.f) {
        why_ = 2;
        finishFail("missed the end");
    }
}

void Game::audio() {
    if (mode_ != Mode::Ride) return;
    float roll = std::clamp(speed_ / 18.f, 0.f, 1.f);
    sys_->apu.noise(0.02f + roll * 0.05f, 280.f + roll * 700.f, false);
    if (int(rideT_ * 2.f) != int((rideT_ - DT) * 2.f) && speed_ > 2.f) sys_->apu.tone(3, 180.f + speed_ * 6.f, 0.04f);
}

void Game::blit(const gs::Mipped& m, float x, float y, float h, int pal, int fog, bool flip) {
    if (m.w <= 0 || h < 2.f) return;
    const gs::Image& img = m.pick(h);
    float s = h / float(m.h);
    gs::Sprite sp;
    sp.img = img;
    sp.w = std::max(1, int(std::lround(m.w * s)));
    sp.h = std::max(1, int(std::lround(h)));
    sp.x = int(std::lround(x - sp.w * 0.5f));
    sp.y = int(std::lround(y - sp.h));
    sp.pal = uint8_t(pal);
    sp.fog = uint8_t(std::clamp(fog, 0, 16));
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::image(const gs::Image& img, float x, float y, int pal) {
    if (!img.w) return;
    gs::Sprite sp;
    sp.img = img;
    sp.x = int(std::lround(x));
    sp.y = int(std::lround(y));
    sp.w = img.w;
    sp.h = img.h;
    sp.pal = uint8_t(pal);
    sys_->vdp.sprite(sp);
}

void Game::skyRoad() {
    gs::VDP& v = sys_->vdp;
    hor_ = 92;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(float(y) / float(hor_), 0.f, 1.f);
        int r = int(4 + 7 * u);
        int g = int(7 + 5 * u);
        int b = int(11 + 3 * u);
        if (y > hor_) {
            r = 2;
            g = 5;
            b = 3;
        }
        v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        v.lineFog[y] = y < hor_ ? uint8_t((1.f - u) * 5.f) : 2;
    }
    v.setFogColor(gs::rgb4(7, 9, 11));
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    v.roadTime = int(t_ * 30.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& L = v.road[y];
        L.on = false;
        if (y <= hor_) continue;
        float dz = FOCAL * CAM_H / float(y - hor_);
        if (dz < 1.2f || dz > 140.f) continue;
        float wz = z_ + dz;
        float half = halfAt(wz);
        L.on = true;
        L.cx = 160.f + (laneAt(wz) - x_) * (FOCAL / dz);
        L.hw = FOCAL * half / dz;
        L.v = wz * 24.f;
        L.pal = PAL_ROAD;
        L.band = (int(std::floor(wz * 0.18f)) & 1) ? 1 : 0;
        L.style = 1;
        L.left = L.right = gs::GROUND_LAND;
    }
}

void Game::sprites() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();

    if (mode_ == Mode::Title) {
        image(art_.title, (320 - art_.title.w) * 0.5f, 26, PAL_TITLE);
        image(art_.sub, (320 - art_.sub.w) * 0.5f, 34.f + art_.title.h, PAL_TITLE);
        image(art_.rule, (320 - art_.rule.w) * 0.5f, 52.f + art_.title.h, PAL_TITLE);
    } else if (mode_ == Mode::Win) {
        image(art_.made, (320 - art_.made.w) * 0.5f, 16, PAL_TITLE);
    } else if (mode_ == Mode::Fail) {
        const gs::Image& im = why_ == 3 ? art_.out : art_.missed;
        image(im, (320 - im.w) * 0.5f, 16, PAL_ALERT);
    }

    int lean = std::clamp(int(std::lround(steer_ * 1.1f)) + 1, 0, 2);
    blit(art_.shadow, 160.f, 206.f, 9.f, PAL_BIKE, 0);
    blit(art_.bike[lean], 160.f + steer_ * 8.f, 198.f, 56.f, PAL_BIKE, 0);

    auto place = [&](const gs::Mipped& img, float wz, float wx, float worldH, int pal, bool flip) {
        float dz = wz - z_;
        if (dz < 1.8f || dz > 120.f) return;
        float sc = FOCAL / dz;
        float sx = 160.f + (wx - x_) * sc;
        float sy = float(hor_) + CAM_H * sc;
        int fog = int(std::clamp((dz - 16.f) / 8.f, 0.f, 14.f));
        blit(img, sx, sy, worldH * sc, pal, fog, flip);
    };

    float step = 16.f;
    float z0 = std::ceil((z_ + 4.f) / step) * step;
    for (float zz = z0; zz < z_ + 110.f; zz += step) {
        float c = laneAt(zz);
        float h = halfAt(zz);
        place(art_.cone, zz, c - h - 0.15f, 1.15f, PAL_CONE, false);
        place(art_.cone, zz, c + h + 0.15f, 1.15f, PAL_CONE, false);
        if (int(zz / step) % 3 == 0) {
            place(art_.tree, zz, c - h - 2.4f, 3.4f, PAL_TREE, false);
            place(art_.tree, zz + 8.f, c + h + 2.6f, 3.1f, PAL_TREE, true);
        }
    }
    place(art_.gate, END_Z, laneAt(END_Z), 3.6f, PAL_GATE, false);
}

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) h.set(x, y, 0);
    auto text = [&](int col, int row, const char* s, int pal) {
        for (int i = 0; s[i]; i++) {
            int x = col + i;
            if (x < 0 || x >= 40 || row < 0 || row >= 28) continue;
            unsigned char ch = (unsigned char)s[i];
            if (ch < 32 || ch > 126) ch = '?';
            int t = art_.font[ch];
            h.set(x, row, t ? gs::entry(t, pal) : 0);
        }
    };
    char buf[48];
    if (mode_ == Mode::Title) {
        text(7, 22, "A PEDAL   B BRAKE", PAL_HUD);
        text(6, 24, "HOLD THE PAINTED LANE", PAL_HUD);
        text(10, 26, "START TO RIDE", PAL_HUD);
        return;
    }
    int clock = std::max(0, int(std::ceil(clock_)));
    std::snprintf(buf, sizeof buf, "CLOCK %02d", clock);
    text(1, 1, buf, clock < 8 ? PAL_ALERT : PAL_HUD);
    std::snprintf(buf, sizeof buf, "%03.0f M", std::min(z_, END_Z));
    text(31, 1, buf, PAL_HUD);
    float err = std::fabs(x_ - laneAt(z_));
    float half = halfAt(z_);
    text(1, 26, err > half * 0.72f ? "EDGE" : "LANE", err > half * 0.72f ? PAL_ALERT : PAL_HUD);
    std::snprintf(buf, sizeof buf, "SPD %02.0f", speed_);
    text(30, 26, buf, PAL_HUD);
    if (mode_ == Mode::Pause) text(16, 12, "PAUSE", PAL_HUD);
    if (mode_ == Mode::Win) text(5, 24, "LEG MADE  LANE HELD TO THE END", PAL_HUD);
    if (mode_ == Mode::Fail) {
        text(why_ == 3 ? 7 : 6, 24, why_ == 3 ? "LEFT THE LANE  LEG FAILED" : "MISSED THE END  LEG FAILED",
             PAL_ALERT);
        text(10, 26, "START TO RETRY", PAL_HUD);
    }
}

void Game::draw() {
    skyRoad();
    sprites();
    hud();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const bool start = sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C);
    if (mode_ == Mode::Title) {
        if (bot_ || start || sys.pad.pressed(gs::BTN_A)) resetRide();
    } else if (mode_ == Mode::Ride) {
        if (!bot_ && start) mode_ = Mode::Pause;
        else {
            logic(DT);
            audio();
        }
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Ride;
    } else if ((mode_ == Mode::Win || mode_ == Mode::Fail) && start && !bot_) {
        resetRide();
        over_ = false;
        won_ = false;
    }
    draw();
}

}  // namespace lane
