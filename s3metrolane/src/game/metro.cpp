#include "game/metro.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace metro {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FOCAL = 176.f;
constexpr float CAM_H = 1.22f;
constexpr float CLOCK0 = 40.f;
constexpr float END_Z = 448.f;

float smooth(float u) { return u * u * (3.f - 2.f * u); }

}  // namespace

float Game::laneAt(float z) const {
    float bend = std::sin(z * 0.017f) * 1.55f + std::sin(z * 0.041f + 0.6f) * 0.38f;
    float shift = 0.f;
    if (z > 180.f && z < 270.f) {
        float up = smooth(std::clamp((z - 180.f) / 20.f, 0.f, 1.f));
        float down = smooth(std::clamp((z - 236.f) / 20.f, 0.f, 1.f));
        shift = -1.35f * (up - down);
    }
    if (z > 340.f) {
        float u = smooth(std::clamp((z - 340.f) / 48.f, 0.f, 1.f));
        bend *= (1.f - u);
        shift *= (1.f - u);
    }
    return bend + shift;
}

float Game::halfAt(float z) const {
    float h = 1.34f;
    if (z > 120.f && z < 230.f) {
        float u = smooth(std::clamp((z - 120.f) / 26.f, 0.f, 1.f));
        float v = smooth(std::clamp((z - 190.f) / 26.f, 0.f, 1.f));
        h = 1.34f + (0.90f - 1.34f) * (u - v);
    }
    if (z > END_Z - 40.f) {
        float u = smooth(std::clamp((z - (END_Z - 40.f)) / 26.f, 0.f, 1.f));
        h = h + (1.12f - h) * u;
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
    speed_ = 8.f;
    steer_ = 0;
    result_[0] = 0;
}

Game::In Game::controls() const {
    In in{};
    if (bot_) {
        float look = z_ + std::clamp(speed_, 4.f, 16.f) * 0.52f;
        float err = x_ - laneAt(look);
        float want = std::clamp(-err * 4.6f, -6.2f, 6.2f);
        in.steer = std::clamp((want - vx_) * 0.74f, -1.f, 1.f);
        float target = z_ < END_Z - 55.f ? 14.6f : 11.2f;
        in.power = speed_ < target - 0.25f;
        in.brake = speed_ > target + 0.7f;
        return in;
    }
    const gs::Pad& p = sys_->pad;
    in.steer = p.axisX;
    if (p.down(gs::BTN_LEFT)) in.steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) in.steer += 1.f;
    in.steer = std::clamp(in.steer, -1.f, 1.f);
    in.power = p.down(gs::BTN_A) || p.accel > 0.2f;
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
                  "S3 METRO LANE  WIN  stayed in the lane to the end  %.0f m  clock %.1f s left  (%.1f s)", z_, clock_,
                  rideT_);
    sys_->apu.tone(0, 392.f, 0.18f);
    sys_->apu.tone(1, 523.f, 0.15f);
    sys_->apu.tone(2, 659.f, 0.12f);
}

void Game::finishFail(const char* why) {
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(result_, sizeof result_, "S3 METRO LANE  FAIL  %s  %.0f m  spd %.1f  clock %.1f  (%.1f s)", why, z_,
                  speed_, clock_, rideT_);
    sys_->apu.noiseBurst(0.38f, 420.f, 0.28f);
    if (!sys_->headless) sys_->rumble(0.7f, 0.35f, 150);
}

void Game::logic(float dt) {
    In in = controls();
    rideT_ += dt;
    clock_ -= dt;
    steer_ += (in.steer - steer_) * std::min(1.f, dt * 8.f);

    float acc = -speed_ * 0.22f;
    if (in.power) acc += 7.4f;
    if (in.brake) acc -= 12.f;
    if (!in.power && !in.brake) acc -= 0.7f;
    speed_ = std::clamp(speed_ + acc * dt, 0.f, 17.5f);

    float grip = 12.2f * (0.45f + 0.55f * std::clamp(speed_ / 12.f, 0.f, 1.f));
    vx_ += steer_ * grip * dt;
    vx_ *= std::exp(-3.4f * dt);
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
    float roll = std::clamp(speed_ / 17.f, 0.f, 1.f);
    sys_->apu.noise(0.03f + roll * 0.06f, 180.f + roll * 520.f, true);
    if (int(rideT_ * 3.f) != int((rideT_ - DT) * 3.f) && speed_ > 2.f) sys_->apu.tone(3, 90.f + speed_ * 4.f, 0.035f);
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

void Game::tunnel() {
    gs::VDP& v = sys_->vdp;
    hor_ = 86;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(float(y) / float(hor_), 0.f, 1.f);
        int r = int(1 + 3 * u);
        int g = int(1 + 2 * u);
        int b = int(2 + 4 * u);
        if (y > hor_) {
            r = 1;
            g = 1;
            b = 2;
        }
        v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        v.lineFog[y] = y < hor_ ? uint8_t((1.f - u) * 8.f) : 3;
    }
    v.setFogColor(gs::rgb4(4, 3, 2));
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    v.roadTime = int(t_ * 24.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& L = v.road[y];
        L.on = false;
        if (y <= hor_) continue;
        float dz = FOCAL * CAM_H / float(y - hor_);
        if (dz < 1.2f || dz > 150.f) continue;
        float wz = z_ + dz;
        float half = halfAt(wz);
        L.on = true;
        L.cx = 160.f + (laneAt(wz) - x_) * (FOCAL / dz);
        L.hw = FOCAL * half / dz;
        L.v = wz * 22.f;
        L.pal = PAL_ROAD;
        L.band = (int(std::floor(wz * 0.22f)) & 1) ? 1 : 0;
        L.style = 1;
        L.left = L.right = gs::GROUND_LAND;
    }
}

void Game::sprites() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();

    if (mode_ == Mode::Title) {
        image(art_.title, (320 - art_.title.w) * 0.5f, 22, PAL_TITLE);
        image(art_.sub, (320 - art_.sub.w) * 0.5f, 30.f + art_.title.h, PAL_TITLE);
        image(art_.rule, (320 - art_.rule.w) * 0.5f, 48.f + art_.title.h, PAL_TITLE);
    } else if (mode_ == Mode::Win) {
        image(art_.made, (320 - art_.made.w) * 0.5f, 14, PAL_TITLE);
    } else if (mode_ == Mode::Fail) {
        const gs::Image& im = why_ == 3 ? art_.out : art_.missed;
        image(im, (320 - im.w) * 0.5f, 14, PAL_ALERT);
    }

    int lean = std::clamp(int(std::lround(steer_ * 1.2f)) + 1, 0, 2);
    blit(art_.shadow, 160.f, 208.f, 10.f, PAL_CAR, 0);
    blit(art_.car[lean], 160.f + steer_ * 6.f, 200.f, 62.f, PAL_CAR, 0);

    auto place = [&](const gs::Mipped& img, float wz, float wx, float worldH, int pal, bool flip) {
        float dz = wz - z_;
        if (dz < 1.8f || dz > 130.f) return;
        float sc = FOCAL / dz;
        float sx = 160.f + (wx - x_) * sc;
        float sy = float(hor_) + CAM_H * sc;
        int fog = int(std::clamp((dz - 14.f) / 7.f, 0.f, 14.f));
        blit(img, sx, sy, worldH * sc, pal, fog, flip);
    };

    float step = 18.f;
    float z0 = std::ceil((z_ + 4.f) / step) * step;
    for (float zz = z0; zz < z_ + 120.f; zz += step) {
        float c = laneAt(zz);
        float h = halfAt(zz);
        place(art_.lamp, zz, c - h - 0.35f, 1.6f, PAL_LAMP, false);
        place(art_.lamp, zz + 9.f, c + h + 0.35f, 1.6f, PAL_LAMP, true);
        if (int(zz / step) % 2 == 0) {
            place(art_.pillar, zz, c - h - 2.2f, 4.2f, PAL_PILLAR, false);
            place(art_.pillar, zz + 9.f, c + h + 2.3f, 4.0f, PAL_PILLAR, true);
        }
    }
    place(art_.stop, END_Z, laneAt(END_Z), 4.2f, PAL_STOP, false);
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
        text(6, 22, "A POWER   B BRAKE", PAL_HUD);
        text(5, 24, "HOLD THE METRO LANE", PAL_HUD);
        text(9, 26, "START TO DEPART", PAL_HUD);
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
    if (mode_ == Mode::Win) text(4, 24, "LEG MADE  LANE HELD TO THE END", PAL_HUD);
    if (mode_ == Mode::Fail) {
        text(why_ == 3 ? 7 : 6, 24, why_ == 3 ? "LEFT THE LANE  LEG FAILED" : "MISSED THE END  LEG FAILED",
             PAL_ALERT);
        text(10, 26, "START TO RETRY", PAL_HUD);
    }
}

void Game::draw() {
    tunnel();
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

}  // namespace metro
