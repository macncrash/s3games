#include "mark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tugmark {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kPi = 3.141592653589793;
constexpr double kTau = 6.283185307179586;
constexpr double kNorth = kPi * 0.5;
constexpr double kMarkX = 0.0;
constexpr double kMarkY = 172.0;
constexpr double kPaintR = 5.5;
constexpr double kMarkR = kPaintR * double(kHeartPx) / double(kPaintPx);
constexpr double kEnd = 206.0;
constexpr double kWall = 21.0;
constexpr double kSouth = 18.0;
constexpr double kHalfW = 3.35;
constexpr double kHalfL = 7.7;
constexpr double kBow = 7.2;
constexpr double kStop = 0.34;
constexpr double kHoldNeed = 0.50;
constexpr double kOutNeed = 2.1;
constexpr double kCurrent = 2.85;
constexpr double kAhead = 12.0;
constexpr double kAstern = 8.0;
constexpr double kNose = 0.58;
constexpr double kStartX = 7.0;
constexpr double kStartY = 44.0;
constexpr double kStartH = 1.02;
constexpr float kPlayZoom = 2.58f;

double wrap(double a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

struct Screw {
    double steer;
    double throttle;
};

// Ground velocity is surge along the heading, plus a flood that always runs north.
Screw screwCommand(double heading, double wantVx, double wantVy, bool preferNorth) {
    double sx = wantVx;
    double sy = wantVy - kCurrent;
    double mag = std::hypot(sx, sy);
    double desH;
    double surgeCmd;
    if (mag < 0.05) {
        desH = kNorth;
        surgeCmd = -kCurrent;
    } else {
        double aheadH = std::atan2(sy, sx);
        double backH = wrap(aheadH + kPi);
        bool useBack;
        if (preferNorth) {
            useBack = std::fabs(wrap(backH - kNorth)) + 0.25 < std::fabs(wrap(aheadH - kNorth));
        } else {
            useBack = std::fabs(wrap(backH - heading)) + 0.12 < std::fabs(wrap(aheadH - heading));
        }
        if (useBack) {
            desH = backH;
            surgeCmd = -mag;
        } else {
            desH = aheadH;
            surgeCmd = mag;
        }
    }
    double err = wrap(desH - heading);
    double steer = clampd(err / 0.32, -1.0, 1.0);
    if (std::fabs(err) > 1.05) surgeCmd *= 0.2;
    else if (std::fabs(err) > 0.55) surgeCmd *= 0.55;
    double throttle = surgeCmd >= 0 ? surgeCmd / kAhead : surgeCmd / kAstern;
    return {steer, clampd(throttle, -1.0, 1.0)};
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.08) return 3;
    if (bowOn_) return 2;
    return 1;
}

int Game::hullFrame() const {
    double u = std::fmod(heading_, kTau);
    if (u < 0) u += kTau;
    int i = int(std::lround(u / kTau * 16.0)) % 16;
    if (i < 0) i += 16;
    return i;
}

void Game::bowAt(double& bx, double& by) const {
    bx = x_ + std::cos(heading_) * kBow;
    by = y_ + std::sin(heading_) * kBow;
}

Game::Ext Game::extents() const {
    const double c = std::cos(heading_), s = std::sin(heading_);
    Ext e{x_, x_, y_, y_};
    const double fx[2] = {-kHalfL, kHalfL};
    const double sy[2] = {-kHalfW, kHalfW};
    for (double f : fx) {
        for (double w : sy) {
            double wx = x_ + f * c + w * s;
            double wy = y_ + f * s - w * c;
            e.minX = std::min(e.minX, wx);
            e.maxX = std::max(e.maxX, wx);
            e.minY = std::min(e.minY, wy);
            e.maxY = std::max(e.maxY, wy);
        }
    }
    return e;
}

double Game::groundSpeed() const {
    double vx = std::cos(heading_) * surge_;
    double vy = std::sin(heading_) * surge_ + kCurrent;
    return std::hypot(vx, vy);
}

void Game::measure() {
    bowAt(bx_, by_);
    bowDist_ = std::hypot(bx_ - kMarkX, by_ - kMarkY);
    bowOn_ = bowDist_ <= kMarkR;
    noseOk_ = std::fabs(wrap(kNorth - heading_)) <= kNose;
    if (hold_ > 0.05) phase_ = 3;
    else if (bowOn_) phase_ = 2;
    else if (bowDist_ < 18.0) phase_ = 1;
    else phase_ = 0;
}

const char* Game::stopWhy() const {
    if (bowOn_ && !noseOk_) return "nose is not set on the mark";
    if (bowDist_ <= kPaintR) return "close to the mark is still off it";
    double dx = bx_ - kMarkX;
    double dy = by_ - kMarkY;
    if (std::fabs(dx) > std::fabs(dy) && std::fabs(dx) > 2.0) return "stopped wide of the mark";
    if (dy < 0) return "stopped short of the mark";
    return "stopped long of the mark";
}

const char* Game::hint() const {
    if (hold_ > 0.05) return "HOLD THE SET";
    if (bowOn_ && !noseOk_) return "SQUARE THE NOSE ON THE MARK";
    if (bowOn_) return "BOW IS ON IT  BACK DOWN AND HOLD";
    if (bowDist_ <= kPaintR) return "CLOSE IS STILL OFF THE MARK";
    if (by_ > kMarkY + 1.0) return "LONG  COME BACK BEFORE THE END";
    if (bowDist_ < 24.0) return "SET THE BOW IN THE HEART";
    return "THE MARK IS UP-CHANNEL";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    surge_ = 0;
    throttle_ = 0;
    hold_ = 0;
    outT_ = 0;
    stuckT_ = 0;
    race_ = 0;
    phase_ = 0;
    won_ = false;
    over_ = false;
    announced_ = false;
    chimeN_ = 0;
    chimeStep_ = 0;
    wakeCursor_ = 0;
    smokeCursor_ = 0;
    wakeT_ = 0;
    smokeT_ = 0;
    hornT_ = 0;
    thumpT_ = 0;
    shake_ = 0;
    why_[0] = 0;
    prevBow_ = 1.0e9;
    stuckX_ = x_;
    stuckY_ = y_;
    for (Puff& p : wake_) p = {};
    for (Puff& p : smoke_) p = {};
    measure();
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 0.f;
    camY_ = 118.f;
    zoom_ = 1.22f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = float(x_);
    camY_ = float(y_);
    zoom_ = kPlayZoom;
    horn(0.38f);
    blip(520.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.14f, 0.16f, 0.08f);
    t_ = 0;
    if (bot_) startRun();
    else showTitle();
}

void Game::controls(double& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    if (p.down(gs::BTN_LEFT)) steer += 1;
    if (p.down(gs::BTN_RIGHT)) steer -= 1;
    if (std::fabs(p.axisX) > 0.18f) steer = clampd(double(-p.axisX), -1.0, 1.0);
    const bool ahead = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f;
    const bool astern = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f;
    if (ahead && !astern) throttle_ = std::min(1.0, throttle_ + kDt * 0.62);
    else if (astern && !ahead) throttle_ = std::max(-1.0, throttle_ - kDt * 0.62);
    if (p.accel > 0.08f) throttle_ = std::min(1.0, throttle_ + p.accel * kDt * 0.9);
    if (p.brake > 0.08f) throttle_ = std::max(-1.0, throttle_ - p.brake * kDt * 0.9);
    if (p.down(gs::BTN_TURBO)) horn(0.12f);
}

void Game::pilot(double& steer) {
    double needX = kMarkX - bx_;
    double needY = kMarkY - by_;
    double dist = std::hypot(needX, needY);
    double vy = std::sin(heading_) * surge_ + kCurrent;
    Ext e = extents();
    if (dist > kMarkR && (e.maxY > kEnd - 10.0 || (by_ > kMarkY + 8.0 && vy > 0.35))) {
        steer = clampd(wrap(kNorth - heading_) / 0.24, -1.0, 1.0);
        throttle_ = -1;
        return;
    }

    double wantVx, wantVy;
    bool preferNorth;
    if (dist < kMarkR) {
        wantVx = clampd(needX * 1.15, -0.24, 0.24);
        wantVy = clampd(needY * 1.15, -0.24, 0.24);
        preferNorth = true;
    } else if (dist < 16.0) {
        double spd = clampd(dist * 0.20, 0.58, 1.9);
        wantVx = needX / dist * spd;
        wantVy = needY / dist * spd;
        preferNorth = true;
    } else {
        double spd = dist > 70.0 ? 6.4 : dist > 36.0 ? 4.2 : 2.6;
        if (std::fabs(needX) > 7.0) spd = std::min(spd, 3.0);
        wantVx = needX / dist * spd;
        wantVy = needY / dist * spd;
        preferNorth = dist < 30.0;
    }
    Screw cmd = screwCommand(heading_, wantVx, wantVy, preferNorth);
    steer = cmd.steer;
    if (dist < 3.5) steer *= 0.7;
    throttle_ = cmd.throttle;
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.08f;
}

void Game::horn(float seconds) {
    if (hornT_ < seconds) hornT_ = seconds;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 6);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    std::snprintf(why_, sizeof why_, "set down on the mark");
    chime(5);
    sys_->rumble(0.32f, 0.14f, 180);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.6f, 0.28f, 200);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.46f, 130.f, 0.4f);
    sys_->apu.tone(0, 74.f, 0.06f);
    tone0_ = 0.42f;
}

void Game::puff(Puff* ring, int& cursor, int n, double x, double y) {
    ring[cursor].x = x;
    ring[cursor].y = y;
    ring[cursor].life = 1;
    cursor = (cursor + 1) % n;
}

void Game::physics(double steer) {
    double rate = 1.12 + std::min(std::fabs(surge_), 12.0) * 0.05;
    heading_ = wrap(heading_ + steer * rate * kDt);
    double target = throttle_ >= 0 ? throttle_ * kAhead : throttle_ * kAstern;
    surge_ += (target - surge_) * (1.0 - std::exp(-2.5 * kDt));
    surge_ = clampd(surge_, -kAstern, kAhead);
    double c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * surge_ * kDt;
    y_ += (s * surge_ + kCurrent) * kDt;
    if (!std::isfinite(x_) || !std::isfinite(y_) || !std::isfinite(surge_)) {
        fail("missed the end");
        return;
    }

    auto thud = [&]() {
        if (thumpT_ > 0) return;
        sys_->apu.noiseBurst(0.24f, 190.f, 0.11f);
        thumpT_ = 0.28f;
        shake_ = std::max(shake_, 0.35f);
    };
    Ext e = extents();
    if (e.minX < -kWall) {
        x_ += -kWall - e.minX;
        surge_ *= 0.5;
        thud();
    }
    e = extents();
    if (e.maxX > kWall) {
        x_ += kWall - e.maxX;
        surge_ *= 0.5;
        thud();
    }
    e = extents();
    if (e.minY < kSouth) {
        y_ += kSouth - e.minY;
        if (s * surge_ + kCurrent < 0) surge_ *= 0.45;
        thud();
    }
    measure();
    e = extents();
    if (e.maxY > kEnd) {
        fail("missed the end");
        return;
    }

    if (bowOn_ && noseOk_ && groundSpeed() < 1.15 && std::fabs(throttle_ + kCurrent / kAstern) < 0.28) {
        double vy = std::sin(heading_) * surge_ + kCurrent;
        surge_ -= vy * std::sin(heading_) * std::min(1.0, 2.2 * kDt);
        heading_ = wrap(heading_ + wrap(kNorth - heading_) * std::min(1.0, 1.3 * kDt));
        measure();
    }

    double g = groundSpeed();
    bool closing = bowDist_ < prevBow_ - 0.004;
    prevBow_ = bowDist_;
    bool set = bowOn_ && noseOk_;
    if (set && g < kStop) {
        outT_ = 0;
        hold_ += kDt;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
    } else if (g < kStop && !closing) {
        hold_ = 0;
        outT_ += kDt;
        if (outT_ >= kOutNeed) {
            fail(stopWhy());
            return;
        }
    } else {
        hold_ = 0;
        if (g >= kStop || closing) outT_ = std::max(0.0, outT_ - kDt);
    }
    measure();

    if (bowOn_ && !announced_) {
        announced_ = true;
        horn(0.22f);
    }

    wakeT_ -= float(kDt);
    smokeT_ -= float(kDt);
    if (wakeT_ <= 0 && g > 1.5) {
        wakeT_ = 0.06f;
        puff(wake_, wakeCursor_, 16, x_ - c * 6.8, y_ - s * 6.8);
    }
    if (smokeT_ <= 0 && (std::fabs(throttle_) > 0.08 || std::fabs(surge_) > 0.7)) {
        smokeT_ = 0.1f;
        puff(smoke_, smokeCursor_, 8, x_ - c * 4.5, y_ - s * 4.5 + 1.2);
    }
    for (Puff& p : wake_)
        if (p.life > 0) p.life -= kDt * 0.55;
    for (Puff& p : smoke_)
        if (p.life > 0) {
            p.life -= kDt * 0.38;
            p.y += kDt * 1.4;
            p.x += kDt * 0.25;
        }

    if (bot_ && !bowOn_) {
        stuckT_ += kDt;
        if (stuckT_ > 2.4) {
            double moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0;
            if (moved < 1.4 && by_ < kEnd - 14.0) heading_ = std::atan2(kMarkY - by_, kMarkX - bx_);
        }
    }
}

void Game::audio() {
    float water = mode_ == Mode::Run ? 0.015f + float(std::fabs(surge_) * 0.00045) : 0.01f;
    sys_->apu.noise(water, 420.f + float(std::fabs(surge_)) * 10.f, false);
    if (hornT_ > 0) {
        hornT_ -= float(kDt);
        float v = hornT_ > 0.06f ? 0.07f : std::max(0.f, hornT_) * 1.05f;
        sys_->apu.tone(0, 92.f, v);
        if (tone1_ <= 0) sys_->apu.tone(1, 138.f, v * 0.45f);
    } else if (tone0_ <= 0 && chimeN_ == 0) {
        sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && (std::fabs(throttle_) > 0.03 || std::fabs(surge_) > 0.5)) {
        float wob = 0.74f + 0.26f * std::sin(float(t_) * (8.f + float(std::fabs(throttle_)) * 14.f));
        float vol = (0.012f + float(std::fabs(throttle_)) * 0.03f) * wob;
        float f = 38.f + float(std::fabs(throttle_)) * 28.f + float(std::fabs(surge_)) * 0.6f;
        sys_->apu.tone(2, f, vol);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (tone0_ > 0) {
        tone0_ -= float(kDt);
        if (tone0_ <= 0 && hornT_ <= 0) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0) {
        tone1_ -= float(kDt);
        if (tone1_ <= 0 && hornT_ <= 0) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (thumpT_ > 0) thumpT_ -= float(kDt);
    if (chimeN_ > 0 && hornT_ <= 0) {
        chimeT_ -= float(kDt);
        if (chimeT_ <= 0) {
            static const float notes[] = {392.f, 494.f, 587.3f, 784.f, 988.f};
            int n = std::min(chimeStep_, 4);
            sys_->apu.tone(0, notes[n], 0.055f);
            tone0_ = 0.16f;
            chimeT_ = 0.13f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - float(kDt) * 1.6f);
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(400.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            double steer = 0;
            if (bot_) pilot(steer);
            else controls(steer);
            physics(steer);
            if (mode_ == Mode::Run && race_ > 70.0) fail("the leg ran out");
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    if (mode_ == Mode::Win) sys.setLight(40, 180, 70);
    else if (mode_ == Mode::Fail) sys.setLight(180, 36, 24);
    else if (hold_ > 0.02) sys.setLight(200, 170, 40);
    else if (mode_ == Mode::Run) sys.setLight(30, 90, 160);
    camera();
    audio();
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = 0.f;
        camY_ = 118.f;
        zoom_ = 1.22f;
        return;
    }
    float lead = bowDist_ > 30.0 ? 12.f : 4.f;
    if (hold_ > 0.02 || mode_ == Mode::Win || mode_ == Mode::Fail) lead = 0.f;
    float tx = float(bx_ + std::cos(heading_) * lead * 0.25);
    float ty = float(by_ + std::sin(heading_) * lead);
    float tz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        tx = float(kMarkX);
        ty = float((kMarkY + kEnd) * 0.5);
        tz = 2.15f;
    }
    float k = 1.f - std::exp(-float(kDt) * 4.2f);
    camX_ += (tx - camX_) * k;
    camY_ += (ty - camY_) * k;
    zoom_ += (tz - zoom_) * k;
}

void Game::tintWater() {
    uint16_t hi = gs::rgb4(3, 8, 12);
    uint16_t lo = gs::rgb4(2, 5, 9);
    uint16_t spark = gs::rgb4(8, 13, 14);
    if (mode_ == Mode::Win) {
        hi = gs::rgb4(3, 10, 8);
        lo = gs::rgb4(2, 7, 6);
        spark = gs::rgb4(12, 15, 11);
    } else if (mode_ == Mode::Fail) {
        hi = gs::rgb4(8, 4, 5);
        lo = gs::rgb4(5, 2, 4);
        spark = gs::rgb4(13, 8, 8);
    }
    sys_->vdp.setColor(PAL_WATER * 16 + 11, hi);
    sys_->vdp.setColor(PAL_WATER * 16 + 12, lo);
    sys_->vdp.setColor(PAL_WATER * 16 + 13, spark);
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
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
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    if (cx + w * 0.5f < -4 || cy + h * 0.5f < -4 || cx - w * 0.5f > gs::SCREEN_W + 4 || cy - h * 0.5f > gs::SCREEN_H + 4)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(std::max(float(sw), float(sh)));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, double wx, double wy, float worldH, int pal, float minPx, bool flip) {
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    float sx = 160.f + float(wx - camX_) * zoom_;
    float sy = 112.f - float(wy - camY_) * zoom_;
    spr(m, sx, sy, h, pal, flip, false);
}

void Game::worldRect(const gs::Mipped& m, double wx, double wy, double ww, double hh, int pal) {
    float sx = 160.f + float(wx - camX_) * zoom_;
    float sy = 112.f - float(wy - camY_) * zoom_;
    sprBox(m, sx, sy, float(ww) * zoom_, float(hh) * zoom_, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    tintWater();

    float jx = 0, jy = 0;
    if (shake_ > 0) {
        jx = std::sin(float(t_) * 47.f) * shake_ * 3.5f;
        jy = std::cos(float(t_) * 39.f) * shake_ * 2.6f;
    }
    float invZ = 1.f / std::max(zoom_, 0.25f);
    camX_ -= jx * invZ;
    camY_ += jy * invZ;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) * invZ;
        bool basin = wy >= float(kSouth - 1.5) && wy <= float(kEnd + 0.8);
        uint16_t deep = gs::rgb4(1, 3, 6);
        uint16_t mid = gs::rgb4(2, 6, 10);
        float u = std::clamp((wy - 8.f) / 230.f, 0.f, 1.f);
        v.lineBackdrop[y] = lerpC(mid, deep, u);
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f + (0.f - camX_) * zoom_;
        r.hw = basin ? std::max(4.f, float(kWall) * zoom_) : 0.7f;
        r.v = wy * 30.f + float(t_) * 22.f;
        r.pal = uint8_t(PAL_WATER);
        r.band = (int(std::floor(wy * 0.18f + t_ * 1.1f)) & 1) ? 1 : 0;
        r.style = basin ? 2 : 0;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }

    auto banner = [&](const gs::Mipped& m, float y, int pal) { spr(m, 160.f, y, float(m.h), pal, false, false); };
    if (mode_ == Mode::Title) banner(art_.title, 16.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Win) banner(art_.setDown, 24.f, PAL_WIN);
    else if (mode_ == Mode::Fail) {
        if (!std::strcmp(why_, "missed the end")) banner(art_.missed, 26.f, PAL_ALERT);
        else if (!std::strcmp(why_, "stopped short of the mark")) banner(art_.shortB, 26.f, PAL_ALERT);
        else if (!std::strcmp(why_, "stopped long of the mark")) banner(art_.longB, 26.f, PAL_ALERT);
        else if (!std::strcmp(why_, "the leg ran out")) banner(art_.late, 26.f, PAL_ALERT);
        else banner(art_.off, 26.f, PAL_ALERT);
    }

    if (mode_ != Mode::Title) {
        auto chart = [&](double wx, double wy, float h, int pal) {
            float sx = 292.f + float(wx) * 1.05f;
            float sy = 34.f - float(wy - kMarkY) * 0.38f;
            spr(art_.pin, sx, sy, h, pal, false, false);
        };
        chart(kMarkX, kMarkY, 5.f, PAL_MARK);
        chart(-kWall + 2, kEnd, 3.5f, PAL_ALERT);
        chart(kWall - 2, kEnd, 3.5f, PAL_ALERT);
        chart(x_, y_, 4.5f, PAL_BANNER);
        float mx = 160.f + float(kMarkX - camX_) * zoom_;
        float my = 112.f - float(kMarkY - camY_) * zoom_;
        if (mx < 14.f || mx > 306.f || my < 16.f || my > 208.f) {
            float dx = mx - 160.f, dy = my - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 136.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 84.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 11.f, PAL_MARK, false, false);
        }
    }

    int pipPal = mode_ == Mode::Win || hold_ > 0.02 ? PAL_WIN : mode_ == Mode::Fail ? PAL_ALERT : PAL_MARK;
    place(art_.pip, bx_, by_, 1.35f, pipPal, mode_ == Mode::Title ? 6.f : 0);

    int fi = hullFrame();
    float bob = std::sin(float(t_) * 2.1f + float(x_) * 0.04f) * 0.35f;
    float bh = float(art_.tug[fi].h) / kPixelsPerMetre * zoom_;
    if (mode_ == Mode::Title) bh = std::max(bh, 22.f);
    float bsx = 160.f + float(x_ - camX_) * zoom_;
    float bsy = 112.f - float(y_ - camY_) * zoom_ + bob * zoom_ * 0.12f;
    spr(art_.shade, bsx + 3.f, bsy + 4.f, bh * 0.7f, PAL_TUG, false, true);
    spr(art_.tug[fi], bsx, bsy, bh, PAL_TUG, false, false);

    if (groundSpeed() > 2.0 && mode_ != Mode::Title) {
        double c = std::cos(heading_), s = std::sin(heading_);
        place(art_.foam, x_ + c * 7.4, y_ + s * 7.4, 2.1f, PAL_FOAM, 0);
        place(art_.foam, x_ - c * 6.6, y_ - s * 6.6, 2.4f, PAL_FOAM, 0);
    }
    for (const Puff& p : smoke_) {
        if (p.life <= 0) continue;
        float h = (2.0f + float(1.0 - p.life) * 3.0f) * (zoom_ / kPlayZoom);
        float sx = 160.f + float(p.x - camX_) * zoom_;
        float sy = 112.f - float(p.y - camY_) * zoom_;
        spr(art_.smoke, sx, sy, std::max(2.f, h), PAL_SMOKE, false, false);
    }
    for (const Puff& p : wake_) {
        if (p.life <= 0) continue;
        float h = (1.6f + float(1.0 - p.life) * 2.2f) * zoom_;
        float sx = 160.f + float(p.x - camX_) * zoom_;
        float sy = 112.f - float(p.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false, false);
    }

    int flap = int(t_ * 3.0) & 1;
    const float gullMin = mode_ == Mode::Title ? 7.f : 0.f;
    place(art_.gull[flap], -12 + std::sin(t_ * 0.33) * 10, 96 + std::cos(t_ * 0.19) * 6, 3.8f, PAL_GULL, gullMin);
    place(art_.gull[1 - flap], 14 + std::cos(t_ * 0.27) * 8, kMarkY + 6 + std::sin(t_ * 0.17) * 4, 3.2f, PAL_GULL, 0);

    int markPal = mode_ == Mode::Win || hold_ > 0.02 ? PAL_WIN : mode_ == Mode::Fail ? PAL_ALERT : PAL_MARK;
    double dia = double(kMarkBmp) * (kPaintR / double(kPaintPx));
    worldRect(art_.mark, kMarkX, kMarkY, dia, dia, markPal);
    place(art_.dolphin, -12.2, kMarkY + 2.0, 8.f, PAL_MARK, mode_ == Mode::Title ? 8.f : 0);
    place(art_.dolphin, 12.2, kMarkY + 2.0, 8.f, PAL_MARK, mode_ == Mode::Title ? 8.f : 0);
    place(art_.dolphin, -12.2, kMarkY - 14.0, 7.f, PAL_QUAY, 0);
    place(art_.dolphin, 12.2, kMarkY - 14.0, 7.f, PAL_QUAY, 0);

    worldRect(art_.boom, 0, kEnd, kWall * 2.0 - 2.0, 1.35, PAL_END);
    for (double x = -16; x <= 16.1; x += 8) place(art_.buoy, x, kEnd + 0.2, 4.8f, PAL_END, mode_ == Mode::Title ? 7.f : 0);
    worldRect(art_.bulk, 0, kEnd + 4.2, kWall * 2.0 + 8.0, 5.2, PAL_QUAY);
    worldRect(art_.bulk, 0, kSouth - 3.4, kWall * 2.0 + 6.0, 4.6, PAL_QUAY);
    place(art_.crane, 30.5, kEnd + 2.0, 16.f, PAL_QUAY, mode_ == Mode::Title ? 12.f : 0);
    place(art_.crane, -31.0, kMarkY + 8.0, 14.f, PAL_QUAY, mode_ == Mode::Title ? 10.f : 0, true);

    for (double y = 24; y <= 214; y += 13) {
        place(art_.quay, -(kWall + 3.6), y, 11.f, PAL_QUAY, 0, false);
        place(art_.quay, kWall + 3.6, y, 11.f, PAL_QUAY, 0, true);
    }
    const double sheds[][2] = {{-34, 36}, {36, 78}, {-33, 128}, {35, 188}};
    for (const double* s : sheds) place(art_.shed, s[0], s[1], 12.f, PAL_QUAY, mode_ == Mode::Title ? 9.f : 0);
    const double lamps[] = {52, 100, 148, 196};
    for (double y : lamps) {
        place(art_.lamp, -(kWall + 0.2), y, 6.5f, PAL_LAMP, 0);
        place(art_.lamp, kWall + 0.2, y, 6.5f, PAL_LAMP, 0);
    }

    camX_ += jx * invZ;
    camY_ -= jy * invZ;

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(21, "SET THE BOW DOWN ON THE MARK", PAL_BANNER);
        hudC(22, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        hudC(23, "CLOSE TO THE MARK IS STILL OFF IT", PAL_HUD);
        hudC(24, "A FLOOD SETS YOU TOWARD THE END", PAL_HUD);
        if ((int(t_ * 2.0) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "ARROWS STEER   UP AHEAD   DOWN ASTERN", PAL_HUD);
        hudC(27, "SPACE HORN", PAL_HUD);
        return;
    }

    hud(1, 0, "S3 TUGBOAT MARK", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "LEG %5.1fS", race_);
    hud(29, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        hudC(19, "ESC BACK TO THE DOCK", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(15, "THE BOW IS ON THE MARK", PAL_WIN);
        hudC(16, "THE LEG IS MADE", PAL_BANNER);
        std::snprintf(buf, sizeof buf, "%.1fS", race_);
        hudC(18, buf, PAL_HUD);
        if (!bot_) hudC(20, "START RUNS THE LEG AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, "THE LEG FAILS", PAL_ALERT);
        if (!std::strcmp(why_, "missed the end")) hudC(17, "YOU CROSSED THE END", PAL_HUD);
        else if (!std::strcmp(why_, "stopped short of the mark")) hudC(17, "SHORT OF THE MARK", PAL_HUD);
        else if (!std::strcmp(why_, "stopped long of the mark")) hudC(17, "LONG OF THE MARK", PAL_HUD);
        else if (!std::strcmp(why_, "stopped wide of the mark")) hudC(17, "WIDE OF THE MARK", PAL_HUD);
        else if (!std::strcmp(why_, "nose is not set on the mark")) hudC(17, "SQUARE THE NOSE IN THE HEART", PAL_HUD);
        else if (!std::strcmp(why_, "the leg ran out")) hudC(17, "THE LEG RAN OUT", PAL_HUD);
        else hudC(17, "CLOSE TO THE MARK IS STILL OFF IT", PAL_HUD);
        if (!bot_) hudC(19, "START TRIES THE LEG AGAIN", PAL_HUD);
        return;
    }

    Ext e = extents();
    double toEnd = kEnd - e.maxY;
    double g = groundSpeed();
    std::snprintf(buf, sizeof buf, "BOW %3.0f", std::max(0.0, bowDist_));
    hud(1, 1, buf, bowOn_ ? PAL_WIN : bowDist_ < kPaintR ? PAL_BANNER : PAL_HUD);
    std::snprintf(buf, sizeof buf, "END %3.0f", std::max(0.0, toEnd));
    hud(10, 1, buf, toEnd < 16 ? PAL_ALERT : PAL_MARK);
    std::snprintf(buf, sizeof buf, "ENG %+4d  SPD %4.1f", int(std::lround(throttle_ * 100.0)), g);
    hud(20, 1, buf, PAL_HUD);
    if (bowOn_ && noseOk_ && g < kStop) {
        int n = std::clamp(int(hold_ / kHoldNeed * 5.0) + 1, 1, 5);
        std::snprintf(buf, sizeof buf, "SET %d/5", n);
        hud(1, 2, buf, PAL_WIN);
    } else {
        hud(1, 2, hint(), bowOn_ ? PAL_BANNER : PAL_HUD);
    }
    if (race_ < 6.0) hud(1, 26, "FLOOD ASTERN  ABOUT ENG -36 HOLDS", PAL_MARK);
    else if (bowDist_ < 20.0) hud(1, 26, "HEART OF THE PAINT, THEN HOLD STILL", PAL_HUD);
    else hud(1, 26, "MISSING THE END FAILS THE LEG", PAL_ALERT);
    hud(1, 27, "ARROWS STEER  UP AHEAD  DOWN ASTERN", PAL_HUD);
}

}  // namespace tugmark
