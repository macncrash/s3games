#include "game/heli.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace heli {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float GRAV = 88.f;
constexpr float FLOOR = 188.f;
constexpr float HY = 18.f;
constexpr float SHORE = 148.f;
constexpr float DISC_X = 214.f;
constexpr float DISC_HW = 34.f;
constexpr float CREW_TIME = 16.5f;

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

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(9, 12, 14));
    resetFlight();
    mode_ = Mode::Title;
}

void Game::resetFlight() {
    x_ = 46.f;
    y_ = 64.f;
    vx_ = vy_ = 0;
    hold_ = 0;
    t_ = 0;
    over_ = false;
    won_ = false;
    loseKind_ = 0;
    rivalX_ = 336.f;
    rivalY_ = 58.f;
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
    float thrust = stickX * 86.f;
    if (up) lift = GRAV * 1.7f;
    if (down) lift = 0;

    if (bot_) {
        float tx = DISC_X;
        float ty = 72.f;
        if (std::fabs(x_ - DISC_X) < 14.f && std::fabs(vx_) < 22.f) ty = FLOOR - HY + 2.f;
        float ux = std::clamp(2.4f * (tx - x_) - 3.4f * vx_, -100.f, 100.f);
        float uy = std::clamp(3.2f * (ty - y_) - 4.2f * vy_, -130.f, 150.f);
        thrust = ux;
        lift = std::clamp(GRAV - uy, 0.f, GRAV * 2.5f);
    }

    vx_ += thrust * dt;
    vy_ += (GRAV - lift) * dt;
    vx_ *= std::exp(-1.25f * dt);
    x_ += vx_ * dt;
    y_ += vy_ * dt;

    if (x_ < 22.f) {
        x_ = 22.f;
        vx_ = std::max(0.f, vx_);
    }
    if (x_ > 304.f) {
        x_ = 304.f;
        vx_ = std::min(0.f, vx_);
    }
    if (y_ < 16.f) {
        y_ = 16.f;
        vy_ = std::max(0.f, vy_);
    }

    // The other crew slides in from the right and sets down on the far grass.
    float crewU = std::clamp(t_ / CREW_TIME, 0.f, 1.f);
    rivalX_ = 336.f + (278.f - 336.f) * std::min(crewU * 1.35f, 1.f);
    if (crewU < 0.72f) rivalY_ = 58.f + std::sin(t_ * 1.7f) * 3.f;
    else rivalY_ = 58.f + (FLOOR - 16.f - 58.f) * ((crewU - 0.72f) / 0.28f);

    float feet = y_ + HY;
    bool onGrass = false;
    if (feet >= FLOOR) {
        if (x_ < SHORE) {
            loseKind_ = 1;
            mode_ = Mode::Lose;
            msgT_ = 1.6f;
            over_ = bot_;
            sys_->apu.tone(1, 80, 0.22f);
            return;
        }
        y_ = FLOOR - HY;
        if (vy_ > 90.f) {
            vy_ = -vy_ * 0.22f;
            sys_->apu.tone(1, 140, 0.08f);
        } else if (vy_ > 0) {
            vy_ = 0;
        }
        vx_ *= 0.78f;
        onGrass = std::fabs(vy_) < 8.f;
    }

    bool onDisc = onGrass && std::fabs(x_ - DISC_X) < DISC_HW;
    float speed = std::sqrt(vx_ * vx_ + vy_ * vy_);
    if (onDisc && speed < 8.f) hold_ += dt;
    else hold_ = 0;

    if (hold_ > 0.4f) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        msgT_ = 2.2f;
        sys_->apu.tone(1, 720, 0.16f);
        return;
    }

    if (t_ >= CREW_TIME) {
        loseKind_ = 2;
        mode_ = Mode::Lose;
        msgT_ = 1.8f;
        over_ = bot_;
        sys_->apu.tone(1, 110, 0.2f);
        return;
    }

    sys_->apu.tone(0, 42.f + std::fabs(lift - GRAV) * 0.08f, 0.045f);
    rotor_ = int(t_ * (20.f + std::fabs(lift - GRAV) * 0.05f)) % 3;
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = int(4 + 7 * u);
        int g = int(7 + 6 * u);
        int b = int(13 + 2 * std::min(u * 1.2f, 1.f));
        if (y > 170) {
            r = 3;
            g = 6;
            b = 10;
        }
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.clear();

    spr(art_.cloud, 90 + std::sin(t_ * 0.2f) * 8.f, 32, 20, PAL_WORLD);
    spr(art_.cloud, 210, 48, 14, PAL_WORLD);
    spr(art_.sock, 168, 150, 32, PAL_GRASS);

    for (int i = 0; i < 2; i++) spr(art_.water, 48.f + i * 70.f, 206, 26, PAL_WORLD);
    for (int i = 0; i < 3; i++) spr(art_.grass, 186.f + i * 62.f, 204, 32, PAL_GRASS);
    spr(art_.tree, 162, 158, 42, PAL_GRASS);
    spr(art_.tree, 300, 162, 36, PAL_GRASS);
    spr(art_.disc, DISC_X, FLOOR - 2.f, 16, PAL_GRASS);

    bool faceL = vx_ < -6.f;
    spr(art_.body, x_, y_, 42, PAL_SHIP, faceL);
    spr(art_.rotor[rotor_], x_ + (faceL ? -8.f : 8.f), y_ - 20.f, 16, PAL_SHIP, faceL);

    int rframe = int(t_ * 18.f) % 3;
    bool rivalDown = rivalY_ > FLOOR - 30.f;
    spr(art_.rival, rivalX_, rivalY_, 28, PAL_RIVAL, true);
    spr(art_.rotor[rframe], rivalX_ - 4.f, rivalY_ - 14.f, 10, PAL_RIVAL, true);
    (void)rivalDown;

    if (mode_ == Mode::Title) {
        text("S3 HELIGRASS", 70, 72, 2.0f, PAL_HUD);
        text("LAND ON THE GRASS", 64, 96, 1.3f, PAL_HUD);
        text("FULL STOP BEFORE THE CREW", 42, 114, 1.15f, PAL_HUD);
        text("ARROWS FLY   ENTER START", 50, 136, 1.1f, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        text("FULL STOP", 100, 40, 2.1f, PAL_HUD);
        text("ON THE GRASS", 92, 62, 1.4f, PAL_HUD);
    } else if (mode_ == Mode::Lose) {
        text(loseKind_ == 1 ? "IN THE DRINK" : "OTHER CREW", 78, 42, 1.7f, PAL_HUD);
    } else {
        int left = std::max(0, int(std::ceil(CREW_TIME - t_)));
        char buf[40];
        std::snprintf(buf, sizeof(buf), "CREW %d", left);
        text(buf, 8, 12, 1.3f, left <= 4 ? PAL_HUD : PAL_HUD);
        float speed = std::sqrt(vx_ * vx_ + vy_ * vy_);
        std::snprintf(buf, sizeof(buf), "SPD %d", int(std::lround(speed)));
        text(buf, 200, 12, 1.2f, PAL_HUD);
        bool lit = std::fabs(x_ - DISC_X) < DISC_HW && (y_ + HY) >= FLOOR - 1.f;
        text(lit ? "GRASS" : "OFF", 250, 28, 1.15f, PAL_HUD);
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
        if (msgT_ < 1.3f) sys.apu.tone(1, 0, 0);
        if (msgT_ <= 0) over_ = true;
    }
    if (mode_ != Mode::Fly) rotor_ = int(t_ * 14.f) % 3;
    draw();
}

}  // namespace heli
