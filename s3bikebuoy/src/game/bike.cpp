#include "game/bike.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace bike {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kZoom = 0.72f;

struct Mark {
    float x, y;
    const char* name;
    int pal;
};

constexpr Mark kMarks[3] = {
    {-100.f, 170.f, "RED", Pal::PAL_RED},
    {20.f, 320.f, "GREEN", Pal::PAL_GREEN},
    {160.f, 180.f, "GOLD", Pal::PAL_GOLD},
};

constexpr float kCatch = 58.f;
constexpr float kHomeX0 = -46.f, kHomeX1 = 46.f;
constexpr float kOtherX0 = 200.f, kOtherX1 = 292.f;
constexpr float kMouth = 6.f;
constexpr float kStop = 26.f;

struct Wp {
    float x, y;
};

constexpr Wp kBot[8] = {
    {0.f, 90.f},    {-100.f, 170.f}, {-100.f, 240.f}, {20.f, 360.f},
    {160.f, 250.f}, {160.f, 180.f},  {40.f, 70.f},    {0.f, -40.f},
};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

bool inSpan(float x, float a, float b) { return x >= a && x <= b; }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.apu.setMaster(0.45f);
    if (bot_) begin();
}

void Game::begin() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    armed_ = false;
    leg_ = 0;
    wp_ = 0;
    t_ = 0;
    raceTime_ = 0;
    hold_ = 0;
    x_ = 0;
    y_ = -42.f;
    heading_ = kPi * 0.5f;
    speed_ = 0;
    camX_ = x_;
    camY_ = y_;
    why_[0] = 0;
    puffN_ = 0;
    sys_->apu.silence();
}

void Game::controls(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    throttle = 0;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.2f) steer = p.axisX;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.accel > 0.15f) throttle = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.15f) throttle = -1.f;
    steer = std::clamp(steer, -1.f, 1.f);
}

void Game::pilot(float& steer, float& throttle) {
    if (leg_ >= 3 && wp_ < 6) wp_ = 6;
    const Wp& g = kBot[std::min(wp_, 7)];
    float dx = g.x - x_;
    float dy = g.y - y_;
    float dist = std::hypot(dx, dy);
    float aim = std::atan2(dy, dx);
    float diff = wrap(aim - heading_);
    steer = std::clamp(diff / 0.55f, -1.f, 1.f);
    throttle = 1.f;
    if (wp_ >= 6) throttle = (y_ > 20.f) ? 0.45f : 0.f;
    if (leg_ >= 3 && y_ < 40.f) throttle = (speed_ > 18.f) ? -1.f : 0.15f;
    if (dist < 26.f && wp_ < 7) wp_++;
    if (wp_ >= 7 && y_ < -10.f) {
        throttle = (speed_ > kStop) ? -1.f : 0.f;
        steer *= 0.3f;
    }
}

void Game::blip() {
    sys_->apu.tone(1, 680.f, 0.18f);
    tone_ = 0.12f;
}

void Game::chime() {
    sys_->apu.tone(1, 523.f, 0.22f);
    sys_->apu.tone(2, 784.f, 0.16f);
    tone_ = 0.4f;
}

void Game::physics(float dt, float steer, float throttle) {
    float turn = 1.35f + speed_ * 0.012f;
    heading_ = wrap(heading_ + steer * turn * dt);
    if (throttle > 0.f) speed_ += throttle * 78.f * dt;
    else if (throttle < 0.f) speed_ -= 90.f * dt;
    speed_ -= speed_ * 0.42f * dt;
    if (speed_ < 0.f) speed_ = 0.f;
    if (speed_ > 130.f) speed_ = 130.f;
    x_ += std::cos(heading_) * speed_ * dt;
    y_ += std::sin(heading_) * speed_ * dt;

    if (x_ < -240.f) {
        x_ = -240.f;
        speed_ *= 0.4f;
    }
    if (x_ > 420.f) {
        x_ = 420.f;
        speed_ *= 0.4f;
    }
    if (y_ > 460.f) {
        y_ = 460.f;
        speed_ *= 0.4f;
    }
    if (y_ < -96.f) {
        y_ = -96.f;
        speed_ *= 0.35f;
    }

    if (y_ > 48.f) armed_ = true;

    if (leg_ < 3) {
        float dx = kMarks[leg_].x - x_;
        float dy = kMarks[leg_].y - y_;
        if (dx * dx + dy * dy < kCatch * kCatch) {
            leg_++;
            blip();
        }
    }

    bool home = inSpan(x_, kHomeX0, kHomeX1);
    bool other = inSpan(x_, kOtherX0, kOtherX1);
    if (y_ < kMouth) {
        if (other && armed_) {
            std::snprintf(why_, sizeof why_, "OTHER DOCK");
            mode_ = Mode::Fail;
            over_ = true;
            won_ = false;
            return;
        }
        if (!home && !other) {
            if (leg_ >= 3 && armed_) {
                std::snprintf(why_, sizeof why_, "MISSED THE END");
                mode_ = Mode::Fail;
                over_ = true;
                won_ = false;
                return;
            }
            y_ = kMouth + 4.f;
            speed_ *= 0.25f;
        }
    }

    if (home && y_ < kMouth && y_ > -90.f && leg_ >= 3 && speed_ < kStop) {
        hold_ += dt;
        if (hold_ > 0.28f) {
            mode_ = Mode::Win;
            over_ = true;
            won_ = true;
            speed_ = 0;
            chime();
            return;
        }
    } else {
        hold_ = 0;
    }

    if (speed_ > 18.f) {
        Puff& p = puffs_[puffN_++ & 15];
        p.x = x_ - std::cos(heading_) * 16.f;
        p.y = y_ - std::sin(heading_) * 16.f;
        p.life = 0.45f;
    }
    for (Puff& p : puffs_) p.life -= dt;
}

void Game::audio() {
    if (tone_ > 0.f) {
        tone_ -= kDt;
        if (tone_ <= 0.f) {
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
    if (mode_ != Mode::Run) {
        sys_->apu.tone(0, 0, 0);
        return;
    }
    float vol = (speed_ > 4.f) ? 0.05f + speed_ * 0.0009f : 0.f;
    sys_->apu.tone(0, 70.f + speed_ * 2.4f, vol);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || bot_) begin();
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) begin();
    } else if (mode_ == Mode::Run) {
        raceTime_ += kDt;
        float steer = 0, throttle = 0;
        if (bot_) pilot(steer, throttle);
        else controls(steer, throttle);
        physics(kDt, steer, throttle);
        if (raceTime_ > 90.f && mode_ == Mode::Run) {
            std::snprintf(why_, sizeof why_, "TOO SLOW");
            mode_ = Mode::Fail;
            over_ = true;
            won_ = false;
        }
    }
    float cx = x_, cy = y_ + 10.f;
    camX_ += (cx - camX_) * 0.12f;
    camY_ += (cy - camY_) * 0.12f;
    audio();
    draw();
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (leg_ >= 3) return 3;
    if (leg_ >= 1) return 2;
    if (armed_) return 1;
    return 0;
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
    if (mode_ == Mode::Title) {
        hudC(8, "S3 BIKE BUOY", PAL_HUD);
        hudC(11, "ROUND THE BUOYS", PAL_HUD);
        hudC(13, "THEN STOP IN THE SAME DOCK", PAL_HUD);
        hudC(18, "ARROWS STEER   UP OPENS THE THROTTLE", PAL_HUD);
        hudC(20, "DOWN BRAKES", PAL_HUD);
        hudC(23, "THE OTHER DOCK FAILS THE LEG", PAL_HUD);
        hudC(24, "MISSING THE END FAILS THE LEG", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_HUD);
        return;
    }
    char buf[48];
    hud(1, 0, "S3 BIKE BUOY", PAL_HUD);
    int sec = int(raceTime_);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hud(33, 0, buf, PAL_HUD);
    if (mode_ == Mode::Win) {
        hudC(12, "SAME DOCK", PAL_HUD);
        std::snprintf(buf, sizeof buf, "TIME %d:%02d", sec / 60, sec % 60);
        hudC(14, buf, PAL_HUD);
        if (!bot_) hudC(17, "START RIDES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(12, why_[0] ? why_ : "MISSED THE END", PAL_HUD);
        if (!bot_) hudC(15, "START TRIES AGAIN", PAL_HUD);
        return;
    }
    if (leg_ < 3) std::snprintf(buf, sizeof buf, "NEXT %s BUOY", kMarks[leg_].name);
    else std::snprintf(buf, sizeof buf, "NEXT SAME DOCK");
    hud(1, 1, buf, leg_ < 3 ? kMarks[leg_].pal : PAL_HUD);
    std::snprintf(buf, sizeof buf, "PACE %.0f", speed_);
    hud(1, 2, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "BUOYS %d/3", std::min(leg_, 3));
    hud(1, 26, buf, PAL_HUD);
    if (leg_ >= 3) hud(1, 27, "EASE OFF INSIDE THE GREEN SLIP", PAL_HUD);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * kZoom;
    sy = 112.f - (wy - camY_) * kZoom;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 400L);
    long sh = std::clamp(std::lround(h), 1L, 400L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::lround(cx - sw * 0.5f));
    s.y = int16_t(std::lround(cy - sh * 0.5f));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * kZoom, pal, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = mode_ != Mode::Title;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int band = 4 + (y * 5) / gs::SCREEN_H;
        v.lineBackdrop[y] = gs::rgb4(1, band, band + 4);
        v.road[y].on = false;
        v.lineFog[y] = 0;
    }
    v.B.scroll(int(camX_ * 0.35f), int(-camY_ * 0.35f));

    if (mode_ == Mode::Title) {
        drawHud();
        return;
    }

    auto quayRun = [&](float x0, float x1, float y) {
        for (float x = x0; x <= x1; x += 46.f) place(art_.quay, x, y, 22.f, PAL_WOOD);
    };
    quayRun(-230.f, -70.f, -8.f);
    quayRun(70.f, 176.f, -8.f);
    quayRun(316.f, 400.f, -8.f);
    for (float y = -80.f; y <= 8.f; y += 26.f) {
        place(art_.pile, kHomeX0 - 6.f, y, 30.f, PAL_WOOD);
        place(art_.pile, kHomeX1 + 6.f, y, 30.f, PAL_WOOD);
        place(art_.pile, kOtherX0 - 6.f, y, 30.f, PAL_WOOD);
        place(art_.pile, kOtherX1 + 6.f, y, 30.f, PAL_WOOD);
    }
    place(art_.shed, -20.f, -78.f, 36.f, PAL_SHED);
    place(art_.shed, 246.f, -78.f, 36.f, PAL_SHED);
    place(art_.flag, 28.f, -60.f, 26.f, PAL_GREEN);
    place(art_.flag, 214.f, -60.f, 26.f, PAL_RED);
    place(art_.lamp, -36.f, -20.f, 18.f, PAL_GREEN);
    place(art_.lamp, 36.f, -20.f, 18.f, PAL_GREEN);
    place(art_.lamp, 214.f, -20.f, 18.f, PAL_GOLD);
    place(art_.lamp, 278.f, -20.f, 18.f, PAL_GOLD);

    int pals[3] = {PAL_RED, PAL_GREEN, PAL_GOLD};
    for (int i = 0; i < 3; i++) {
        float bob = (i == leg_ && mode_ == Mode::Run) ? std::sin(t_ * 4.f) * 3.f : 0.f;
        place(art_.buoy, kMarks[i].x, kMarks[i].y + bob, (i == leg_) ? 48.f : 40.f, pals[i]);
    }
    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        place(art_.wake, p.x, p.y, 10.f + (0.45f - p.life) * 18.f, PAL_WAKE);
    }

    float sx, sy;
    worldToScreen(x_, y_, sx, sy);
    spr(art_.bike[0], sx + 3.f, sy + 4.f, 34.f, PAL_BIKE, true);
    float u = heading_;
    if (u < 0.f) u += kTau;
    int dir = int(std::lround(u / kTau * 8.f)) & 7;
    spr(art_.bike[dir], sx, sy, 36.f, PAL_BIKE, false);
    drawHud();
}

}  // namespace bike
