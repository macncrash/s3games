#include "game/heli.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace heli {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float GROUND = 186.f;
constexpr float PAD_X = 214.f;
constexpr float G = 46.f;
constexpr float LIFT = 92.f;
constexpr float HACC = 96.f;
constexpr float FEET = 16.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::resetCraft() {
    x_ = 46.f;
    y_ = 58.f;
    vx_ = 0;
    vy_ = 0;
    faceL_ = false;
    lift_ = false;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = bot_ ? Mode::Fly : Mode::Title;
    over_ = false;
    won_ = false;
    lives_ = 3;
    t_ = 0;
    boom_ = 0;
    fan_ = 0;
    fanStep_ = -1;
    resetCraft();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
}

void Game::pilot(bool& left, bool& right, bool& lift) {
    left = right = lift = false;
    if (bot_) {
        float dx = PAD_X - x_;
        float wantV = clampf(dx * 1.05f, -62.f, 62.f);
        left = vx_ > wantV + 2.5f;
        right = vx_ < wantV - 2.5f;
        float alt = GROUND - (y_ + FEET);
        float wantAlt = std::min(78.f, std::fabs(dx) * 0.72f + 16.f);
        float wantVy = clampf((alt - wantAlt) * 0.85f, -24.f, 16.f);
        if (std::fabs(dx) < 16.f && std::fabs(vx_) < 18.f) wantVy = clampf(alt * 0.28f, 2.f, 11.f);
        lift = vy_ > wantVy;
        return;
    }
    const gs::Pad& p = sys_->pad;
    float ax = p.axisX;
    if (p.down(gs::BTN_LEFT) || ax < -0.28f) left = true;
    if (p.down(gs::BTN_RIGHT) || ax > 0.28f) right = true;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_UP) || p.axisY > 0.28f || p.accel > 0.2f) lift = true;
}

void Game::physics(float dt, bool left, bool right, bool lift) {
    lift_ = lift;
    float wind = 16.f * std::sin(t_ * 0.55f);
    float ax = wind;
    if (left) ax -= HACC;
    if (right) ax += HACC;
    float ay = G;
    if (lift) ay -= LIFT;
    vx_ += ax * dt;
    vy_ += ay * dt;
    vx_ *= (1.f - 1.15f * dt);
    vy_ *= (1.f - 0.25f * dt);
    vx_ = clampf(vx_, -120.f, 120.f);
    vy_ = clampf(vy_, -90.f, 140.f);
    x_ += vx_ * dt;
    y_ += vy_ * dt;
    if (x_ < 18.f) {
        x_ = 18.f;
        vx_ = std::fabs(vx_) * 0.3f;
    }
    if (x_ > 302.f) {
        x_ = 302.f;
        vx_ = -std::fabs(vx_) * 0.3f;
    }
    if (y_ < 16.f) {
        y_ = 16.f;
        vy_ = std::max(0.f, vy_);
    }
    if (vx_ < -8.f) faceL_ = true;
    if (vx_ > 8.f) faceL_ = false;

    float feet = y_ + FEET;
    if (feet < GROUND) return;
    y_ = GROUND - FEET;
    bool onMark = std::fabs(x_ - PAD_X) < 26.f;
    bool gentle = vy_ < 38.f && vy_ > -8.f && std::fabs(vx_) < 30.f;
    if (onMark && gentle) {
        vx_ = 0;
        vy_ = 0;
        mode_ = Mode::Down;
        won_ = true;
        fanStep_ = 0;
        fan_ = 0;
        sys_->apu.noiseBurst(0.25f, 900.f, 0.2f);
        return;
    }
    mode_ = Mode::Boom;
    boom_ = 0.7f;
    lives_--;
    vx_ *= 0.2f;
    vy_ = -28.f;
    sys_->apu.noiseBurst(0.45f, 2400.f, 0.35f);
    if (lives_ <= 0) {
        over_ = true;
        won_ = false;
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float sc = h / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.w = int16_t(std::clamp(int(std::lround(m.w * sc)), 1, 400));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int band = y / 28;
        v.lineBackdrop[y] = gs::rgb4(3 + band / 3, 6 + band / 2, 12 - band / 2);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    spr(art_.sun, 286, 28, 26, PAL_SUN);
    float drift = std::fmod(t_ * 6.f, 360.f);
    spr(art_.cloud, 40.f + drift * 0.3f, 36, 18, PAL_CLOUD);
    spr(art_.cloud, 150.f + std::fmod(drift * 0.5f, 200.f) - 40.f, 58, 14, PAL_CLOUD);
    spr(art_.cloud, 240.f - drift * 0.2f, 24, 16, PAL_CLOUD);
    spr(art_.tree, 28, 166, 40, PAL_TREE);
    spr(art_.tree, 78, 170, 32, PAL_TREE);
    spr(art_.tree, 300, 164, 44, PAL_TREE);
    spr(art_.ground, 160, 204, 40, PAL_GROUND);
    spr(art_.mark, PAD_X, GROUND - 6, 20, PAL_MARK);

    float sock = std::sin(t_ * 0.55f);
    spr(art_.sock, 118, 150, 16, PAL_MARK, sock < 0);

    float hy = y_;
    if (mode_ == Mode::Title) hy = 78.f + std::sin(t_ * 2.2f) * 4.f;
    bool spin = (int(t_ * 30.f) & 1) != 0;
    spr(art_.heli, x_, hy, 34, PAL_HELI, faceL_);
    float rotorW = spin ? 70.f : 54.f;
    spr(art_.rotor, x_, hy - 16.f, spin ? 7.f : 5.f, PAL_HELI, faceL_);
    (void)rotorW;
    spr(art_.heli, x_, GROUND - 2.f, 10, PAL_HELI, false, true);

    char buf[48];
    std::snprintf(buf, sizeof(buf), "LIVES %d", lives_);
    hud(1, 1, buf, PAL_HUD);
    if (mode_ == Mode::Title) {
        hudC(8, "S3 HELIMARK", PAL_HUD);
        hudC(11, "SET DOWN ON THE MARK", PAL_HUD);
        hudC(14, "A LIFTS   ARROWS FLY", PAL_HUD);
        hudC(18, "PRESS START", PAL_HUD);
    } else if (mode_ == Mode::Down) {
        hudC(8, "ON THE MARK", PAL_HUD);
    } else if (mode_ == Mode::Boom && lives_ <= 0) {
        hudC(8, "HULL DOWN", PAL_HUD);
    } else if (mode_ == Mode::Fly) {
        hudC(1, "THE MARK", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& p = sys.pad;

    if (mode_ == Mode::Title) {
        x_ = 150.f;
        y_ = 78.f;
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) {
            mode_ = Mode::Fly;
            resetCraft();
        }
    } else if (mode_ == Mode::Fly) {
        bool left, right, lift;
        pilot(left, right, lift);
        physics(DT, left, right, lift);
    } else if (mode_ == Mode::Boom) {
        boom_ -= DT;
        y_ += vy_ * DT;
        vy_ += G * DT * 0.4f;
        if (y_ + FEET > GROUND) {
            y_ = GROUND - FEET;
            vy_ = 0;
        }
        if (boom_ <= 0.f && !over_) {
            mode_ = Mode::Fly;
            resetCraft();
        }
    } else if (mode_ == Mode::Down) {
        fan_ += DT;
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        int step = int(fan_ / 0.14f);
        if (step != fanStep_ && step < 4) {
            fanStep_ = step;
            sys.apu.tone(1, notes[step], 0.12f);
        }
        if (step >= 4) sys.apu.tone(1, 0, 0);
        if (fan_ > 1.1f && !over_) over_ = true;
        if (!bot_ && over_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) {
            over_ = false;
            won_ = false;
            lives_ = 3;
            mode_ = Mode::Fly;
            resetCraft();
        }
    }

    float hum = lift_ || mode_ == Mode::Title || mode_ == Mode::Fly ? 0.045f : 0.02f;
    float beat = 70.f + (lift_ ? 36.f : 0.f) + std::sin(t_ * 48.f) * 6.f;
    if (mode_ != Mode::Down) sys.apu.tone(0, beat, hum);
    else sys.apu.tone(0, 0, 0);

    draw();
}

}  // namespace heli
