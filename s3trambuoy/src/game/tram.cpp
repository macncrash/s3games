#include "game/tram.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace tram {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kZoom = 2.15f;
constexpr float kClock = 100.f;
constexpr float kCapture = 11.f;
constexpr float kDockR = 7.5f;
constexpr float kDockX = 0.f;
constexpr float kDockY = 4.f;

struct Mark {
    float x, y;
    int pal;
};

constexpr Mark kBuoys[3] = {
    {-46.f, 30.f, PAL_BUOY0},
    {-12.f, 74.f, PAL_BUOY1},
    {44.f, 42.f, PAL_BUOY2},
};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

}  // namespace

void Game::begin() {
    x_ = 0.f;
    y_ = 8.f;
    heading_ = kPi * 0.5f;
    speed_ = 0.f;
    next_ = 0;
    hold_ = 0.f;
    clock_ = kClock;
    t_ = 0.f;
    bell_ = 0.f;
    wakeT_ = 0.f;
    wakeN_ = 0;
    over_ = false;
    won_ = false;
    camX_ = x_;
    camY_ = y_;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(1, 5, 8));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.08f, 0.16f, 0.06f);
    begin();
    mode_ = bot_ ? Mode::Play : Mode::Title;
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.18f);
    bell_ = 0.16f;
}

void Game::controls(float& thrust, float& rudder) {
    const gs::Pad& p = sys_->pad;
    thrust = 0.f;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A)) thrust += 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) thrust -= 1.f;
    if (p.accel > 0.12f) thrust = p.accel;
    if (p.brake > 0.12f) thrust = -p.brake;
    thrust = std::clamp(thrust, -1.f, 1.f);
    rudder = 0.f;
    if (p.down(gs::BTN_LEFT)) rudder -= 1.f;
    if (p.down(gs::BTN_RIGHT)) rudder += 1.f;
    if (std::fabs(p.axisX) > 0.18f) rudder = std::clamp(p.axisX, -1.f, 1.f);
}

void Game::pilot(float& thrust, float& rudder) {
    float tx = kDockX, ty = kDockY;
    bool home = next_ >= 3;
    if (!home) {
        tx = kBuoys[next_].x;
        ty = kBuoys[next_].y;
    }
    float dx = tx - x_, dy = ty - y_;
    float dist = std::hypot(dx, dy);
    float wantH = std::atan2(dy, dx);
    float err = wrap(wantH - heading_);
    rudder = std::clamp(err * 2.1f, -1.f, 1.f);
    float want = home ? std::min(9.f, dist * 0.40f) : std::min(13.5f, std::max(4.2f, dist * 0.48f));
    if (home && dist < 12.f) want = dist * 0.26f;
    if (!home && dist < 18.f) want = std::max(3.4f, dist * 0.38f);
    if (home && dist < 3.2f) {
        thrust = speed_ > 0.35f ? -0.85f : 0.f;
        rudder *= 0.35f;
        return;
    }
    thrust = std::clamp((want - speed_) * 0.42f, -1.f, 1.f);
}

void Game::physics(float dt, float thrust, float rudder) {
    float turn = 2.55f * std::clamp(0.35f + std::fabs(speed_) / 10.f, 0.35f, 1.f);
    heading_ = wrap(heading_ + rudder * turn * dt);
    float drive = thrust >= 0.f ? thrust * 16.f : thrust * 11.f;
    speed_ += (drive - speed_ * 0.85f) * dt;
    speed_ = std::clamp(speed_, -3.5f, 15.f);
    x_ += std::cos(heading_) * speed_ * dt;
    y_ += std::sin(heading_) * speed_ * dt;
    if (x_ < -88.f) {
        x_ = -88.f;
        speed_ *= 0.4f;
    }
    if (x_ > 88.f) {
        x_ = 88.f;
        speed_ *= 0.4f;
    }
    if (y_ < -16.f) {
        y_ = -16.f;
        speed_ *= 0.4f;
    }
    if (y_ > 108.f) {
        y_ = 108.f;
        speed_ *= 0.4f;
    }
}

void Game::logic(float dt) {
    if (next_ < 3) {
        float d = std::hypot(x_ - kBuoys[next_].x, y_ - kBuoys[next_].y);
        if (d < kCapture) {
            next_++;
            blip(next_ >= 3 ? 660.f : 520.f + next_ * 70.f);
        }
    }
    bool inDock = next_ >= 3 && std::hypot(x_ - kDockX, y_ - kDockY) < kDockR && std::fabs(speed_) < 1.25f;
    if (inDock) hold_ += dt;
    else hold_ = 0.f;
    if (hold_ > 0.5f) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        blip(880.f);
    }
    clock_ -= dt;
    if (clock_ <= 0.f && mode_ == Mode::Play) {
        clock_ = 0.f;
        mode_ = Mode::Fail;
        over_ = true;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    t_ += dt;
    if (bell_ > 0.f) {
        bell_ -= dt;
        if (bell_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            begin();
            mode_ = Mode::Play;
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float thrust = 0, rudder = 0;
            if (bot_) pilot(thrust, rudder);
            else controls(thrust, rudder);
            physics(dt, thrust, rudder);
            logic(dt);
            wakeT_ += dt;
            if (wakeT_ > 0.07f && std::fabs(speed_) > 1.2f) {
                wakeT_ = 0.f;
                Wake& w = wakes_[wakeN_++ % 28];
                w.x = x_ - std::cos(heading_) * 6.f;
                w.y = y_ - std::sin(heading_) * 6.f;
                w.life = 1.f;
            }
            for (int i = 0; i < 28; i++) {
                if (wakes_[i].life > 0.f) wakes_[i].life -= dt * 0.7f;
            }
            float hum = std::fabs(speed_) > 0.5f ? 0.035f : 0.f;
            sys.apu.tone(0, 70.f + std::fabs(speed_) * 6.f, hum);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        begin();
        mode_ = Mode::Play;
    }
    float follow = mode_ == Mode::Title ? 0.04f : 0.12f;
    camX_ += (x_ - camX_) * follow;
    camY_ += (y_ - camY_) * follow;
    draw();
}

int Game::hullFrame() const {
    float u = heading_;
    while (u < 0.f) u += kTau;
    while (u >= kTau) u -= kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -20 || cy + h * 0.5f < -20 || cx - w * 0.5f > gs::SCREEN_W + 20 || cy - h * 0.5f > gs::SCREEN_H + 20)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 400L);
    long sh = std::clamp(std::lround(h), 1L, 400L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::lround(cx - sw * 0.5f));
    s.y = int16_t(std::lround(cy - sh * 0.5f));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * kZoom;
    sy = 112.f - (wy - camY_) * kZoom;
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * kZoom, pal);
}

void Game::drawHud() {
    char buf[64];
    int left = std::max(0, int(clock_ + 0.5f));
    std::snprintf(buf, sizeof buf, "TIME %02d:%02d", left / 60, left % 60);
    if (mode_ == Mode::Title) {
        hudC(16, "ROUND THE BUOYS", PAL_BANNER);
        hudC(17, "RETURN TO THE SAME DOCK", PAL_HUD);
        hudC(20, "ARROWS STEER THE TRAM", PAL_HUD);
        hudC(21, "UP DRIVES   DOWN BRAKES", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(24, "START", PAL_WIN);
        return;
    }
    hud(1, 0, "S3 TRAMBUOY", PAL_BANNER);
    hud(28, 0, buf, left < 16 ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(16, "PAUSED", PAL_BANNER);
        hudC(18, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(15, "BACK ON THE SAME DOCK", PAL_WIN);
        std::snprintf(buf, sizeof buf, "BUOYS %d   %02d:%02d", 3, left / 60, left % 60);
        hudC(17, buf, PAL_HUD);
        if (!bot_) hudC(19, "START RUNS IT AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, "THE CLOCK RAN OUT", PAL_ALERT);
        if (!bot_) hudC(18, "START TRIES AGAIN", PAL_HUD);
        return;
    }
    if (next_ < 3) std::snprintf(buf, sizeof buf, "BUOY %d OF 3", next_ + 1);
    else std::snprintf(buf, sizeof buf, "RETURN TO THE DOCK");
    hud(1, 1, buf, next_ < 3 ? PAL_BANNER : PAL_WIN);
    std::snprintf(buf, sizeof buf, "SPD %04.1f", std::fabs(speed_));
    hud(30, 1, buf, PAL_HUD);
    if (next_ >= 3 && std::hypot(x_ - kDockX, y_ - kDockY) < kDockR) hud(1, 26, "HOLD HER STEADY", PAL_WIN);
    else if (next_ >= 3) hud(1, 26, "THE SAME DOCK IS SOUTH", PAL_HUD);
    else hud(1, 26, "PASS CLOSE AND KEEP THE ORDER", PAL_HUD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / kZoom;
        float shore = std::clamp((wy + 30.f) / 120.f, 0.f, 1.f);
        uint16_t deep = gs::rgb4(1, 4, 9);
        uint16_t mid = gs::rgb4(2, 8, 12);
        int r = int(((deep >> 8) & 15) * (1.f - shore) + ((mid >> 8) & 15) * shore);
        int g = int(((deep >> 4) & 15) * (1.f - shore) + ((mid >> 4) & 15) * shore);
        int b = int((deep & 15) * (1.f - shore) + (mid & 15) * shore);
        float shimmer = 0.5f + 0.5f * std::sin(y * 0.11f + t_ * 1.7f);
        g = std::min(15, g + int(shimmer * 1.2f));
        if (wy < -2.f) {
            float u = std::clamp((-2.f - wy) / 14.f, 0.f, 1.f);
            r = int(r * (1.f - u) + 8 * u);
            g = int(g * (1.f - u) + 7 * u);
            b = int(b * (1.f - u) + 4 * u);
        }
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) spr(art_.title, 160.f, 48.f, float(art_.title.h), PAL_BANNER);
    else if (mode_ == Mode::Win) spr(art_.home, 160.f, 70.f, float(art_.home.h), PAL_WIN);

    // Earlier sprites sit on top, so the tram is submitted before the harbour.
    float tsx, tsy;
    worldToScreen(x_, y_, tsx, tsy);
    tsy += std::sin(t_ * 2.2f) * 0.6f;
    spr(art_.hull[hullFrame()], tsx, tsy, 34.f, PAL_TRAM);

    for (int i = 0; i < 28; i++) {
        if (wakes_[i].life <= 0.f) continue;
        place(art_.wake, wakes_[i].x, wakes_[i].y, 2.2f + wakes_[i].life, PAL_WAKE);
    }
    if (next_ < 3 && mode_ == Mode::Play) {
        float pulse = 5.5f + std::sin(t_ * 5.f) * 0.8f;
        place(art_.ring, kBuoys[next_].x, kBuoys[next_].y, pulse, PAL_MARK);
    }
    for (int i = 0; i < 3; i++) {
        float bob = std::sin(t_ * 1.8f + i) * 0.35f;
        int pal = kBuoys[i].pal;
        place(art_.buoy, kBuoys[i].x, kBuoys[i].y + bob, i < next_ ? 7.f : 9.f, pal);
    }
    place(art_.dock, 0.f, -2.f, 22.f, PAL_DOCK);
    drawHud();
}

}  // namespace tram
