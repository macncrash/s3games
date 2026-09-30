#include "game/heli.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace heli {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PPM = 18.f;
constexpr float GROUND_Y = 200.f;
constexpr float SHIP_X = 96.f;
constexpr float HELI_H = 0.72f;
constexpr float HELI_L = 1.35f;
constexpr float CRUISE = 15.f;
constexpr float MIN_SPD = 9.f;
constexpr float MAX_SPD = 18.f;

struct Shape {
    float alt;
    float rad;
};

Shape shapeOf(int kind) {
    switch (kind) {
    case 0: return {1.15f, 0.95f};  // low
    case 1: return {7.45f, 1.15f};  // high
    case 2: return {1.70f, 1.90f};  // tall
    default: return {6.35f, 1.55f}; // drop
    }
}

}  // namespace

float Game::screenY(float alt) const { return GROUND_Y - alt * PPM; }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    for (char ch : s) {
        if (ch < 32 || ch > 126) {
            x += 6.f * scale;
            continue;
        }
        const gs::Mipped& m = art_.glyph[ch - 32];
        float h = std::max(7.f * scale, 1.f);
        spr(m, x + h * 0.4f, y, h, pal);
        x += (ch == ' ' ? 5.f : 6.2f) * scale;
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildCourse();
    sys.vdp.setFogColor(gs::rgb4(8, 11, 14));
    sys.apu.setMaster(0.7f);
    won_ = false;
    over_ = false;
    meters_ = 0;
    t_ = 0;
    std::snprintf(result_, sizeof result_, "S3 HELIKILO  FAIL  unfinished");
    if (bot_) resetRun();
    else mode_ = Mode::Title;
}

void Game::buildCourse() {
    wheels_.clear();
    float z = 42.f;
    int n = 0;
    while (z < 970.f) {
        Kind k;
        int m = n % 8;
        if (m == 3) k = Kind::Tall;
        else if (m == 6) k = Kind::Drop;
        else if ((n % 2) == 0) k = Kind::Low;
        else k = Kind::High;
        wheels_.push_back({z, k});
        z += 32.f;
        n++;
    }
}

void Game::resetRun() {
    mode_ = Mode::Fly;
    odo_ = 0;
    speed_ = CRUISE;
    alt_ = 4.1f;
    valt_ = 0;
    raceT_ = 0;
    won_ = false;
    over_ = false;
    meters_ = 0;
}

void Game::finishWin() {
    mode_ = Mode::Win;
    won_ = true;
    meters_ = 1000;
    odo_ = 1000.f;
    std::snprintf(result_, sizeof result_,
                  "S3 HELIKILO  WIN  kilometer finished  1000 m  wheels untouched  (%.1f s)", raceT_);
    sys_->apu.tone(1, 660, 0.16f);
}

void Game::finishFail(const char* why) {
    mode_ = Mode::Fail;
    won_ = false;
    meters_ = std::min(999, int(odo_));
    std::snprintf(result_, sizeof result_, "S3 HELIKILO  FAIL  %s at %d m  (%.1f s)", why, meters_, raceT_);
    sys_->apu.noiseBurst(0.4f, 720.f, 0.24f);
    if (!sys_->headless) sys_->rumble(0.7f, 0.8f, 140);
}

void Game::collide() {
    if (alt_ <= HELI_H) {
        finishFail("wheels down");
        return;
    }
    for (const Wheel& w : wheels_) {
        float dz = w.z - odo_;
        if (std::fabs(dz) > HELI_L + 2.2f) continue;
        Shape s = shapeOf(int(w.kind));
        if (std::fabs(dz) < HELI_L + s.rad * 0.55f && std::fabs(alt_ - s.alt) < HELI_H + s.rad) {
            finishFail("touched a wheel");
            return;
        }
    }
}

void Game::flyLogic(float dt) {
    const gs::Pad& pad = sys_->pad;
    float climb = 0;
    float throttle = 0;
    if (bot_) {
        float target = 4.1f;
        const Wheel* next = nullptr;
        float best = 1e9f;
        for (const Wheel& w : wheels_) {
            float dz = w.z - odo_;
            if (dz < -2.6f || dz > 24.f) continue;
            if (dz < best) {
                best = dz;
                next = &w;
            }
        }
        if (next) {
            if (next->kind == Kind::Tall) target = 5.15f;
            else if (next->kind == Kind::Drop) target = 3.05f;
            else target = 4.1f;
        }
        float err = target - alt_;
        climb = std::clamp(err * 7.5f - valt_ * 3.4f, -9.f, 9.f);
        throttle = 0;
        speed_ = CRUISE;
    } else {
        if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C)) climb = 7.2f;
        if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B)) climb = -7.2f;
        if (pad.axisY > 0.3f) climb = pad.axisY * 7.2f;
        if (pad.axisY < -0.3f) climb = pad.axisY * 7.2f;
        if (pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_Z) || pad.accel > 0.3f) throttle = 4.5f;
        if (pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_X) || pad.brake > 0.3f) throttle = -6.f;
        if (std::fabs(pad.axisX) > 0.3f && throttle == 0) throttle = pad.axisX * 4.5f;
    }

    valt_ += climb * dt;
    valt_ *= std::exp(-1.6f * dt);
    alt_ += valt_ * dt;
    if (alt_ > 9.2f) {
        alt_ = 9.2f;
        valt_ = std::min(0.f, valt_);
    }
    if (alt_ < 0.2f) {
        alt_ = 0.2f;
        valt_ = std::max(0.f, valt_);
    }

    if (!bot_) {
        speed_ += throttle * dt;
        if (throttle == 0) speed_ += (CRUISE - speed_) * 0.4f * dt;
        speed_ = std::clamp(speed_, MIN_SPD, MAX_SPD);
    }
    odo_ += speed_ * dt;
    raceT_ += dt;
    meters_ = std::min(1000, int(odo_));

    collide();
    if (mode_ != Mode::Fly) return;
    if (odo_ >= 1000.f) finishWin();
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = int(4 + 7 * u);
        int g = int(7 + 6 * u);
        int b = int(13 + 2 * std::min(u * 1.2f, 1.f));
        if (y > 188) {
            r = 4;
            g = 7;
            b = 3;
        }
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.clear();

    float scroll = odo_ * PPM;
    spr(art_.sun, 280, 26, 26, PAL_WORLD);
    spr(art_.cloud, std::fmod(420.f - scroll * 0.15f, 400.f) - 20.f, 34, 20, PAL_FX);
    spr(art_.cloud, std::fmod(180.f - scroll * 0.1f, 420.f) - 10.f, 52, 14, PAL_FX);
    spr(art_.hill, std::fmod(80.f - scroll * 0.35f, 360.f), 176, 34, PAL_WORLD);
    spr(art_.hill, std::fmod(240.f - scroll * 0.35f, 360.f), 180, 26, PAL_WORLD);

    for (int i = -1; i < 4; i++) {
        float gx = std::fmod(-scroll, 140.f) + i * 140.f;
        spr(art_.ground, gx, 208, 32, PAL_WORLD);
    }

    for (const Wheel& w : wheels_) {
        float dz = w.z - odo_;
        float sx = SHIP_X + dz * PPM;
        if (sx < -40 || sx > gs::SCREEN_W + 40) continue;
        Shape s = shapeOf(int(w.kind));
        float sy = screenY(s.alt);
        float h = s.rad * 2.f * PPM;
        if (w.kind == Kind::High || w.kind == Kind::Drop) {
            spr(art_.cable, sx, sy - h * 0.5f - 18.f, 36, PAL_WHEEL);
        }
        spr(art_.wheel, sx, sy, h, PAL_WHEEL);
    }

    float bannerX = SHIP_X + (1000.f - odo_) * PPM;
    if (bannerX > -30 && bannerX < gs::SCREEN_W + 30) spr(art_.banner, bannerX, 120, 70, PAL_WORLD);

    float hy = screenY(alt_);
    spr(art_.body, SHIP_X, hy, 36, PAL_SHIP);
    spr(art_.rotor[rotor_], SHIP_X + 4.f, hy - 16.f, 12, PAL_SHIP);

    if (mode_ == Mode::Title) {
        text("S3 HELIKILO", 78, 70, 2.1f, PAL_HUD);
        text("FINISH THE KILOMETER", 58, 96, 1.25f, PAL_HUD);
        text("DO NOT TOUCH A WHEEL", 60, 114, 1.2f, PAL_HUD);
        text("ARROWS FLY   ENTER START", 46, 138, 1.05f, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        text("KILOMETER", 92, 40, 2.f, PAL_HUD);
        text("WHEELS UNTOUCHED", 68, 64, 1.3f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text("TOUCHED A WHEEL", 70, 42, 1.5f, PAL_HUD);
    } else {
        char buf[40];
        std::snprintf(buf, sizeof buf, "%d M", meters_);
        text(buf, 8, 12, 1.4f, PAL_HUD);
        text("1000", 262, 12, 1.2f, PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
    if (bot_ && mode_ == Mode::Title) start = true;

    if (mode_ == Mode::Title) {
        alt_ = 4.1f + std::sin(t_ * 1.6f) * 0.15f;
        if (start) resetRun();
    } else if (mode_ == Mode::Fly) {
        flyLogic(DT);
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && start) resetRun();
        over_ = true;
    }

    float rotorHz = 16.f + std::fabs(valt_) * 1.4f;
    rotor_ = int(t_ * rotorHz) % 3;
    if (mode_ == Mode::Fly) sys.apu.tone(0, 48.f + std::fabs(valt_) * 4.f, 0.045f);
    else sys.apu.tone(0, 0, 0);
    draw();
}

}  // namespace heli
