#include "lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tuglane {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kPi = 3.141592653589793;
constexpr double kTau = 6.283185307179586;
constexpr double kLeg = 320.0;
constexpr double kClock = 78.0;
constexpr double kStartY = 22.0;
constexpr double kStartLine = 12.0;
constexpr double kHalfL = 6.4;
constexpr double kHalfW = 2.7;
constexpr double kAhead = 9.0;
constexpr double kAstern = 4.6;
constexpr double kArtScale = 5.0;
constexpr float kPlayZoom = 3.25f;
constexpr int kTugFrames = 16;

double wrap(double a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

double smooth01(double t) {
    t = clampd(t, 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

double rawCenter(double y) { return 14.0 * std::sin(y * 0.020) + 6.0 * std::sin(y * 0.046 + 0.7); }

double laneCenter(double y) {
    double s = rawCenter(y) * smooth01(y / 46.0);
    const double at = kLeg - 64.0;
    if (y > at) {
        double hold = rawCenter(at) * smooth01(at / 46.0);
        double t = smooth01((y - at) / 52.0);
        s = s * (1.0 - t) + hold * t;
    }
    return s;
}

double laneHalf(double y) {
    double h = 17.2;
    auto pinch = [&](double at, double wid, double depth) {
        double d = (y - at) / wid;
        h -= depth * std::exp(-d * d);
    };
    pinch(108.0, 26.0, 1.7);
    pinch(188.0, 22.0, 2.0);
    pinch(258.0, 24.0, 1.6);
    if (y < 42.0) {
        double t = clampd(y / 42.0, 0.0, 1.0);
        h = 22.0 * (1.0 - t) + h * t;
    }
    if (y > kLeg - 50.0) {
        double t = smooth01((y - (kLeg - 50.0)) / 50.0);
        h = h * (1.0 - t) + 16.4 * t;
    }
    return h;
}

double currentX(double y) {
    double env = smooth01((y - 30.0) / 40.0) * (1.0 - smooth01((y - (kLeg - 78.0)) / 46.0));
    return env * (1.65 * std::sin(y * 0.019 + 0.4) + 0.7 * std::sin(y * 0.043));
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

float Game::speed() const {
    double vx = std::cos(heading_) * surge_ + currentX(y_);
    double vy = std::sin(heading_) * surge_;
    return float(std::hypot(vx, vy));
}

float Game::lateral() const { return float(x_ - laneCenter(y_)); }

double Game::cornerClearance() const {
    const double c = std::cos(heading_), s = std::sin(heading_);
    const double fs[3] = {-kHalfL, 0.0, kHalfL};
    const double ws[2] = {-kHalfW, kHalfW};
    double best = 1e9;
    for (double f : fs) {
        for (double w : ws) {
            double wx = x_ + f * c + w * s;
            double wy = y_ + f * s - w * c;
            double lim = wy >= kLeg ? laneHalf(kLeg) : laneHalf(wy);
            double lat = wx - (wy >= kLeg ? laneCenter(kLeg) : laneCenter(wy));
            best = std::min(best, lim - std::fabs(lat));
        }
    }
    return best;
}

float Game::clearance() const { return float(cornerClearance()); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (kLeg - y_ < 52.0) return 3;
    if (laneHalf(y_) - std::fabs(x_ - laneCenter(y_)) < laneHalf(y_) * 0.42) return 2;
    return 1;
}

int Game::hullFrame() const {
    double u = std::fmod(heading_, kTau);
    if (u < 0) u += kTau;
    int i = int(std::lround(u / kTau * kTugFrames)) % kTugFrames;
    if (i < 0) i += kTugFrames;
    return i;
}

void Game::begin() {
    y_ = kStartY;
    x_ = laneCenter(y_);
    double look = 16.0;
    double dx = laneCenter(y_ + look) - x_;
    heading_ = std::atan2(look, dx);
    surge_ = 0;
    throttle_ = 0;
    yaw_ = 0;
    race_ = 0;
    won_ = false;
    over_ = false;
    warned_ = false;
    sawGate_ = false;
    why_ = "";
    chimeN_ = 0;
    chimeStep_ = 0;
    chimeT_ = 0;
    wakeCursor_ = 0;
    smokeCursor_ = 0;
    wakeT_ = 0;
    smokeT_ = 0;
    hornT_ = 0;
    tone0_ = 0;
    tone1_ = 0;
    shake_ = 0;
    for (Puff& p : wake_) p = {};
    for (Puff& p : smoke_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = float(laneCenter(86.0));
    camY_ = 78.f;
    zoom_ = 2.15f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = float(x_);
    camY_ = float(y_ + 12.0);
    zoom_ = kPlayZoom;
    horn(0.32f);
    blip(520.f);
}

void Game::buildCourse() {
    props_.clear();
    auto add = [&](double x, double y, float h, Kind k, int pal, bool flip = false) {
        props_.push_back(Prop{x, y, h, k, pal, flip});
    };
    for (double y = 20.0; y < kLeg - 8.0; y += 18.0) {
        double c = laneCenter(y);
        double w = laneHalf(y);
        add(c - w + 1.5, y, 4.4f, Kind::BuoyR, PAL_RED);
        add(c + w - 1.5, y, 4.4f, Kind::BuoyG, PAL_GREEN);
    }
    for (double y = 16.0; y < kLeg - 4.0; y += 26.0) {
        double c = laneCenter(y);
        double w = laneHalf(y);
        add(c - w - 1.3, y, 3.1f, Kind::Bollard, PAL_MARK);
        add(c + w + 1.3, y, 3.1f, Kind::Bollard, PAL_MARK);
    }
    for (double y = 28.0; y < kLeg + 6.0; y += 34.0) {
        double c = laneCenter(y);
        double w = laneHalf(y);
        add(c - w - 2.4, y, 6.2f, Kind::Lamp, PAL_LAMP);
        add(c + w + 2.4, y, 6.2f, Kind::Lamp, PAL_LAMP);
    }
    const double sheds[] = {48, 132, 214, 292};
    for (int i = 0; i < 4; i++) {
        double y = sheds[i];
        double c = laneCenter(y);
        double w = laneHalf(y);
        bool left = (i & 1) == 0;
        add(c + (left ? -(w + 11.0) : (w + 11.0)), y, i == 1 || i == 2 ? 12.f : 9.5f, (i & 2) ? Kind::House : Kind::Shed,
            PAL_QUAY, !left);
        add(c + (left ? -(w + 6.5) : (w + 6.5)), y + 8.0, 4.2f, Kind::Crate, PAL_QUAY, false);
    }
    double cs = laneCenter(kStartLine);
    double ws = laneHalf(kStartLine);
    add(cs - ws, kStartLine, 7.2f, Kind::Post, PAL_MARK);
    add(cs + ws, kStartLine, 7.2f, Kind::Post, PAL_MARK);
    double cg = laneCenter(kLeg);
    double wg = laneHalf(kLeg);
    add(cg - wg, kLeg, 8.4f, Kind::Post, PAL_END);
    add(cg + wg, kLeg, 8.4f, Kind::Post, PAL_END);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.11f, 0.16f, 0.09f);
    t_ = 0;
    buildCourse();
    if (bot_) startRun();
    else showTitle();
}

void Game::controls(double& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    if (p.down(gs::BTN_LEFT)) steer += 1;
    if (p.down(gs::BTN_RIGHT)) steer -= 1;
    if (std::fabs(p.axisX) > 0.16f) steer = clampd(double(-p.axisX), -1.0, 1.0);
    const bool ahead = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A);
    const bool astern = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    if (ahead) throttle_ = std::min(1.0, throttle_ + kDt * 0.85);
    if (astern) throttle_ = std::max(-1.0, throttle_ - kDt * 0.75);
    if (p.accel > 0.08f) throttle_ = std::max(throttle_, double(p.accel));
    if (p.brake > 0.08f) throttle_ = std::min(throttle_, -double(p.brake));
    if (p.down(gs::BTN_TURBO)) horn(0.12f);
}

void Game::pilot(double& steer) {
    const double north = kPi * 0.5;
    const double distEnd = kLeg + kHalfL - y_;
    double look = 18.0;
    if (distEnd < 40.0) look = std::max(8.0, distEnd * 0.7);
    double aimX = laneCenter(y_ + look);
    double aimY = y_ + look;
    double off = x_ - laneCenter(y_);
    double dx = aimX - x_ - off * 0.35;
    double dy = aimY - y_;
    double clear = cornerClearance();
    double V = distEnd < 28.0 ? 6.2 : 6.9;
    if (clear < 4.2) {
        V = 4.4;
        dx = -off * 2.6;
        dy = 9.0;
    }
    double dist = std::max(0.5, std::hypot(dx, dy));
    double wantVx = dx / dist * V;
    double wantVy = dy / dist * V;
    double set = currentX(y_) * 0.4 + currentX(y_ + look * 0.5) * 0.6;
    double sx = wantVx - set;
    double sy = wantVy;
    if (sy < 1.2) sy = 1.2;
    double hdes = std::atan2(sy, sx);
    double crab = clampd(wrap(hdes - north), -0.72, 0.72);
    hdes = north + crab;
    double herr = wrap(hdes - heading_);
    steer = clampd(herr / 0.30, -1.0, 1.0);
    double u = std::hypot(sx, sy);
    if (std::fabs(herr) > 0.5) u *= 0.65;
    throttle_ = clampd(u / kAhead, 0.28, 0.92);
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
    why_ = "held";
    chime(4);
    sys_->rumble(0.32f, 0.14f, 180);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    shake_ = 1.f;
    sys_->rumble(0.55f, 0.22f, 190);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.44f, 120.f, 0.4f);
    sys_->apu.tone(0, 74.f, 0.06f);
    tone0_ = 0.42f;
}

void Game::puff(Puff* ring, int& cursor, int n, double x, double y) {
    ring[cursor].x = x;
    ring[cursor].y = y;
    ring[cursor].life = 1;
    cursor = (cursor + 1) % n;
}

void Game::stackAt(double& sx, double& sy) const {
    const double c = std::cos(heading_), s = std::sin(heading_);
    sx = x_ + (-3.15) * c;
    sy = y_ + (-3.15) * s;
}

void Game::judge() {
    const double c = std::cos(heading_), s = std::sin(heading_);
    const double fs[3] = {-kHalfL, 0.0, kHalfL};
    const double ws[3] = {-kHalfW, 0.0, kHalfW};
    const double gateC = laneCenter(kLeg);
    const double gateH = laneHalf(kLeg);
    bool allThrough = true;
    bool left = false;
    bool missed = false;
    for (double f : fs) {
        for (double w : ws) {
            if (f == 0.0 && w == 0.0) continue;
            double wx = x_ + f * c + w * s;
            double wy = y_ + f * s - w * c;
            if (wy < kLeg) {
                allThrough = false;
                if (wy < 0.0 || std::fabs(wx - laneCenter(wy)) > laneHalf(wy) + 0.04) left = true;
            } else if (std::fabs(wx - gateC) > gateH + 0.04) {
                missed = true;
            }
        }
    }
    if (left) fail("left the lane");
    else if (missed) fail("missed the end");
    else if (allThrough) win();
}

void Game::physics(double steer) {
    steer = clampd(steer, -1.0, 1.0);
    double rate = 0.95 + std::min(std::fabs(surge_), 9.0) * 0.035;
    double yawCmd = steer * rate;
    yaw_ += (yawCmd - yaw_) * (1.0 - std::exp(-8.0 * kDt));
    heading_ = wrap(heading_ + yaw_ * kDt);
    double target = throttle_ >= 0 ? throttle_ * kAhead : throttle_ * kAstern;
    surge_ += (target - surge_) * (1.0 - std::exp(-2.5 * kDt));
    surge_ = clampd(surge_, -kAstern, kAhead);
    double c = std::cos(heading_), s = std::sin(heading_);
    x_ += (c * surge_ + currentX(y_)) * kDt;
    y_ += s * surge_ * kDt;
    if (!std::isfinite(x_) || !std::isfinite(y_) || !std::isfinite(surge_)) {
        fail("left the lane");
        return;
    }
    judge();
    if (mode_ != Mode::Run) return;

    double clear = cornerClearance();
    if (clear < 2.6 && !warned_) {
        warned_ = true;
        blip(170.f);
        sys_->rumble(0.25f, 0.05f, 80);
    } else if (clear > 3.8) {
        warned_ = false;
    }
    if (!sawGate_ && kLeg - y_ < 56.0) {
        sawGate_ = true;
        blip(720.f);
    }

    double g = speed();
    wakeT_ -= float(kDt);
    smokeT_ -= float(kDt);
    if (wakeT_ <= 0 && g > 1.4) {
        wakeT_ = 0.07f;
        puff(wake_, wakeCursor_, 16, x_ - c * (kHalfL + 0.3), y_ - s * (kHalfL + 0.3));
    }
    if (smokeT_ <= 0 && (std::fabs(throttle_) > 0.05 || std::fabs(surge_) > 0.6)) {
        smokeT_ = 0.1f;
        double sx, sy;
        stackAt(sx, sy);
        puff(smoke_, smokeCursor_, 8, sx, sy);
    }
    for (Puff& p : wake_)
        if (p.life > 0) {
            p.life -= kDt * 0.7;
            p.x += currentX(p.y) * kDt * 0.25;
        }
    for (Puff& p : smoke_)
        if (p.life > 0) {
            p.life -= kDt * 0.42;
            p.y += kDt * 1.3;
            p.x += kDt * 0.35;
        }
}

void Game::audio() {
    float bed = mode_ == Mode::Run ? 0.016f + float(std::fabs(surge_)) * 0.0012f : 0.01f;
    sys_->apu.noise(bed, 420.f + float(std::fabs(surge_)) * 14.f, false);
    if (hornT_ > 0) {
        hornT_ -= float(kDt);
        float v = hornT_ > 0.05f ? 0.07f : std::max(0.f, hornT_) * 1.2f;
        sys_->apu.tone(0, 104.f, v);
        if (tone1_ <= 0) sys_->apu.tone(1, 156.f, v * 0.45f);
    } else if (tone0_ <= 0 && chimeN_ == 0) {
        sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && (std::fabs(throttle_) > 0.03 || std::fabs(surge_) > 0.5)) {
        float wob = 0.74f + 0.26f * std::sin(float(t_) * (9.f + float(std::fabs(throttle_)) * 14.f));
        float vol = (0.012f + float(std::fabs(throttle_)) * 0.026f) * wob;
        float f = 42.f + float(std::fabs(throttle_)) * 30.f + float(std::fabs(surge_)) * 0.6f;
        sys_->apu.tone(2, f, vol);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (tone0_ > 0) {
        tone0_ -= float(kDt);
        if (tone0_ <= 0 && hornT_ <= 0 && chimeN_ == 0) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0) {
        tone1_ -= float(kDt);
        if (tone1_ <= 0 && hornT_ <= 0) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (chimeN_ > 0 && hornT_ <= 0) {
        chimeT_ -= float(kDt);
        if (chimeT_ <= 0) {
            static const float notes[] = {392.f, 494.f, 587.f, 784.f};
            int n = std::min(chimeStep_, 3);
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.16f;
            chimeT_ = 0.14f;
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
        if (pad.down(gs::BTN_TURBO)) horn(0.12f);
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
        if (smokeT_ <= 0) {
            smokeT_ = 0.18f;
            double sx, sy;
            stackAt(sx, sy);
            puff(smoke_, smokeCursor_, 8, sx, sy);
        } else {
            smokeT_ -= float(kDt);
        }
        for (Puff& p : smoke_)
            if (p.life > 0) {
                p.life -= kDt * 0.35;
                p.y += kDt * 0.8;
            }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(400.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            if (race_ > kClock) {
                fail("the leg ran out");
            } else {
                double steer = 0;
                if (bot_) pilot(steer);
                else controls(steer);
                physics(steer);
            }
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
    else if (mode_ == Mode::Run && cornerClearance() < 2.8) sys.setLight(200, 150, 30);
    else if (mode_ == Mode::Run) sys.setLight(30, 90, 150);
    else sys.setLight(40, 70, 90);
    camera();
    audio();
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = float(laneCenter(86.0));
        camY_ = 78.f;
        zoom_ = 2.15f;
        return;
    }
    float lead = 13.f;
    if (kLeg - y_ < 36.0) lead = std::max(2.f, float(kLeg - y_) * 0.28f);
    if (mode_ == Mode::Win || mode_ == Mode::Fail) lead = 0.f;
    float tx = float(x_);
    float ty = float(y_ + lead);
    float tz = (mode_ == Mode::Win || mode_ == Mode::Fail) ? 2.45f : kPlayZoom;
    float k = 1.f - std::exp(-float(kDt) * 4.6f);
    camX_ += (tx - camX_) * k;
    camY_ += (ty - camY_) * k;
    zoom_ += (tz - zoom_) * k;
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

    float shakeX = 0, shakeY = 0;
    if (shake_ > 0) {
        shakeX = std::sin(float(t_) * 47.f) * shake_ * 3.4f;
        shakeY = std::cos(float(t_) * 38.f) * shake_ * 2.6f;
    }
    float invZ = 1.f / std::max(zoom_, 0.25f);
    const uint16_t bank = gs::rgb4(5, 5, 4);
    const uint16_t deep = gs::rgb4(1, 5, 8);
    for (int sy = 0; sy < gs::SCREEN_H; sy++) {
        float wy = camY_ + (112.f + shakeY - float(sy)) * invZ;
        float cx = float(laneCenter(wy));
        float hw = float(laneHalf(wy));
        v.lineBackdrop[sy] = lerpC(bank, deep, 0.35f);
        v.lineFog[sy] = 0;
        gs::RoadLine& r = v.road[sy];
        r.on = true;
        r.cx = 160.f + (cx - camX_) * zoom_ + shakeX;
        r.hw = std::max(3.f, hw * zoom_);
        r.v = wy * 22.f + float(t_) * 34.f;
        r.pal = uint8_t(PAL_LANE);
        r.band = (int(std::floor(wy * 0.22)) & 1) ? 1 : 0;
        r.style = 2;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }

    auto toScreen = [&](double wx, double wy, float& sx, float& sy) {
        sx = 160.f + float(wx - camX_) * zoom_ + shakeX;
        sy = 112.f - float(wy - camY_) * zoom_ + shakeY;
    };

    const gs::Mipped* glyph = nullptr;
    auto kindSprite = [&](Kind k) -> const gs::Mipped* {
        switch (k) {
        case Kind::BuoyR: return &art_.buoyR;
        case Kind::BuoyG: return &art_.buoyG;
        case Kind::Shed: return &art_.shed;
        case Kind::House: return &art_.house;
        case Kind::Crate: return &art_.crate;
        case Kind::Bollard: return &art_.bollard;
        case Kind::Lamp: return &art_.lamp;
        case Kind::Post: return &art_.post;
        }
        return &art_.post;
    };
    (void)glyph;

    // World first so the tug, drawn earlier, stays on top.
    for (const Prop& p : props_) {
        double wy = p.y;
        if (p.kind == Kind::BuoyR || p.kind == Kind::BuoyG) wy += std::sin(t_ * 1.6 + p.y * 0.17) * 0.12;
        float sx, sy;
        toScreen(p.x, wy, sx, sy);
        float h = p.h * zoom_;
        float minPx = (p.kind == Kind::Shed || p.kind == Kind::House) ? 11.f : (p.kind == Kind::BuoyR || p.kind == Kind::BuoyG) ? 8.f : 0.f;
        if (h < minPx) h = minPx;
        spr(*kindSprite(p.kind), sx, sy, h, p.pal, p.flip, false);
    }

    double c0 = laneCenter(kStartLine);
    double w0 = laneHalf(kStartLine);
    {
        float sx, sy;
        toScreen(c0, kStartLine, sx, sy);
        sprBox(art_.line, sx, sy, float(w0 * 1.7) * zoom_, std::max(2.f, 0.55f * zoom_), PAL_MARK);
    }
    double c1 = laneCenter(kLeg);
    double w1 = laneHalf(kLeg);
    {
        float sx, sy;
        toScreen(c1, kLeg, sx, sy);
        sprBox(art_.line, sx, sy, float(w1 * 1.86) * zoom_, std::max(2.f, 0.72f * zoom_), PAL_END);
        toScreen(c1, kLeg + 9.0, sx, sy);
        spr(art_.gate, sx, sy, std::max(14.f, 7.5f * zoom_), PAL_END, false, false);
    }

    for (const Puff& p : wake_) {
        if (p.life <= 0) continue;
        float sx, sy;
        toScreen(p.x, p.y, sx, sy);
        float h = (1.7f + float(1.0 - p.life) * 2.2f) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false, false);
    }
    for (const Puff& p : smoke_) {
        if (p.life <= 0) continue;
        float sx, sy;
        toScreen(p.x, p.y, sx, sy);
        float h = (2.0f + float(1.0 - p.life) * 2.8f) * (zoom_ / kPlayZoom);
        spr(art_.smoke, sx, sy, std::max(2.f, h), PAL_SMOKE, false, false);
    }

    int flap = int(t_ * 3.0) & 1;
    for (int i = 0; i < 3; i++) {
        double gy = std::fmod(40.0 + i * 78.0 + t_ * (6.0 + i), kLeg + 20.0);
        double gx = laneCenter(gy) + ((i & 1) ? 1.0 : -1.0) * (laneHalf(gy) + 5.5 + std::sin(t_ * 0.7 + i) * 1.4);
        float sx, sy;
        toScreen(gx, gy, sx, sy);
        spr(art_.gull[flap ^ (i & 1)], sx, sy, 11.f, PAL_GULL, i == 2, false);
    }

    int fi = hullFrame();
    float bob = std::sin(float(t_) * 2.2f) * 0.35f;
    float bh = float(art_.tug[fi].h) / kArtScale * zoom_;
    float bsx, bsy;
    toScreen(x_, y_, bsx, bsy);
    bsy += bob * zoom_ * 0.12f;
    spr(art_.shade, bsx + 3.f, bsy + 4.f, bh * 0.55f, PAL_TUG, false, true);
    spr(art_.tug[fi], bsx, bsy, bh, PAL_TUG, false, false);

    auto banner = [&](const gs::Mipped& m, float y, int pal) { spr(m, 160.f, y, float(m.h), pal, false, false); };
    if (mode_ == Mode::Title) banner(art_.title, 16.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Win) banner(art_.held, 26.f, PAL_WIN);
    else if (mode_ == Mode::Fail) {
        if (!std::strcmp(why_, "missed the end")) banner(art_.missed, 28.f, PAL_ALERT);
        else if (!std::strcmp(why_, "the leg ran out")) banner(art_.ranout, 28.f, PAL_ALERT);
        else banner(art_.left, 28.f, PAL_ALERT);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(21, "STAY IN THE LANE", PAL_BANNER);
        hudC(22, "THE WHOLE LEG", PAL_HUD);
        hudC(23, "POINT INTO THE CROSS-SET", PAL_MARK);
        hudC(24, "LEAVING THE LANE FAILS IT", PAL_ALERT);
        if ((int(t_ * 2.0) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "ARROWS STEER   UP AHEAD   DOWN ASTERN", PAL_HUD);
        hudC(27, "SPACE HORN", PAL_HUD);
        return;
    }

    hud(1, 0, "S3 TUGBOAT LANE", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "LEG %4.1fS", race_);
    hud(29, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        hudC(19, "ESC BACK TO THE DOCK", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(15, "STAYED IN THE LANE", PAL_WIN);
        hudC(16, "THE WHOLE LEG", PAL_BANNER);
        std::snprintf(buf, sizeof buf, "%.1fS", race_);
        hudC(18, buf, PAL_HUD);
        if (!bot_) hudC(20, "START RUNS THE LEG AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, "THE LEG FAILS", PAL_ALERT);
        if (!std::strcmp(why_, "missed the end")) hudC(17, "YOU MISSED THE GATE", PAL_HUD);
        else if (!std::strcmp(why_, "the leg ran out")) hudC(17, "THE LEG RAN OUT", PAL_HUD);
        else hudC(17, "YOU LEFT THE LANE", PAL_HUD);
        if (!bot_) hudC(19, "START TRIES THE LEG AGAIN", PAL_HUD);
        return;
    }

    double off = x_ - laneCenter(y_);
    double half = laneHalf(y_);
    double toEnd = std::max(0.0, kLeg - y_);
    double set = currentX(y_);
    int palEdge = cornerClearance() < 3.0 ? PAL_ALERT : PAL_HUD;
    std::snprintf(buf, sizeof buf, "END %3.0f  OFF %+4.1f  SET %+4.1f", toEnd, off, set);
    hud(1, 1, buf, palEdge);
    int cells = 11;
    double u = clampd((off / std::max(1.0, half) + 1.0) * 0.5, 0.0, 1.0);
    int at = std::clamp(int(std::lround(u * (cells - 1))), 0, cells - 1);
    char bar[20];
    bar[0] = '[';
    for (int i = 0; i < cells; i++) bar[i + 1] = (i == at) ? 'O' : '.';
    bar[cells + 1] = ']';
    bar[cells + 2] = 0;
    std::snprintf(buf, sizeof buf, "LANE %s", bar);
    hud(1, 2, buf, palEdge);
    std::snprintf(buf, sizeof buf, "ENG %+4d  SPD %4.1f", int(std::lround(throttle_ * 100.0)), speed());
    hud(1, 3, buf, PAL_HUD);
    double left = kClock - race_;
    if (left < 18.0) {
        std::snprintf(buf, sizeof buf, "TIME %3.0f", std::max(0.0, left));
        hud(30, 3, buf, PAL_ALERT);
    }
    if (race_ < 4.5) hud(1, 26, "POINT THE BOW INTO THE SET", PAL_MARK);
    else if (cornerClearance() < 3.0) hud(1, 26, "CLOSE TO THE LANE EDGE", PAL_ALERT);
    else if (toEnd < 52.0) hud(1, 26, "HOLD THE LANE THROUGH THE GATE", PAL_BANNER);
    else hud(1, 26, "STAY IN THE LANE THE WHOLE LEG", PAL_HUD);
    hud(1, 27, "ARROWS STEER  UP AHEAD  DOWN ASTERN", PAL_HUD);
}

}  // namespace tuglane
