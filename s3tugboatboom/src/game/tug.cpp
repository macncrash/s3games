#include "tug.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tugboom {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kPi = 3.141592653589793;
constexpr double kTau = 6.283185307179586;
constexpr double kNorth = kPi * 0.5;

constexpr double kWall = 26.0;
constexpr double kSouth = 14.0;
constexpr double kMouth = 152.0;
constexpr double kHead = 198.0;
constexpr double kEnd = 210.0;
constexpr double kStickIn = 12.6;
constexpr double kStickOut = 16.4;
constexpr double kFit = 0.45;

constexpr double kTL = 6.5;
constexpr double kTW = 2.75;
constexpr double kDL = 7.7;
constexpr double kDW = 3.9;
constexpr double kLead = 15.4;
constexpr double kNestDriveY = 176.0;

constexpr double kCurrent = 2.7;
constexpr double kAhead = 10.8;
constexpr double kAstern = 8.0;
constexpr double kStop = 0.58;
constexpr double kHoldNeed = 0.42;
constexpr double kOutNeed = 1.15;
constexpr double kBreak = 8.8;
constexpr double kOverrun = 3.35;
constexpr double kLeg = 96.0;
constexpr double kPx = 4.4;

constexpr double kStartX = -6.5;
constexpr double kStartY = 38.0;
constexpr double kStartH = 1.05;
constexpr float kPlayZoom = 2.35f;

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

struct Box {
    double x0, x1, y0, y1;
    bool boom;
};

}  // namespace

int Game::frameOf(double h) const {
    double u = std::fmod(h, kTau);
    if (u < 0) u += kTau;
    int i = int(std::lround(u / kTau * 16.0)) % 16;
    if (i < 0) i += 16;
    return i;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.08) return 3;
    if (driveInside()) return 2;
    return 1;
}

Game::Bounds Game::driveBounds() const {
    const double c = std::cos(heading_), s = std::sin(heading_);
    const double cx = x_ + kLead * c;
    const double cy = y_ + kLead * s;
    Bounds b{cx, cx, cy, cy};
    const double fs[2] = {-kDL, kDL};
    const double bs[2] = {-kDW, kDW};
    for (double f : fs) {
        for (double w : bs) {
            double px = cx + f * c + w * s;
            double py = cy + f * s - w * c;
            b.minX = std::min(b.minX, px);
            b.maxX = std::max(b.maxX, px);
            b.minY = std::min(b.minY, py);
            b.maxY = std::max(b.maxY, py);
        }
    }
    return b;
}

bool Game::driveInside() const {
    Bounds b = driveBounds();
    return b.minX >= -kStickIn + kFit && b.maxX <= kStickIn - kFit && b.minY >= kMouth + kFit && b.maxY <= kHead - kFit;
}

double Game::flood() const {
    const double c = std::cos(heading_), s = std::sin(heading_);
    const double dx = x_ + kLead * c;
    const double dy = y_ + kLead * s;
    if (dy > kMouth - 2.0 && std::fabs(dx) < kStickIn - 0.3) {
        double u = clampd((dy - (kMouth - 2.0)) / 12.0, 0.0, 1.0);
        u = u * u * (3.0 - 2.0 * u);
        return kCurrent * (1.0 - 0.88 * u);
    }
    return kCurrent;
}

double Game::groundSpeed() const {
    const double vx = std::cos(heading_) * surge_;
    const double vy = std::sin(heading_) * surge_ + flood();
    return std::hypot(vx, vy);
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    surge_ = 0;
    throttle_ = 0;
    hold_ = 0;
    outT_ = 0;
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
    for (Puff& p : wake_) p = {};
    for (Puff& p : smoke_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 0;
    camY_ = 118.f;
    zoom_ = 0.92f;
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
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.12f, 0.18f, 0.10f);
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
    const bool ahead = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C);
    const bool astern = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    if (ahead && !astern) throttle_ = std::min(1.0, throttle_ + kDt * 1.45);
    else if (astern && !ahead) throttle_ = std::max(-1.0, throttle_ - kDt * 1.45);
    else throttle_ *= std::exp(-1.7 * kDt);
    if (p.accel > 0.08f) throttle_ = std::min(1.0, throttle_ + p.accel * kDt * 1.2);
    if (p.brake > 0.08f) throttle_ = std::max(-1.0, throttle_ - p.brake * kDt * 1.2);
    if (p.down(gs::BTN_TURBO) || p.pressed(gs::BTN_Y)) horn(0.16f);
}

void Game::pilot(double& steer) {
    const double nestTugY = kNestDriveY - kLead;
    const double errH = wrap(kNorth - heading_);
    const Bounds d = driveBounds();
    const double flo = flood();
    const bool in = driveInside();
    const bool lined = std::fabs(x_) < 1.8 && std::fabs(errH) < 0.24;

    auto throttleFor = [&](double wantVy, double sign) {
        double snh = std::sin(heading_);
        if (std::fabs(snh) < 0.42) snh = std::copysign(0.42, snh);
        double surgeCmd = (wantVy - flo) / snh;
        if (sign < 0 && surgeCmd > 0) surgeCmd = 0;
        if (sign > 0 && surgeCmd < 0) surgeCmd = 0;
        double cap = surgeCmd >= 0 ? kAhead : kAstern;
        throttle_ = clampd(surgeCmd / cap, -1.0, 1.0);
    };

    if (d.maxY > kHead - 7.5) {
        double sign = -1.0;
        double hdes = kNorth + sign * clampd(x_ * 0.08, -0.4, 0.4);
        steer = clampd(wrap(hdes - heading_) / 0.2, -1.0, 1.0);
        throttleFor(-1.5, sign);
        return;
    }

    if (in && lined) {
        double wantVy = clampd((nestTugY - y_) * 0.62, -0.85, 1.8);
        double sign = (wantVy >= flo - 0.12) ? 1.0 : -1.0;
        double hdes = kNorth + sign * clampd(x_ * 0.08, -0.2, 0.2);
        steer = clampd(wrap(hdes - heading_) / 0.18, -1.0, 1.0);
        throttleFor(wantVy, sign);
        return;
    }

    double dist = nestTugY - y_;
    double wantVy;
    if (dist > 62.0) wantVy = 7.4;
    else if (dist > 30.0) wantVy = 4.1;
    else if (dist > 13.0) wantVy = 2.25;
    else if (dist > 0.0) wantVy = 1.35;
    else wantVy = -1.45;

    if (y_ > kMouth - 42.0 && (std::fabs(x_) > 1.7 || std::fabs(errH) > 0.3)) wantVy = std::min(wantVy, 1.2);
    if (y_ > kMouth - 18.0 && std::fabs(x_) > kStickIn - 6.5) wantVy = std::min(wantVy, 0.95);

    double sign = (wantVy >= flo - 0.12) ? 1.0 : -1.0;
    double gain = (y_ > kMouth - 50.0) ? 0.14 : 0.08;
    double hdes = kNorth + sign * clampd(x_ * gain, -0.82, 0.82);
    if (std::fabs(wrap(hdes - heading_)) > 0.95) wantVy = std::min(wantVy, 2.5);
    steer = clampd(wrap(hdes - heading_) / 0.2, -1.0, 1.0);
    sign = (wantVy >= flo - 0.12) ? 1.0 : -1.0;
    hdes = kNorth + sign * clampd(x_ * gain, -0.82, 0.82);
    steer = clampd(wrap(hdes - heading_) / 0.2, -1.0, 1.0);
    throttleFor(wantVy, sign);
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
    phase_ = 3;
    std::snprintf(why_, sizeof why_, "delivered");
    chime(5);
    sys_->rumble(0.32f, 0.18f, 180);
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
    sys_->apu.noiseBurst(0.46f, 130.f, 0.42f);
    sys_->apu.tone(0, 74.f, 0.06f);
    tone0_ = 0.45f;
}

void Game::puff(Puff* ring, int& cursor, int n, double x, double y) {
    ring[cursor].x = x;
    ring[cursor].y = y;
    ring[cursor].life = 1;
    cursor = (cursor + 1) % n;
}

void Game::physics(double steer) {
    double rate = 1.2 + std::min(std::fabs(surge_), 10.0) * 0.05;
    heading_ = wrap(heading_ + steer * rate * kDt);
    double target = throttle_ >= 0 ? throttle_ * kAhead : throttle_ * kAstern;
    surge_ += (target - surge_) * (1.0 - std::exp(-2.5 * kDt));
    surge_ = clampd(surge_, -kAstern, kAhead);

    const double flo = flood();
    const double c0 = std::cos(heading_), s0 = std::sin(heading_);
    x_ += c0 * surge_ * kDt;
    y_ += (s0 * surge_ + flo) * kDt;
    if (!std::isfinite(x_) || !std::isfinite(y_) || !std::isfinite(surge_)) {
        fail("missed the end");
        return;
    }

    const double hitSpd = std::hypot(c0 * surge_, s0 * surge_ + flo);
    auto thud = [&]() {
        if (thumpT_ > 0) return;
        sys_->apu.noiseBurst(0.24f, 190.f, 0.11f);
        thumpT_ = 0.26f;
        shake_ = std::max(shake_, 0.35f);
    };

    const Box walls[] = {
        {-400, -kWall, -400, kMouth, false},
        {kWall, 400, -400, kMouth, false},
        {-400, 400, -400, kSouth, false},
        {-400, -kStickOut, kMouth - 0.4, kEnd + 18, false},
        {kStickOut, 400, kMouth - 0.4, kEnd + 18, false},
        {-kStickOut, -kStickIn, kMouth, kHead + 2.0, true},
        {kStickIn, kStickOut, kMouth, kHead + 2.0, true},
    };

    bool boomHit = false;
    for (int iter = 0; iter < 3; iter++) {
        double px[16], py[16];
        int n = 0;
        const double c = std::cos(heading_), s = std::sin(heading_);
        const double dx = x_ + kLead * c;
        const double dy = y_ + kLead * s;
        auto add = [&](double ox, double oy, double f, double b) {
            px[n] = ox + f * c + b * s;
            py[n] = oy + f * s - b * c;
            n++;
        };
        const double tf[7] = {kTL, -kTL, 2.2, 2.2, -2.0, -2.0, 0};
        const double tb[7] = {0, 0, kTW, -kTW, kTW, -kTW, 0};
        for (int i = 0; i < 7; i++) add(x_, y_, tf[i], tb[i]);
        const double df[8] = {kDL, -kDL, kDL, kDL, -kDL, -kDL, 0, 0};
        const double db[8] = {0, 0, kDW, -kDW, kDW, -kDW, kDW, -kDW};
        for (int i = 0; i < 8; i++) add(dx, dy, df[i], db[i]);

        double x0 = x_, y0 = y_;
        for (int i = 0; i < n; i++) {
            for (const Box& w : walls) {
                if (px[i] < w.x0 || px[i] > w.x1 || py[i] < w.y0 || py[i] > w.y1) continue;
                double dl = px[i] - w.x0, dr = w.x1 - px[i], dbm = py[i] - w.y0, dtp = w.y1 - py[i];
                if (dl <= dr && dl <= dbm && dl <= dtp) x_ -= dl + 0.06;
                else if (dr <= dbm && dr <= dtp) x_ += dr + 0.06;
                else if (dbm <= dtp) y_ -= dbm + 0.06;
                else y_ += dtp + 0.06;
                if (w.boom) boomHit = true;
            }
        }
        double mx = x_ - x0, my = y_ - y0;
        double md = std::hypot(mx, my);
        if (md > 2.2) {
            x_ = x0 + mx / md * 2.2;
            y_ = y0 + my / md * 2.2;
        }
    }

    if (boomHit) {
        if (hitSpd > kBreak) {
            fail("broke the boom");
            return;
        }
        surge_ *= 0.62;
        thud();
    }

    double vy = std::sin(heading_) * surge_ + flo;
    double px[16], py[16];
    int n = 0;
    {
        const double c = std::cos(heading_), s = std::sin(heading_);
        const double dx = x_ + kLead * c;
        const double dy = y_ + kLead * s;
        auto add = [&](double ox, double oy, double f, double b) {
            px[n] = ox + f * c + b * s;
            py[n] = oy + f * s - b * c;
            n++;
        };
        add(x_, y_, kTL, 0);
        add(x_, y_, -kTL, 0);
        add(dx, dy, kDL, 0);
        add(dx, dy, -kDL, 0);
        add(dx, dy, kDL, kDW);
        add(dx, dy, kDL, -kDW);
        add(dx, dy, -kDL, kDW);
        add(dx, dy, -kDL, -kDW);
    }
    double worst = -1e9;
    bool head = false;
    for (int i = 0; i < n; i++) {
        if (px[i] < -kStickOut - 0.3 || px[i] > kStickOut + 0.3) continue;
        if (py[i] > kHead && py[i] < kEnd + 0.4 && py[i] > worst) {
            worst = py[i];
            head = true;
        }
    }
    if (head) {
        if (vy > kBreak) {
            fail("broke the boom");
            return;
        }
        if (vy <= kOverrun) {
            y_ -= worst - (kHead - 0.12);
            surge_ *= 0.5;
            thud();
        }
    }

    for (int i = 0; i < n; i++) {
        if (py[i] > kEnd) {
            fail("missed the end");
            return;
        }
    }

    const bool in = driveInside();
    const Bounds db = driveBounds();
    const double g = groundSpeed();
    if (in && !announced_) {
        announced_ = true;
        horn(0.22f);
        blip(660.f);
    }
    phase_ = in ? (hold_ > 0.05 ? 2 : 1) : 0;

    const double c = std::cos(heading_), s = std::sin(heading_);
    wakeT_ -= float(kDt);
    smokeT_ -= float(kDt);
    if (wakeT_ <= 0 && g > 1.7) {
        wakeT_ = 0.07f;
        puff(wake_, wakeCursor_, 16, x_ - c * 6.2, y_ - s * 6.2);
    }
    if (smokeT_ <= 0 && (std::fabs(throttle_) > 0.08 || std::fabs(surge_) > 0.7)) {
        smokeT_ = 0.1f;
        puff(smoke_, smokeCursor_, 8, x_ - c * 4.6, y_ - s * 4.6);
    }
    for (Puff& p : wake_)
        if (p.life > 0) p.life -= kDt * 0.55;
    for (Puff& p : smoke_)
        if (p.life > 0) {
            p.life -= kDt * 0.42;
            p.y += kDt * 1.4;
            p.x += kDt * 0.25;
        }

    if (g < kStop && in) {
        outT_ = 0;
        hold_ += kDt;
        if (hold_ >= kHoldNeed) win();
    } else if (g < kStop) {
        hold_ = 0;
        outT_ += kDt;
        if (outT_ >= kOutNeed) fail(db.maxY < kMouth ? "stopped short of the boom" : "the drive is not on the boom");
    } else {
        hold_ = 0;
        outT_ = 0;
    }
}

void Game::audio() {
    float water = mode_ == Mode::Run ? 0.016f + float(std::fabs(surge_) * 0.00045) : 0.01f;
    sys_->apu.noise(water, 440.f + float(std::fabs(surge_)) * 10.f, false);
    if (hornT_ > 0) {
        hornT_ -= float(kDt);
        float v = hornT_ > 0.06f ? 0.07f : std::max(0.f, hornT_) * 1.05f;
        sys_->apu.tone(0, 96.f, v);
        if (tone1_ <= 0) sys_->apu.tone(1, 144.f, v * 0.45f);
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
            static const float notes[] = {392.f, 494.f, 587.f, 784.f, 988.f};
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
            if (mode_ == Mode::Run && race_ > kLeg) fail("the leg ran out");
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
    else if (hold_ > 0.02) sys.setLight(200, 160, 40);
    else if (mode_ == Mode::Run) sys.setLight(30, 90, 150);
    camera();
    audio();
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = 0.f;
        camY_ = 118.f;
        zoom_ = 0.92f;
        return;
    }
    const double c = std::cos(heading_), s = std::sin(heading_);
    float lead = 16.f;
    if (y_ > kMouth - 24.0) lead = 8.f;
    if (hold_ > 0.02 || mode_ == Mode::Win || mode_ == Mode::Fail) lead = 6.f;
    float tx = float(x_ + c * lead);
    float ty = float(y_ + s * lead + kLead * 0.35);
    float tz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        tx = 0.f;
        ty = float((kMouth + kHead) * 0.5);
        tz = 1.85f;
    }
    float k = 1.f - std::exp(-float(kDt) * 4.2f);
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

    float jx = 0, jy = 0;
    if (shake_ > 0) {
        jx = std::sin(float(t_) * 47.f) * shake_ * 4.f;
        jy = std::cos(float(t_) * 39.f) * shake_ * 3.f;
    }
    float invZ = 1.f / std::max(zoom_, 0.25f);
    camX_ -= jx * invZ;
    camY_ += jy * invZ;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) * invZ;
        float half = float(kWall);
        if (wy >= float(kMouth) && wy < float(kEnd + 2.0)) half = float(kStickIn);
        else if (wy >= float(kEnd + 2.0)) half = 0.6f;
        uint16_t shore = gs::rgb4(3, 5, 3);
        uint16_t far = gs::rgb4(2, 3, 2);
        float u = std::clamp((wy - 8.f) / 230.f, 0.f, 1.f);
        v.lineBackdrop[y] = lerpC(shore, far, u);
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f + (0.f - camX_) * zoom_;
        r.hw = std::max(2.f, half * zoom_);
        r.v = wy * 32.f + float(t_) * 22.f;
        r.pal = uint8_t(PAL_CH);
        r.band = (int(std::floor(wy * 0.18f + t_ * 1.1f)) & 1) ? 1 : 0;
        r.style = 2;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }

    const float postMin = mode_ == Mode::Title ? 8.f : 0.f;
    const int gatePal = hold_ > 0.02 ? PAL_WIN : PAL_MARK;
    for (double y = 8; y <= 230; y += 14) {
        place(art_.quay, -(kWall + 3.4), y, 11.f, PAL_QUAY, 0, false);
        place(art_.quay, kWall + 3.4, y, 11.f, PAL_QUAY, 0, true);
    }
    const double sheds[][2] = {{-34, 30}, {34, 58}, {-33, 104}, {35, 146}, {-34, 196}};
    for (const double* s : sheds) place(art_.shed, s[0], s[1], 10.f, PAL_QUAY, mode_ == Mode::Title ? 8.f : 0);
    const double lamps[] = {42, 86, 128, 170};
    for (double y : lamps) {
        place(art_.lamp, -(kWall + 0.6), y, 6.5f, PAL_QUAY, 0);
        place(art_.lamp, kWall + 0.6, y, 6.5f, PAL_QUAY, 0);
    }

    const double stickX = (kStickIn + kStickOut) * 0.5;
    for (double y = kMouth + 3.0; y < kHead - 1.0; y += 5.4) {
        place(art_.logV, -stickX, y, 5.2f, PAL_BOOM, postMin);
        place(art_.logV, stickX, y, 5.2f, PAL_BOOM, postMin);
    }
    for (double x = -kStickIn + 2.0; x <= kStickIn - 1.5; x += 5.2)
        place(art_.logH, x, kHead + 1.6, 1.7f, PAL_BOOM, postMin);
    place(art_.post, -kStickIn, kMouth, 7.2f, gatePal, postMin);
    place(art_.post, kStickIn, kMouth, 7.2f, gatePal, postMin);
    place(art_.post, -kStickIn, kHead, 7.2f, PAL_END, postMin);
    place(art_.post, kStickIn, kHead, 7.2f, PAL_END, postMin);
    worldRect(art_.stripe, 0, kEnd, kStickIn * 2.0 - 1.0, 1.15, PAL_END);
    for (double x = -10; x <= 10.1; x += 5) place(art_.buoy, x, kEnd + 0.2, 4.6f, PAL_END, mode_ == Mode::Title ? 7.f : 0);
    const double marks[] = {70, 104, 136};
    for (double y : marks) {
        place(art_.buoy, -kWall + 3.2, y, 3.8f, PAL_END, 0);
        place(art_.buoy, kWall - 3.2, y, 3.8f, PAL_END, 0);
    }

    int flap = int(t_ * 3.0) & 1;
    place(art_.gull[flap], -12 + std::sin(t_ * 0.33) * 14, 88 + std::cos(t_ * 0.2) * 6, 3.6f, PAL_GULL,
          mode_ == Mode::Title ? 7.f : 0);
    place(art_.gull[1 - flap], 16 + std::cos(t_ * 0.27) * 10, 168 + std::sin(t_ * 0.18) * 5, 3.2f, PAL_GULL, 0);

    for (const Puff& p : wake_) {
        if (p.life <= 0) continue;
        float h = (1.6f + float(1.0 - p.life) * 2.2f) * zoom_;
        float sx = 160.f + float(p.x - camX_) * zoom_;
        float sy = 112.f - float(p.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false, false);
    }
    for (const Puff& p : smoke_) {
        if (p.life <= 0) continue;
        float h = (2.0f + float(1.0 - p.life) * 2.8f) * (zoom_ / kPlayZoom);
        float sx = 160.f + float(p.x - camX_) * zoom_;
        float sy = 112.f - float(p.y - camY_) * zoom_;
        spr(art_.smoke, sx, sy, std::max(2.f, h), PAL_SMOKE, false, false);
    }

    const double c = std::cos(heading_), s = std::sin(heading_);
    const double dx = x_ + kLead * c;
    const double dy = y_ + kLead * s;
    for (int i = 1; i <= 3; i++) {
        double u = i / 4.0;
        double lx = x_ + c * (kTL + (kLead - kTL - kDL) * u);
        double ly = y_ + s * (kTL + (kLead - kTL - kDL) * u);
        place(art_.link, lx, ly, 1.15f, PAL_DRIVE, 0);
    }

    int fi = frameOf(heading_);
    float bob = std::sin(float(t_) * 2.2f + float(x_) * 0.04f) * 0.35f;
    auto blitCraft = [&](const gs::Mipped& m, double wx, double wy, int pal) {
        float bh = float(m.h) / float(kPx) * zoom_;
        if (mode_ == Mode::Title) bh = std::max(bh, 22.f);
        float sx = 160.f + float(wx - camX_) * zoom_;
        float sy = 112.f - float(wy - camY_) * zoom_ + bob * zoom_ * 0.12f;
        spr(art_.shade, sx + 3.f, sy + 4.f, bh * 0.62f, pal, false, true);
        spr(m, sx, sy, bh, pal, false, false);
    };
    blitCraft(art_.drive[fi], dx, dy, PAL_DRIVE);
    blitCraft(art_.tug[fi], x_, y_, PAL_TUG);

    if (mode_ != Mode::Title) {
        float ax = 160.f + (0.f - camX_) * zoom_;
        float ay = 112.f - float((kMouth + kHead) * 0.5 - camY_) * zoom_;
        bool off = ax < 16.f || ax > 304.f || ay < 18.f || ay > 206.f;
        if (off) {
            float ox = ax - 160.f, oy = ay - 112.f;
            float k = 1.f;
            if (std::fabs(ox) > 1.f) k = std::min(k, 132.f / std::fabs(ox));
            if (std::fabs(oy) > 1.f) k = std::min(k, 80.f / std::fabs(oy));
            spr(art_.pin, 160.f + ox * k, 112.f + oy * k, 10.f, PAL_MARK, false, false);
        }
        auto chart = [&](double wx, double wy, float h, int pal) {
            float sx = 292.f + float(wx) * 0.72f;
            float sy = 74.f - float(wy - 120.0) * 0.26f;
            spr(art_.pin, sx, sy, h, pal, false, false);
        };
        chart(-kStickIn, kMouth, 3.2f, PAL_MARK);
        chart(kStickIn, kMouth, 3.2f, PAL_MARK);
        chart(-kStickIn, kHead, 3.2f, PAL_END);
        chart(kStickIn, kHead, 3.2f, PAL_END);
        chart(0, kEnd, 3.4f, PAL_ALERT);
        chart(x_, y_, 4.6f, PAL_BANNER);
    }

    camX_ += jx * invZ;
    camY_ -= jy * invZ;

    auto banner = [&](const gs::Mipped& m, float y, int pal) { spr(m, 160.f, y, float(m.h), pal, false, false); };
    if (mode_ == Mode::Title) banner(art_.title, 16.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Win) banner(art_.delivered, 22.f, PAL_WIN);
    else if (mode_ == Mode::Fail) {
        if (!std::strcmp(why_, "missed the end")) banner(art_.missed, 24.f, PAL_ALERT);
        else if (!std::strcmp(why_, "stopped short of the boom")) banner(art_.shortOf, 24.f, PAL_ALERT);
        else if (!std::strcmp(why_, "broke the boom")) banner(art_.broke, 24.f, PAL_ALERT);
        else if (!std::strcmp(why_, "the drive is not on the boom")) banner(art_.offBoom, 24.f, PAL_ALERT);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(21, "DELIVER THE DRIVE TO THE BOOM", PAL_BANNER);
        hudC(22, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        hudC(23, "A FLOOD SETS YOU TOWARD THE END", PAL_HUD);
        if ((int(t_ * 2.0) & 1) == 0) hudC(25, "START", PAL_WIN);
        else hudC(25, "ARROWS STEER   UP AHEAD   DOWN ASTERN", PAL_HUD);
        hudC(26, "HOLD THE DRIVE INSIDE THE BOOM", PAL_HUD);
        hudC(27, "SPACE HORN", PAL_HUD);
        return;
    }

    hud(1, 0, "S3 TUGBOAT BOOM", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "LEG %4.1fS", race_);
    hud(29, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        hudC(19, "ESC BACK TO THE DOCK", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(15, "THE DRIVE IS ON THE BOOM", PAL_WIN);
        hudC(16, "THE LEG IS MADE", PAL_BANNER);
        std::snprintf(buf, sizeof buf, "%.1fS", race_);
        hudC(18, buf, PAL_HUD);
        if (!bot_) hudC(20, "START RUNS THE LEG AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, "THE LEG FAILS", PAL_ALERT);
        if (!std::strcmp(why_, "missed the end")) hudC(17, "YOU CROSSED THE END", PAL_HUD);
        else if (!std::strcmp(why_, "stopped short of the boom")) hudC(17, "SHORT OF THE BOOM", PAL_HUD);
        else if (!std::strcmp(why_, "broke the boom")) hudC(17, "THE BOOM BROKE", PAL_HUD);
        else if (!std::strcmp(why_, "the leg ran out")) hudC(17, "THE LEG RAN OUT", PAL_HUD);
        else hudC(17, "THE DRIVE IS NOT ON THE BOOM", PAL_HUD);
        if (!bot_) hudC(19, "START TRIES THE LEG AGAIN", PAL_HUD);
        return;
    }

    const Bounds db = driveBounds();
    const double toEnd = kEnd - db.maxY;
    const bool in = driveInside();
    const double g = groundSpeed();
    std::snprintf(buf, sizeof buf, "END %3.0f", std::max(0.0, toEnd));
    hud(1, 1, buf, toEnd < 16 ? PAL_ALERT : PAL_MARK);
    std::snprintf(buf, sizeof buf, "ENG %+4d  SPD %4.1f", int(std::lround(throttle_ * 100.0)), g);
    hud(20, 1, buf, PAL_HUD);
    if (in && g < kStop) {
        int n = std::clamp(int(hold_ / kHoldNeed * 5.0) + 1, 1, 5);
        std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
        hud(1, 2, buf, PAL_WIN);
    } else if (in) {
        hud(1, 2, "DRIVE IN THE BOOM  HOLD STILL", PAL_BANNER);
    } else if (db.maxY > kHead - 6.0) {
        hud(1, 2, "THE END IS AHEAD  BACK DOWN", PAL_ALERT);
    } else if (db.minY > kMouth - 8.0) {
        hud(1, 2, "PUT THE WHOLE DRIVE IN THE BOOM", PAL_HUD);
    } else {
        hud(1, 2, "DELIVER THE DRIVE TO THE BOOM", PAL_HUD);
    }
    if (race_ < 4.5) hud(1, 26, "FLOOD ASTERN  CHECK DOWN NEAR THE BOOM", PAL_MARK);
    else if (in) hud(1, 26, "WHOLE DRIVE INSIDE, THEN HOLD STILL", PAL_HUD);
    else hud(1, 26, "MISSING THE END FAILS THE LEG", PAL_ALERT);
    hud(1, 27, "ARROWS STEER  UP AHEAD  DOWN ASTERN", PAL_HUD);
}

}  // namespace tugboom
