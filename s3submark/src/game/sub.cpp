#include "game/sub.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace submark {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kScale = 7.5;
constexpr double kFloor = 4.2;
constexpr double kRest = 6.15;
constexpr double kCeil = 26.5;
constexpr double kMark = 92.0;
constexpr double kHalf = 9.0;
constexpr double kEnd = 128.0;
constexpr double kAir = 42.0;
constexpr double kHoldNeed = 1.05;
constexpr double kHard = -7.4;

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    showTitle();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    phase_ = 0;
    t_ = 0;
    race_ = 0;
    hold_ = 0;
    x_ = 18;
    y_ = 16;
    vx_ = 0;
    vy_ = 0;
    throttle_ = 0;
    climb_ = 0;
    camX_ = 28;
    camY_ = 14;
    why_[0] = 0;
    for (Bubble& b : bub_) b.life = 0;
}

void Game::startRun() {
    showTitle();
    mode_ = Mode::Run;
    phase_ = 1;
    blip(220);
}

void Game::win() {
    if (mode_ == Mode::Win) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    phase_ = 4;
    vx_ = 0;
    vy_ = 0;
    std::snprintf(why_, sizeof why_, "set");
    chime(4);
}

void Game::fail(const char* why) {
    if (mode_ == Mode::Fail || mode_ == Mode::Win) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    blip(90);
}

bool Game::onMark() const { return std::fabs(x_ - kMark) <= kHalf; }

bool Game::settled() const {
    return onMark() && std::fabs(y_ - kRest) < 0.35 && std::fabs(vy_) < 0.55 && std::fabs(vx_) < 1.35;
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
    double aimX = kMark;
    double aimY = 14.0;
    double want = 6.4;
    if (x_ > 48) {
        aimY = 9.5;
        want = 3.6;
    }
    if (x_ > 70) {
        aimY = kRest + 1.4;
        want = 1.6;
    }
    if (x_ > kMark - 14) {
        aimY = kRest;
        want = (std::fabs(x_ - kMark) < 2.2) ? 0.0 : (x_ < kMark ? 0.85 : -0.7);
    }
    if (settled() || hold_ > 0.02) {
        aimY = kRest;
        want = 0;
    }
    double yErr = aimY - y_;
    climb = clampd(yErr * 0.85 - vy_ * 0.55, -1.0, 1.0);
    thrust = clampd((want - vx_) * 0.7 + (aimX - x_) * 0.04, -1.0, 1.0);
    if (x_ > kEnd - 16) thrust = std::min(thrust, -0.35);
}

void Game::physics(double thrust, double climbIn) {
    throttle_ += (thrust - throttle_) * 0.14;
    climb_ += (climbIn - climb_) * 0.22;
    const double drift = 2.4;
    vx_ += (throttle_ * 15.0 + drift) * kDt;
    vx_ *= 0.986;
    vy_ += (climb_ * 22.0 - 3.1) * kDt;
    vy_ *= 0.88;
    x_ += vx_ * kDt;
    y_ += vy_ * kDt;

    if (x_ < 6) {
        x_ = 6;
        if (vx_ < 0) vx_ = 0;
    }
    if (y_ > kCeil) {
        y_ = kCeil;
        if (vy_ > 0) vy_ = 0;
    }

    if (y_ < kRest) {
        double hit = vy_;
        y_ = kRest;
        if (vy_ < 0) vy_ = 0;
        vx_ *= 0.9;
        if (hit < kHard) {
            fail(onMark() ? "set too hard" : "off the mark");
            return;
        }
    }

    if (x_ > kEnd && hold_ < kHoldNeed) {
        fail("missed the end");
        return;
    }

    if (settled()) hold_ += kDt;
    else if (!onMark() || std::fabs(y_ - kRest) > 0.8) hold_ = 0;
    else hold_ = std::max(0.0, hold_ - kDt * 0.35);

    if (hold_ >= kHoldNeed) {
        win();
        return;
    }
    phase_ = hold_ > 0.04 ? 3 : (onMark() && y_ < 10 ? 2 : 1);
    if (race_ >= kAir) fail("the leg ran out");
}

int Game::marker() const {
    if (mode_ == Mode::Win) return 4;
    if (mode_ != Mode::Run && mode_ != Mode::Pause) return 0;
    return phase_;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ && t_ > 0.4) startRun();
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
            static const float notes[] = {392, 523, 659, 784};
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
    if (mode_ == Mode::Run) hum = 0.03f + float(std::fabs(throttle_)) * 0.04f;
    float hz = 40.f + float(std::fabs(vx_)) * 2.8f;
    sys_->apu.tone(0, hum > 0 ? hz : 0, hum);
}

void Game::place(const gs::Mipped& m, double wx, double wy, float worldH, int pal, bool flip) {
    if (m.h < 1 || worldH <= 0) return;
    float h = worldH * float(kScale);
    float w = h * float(m.w) / float(m.h);
    float sx = float((wx - camX_) * kScale + 150.0);
    float sy = float(120.0 - (wy - camY_) * kScale);
    if (sx + w < -24 || sx - w > gs::SCREEN_W + 24 || sy + h < -24 || sy - h > gs::SCREEN_H + 24) return;
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
    s.y = 78;
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
    vdp.A.clear();
    vdp.B.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int deep = y * 10 / gs::SCREEN_H;
        vdp.lineBackdrop[y] = gs::rgb4(0, 2 + (10 - deep) / 4, 5 + (8 - deep) / 3);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    float look = float(x_ + 8);
    if (mode_ == Mode::Title) look = 40;
    camX_ += (look - camX_) * 0.08f;
    float aimY = float(std::max(8.0, y_));
    camY_ += (aimY - camY_) * 0.07f;

    for (double sx = -8; sx < 170; sx += 6.2) place(art_.sand, sx, kFloor - 0.4, 2.6f, PAL_BED);
    for (double kx = 12; kx < 78; kx += 16) place(art_.kelp, kx, kFloor + 2.2, 3.6f, PAL_KELP);
    for (double kx = 108; kx < 150; kx += 18) place(art_.kelp, kx, kFloor + 1.8, 2.8f, PAL_KELP, true);

    place(art_.pad, kMark, kFloor + 0.35, 1.7f, PAL_MARK);
    place(art_.cross, kMark, kFloor + 1.15, 3.1f, PAL_MARK);
    place(art_.lamp, kMark - kHalf, kFloor + 1.6, 1.15f, PAL_MARK);
    place(art_.lamp, kMark + kHalf, kFloor + 1.6, 1.15f, PAL_MARK);
    place(art_.pylon, kEnd + 1.2, kFloor + 5.2, 9.4f, PAL_END);
    place(art_.lamp, kEnd - 1.4, kFloor + 8.4, 1.3f, PAL_END);

    int fishN = int(t_ * 14) % 36;
    place(art_.fish, 24 + fishN * 0.35, 18, 1.15f, PAL_FISH);
    place(art_.fish, 60 - fishN * 0.15, 12, 0.95f, PAL_FISH, true);

    if (mode_ != Mode::Title) {
        int slot = int(t_ * 9) % 10;
        if (bub_[slot].life <= 0 && (std::fabs(vx_) > 0.3 || std::fabs(vy_) > 0.3)) {
            bub_[slot] = {x_ - 3.6, y_ + 0.3, 1.0, 0.32};
        }
        for (Bubble& b : bub_) {
            if (b.life <= 0) continue;
            b.life -= kDt;
            b.y += 1.4 * kDt;
            place(art_.bub, b.x, b.y, float(b.r + (1.0 - b.life) * 0.35), PAL_BUB);
        }
        bool flash = hold_ > 0.04 || mode_ == Mode::Win;
        place(art_.sub, x_, y_, 3.2f, flash ? PAL_WIN : PAL_SUB, vx_ < -0.4);
    } else {
        place(art_.sub, 22, 15.5, 3.4f, PAL_SUB);
        banner(art_.title, 28, PAL_HUD);
        hudC(18, "SET DOWN ON THE MARK", PAL_HUD);
        hudC(20, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        hudC(23, "A THRUST   B REVERSE", PAL_HUD);
        hudC(24, "UP DOWN   START", PAL_HUD);
    }

    if (mode_ == Mode::Win) {
        banner(art_.set, 22, PAL_WIN);
        hudC(18, "THE SUB IS ON THE MARK", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped* ban = &art_.missed;
        if (why_[0] == 'o') ban = &art_.off;
        else if (why_[0] == 't') ban = &art_.ran;
        else if (why_[0] == 's') ban = &art_.hard;
        banner(*ban, 20, PAL_ALERT);
        hudC(18, why_, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        banner(art_.paused, 22, PAL_HUD);
    } else if (mode_ == Mode::Run) {
        char buf[40];
        std::snprintf(buf, sizeof buf, "LEG %4.1f", std::max(0.0, kAir - race_));
        hud(1, 1, buf, race_ > kAir - 8 ? PAL_ALERT : PAL_HUD);
        double gap = kEnd - x_;
        std::snprintf(buf, sizeof buf, "END %3.0f", std::max(0.0, gap));
        hud(30, 1, buf, gap < 18 ? PAL_ALERT : PAL_HUD);
        std::snprintf(buf, sizeof buf, "SET %d", int(hold_ * 10));
        hud(1, 26, buf, hold_ > 0 ? PAL_WIN : PAL_HUD);
        std::snprintf(buf, sizeof buf, "ENG %+4d", int(std::lround(throttle_ * 100)));
        hud(28, 26, buf, PAL_HUD);
        if (onMark() && y_ > kRest + 1.2) hudC(3, "MARK BELOW", PAL_MARK);
    }
}

}  // namespace submark
