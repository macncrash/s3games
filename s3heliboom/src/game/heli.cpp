#include "game/heli.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace heliboom {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float GRAV = 88.f;
constexpr float FLOOR = 204.f;
constexpr float DROP = 42.f;  // heli centre to the bottom of the drive
constexpr float DECK = 142.f;
constexpr float BOOM_L = 176.f;
constexpr float BOOM_R = 292.f;
constexpr float BOOM_X = 234.f;
constexpr float LIMIT = 22.f;

}  // namespace

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
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

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win) return 3;
    float bottom = y_ + DROP;
    bool over = x_ > BOOM_L + 10.f && x_ < BOOM_R - 10.f;
    if (over && bottom >= DECK - 2.f) return 2;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(8, 10, 12));
    resetFlight();
    mode_ = Mode::Title;
}

void Game::resetFlight() {
    x_ = 48.f;
    y_ = 58.f;
    vx_ = vy_ = 0;
    hold_ = 0;
    t_ = 0;
    race_ = 0;
    over_ = false;
    won_ = false;
    why_[0] = 0;
}

void Game::fail(const char* why) {
    std::snprintf(why_, sizeof(why_), "%s", why);
    mode_ = Mode::Lose;
    msgT_ = 1.7f;
    over_ = bot_;
    won_ = false;
    sys_->apu.tone(1, 90, 0.2f);
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

    float lift = GRAV * 0.72f;
    float thrust = stickX * 92.f;
    if (up) lift = GRAV * 1.75f;
    if (down) lift = 0;

    if (bot_) {
        float tx = BOOM_X;
        float ty = 64.f;
        if (std::fabs(x_ - BOOM_X) < 22.f && std::fabs(vx_) < 28.f) ty = DECK - DROP + 1.f;
        float ux = std::clamp(2.6f * (tx - x_) - 3.6f * vx_, -110.f, 110.f);
        float uy = std::clamp(3.0f * (ty - y_) - 4.0f * vy_, -140.f, 150.f);
        thrust = ux;
        lift = std::clamp(GRAV - uy, 0.f, GRAV * 2.4f);
    }

    vx_ += thrust * dt;
    vy_ += (GRAV - lift) * dt;
    vx_ *= std::exp(-1.35f * dt);
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

    float bottom = y_ + DROP;
    bool overBoom = x_ > BOOM_L + 12.f && x_ < BOOM_R - 12.f;
    bool onBoom = false;
    if (overBoom && bottom >= DECK && vy_ >= 0.f) {
        if (vy_ > 110.f) {
            fail("broke the boom");
            return;
        }
        y_ = DECK - DROP;
        vy_ = 0;
        vx_ *= 0.72f;
        onBoom = true;
        bottom = DECK;
    } else if (bottom >= FLOOR) {
        fail(x_ < BOOM_L ? "stopped short of the boom" : "the drive is not on the boom");
        return;
    }

    float speed = std::sqrt(vx_ * vx_ + vy_ * vy_);
    if (onBoom && speed < 12.f) hold_ += dt;
    else hold_ = 0;

    if (hold_ > 0.45f) {
        won_ = true;
        race_ = t_;
        mode_ = Mode::Win;
        msgT_ = 2.0f;
        sys_->apu.tone(1, 740, 0.16f);
        return;
    }

    if (t_ >= LIMIT) {
        fail("missed the boom");
        return;
    }

    sys_->apu.tone(0, 40.f + std::fabs(lift - GRAV) * 0.08f, 0.04f);
    rotor_ = int(t_ * (18.f + std::fabs(lift - GRAV) * 0.05f)) % 3;
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = int(5 + 4 * u);
        int g = int(8 + 4 * u);
        int b = int(12 + 2 * u);
        if (y > 186) {
            r = 2;
            g = 5;
            b = 9;
        }
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.clear();

    spr(art_.cloud, 70 + std::sin(t_ * 0.25f) * 6.f, 28, 18, PAL_WORLD);
    spr(art_.cloud, 200, 42, 14, PAL_WORLD);
    for (int i = 0; i < 4; i++) spr(art_.water, 40.f + i * 78.f, 212, 24, PAL_WORLD);

    spr(art_.post, BOOM_L + 8.f, 168, 52, PAL_BOOM);
    spr(art_.post, BOOM_R - 8.f, 168, 52, PAL_BOOM);
    for (int i = 0; i < 3; i++) spr(art_.girder, 196.f + i * 38.f, DECK + 6.f, 14, PAL_BOOM);
    spr(art_.pad, BOOM_X, DECK - 2.f, 12, PAL_BOOM);

    float driveY = y_ + DROP - 10.f;
    spr(art_.sling, x_, (y_ + 16.f + driveY) * 0.5f, std::max(8.f, driveY - (y_ + 16.f)), PAL_DRIVE);
    spr(art_.drive, x_, driveY, 22, PAL_DRIVE);

    bool faceL = vx_ < -6.f;
    spr(art_.body, x_, y_, 40, PAL_SHIP, faceL);
    spr(art_.rotor[rotor_ % 3], x_ + (faceL ? -6.f : 6.f), y_ - 18.f, 14, PAL_SHIP, faceL);

    if (mode_ == Mode::Title) {
        text("S3 HELI BOOM", 78, 64, 2.0f, PAL_HUD);
        text("DELIVER THE DRIVE", 72, 90, 1.35f, PAL_HUD);
        text("SET IT ON THE BOOM", 68, 108, 1.3f, PAL_HUD);
        text("ARROWS FLY   ENTER START", 48, 132, 1.1f, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        text("DELIVERED", 96, 36, 2.0f, PAL_HUD);
        text("THE DRIVE IS ON THE BOOM", 46, 60, 1.25f, PAL_HUD);
    } else if (mode_ == Mode::Lose) {
        text(why_[0] ? why_ : "MISSED", 36, 40, 1.35f, PAL_HUD);
    } else {
        int left = std::max(0, int(std::ceil(LIMIT - t_)));
        char buf[40];
        std::snprintf(buf, sizeof(buf), "TIME %d", left);
        text(buf, 8, 12, 1.25f, PAL_HUD);
        bool lit = marker() >= 2;
        text(lit ? "ON THE BOOM" : "CARRY THE DRIVE", 150, 12, 1.15f, PAL_HUD);
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
    } else if (mode_ == Mode::Lose) {
        msgT_ -= DT;
        sys.apu.tone(0, 0, 0);
        if (msgT_ <= 0 || start) {
            if (bot_) over_ = true;
            else {
                resetFlight();
                mode_ = Mode::Fly;
            }
        }
    } else if (mode_ == Mode::Win) {
        msgT_ -= DT;
        t_ += DT;
        sys.apu.tone(0, 0, 0);
        if (msgT_ < 1.2f) sys.apu.tone(1, 0, 0);
        if (msgT_ <= 0) over_ = true;
    }
    if (mode_ != Mode::Fly) rotor_ = int(t_ * 14.f) % 3;
    draw();
}

}  // namespace heliboom
