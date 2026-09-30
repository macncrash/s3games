#include "game/heli.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace heli {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float GRAV = 92.f;
constexpr float FLOOR = 186.f;
constexpr float BOX_L = 168.f;
constexpr float BOX_R = 292.f;
constexpr float BOX_TOP = 112.f;
constexpr float CX = (BOX_L + BOX_R) * 0.5f;
constexpr float HY = 16.f;
constexpr float HX = 28.f;

}  // namespace

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
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
    sys.vdp.setFogColor(gs::rgb4(8, 11, 14));
    resetFlight();
    mode_ = Mode::Title;
}

void Game::resetFlight() {
    x_ = 52.f;
    y_ = 78.f;
    vx_ = vy_ = 0;
    hold_ = 0;
    t_ = 0;
    over_ = false;
    won_ = false;
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    bool up = pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C);
    bool down = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B);
    bool left = pad.down(gs::BTN_LEFT);
    bool right = pad.down(gs::BTN_RIGHT);
    float stickX = pad.axisX;
    if (left) stickX = -1;
    if (right) stickX = 1;

    float lift = GRAV * 0.78f;
    float thrust = stickX * 78.f;
    if (up) lift = GRAV * 1.62f;
    if (down) lift = 0;

    if (bot_) {
        float tx = CX;
        float ty = 86.f;
        if (std::fabs(x_ - CX) < 18.f && std::fabs(vx_) < 28.f) ty = FLOOR - HY + 4.f;
        float ux = std::clamp(2.1f * (tx - x_) - 3.1f * vx_, -90.f, 90.f);
        float uy = std::clamp(3.0f * (ty - y_) - 4.0f * vy_, -120.f, 140.f);
        thrust = ux;
        lift = std::clamp(GRAV - uy, 0.f, GRAV * 2.4f);
    }

    vx_ += thrust * dt;
    vy_ += (GRAV - lift) * dt;
    vx_ *= std::exp(-1.15f * dt);
    x_ += vx_ * dt;
    y_ += vy_ * dt;

    if (x_ < 24.f) {
        x_ = 24.f;
        vx_ = std::max(0.f, vx_);
    }
    if (x_ > 300.f) {
        x_ = 300.f;
        vx_ = std::min(0.f, vx_);
    }
    if (y_ < 18.f) {
        y_ = 18.f;
        vy_ = std::max(0.f, vy_);
    }

    // The uprights of the box shove the ship back out. The floor catches it.
    float feet = y_ + HY;
    bool overMouth = x_ > BOX_L + HX * 0.35f && x_ < BOX_R - HX * 0.35f;
    if (feet > BOX_TOP && feet < FLOOR && !overMouth) {
        if (x_ < CX) {
            x_ = std::min(x_, BOX_L - 4.f);
            vx_ = std::min(0.f, vx_);
        } else {
            x_ = std::max(x_, BOX_R + 4.f);
            vx_ = std::max(0.f, vx_);
        }
    }

    bool onPad = false;
    if (feet >= FLOOR) {
        bool inside = x_ > BOX_L + 8.f && x_ < BOX_R - 8.f;
        if (!inside) {
            mode_ = Mode::Crash;
            msgT_ = 1.4f;
            sys_->apu.tone(1, 90, 0.2f);
            return;
        }
        y_ = FLOOR - HY;
        if (vy_ > 0) vy_ *= (vy_ > 70.f ? -0.25f : 0.f);
        vx_ *= 0.82f;
        onPad = std::fabs(vy_) < 18.f;
    }

    bool inBox = onPad && x_ > BOX_L + 10.f && x_ < BOX_R - 10.f;
    float speed = std::sqrt(vx_ * vx_ + vy_ * vy_);
    if (inBox && speed < 12.f) hold_ += dt;
    else hold_ = 0;

    if (hold_ > 0.45f) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        msgT_ = 2.f;
        sys_->apu.tone(1, 660, 0.15f);
    }

    float rotorHz = 18.f + std::fabs(lift - GRAV) * 0.04f;
    sys_->apu.tone(0, 46.f + std::fabs(vy_) * 0.15f, mode_ == Mode::Fly ? 0.05f : 0.f);
    rotor_ = int(t_ * rotorHz) % 3;
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = int(3 + (11 - 3) * u);
        int g = int(6 + (13 - 6) * u);
        int b = int(12 + (15 - 12) * std::min(u * 1.4f, 1.f));
        if (y > 168) {
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

    spr(art_.sun, 286, 28, 28, PAL_WORLD);
    spr(art_.cloud, 70 + std::sin(t_ * 0.15f) * 6.f, 36, 22, PAL_FX);
    spr(art_.cloud, 180, 52, 16, PAL_FX);
    spr(art_.hill, 48, 168, 36, PAL_WORLD);
    spr(art_.hill, 200, 172, 28, PAL_WORLD);
    for (int i = 0; i < 3; i++) spr(art_.ground, 40.f + i * 120.f, 204, 28, PAL_WORLD);

    // Box sits on the floor. Sprite origin is the bitmap centre.
    spr(art_.box, CX, FLOOR - 36.f, 78, PAL_BOX);

    float lean = std::clamp(vx_ * 0.004f, -0.15f, 0.15f);
    (void)lean;
    bool faceL = vx_ < -4.f;
    spr(art_.body, x_, y_, 40, PAL_SHIP, faceL);
    spr(art_.rotor[rotor_], x_ + (faceL ? -6.f : 6.f), y_ - 18.f, 14, PAL_SHIP, faceL);

    if (mode_ == Mode::Title) {
        text("S3 HELIBOX", 78, 78, 2.2f, PAL_HUD);
        text("STOP INSIDE THE BOX", 62, 102, 1.3f, PAL_HUD);
        text("ARROWS FLY   ENTER START", 48, 126, 1.1f, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        text("STOPPED", 108, 48, 2.2f, PAL_HUD);
    } else if (mode_ == Mode::Crash) {
        text("MISSED THE BOX", 78, 48, 1.6f, PAL_HUD);
    } else {
        float speed = std::sqrt(vx_ * vx_ + vy_ * vy_);
        char buf[48];
        std::snprintf(buf, sizeof(buf), "SPEED %d", int(std::lround(speed)));
        text(buf, 8, 12, 1.2f, PAL_HUD);
        bool lit = x_ > BOX_L + 8.f && x_ < BOX_R - 8.f && (y_ + HY) >= FLOOR - 2.f;
        text(lit ? "IN BOX" : "OUT", 230, 12, 1.2f, PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
    if (bot_ && mode_ == Mode::Title) start = true;

    if (mode_ == Mode::Title) {
        t_ += DT;
        if (start) {
            resetFlight();
            mode_ = Mode::Fly;
        }
    } else if (mode_ == Mode::Fly) {
        t_ += DT;
        update(DT);
    } else if (mode_ == Mode::Crash) {
        msgT_ -= DT;
        sys.apu.tone(0, 0, 0);
        if (msgT_ <= 0 || start) {
            if (bot_) {
                over_ = true;
            } else {
                resetFlight();
                mode_ = Mode::Fly;
            }
        }
    } else if (mode_ == Mode::Win) {
        msgT_ -= DT;
        t_ += DT;
        sys.apu.tone(0, 0, 0);
        if (msgT_ < 1.2f) sys.apu.tone(1, 0, 0);
    }
    rotor_ = int(t_ * 16.f) % 3;
    draw();
}

}  // namespace heli
