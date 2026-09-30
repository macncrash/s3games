#include "game/kart.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace kartkilo {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kTrack = 6.15;
constexpr double kPlayerR = 0.48;
constexpr double kTop = 18.5;
constexpr double kFinish = 1000.0;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    showTitle();
}

int Game::marker() const {
    if (mode_ == Mode::Win || y_ >= kFinish) return 4;
    if (y_ > 760.0) return 3;
    if (mode_ == Mode::Run && danger_) return 2;
    if (mode_ == Mode::Run && y_ > 12.0) return 1;
    return 0;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    danger_ = false;
    meters_ = 0;
    t_ = 0;
    time_ = 0;
    x_ = 0;
    y_ = -2.2;
    speed_ = 0;
    why_[0] = 0;
    disks_.clear();
    for (Puff& p : puffs_) p = {};
}

void Game::layCourse() {
    disks_.clear();
    for (int i = 0; i < 19; i++) {
        const double gy = 68.0 + i * 48.0;
        const double open = (i % 3 - 1) * 2.35;
        for (int side = -1; side <= 1; side += 2) {
            Disk d;
            d.x = open + side * 3.15;
            d.y = gy;
            d.r = 1.18;
            d.kind = 0;
            if (std::fabs(d.x) < kTrack + 0.4) disks_.push_back(d);
        }
    }
    const double looseY[6] = {128, 268, 412, 556, 712, 868};
    const double phase[6] = {0.4, 1.7, 2.8, 0.9, 2.1, 3.4};
    for (int i = 0; i < 6; i++) {
        Disk d;
        d.y = looseY[i];
        d.r = 0.72;
        d.amp = 4.35;
        d.rate = 0.92 + (i % 3) * 0.08;
        d.phase = phase[i];
        d.kind = 1;
        disks_.push_back(d);
    }
}

double Game::diskX(const Disk& d, double t) const {
    if (d.kind == 0) return d.x;
    return d.amp * std::sin(t * d.rate + d.phase);
}

void Game::startRun() {
    showTitle();
    layCourse();
    mode_ = Mode::Run;
    y_ = 0;
    x_ = 0;
    speed_ = 0;
    time_ = 0;
}

void Game::pilot(double& steer, bool& gas, bool& brake) const {
    steer = 0;
    gas = true;
    brake = false;
    double best = -1e9;
    double pick = x_;
    for (double cand = -4.7; cand <= 4.7 + 1e-6; cand += 0.32) {
        double clearance = 3.4;
        for (const Disk& d : disks_) {
            const double dy = d.y - y_;
            if (dy < -1.6 || dy > 26.0) continue;
            const double eta = speed_ > 2.5 ? dy / speed_ : 0.45;
            const double dx = diskX(d, t_ + std::max(0.0, eta)) - cand;
            const double near = dy < 7.0 ? 1.0 : 0.35;
            const double c = (std::hypot(dx, dy * near * 0.22) - d.r - kPlayerR);
            clearance = std::min(clearance, c);
        }
        const double score = clearance * 5.0 - std::fabs(cand - x_) * 0.22 - std::fabs(cand) * 0.03;
        if (score > best) {
            best = score;
            pick = cand;
        }
    }
    const double err = pick - x_;
    steer = std::clamp(err * 0.85, -1.0, 1.0);
    if (best < 0.35) {
        gas = false;
        brake = true;
    } else if (best < 0.85 && speed_ > 11.0) {
        gas = false;
    }
    if (y_ > 980.0) gas = true;
}

void Game::fail(const char* why) {
    if (mode_ == Mode::Fail || mode_ == Mode::Win) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    std::printf("S3 KART KILO  FAIL  %s at %d m\n", why, meters_);
    std::fflush(stdout);
    tone_ = 0;
    if (sys_) sys_->apu.tone(0, 90.f, 0.25f);
}

void Game::win() {
    if (mode_ == Mode::Win) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    meters_ = 1000;
    y_ = kFinish;
    speed_ = 0;
    std::printf("S3 KART KILO  PASS  finished the kilometer clean  1000 m  wheels untouched  (%.1fs)\n", time_);
    std::fflush(stdout);
    tone_ = 0;
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.18f);
        sys_->apu.tone(1, 659.f, 0.14f);
    }
}

void Game::physics(double steer, bool gas, bool brake) {
    const double accel = gas ? 11.5 : (brake ? -16.0 : -2.4);
    speed_ = std::clamp(speed_ + accel * kDt, 0.0, kTop);
    x_ += steer * (6.4 + speed_ * 0.22) * kDt;
    y_ += speed_ * kDt;
    if (std::fabs(x_) > kTrack) {
        fail("left the tarmac");
        return;
    }
    danger_ = false;
    for (const Disk& d : disks_) {
        const double dx = diskX(d, t_) - x_;
        const double dy = d.y - y_;
        if (dy > -2.0 && dy < 14.0 && std::fabs(dx) < d.r + 1.6) danger_ = true;
        const double hit = d.r + kPlayerR;
        if (dx * dx + dy * dy < hit * hit) {
            fail(d.kind == 0 ? "touched a kart wheel" : "touched a loose wheel");
            return;
        }
    }
    if (y_ >= kFinish) win();
    meters_ = int(std::clamp(y_, 0.0, kFinish));
    if (speed_ > 4.0 && (int(t_ * 18.0) % 3) == 0) {
        Puff& p = puffs_[puffN_++ % 8];
        p.x = x_ + (puffN_ & 1 ? -0.35 : 0.35);
        p.y = y_ - 1.1;
        p.life = 0.35;
    }
    for (Puff& p : puffs_)
        if (p.life > 0) p.life -= kDt;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) startRun();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            double steer = 0;
            bool gas = false, brake = false;
            if (bot_) pilot(steer, gas, brake);
            else {
                if (pad.down(gs::BTN_LEFT)) steer -= 1;
                if (pad.down(gs::BTN_RIGHT)) steer += 1;
                if (std::fabs(pad.axisX) > 0.2) steer = pad.axisX;
                gas = pad.down(gs::BTN_A) || pad.down(gs::BTN_UP) || pad.accel > 0.2f;
                brake = pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.brake > 0.2f;
            }
            time_ += kDt;
            physics(steer, gas, brake);
        }
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        showTitle();
    }

    if (mode_ == Mode::Run && speed_ > 1.0) {
        tone_ = 70.f + float(speed_) * 9.f;
        sys.apu.tone(0, tone_, 0.05f);
        sys.apu.noise(0.02f, 4000.f + float(speed_) * 200.f, false);
    } else if (mode_ != Mode::Win && mode_ != Mode::Fail) {
        sys.apu.tone(0, 0, 0);
        sys.apu.noise(0, 0, false);
    }
    draw();
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (shadow) {
        cx += 2.f;
        cy += 3.f;
    }
    if (cx + w * 0.5f < -8 || cy + h * 0.5f < -8 || cx - w * 0.5f > gs::SCREEN_W + 8 || cy - h * 0.5f > gs::SCREEN_H + 8)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.clear();
    vdp.B.clear();
    const uint16_t grass = gs::rgb4(3, 7, 3);
    const uint16_t verge = gs::rgb4(5, 9, 4);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = (y & 16) ? verge : grass;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    const double camY = y_ - 3.2;
    auto toX = [&](double wx) { return float(160.0 + wx * zoom_); };
    auto toY = [&](double wy) { return float(176.0 - (wy - camY) * zoom_); };

    const double y0 = camY - 2.0;
    const double y1 = camY + 20.0;
    for (double ry = std::floor(y0 / 4.0) * 4.0; ry < y1; ry += 4.0) {
        const float sy = toY(ry + 2.0);
        spr(art_.road, 160.f, sy, 4.f * zoom_, PAL_ROAD, false);
    }
    spr(art_.ribbon, 160.f, toY(2.0), 1.3f * zoom_, PAL_BANNER, false);
    spr(art_.ribbon, 160.f, toY(1000.0), 1.5f * zoom_, PAL_WIN, false);

    const int spin = int(std::fmod(std::fabs(y_) * 1.6, 4.0));
    for (const Disk& d : disks_) {
        if (d.y < y0 - 2 || d.y > y1 + 2) continue;
        const double wx = diskX(d, t_);
        if (d.kind == 0) {
            spr(art_.rival, toX(wx), toY(d.y), 2.5f * zoom_, PAL_RIVAL, true);
            spr(art_.rival, toX(wx), toY(d.y), 2.5f * zoom_, PAL_RIVAL, false);
        } else {
            const float hh = 1.55f * zoom_;
            spr(art_.wheel[spin & 3], toX(wx), toY(d.y), hh, PAL_WHEEL, true);
            spr(art_.wheel[spin & 3], toX(wx), toY(d.y), hh, PAL_WHEEL, false);
        }
    }
    for (const Puff& p : puffs_) {
        if (p.life <= 0) continue;
        spr(art_.puff, toX(p.x), toY(p.y), (0.7f + float(0.35 - p.life)) * zoom_, PAL_PUFF, false);
    }
    spr(art_.kart, toX(x_), toY(y_), 2.35f * zoom_, PAL_KART, true);
    spr(art_.kart, toX(x_), toY(y_), 2.35f * zoom_, PAL_KART, false);

    char buf[64];
    if (mode_ == Mode::Title) {
        spr(art_.title, 160.f, 78.f, 28.f, PAL_BANNER, false);
        hudC(16, "FINISH THE KILOMETER", PAL_HUD);
        hudC(17, "DO NOT TOUCH A WHEEL", PAL_ALERT);
        if ((int(t_ * 2.0) & 1) == 0) hudC(20, "START", PAL_WIN);
        else hudC(20, "ARROWS STEER   A GAS   B BRAKE", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 KART KILO", PAL_BANNER);
    if (mode_ == Mode::Pause) {
        hudC(14, "PAUSED", PAL_HUD);
        hudC(16, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        spr(art_.clean, 160.f, 96.f, 22.f, PAL_WIN, false);
        std::snprintf(buf, sizeof buf, "1000 M   %.1f S", time_);
        hudC(16, buf, PAL_HUD);
        hudC(17, "WHEELS UNTOUCHED", PAL_WIN);
        if (!bot_) hudC(20, "START RUNS IT AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(14, why_, PAL_ALERT);
        std::snprintf(buf, sizeof buf, "%d M", meters_);
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(20, "START TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "%d/1000 M", meters_);
    hud(1, 1, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "M/S %4.1f", speed_);
    hud(29, 1, buf, PAL_HUD);
    if (danger_) hud(1, 26, "WHEEL CLOSE  DO NOT TOUCH IT", PAL_ALERT);
    else hud(1, 26, "KEEP THE KILOMETER CLEAN", PAL_WIN);
    hud(1, 27, "ARROWS STEER   A GAS   B BRAKE", PAL_HUD);
}

}  // namespace kartkilo
