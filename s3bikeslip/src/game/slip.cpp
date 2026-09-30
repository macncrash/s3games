#include "game/slip.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace slip {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FOCAL = 168.f;
constexpr float CAM_H = 1.2f;
constexpr float CLOCK0 = 52.f;
constexpr float BERTH0 = 456.f;
constexpr float BERTH1 = 486.f;
constexpr float END_Z = 500.f;
constexpr float SLIP_AIM = 470.f;

gs::FMPatch chainPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.vol = 0.12f;
    p.fb = 0.15f;
    p.tone = 1800.f;
    for (int i = 0; i < 4; i++) {
        p.op[i].mul = i == 0 ? 1.f : 2.f;
        p.op[i].level = i == 0 ? 1.f : 0.35f;
        p.op[i].ar = 0.02f;
        p.op[i].dr = 0.25f;
        p.op[i].sl = 0.7f;
        p.op[i].rr = 0.2f;
    }
    return p;
}

}  // namespace

float Game::laneAt(float z) const {
    const float wave = std::sin(z * 0.021f) * 1.75f;
    if (z <= 400.f) return wave;
    float u = std::clamp((z - 400.f) / 32.f, 0.f, 1.f);
    float held = std::sin(400.f * 0.021f) * 1.75f;
    return held * (1.f - u);
}

float Game::halfAt(float z) const {
    if (z < 392.f) return 3.3f;
    if (z < 432.f) {
        float u = (z - 392.f) / 40.f;
        return 3.3f + (1.02f - 3.3f) * u;
    }
    return 1.02f;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setPatch(0, chainPatch());
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
    hold_ = 0;
    why_ = 0;
    rideT_ = 0;
    clock_ = CLOCK0;
    z_ = 0;
    x_ = laneAt(0);
    vx_ = 0;
    speed_ = 6.f;
    steer_ = 0;
    chainOn_ = false;
    result_[0] = 0;
}

Game::In Game::controls() const {
    In in{};
    if (bot_) {
        float look = z_ + std::clamp(speed_, 0.f, 18.f) * 0.42f;
        float err = x_ - laneAt(look);
        float want = std::clamp(-err * 3.1f, -7.f, 7.f);
        in.steer = std::clamp((want - vx_) * 0.55f, -1.f, 1.f);
        float target;
        if (z_ < 350.f) target = 16.5f;
        else if (z_ < 440.f) target = 9.f;
        else if (z_ < BERTH0) target = 4.2f;
        else if (z_ < BERTH1 && std::fabs(x_ - laneAt(z_)) < 0.62f) target = 0.f;
        else target = 2.4f;
        in.pedal = speed_ < target - 0.35f;
        in.brake = speed_ > target + 0.45f;
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
                  "S3 BIKE SLIP  WIN  berthed in the slip before the tide  %.0f m  tide %.1f s left  (%.1f s)",
                  z_, clock_, rideT_);
    sys_->apu.keyOff(0);
    chainOn_ = false;
    sys_->apu.tone(0, 523.f, 0.2f);
    sys_->apu.tone(1, 659.f, 0.16f);
    sys_->apu.tone(2, 784.f, 0.14f);
}

void Game::finishFail(const char* why) {
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(result_, sizeof result_, "S3 BIKE SLIP  FAIL  %s  %.0f m  spd %.1f  tide %.1f  (%.1f s)", why, z_,
                  speed_, clock_, rideT_);
    sys_->apu.keyOff(0);
    chainOn_ = false;
    sys_->apu.noiseBurst(0.4f, 700.f, 0.3f);
    if (!sys_->headless) sys_->rumble(0.7f, 0.5f, 160);
}

void Game::logic(float dt) {
    In in = controls();
    rideT_ += dt;
    clock_ -= dt;
    steer_ += (in.steer - steer_) * std::min(1.f, dt * 8.f);

    float acc = -speed_ * 0.35f;
    if (in.pedal) acc += 9.5f;
    if (in.brake) acc -= 16.f;
    if (!in.pedal && !in.brake) acc -= 1.4f;
    speed_ = std::clamp(speed_ + acc * dt, 0.f, 20.f);

    float grip = 12.f * (0.45f + 0.55f * std::clamp(speed_ / 12.f, 0.f, 1.f));
    vx_ += steer_ * grip * dt;
    vx_ *= std::exp(-2.4f * dt);
    x_ += vx_ * dt;
    z_ += speed_ * dt;

    float mid = laneAt(z_);
    float half = halfAt(z_);
    float err = x_ - mid;
    bool inBox = z_ >= BERTH0 && z_ <= BERTH1 && std::fabs(err) < 0.62f && speed_ < 2.6f;
    if (inBox) {
        if (++hold_ >= 18) {
            finishWin();
            return;
        }
    } else {
        hold_ = 0;
    }

    if (clock_ <= 0.f) {
        why_ = 2;
        finishFail("the tide turned");
        return;
    }
    if (std::fabs(err) > half + 0.18f) {
        why_ = 3;
        finishFail("left the pier");
        return;
    }
    if (z_ > END_Z) {
        why_ = 4;
        finishFail("missed the end");
    }
}

void Game::audio() {
    if (mode_ != Mode::Ride) return;
    float hz = 70.f + speed_ * 9.f;
    if (speed_ > 0.4f) {
        if (!chainOn_) {
            sys_->apu.keyOn(0, hz, 0.2f);
            chainOn_ = true;
        } else {
            sys_->apu.setFreq(0, hz);
        }
    } else if (chainOn_) {
        sys_->apu.keyOff(0);
        chainOn_ = false;
    }
    float wash = std::clamp(1.f - clock_ / CLOCK0, 0.f, 1.f);
    sys_->apu.noise(0.03f + wash * 0.05f, 400.f + wash * 900.f, false);
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
    float tide = mode_ == Mode::Ride || mode_ == Mode::Win || mode_ == Mode::Fail
                     ? 1.f - std::clamp(clock_ / CLOCK0, 0.f, 1.f)
                     : 0.15f;
    hor_ = 90 + int(tide * 10.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(float(y) / float(std::max(hor_, 1)), 0.f, 1.f);
        int r = int(3 + 6 * u - tide * 2);
        int g = int(5 + 5 * u - tide);
        int b = int(9 + 4 * u);
        if (y > hor_) {
            r = 1 + int(tide * 2);
            g = 4;
            b = 7 + int((1.f - tide) * 3);
        }
        v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        v.lineFog[y] = y < hor_ ? uint8_t((1.f - u) * 4.f + tide * 3.f) : uint8_t(2 + tide * 4.f);
    }
    v.setFogColor(gs::rgb4(4, 6 + int(tide * 2), 9));
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    v.roadTime = int(t_ * 40.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& L = v.road[y];
        L.on = false;
        if (y <= hor_) continue;
        float dz = FOCAL * CAM_H / float(y - hor_);
        if (dz < 1.4f || dz > 150.f) continue;
        float wz = z_ + dz;
        float half = halfAt(wz);
        float hw = FOCAL * half / dz;
        L.on = true;
        L.cx = 160.f + (laneAt(wz) - x_) * (FOCAL / dz);
        L.hw = hw;
        L.v = wz * 22.f;
        L.pal = PAL_ROAD;
        L.band = (int(std::floor(wz * 0.22f)) & 1) ? 1 : 0;
        L.style = 1;
        L.left = L.right = gs::GROUND_WATER;
    }
}

void Game::sprites() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();

    if (mode_ == Mode::Title) {
        image(art_.title, (320 - art_.title.w) * 0.5f, 28, PAL_TITLE);
        image(art_.sub, (320 - art_.sub.w) * 0.5f, 36.f + art_.title.h, PAL_TITLE);
        image(art_.rule, (320 - art_.rule.w) * 0.5f, 54.f + art_.title.h, PAL_TITLE);
    } else if (mode_ == Mode::Win) {
        image(art_.berthed, (320 - art_.berthed.w) * 0.5f, 18, PAL_TITLE);
    } else if (mode_ == Mode::Fail) {
        const gs::Image& im = why_ == 2 ? art_.tide : why_ == 3 ? art_.edged : art_.missed;
        image(im, (320 - im.w) * 0.5f, 18, PAL_ALERT);
    }

    int lean = std::clamp(int(std::lround(steer_ * 1.2f)) + 1, 0, 2);
    blit(art_.bike[lean], 160.f + steer_ * 10.f, 200.f, 58.f, PAL_BIKE, 0);
    blit(art_.shadow, 160.f, 206.f, 10.f, PAL_BIKE, 0);

    auto place = [&](const gs::Mipped& img, float wz, float wx, float worldH, int pal) {
        float dz = wz - z_;
        if (dz < 2.f || dz > 130.f) return;
        float sc = FOCAL / dz;
        float sx = 160.f + (wx - x_) * sc;
        float sy = float(hor_) + CAM_H * sc;
        int fog = int(std::clamp((dz - 18.f) / 8.f, 0.f, 14.f));
        blit(img, sx, sy, worldH * sc, pal, fog);
    };

    float step = 14.f;
    float z0 = std::ceil((z_ + 6.f) / step) * step;
    for (float zz = z0; zz < z_ + 120.f; zz += step) {
        float c = laneAt(zz);
        float h = halfAt(zz) + 0.45f;
        place(art_.post, zz, c - h, 2.4f, PAL_POST);
        place(art_.post, zz, c + h, 2.4f, PAL_POST);
    }
    place(art_.mouth, 430.f, laneAt(430.f), 3.2f, PAL_POST);
    place(art_.buoy, BERTH0, laneAt(BERTH0) - halfAt(BERTH0) - 0.2f, 1.5f, PAL_BUOY);
    place(art_.buoy, BERTH0, laneAt(BERTH0) + halfAt(BERTH0) + 0.2f, 1.5f, PAL_BUOY);
    place(art_.buoy, BERTH1, laneAt(BERTH1) - 0.9f, 1.3f, PAL_BUOY);
    place(art_.buoy, BERTH1, laneAt(BERTH1) + 0.9f, 1.3f, PAL_BUOY);
    (void)SLIP_AIM;
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
        text(8, 22, "A PEDAL   B BRAKE", PAL_HUD);
        text(7, 24, "STEER INTO THE SLIP", PAL_HUD);
        text(9, 26, "START TO RIDE", PAL_HUD);
        return;
    }
    int tide = std::max(0, int(std::ceil(clock_)));
    std::snprintf(buf, sizeof buf, "TIDE %02d", tide);
    text(1, 1, buf, tide < 10 ? PAL_ALERT : PAL_HUD);
    std::snprintf(buf, sizeof buf, "%03.0f M", z_);
    text(30, 1, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "SPD %02.0f", speed_);
    text(1, 26, buf, PAL_HUD);
    if (z_ >= BERTH0 && z_ <= BERTH1) text(14, 26, "SLIP", PAL_HUD);
    else if (z_ > 400.f) text(13, 26, "NARROW", PAL_ALERT);
    if (mode_ == Mode::Pause) text(16, 12, "PAUSE", PAL_HUD);
    if (mode_ == Mode::Win) text(6, 24, "LEG MADE  BEFORE THE TIDE", PAL_HUD);
    if (mode_ == Mode::Fail) {
        const char* line = why_ == 2 ? "TIDE TURNED  LEG FAILED" : why_ == 3 ? "OFF THE PIER  LEG FAILED"
                                                                              : "MISSED THE END  LEG FAILED";
        text(6, 24, line, PAL_ALERT);
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
        if (!bot_ && start) {
            mode_ = Mode::Pause;
            sys.apu.keyOff(0);
            chainOn_ = false;
        } else {
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

}  // namespace slip
