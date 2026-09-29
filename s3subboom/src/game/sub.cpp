#include "game/sub.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace subboom {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kAir = 78.0;
constexpr double kHoldNeed = 1.15;
constexpr double kSlot = 16.0;
constexpr double kHalf = 2.7;
constexpr double kMouth = 160.0;
constexpr double kStop = 172.2;
constexpr double kAhead = 7.1;
constexpr double kHalfL = 2.55;
constexpr double kHalfH = 1.08;
constexpr double kScale = 7.4;

struct Gate {
    double x, y, half;
};

constexpr Gate kGates[] = {{48.0, 22.0, 4.4}, {92.0, 11.2, 4.3}, {132.0, 19.2, 4.4}};

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.5f);
    showTitle();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    phase_ = 0;
    over_ = false;
    won_ = false;
    hold_ = 0;
    race_ = 0;
    x_ = 14;
    y_ = 18;
    vx_ = 0;
    vy_ = 0;
    throttle_ = 0;
    climb_ = 0;
    why_[0] = 0;
    t_ = 0;
    camX_ = 28;
    camY_ = 16;
}

void Game::startRun() {
    mode_ = Mode::Run;
    phase_ = 1;
    over_ = false;
    won_ = false;
    hold_ = 0;
    race_ = 0;
    x_ = 12;
    y_ = 20;
    vx_ = 0;
    vy_ = 0;
    throttle_ = 0;
    climb_ = 0;
    why_[0] = 0;
    for (int i = 0; i < 12; i++) bub_[i].life = 0;
    blip(180);
}

Game::Box Game::driveBox() const {
    Box b;
    b.minX = x_ + kAhead - kHalfL;
    b.maxX = x_ + kAhead + kHalfL;
    b.minY = y_ - kHalfH;
    b.maxY = y_ + kHalfH;
    return b;
}

bool Game::driveAligned() const {
    Box b = driveBox();
    return b.minY >= kSlot - kHalf + 0.08 && b.maxY <= kSlot + kHalf - 0.08;
}

bool Game::driveInside() const {
    Box b = driveBox();
    return b.minX >= kMouth + 0.55 && b.maxX <= kStop - 0.12 && driveAligned();
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.15) return 3;
    if (driveInside()) return 2;
    return 1;
}

void Game::win() {
    if (mode_ == Mode::Win) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    phase_ = 4;
    vx_ = 0;
    vy_ = 0;
    std::snprintf(why_, sizeof why_, "delivered");
    chime(4);
}

void Game::fail(const char* why) {
    if (mode_ == Mode::Fail || mode_ == Mode::Win) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    phase_ = 4;
    vx_ = 0;
    std::snprintf(why_, sizeof why_, "%s", why);
    blip(70);
}

void Game::controls(double& thrust, double& climb) {
    const gs::Pad& p = sys_->pad;
    climb = 0;
    thrust = 0;
    if (p.down(gs::BTN_UP)) climb += 1;
    if (p.down(gs::BTN_DOWN)) climb -= 1;
    if (p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A)) thrust += 1;
    if (p.down(gs::BTN_LEFT) || p.down(gs::BTN_B)) thrust -= 1;
    if (std::fabs(p.axisY) > 0.2) climb += p.axisY;
    if (std::fabs(p.axisX) > 0.2) thrust += p.axisX;
    climb = clampd(climb, -1, 1);
    thrust = clampd(thrust, -1, 1);
}

void Game::pilot(double& thrust, double& climb) {
    double aim = 22.0;
    if (x_ >= 62 && x_ < 112) aim = 11.2;
    else if (x_ >= 112 && x_ < 142) aim = 19.2;
    else if (x_ >= 142) aim = kSlot;

    const bool in = driveInside();
    const bool lined = std::fabs(y_ - kSlot) < 0.42 && std::fabs(vy_) < 0.55;
    double want = 8.2;
    if (x_ > 142) want = 4.2;
    if (x_ > kMouth - 18) {
        aim = kSlot;
        want = lined ? 2.15 : -0.2;
    }
    if (x_ + kAhead + kHalfL > kMouth - 1.2 && !driveAligned()) want = -0.8;
    if (in) {
        aim = kSlot;
        Box b = driveBox();
        want = (b.maxX > kStop - 1.45) ? 0.0 : 0.55;
    }

    for (const Gate& g : kGates) {
        if (std::fabs(x_ - g.x) < 8.0 && std::fabs(y_ - g.y) > g.half - 2.3) {
            aim = g.y;
            want = std::min(want, 2.2);
        }
    }

    double yErr = aim - y_;
    climb = clampd(yErr * 1.15 - vy_ * 0.18, -1.0, 1.0);
    thrust = clampd((want - vx_) * 0.55, -1.0, 1.0);
}

void Game::physics(double thrust, double climbIn) {
    throttle_ += (thrust - throttle_) * 0.12;
    climb_ += (climbIn - climb_) * 0.2;
    vx_ += throttle_ * 16.0 * kDt;
    vx_ *= 0.988;
    vy_ += climb_ * 26.0 * kDt;
    vy_ *= 0.86;
    x_ += vx_ * kDt;
    y_ += vy_ * kDt;
    if (x_ < 4) {
        x_ = 4;
        if (vx_ < 0) vx_ = 0;
    }
    if (y_ < 6.4) {
        y_ = 6.4;
        if (vy_ < 0) vy_ = 0;
    }
    if (y_ > 27.6) {
        y_ = 27.6;
        if (vy_ > 0) vy_ = 0;
    }

    const double hull = 1.7;
    for (const Gate& g : kGates) {
        if (std::fabs(x_ - g.x) > 2.3) continue;
        double lo = g.y - g.half + hull;
        double hi = g.y + g.half - hull;
        if (y_ < lo || y_ > hi) {
            if (std::fabs(vx_) > 6.2) {
                fail("off the leg");
                return;
            }
            x_ = (x_ >= g.x) ? g.x + 2.35 : g.x - 2.35;
            vx_ *= -0.25;
        }
    }

    Box d = driveBox();
    if (!driveAligned() && d.maxX >= kMouth) {
        x_ -= d.maxX - (kMouth - 0.05);
        if (vx_ > 0.15) {
            fail("missed the end");
            return;
        }
        vx_ = 0;
    }
    d = driveBox();
    if (driveAligned() && d.maxX > kStop) {
        double over = d.maxX - kStop;
        x_ -= over;
        if (vx_ > 3.6) {
            fail("missed the end");
            return;
        }
        vx_ = 0;
        vy_ *= 0.4;
    }

    const bool in = driveInside();
    if (in && std::fabs(vx_) < 1.15 && std::fabs(vy_) < 0.7) hold_ += kDt;
    else if (!in) hold_ = 0;
    phase_ = in ? (hold_ > 0.05 ? 3 : 2) : 1;
    if (hold_ >= kHoldNeed) win();
    if (race_ >= kAir) fail("the leg ran out");
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ && t_ > 0.35) startRun();
        else if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) startRun();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            race_ += kDt;
            double thrust = 0, climb = 0;
            if (bot_) pilot(thrust, climb);
            else controls(thrust, climb);
            physics(thrust, climb);
        }
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) mode_ = Mode::Run;
    } else if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || (bot_ && t_ > 0)) {
        if (!bot_) showTitle();
    }
    audio();
    draw();
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.12f);
    tone_ = 0.12f;
}

void Game::chime(int notes) {
    chimeN_ = notes;
    chimeStep_ = 0;
    chimeT_ = 0;
}

void Game::audio() {
    if (tone_ > 0) {
        tone_ -= float(kDt);
        if (tone_ <= 0) sys_->apu.tone(1, 0, 0);
    }
    if (chimeN_ > 0) {
        chimeT_ -= float(kDt);
        if (chimeT_ <= 0) {
            static const float notes[] = {523, 659, 784, 1046};
            int i = chimeStep_ < 4 ? chimeStep_ : 3;
            sys_->apu.tone(2, notes[i], 0.1f);
            chimeStep_++;
            chimeT_ = 0.12f;
            if (chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (mode_ != Mode::Win) {
        sys_->apu.tone(2, 0, 0);
    }
    float hum = 0;
    if (mode_ == Mode::Run) hum = 0.03f + float(std::fabs(throttle_)) * 0.05f;
    float hz = 46.f + float(std::fabs(vx_)) * 3.2f;
    sys_->apu.tone(0, hum > 0 ? hz : 0, hum);
}

void Game::place(const gs::Mipped& m, double wx, double wy, float worldH, int pal, bool flip) {
    if (m.h < 1 || worldH <= 0) return;
    float h = worldH * float(kScale);
    float w = h * float(m.w) / float(m.h);
    float sx = float((wx - camX_) * kScale + 150.0);
    float sy = float(112.0 - (wy - camY_) * kScale);
    if (sx + w < -20 || sx - w > gs::SCREEN_W + 20 || sy + h < -20 || sy - h > gs::SCREEN_H + 20) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 400L);
    long sh = std::clamp(std::lround(h), 1L, 400L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::lround(sx - sw * 0.5));
    s.y = int16_t(std::lround(sy - sh * 0.5));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::banner(const gs::Mipped& m, float h, int pal) {
    if (m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(160 - s.w / 2);
    s.y = 88;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
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

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int deep = y * 8 / gs::SCREEN_H;
        vdp.lineBackdrop[y] = gs::rgb4(1, 3 + (8 - deep) / 3, 6 + (6 - deep / 2));
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    float look = float(x_ + 10);
    if (mode_ == Mode::Title) look = 36;
    camX_ += (look - camX_) * 0.08f;
    float aimY = float(y_);
    if (x_ > kMouth - 40) aimY = float(kSlot);
    camY_ += (aimY - camY_) * 0.06f;

    for (double rx = -4; rx < 200; rx += 9.5) {
        place(art_.rock, rx, 3.2, 4.2f, PAL_ROCK);
        place(art_.rock, rx + 3.0, 30.5, 3.6f, PAL_ROCK, true);
    }
    for (double kx = 8; kx < 150; kx += 14) place(art_.kelp, kx, 5.5, 3.4f, PAL_ROCK);

    for (const Gate& g : kGates) {
        place(art_.pile, g.x, g.y + g.half + 6.5, 12.f, PAL_ROCK);
        place(art_.pile, g.x, g.y - g.half - 6.5, 12.f, PAL_ROCK);
        place(art_.rock, g.x, g.y + g.half + 1.2, 3.2f, PAL_ROCK);
        place(art_.rock, g.x, g.y - g.half - 1.2, 3.2f, PAL_ROCK, true);
    }

    place(art_.pile, kMouth - 0.4, kSlot + kHalf + 7.2, 14.f, PAL_BOOM);
    place(art_.pile, kMouth - 0.4, kSlot - kHalf - 7.2, 14.f, PAL_BOOM);
    place(art_.boom, (kMouth + kStop) * 0.5, kSlot + kHalf + 0.15, 1.5f, PAL_BOOM);
    place(art_.boom, (kMouth + kStop) * 0.5, kSlot - kHalf - 0.15, 1.5f, PAL_BOOM);
    place(art_.pile, kStop + 0.6, kSlot, 6.4f, PAL_BOOM);
    place(art_.lamp, kMouth + 1.2, kSlot + kHalf - 0.2, 1.3f, PAL_LAMP);
    place(art_.lamp, kStop - 1.0, kSlot, 1.1f, PAL_LAMP);

    int fishN = int(t_ * 18) % 40;
    place(art_.fish, 30 + fishN * 0.4, 14, 1.3f, PAL_FISH);
    place(art_.fish, 70 - fishN * 0.2, 24, 1.1f, PAL_FISH, true);

    if (mode_ != Mode::Title) {
        int slot = int(t_ * 10) % 12;
        if (bub_[slot].life <= 0 && std::fabs(vx_) > 0.4) {
            bub_[slot] = {x_ - 4.2, y_ + 0.4, 1.0, 0.35};
        }
        for (Bubble& b : bub_) {
            if (b.life <= 0) continue;
            b.life -= kDt;
            b.y += 1.6 * kDt;
            b.x -= 0.2 * kDt;
            place(art_.bub, b.x, b.y, float(b.r + (1.0 - b.life) * 0.4), PAL_BUB);
        }
        bool flash = hold_ > 0.05;
        place(art_.sub, x_, y_, 3.5f, flash ? PAL_WIN : PAL_SUB);
        place(art_.drive, x_ + kAhead, y_, 2.15f, PAL_DRIVE);
    } else {
        place(art_.sub, 22, 15, 5.2f, PAL_SUB);
        place(art_.drive, 22 + kAhead * 1.15, 15, 3.0f, PAL_DRIVE);
        place(art_.boom, 48, 18.4, 2.2f, PAL_BOOM);
        place(art_.boom, 48, 12.2, 2.2f, PAL_BOOM);
    }

    if (mode_ == Mode::Title) {
        banner(art_.title, 28, PAL_HUD);
        hudC(16, "DELIVER THE DRIVE TO THE BOOM", PAL_HUD);
        hudC(18, "MISS THE END AND THE LEG FAILS", PAL_HUD);
        hudC(22, "A THRUST   UP DOWN DEPTH", PAL_HUD);
        hudC(24, "START TO DIVE", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        banner(art_.paused, 22, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        banner(art_.delivered, 20, PAL_WIN);
        hudC(18, "THE DRIVE IS ON THE BOOM", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        if (!std::strcmp(why_, "missed the end")) banner(art_.missed, 22, PAL_ALERT);
        else if (!std::strcmp(why_, "off the leg")) banner(art_.offLeg, 22, PAL_ALERT);
        else banner(art_.out, 22, PAL_ALERT);
        hudC(18, why_, PAL_ALERT);
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        char buf[40];
        std::snprintf(buf, sizeof buf, "LEG %4.1f", std::max(0.0, kAir - race_));
        hud(1, 1, buf, PAL_HUD);
        Box d = driveBox();
        double gap = std::max(0.0, kMouth - d.maxX);
        std::snprintf(buf, sizeof buf, "END %3.0f", gap);
        hud(30, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "HOLD %d", int(hold_ * 5));
        hud(1, 26, buf, hold_ > 0 ? PAL_WIN : PAL_HUD);
        std::snprintf(buf, sizeof buf, "ENG %+4d", int(std::lround(throttle_ * 100)));
        hud(28, 26, buf, PAL_HUD);
    }
}

}  // namespace subboom
