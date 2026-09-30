#include "bus.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace busboom {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kPi = 3.141592653589793;
constexpr double kTau = 6.283185307179586;
constexpr double kNorth = kPi * 0.5;

constexpr double kCurb = 22.0;
constexpr double kSouth = 12.0;
constexpr double kMouth = 146.0;
constexpr double kHead = 188.0;
constexpr double kEnd = 200.0;
constexpr double kIn = 11.0;
constexpr double kOut = 14.6;
constexpr double kFit = 0.4;

constexpr double kNose = 5.05;
constexpr double kTail = 4.7;
constexpr double kBeam = 1.9;
constexpr double kDL = 6.35;
constexpr double kDW = 3.2;
constexpr double kLead = 13.1;
constexpr double kNestY = 166.5;

constexpr double kGrade = 2.15;
constexpr double kAhead = 9.2;
constexpr double kAstern = 6.6;
constexpr double kStop = 0.55;
constexpr double kHoldNeed = 0.45;
constexpr double kOutNeed = 1.2;
constexpr double kBreak = 7.4;
constexpr double kOverrun = 2.9;
constexpr double kLeg = 88.0;
constexpr double kPx = 4.2;

constexpr double kStartX = 5.4;
constexpr double kStartY = 34.0;
constexpr double kStartH = 1.12;
constexpr float kPlayZoom = 2.55f;

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

struct Wall {
    double x0, x1, y0, y1;
    bool boom;
};

}  // namespace

int Game::frameOf(double h) const {
    double u = std::fmod(h, kTau);
    if (u < 0) u += kTau;
    int i = int(std::lround(u / kTau * 8.0)) % 8;
    if (i < 0) i += 8;
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
    return b.minX >= -kIn + kFit && b.maxX <= kIn - kFit && b.minY >= kMouth + kFit && b.maxY <= kHead - kFit;
}

double Game::grade() const {
    const double c = std::cos(heading_), s = std::sin(heading_);
    const double dy = y_ + kLead * s;
    const double dx = x_ + kLead * c;
    if (dy > kMouth - 4.0 && std::fabs(dx) < kIn - 0.2) {
        double u = clampd((dy - (kMouth - 4.0)) / 14.0, 0.0, 1.0);
        u = u * u * (3.0 - 2.0 * u);
        return kGrade * (1.0 - 0.9 * u);
    }
    return kGrade;
}

double Game::groundSpeed() const {
    const double vx = std::cos(heading_) * surge_;
    const double vy = std::sin(heading_) * surge_ + grade();
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
    exhaustCursor_ = 0;
    exhaustT_ = 0;
    hornT_ = 0;
    thumpT_ = 0;
    shake_ = 0;
    why_[0] = 0;
    for (Puff& p : exhaust_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 0;
    camY_ = 112.f;
    zoom_ = 0.95f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = float(x_);
    camY_ = float(y_);
    zoom_ = kPlayZoom;
    horn(0.28f);
    blip(480.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.12f, 0.16f, 0.08f);
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
    if (ahead && !astern) throttle_ = std::min(1.0, throttle_ + kDt * 1.5);
    else if (astern && !ahead) throttle_ = std::max(-1.0, throttle_ - kDt * 1.5);
    else throttle_ *= std::exp(-1.8 * kDt);
    if (p.accel > 0.08f) throttle_ = std::min(1.0, throttle_ + p.accel * kDt * 1.2);
    if (p.brake > 0.08f) throttle_ = std::max(-1.0, throttle_ - p.brake * kDt * 1.2);
    if (p.down(gs::BTN_TURBO) || p.pressed(gs::BTN_Y)) horn(0.14f);
}

void Game::pilot(double& steer) {
    const double targetY = kNestY - kLead;
    const double errH = wrap(kNorth - heading_);
    const Bounds d = driveBounds();
    const double flo = grade();
    const bool in = driveInside();
    const bool lined = std::fabs(x_) < 1.5 && std::fabs(errH) < 0.22;

    auto throttleFor = [&](double wantVy, double sign) {
        double snh = std::sin(heading_);
        if (std::fabs(snh) < 0.45) snh = std::copysign(0.45, snh);
        double surgeCmd = (wantVy - flo) / snh;
        if (sign < 0 && surgeCmd > 0) surgeCmd = 0;
        if (sign > 0 && surgeCmd < 0) surgeCmd = 0;
        double cap = surgeCmd >= 0 ? kAhead : kAstern;
        throttle_ = clampd(surgeCmd / cap, -1.0, 1.0);
    };

    if (d.maxY > kHead - 6.0) {
        steer = clampd(wrap(kNorth - heading_) / 0.2, -1.0, 1.0);
        throttleFor(-1.6, -1.0);
        return;
    }

    if (in && lined) {
        double wantVy = clampd((targetY - y_) * 0.7, -0.7, 1.4);
        if (std::fabs(targetY - y_) < 0.45 && std::fabs(x_) < 0.7) wantVy = 0.0;
        double sign = (wantVy >= flo - 0.08) ? 1.0 : -1.0;
        double hdes = kNorth + sign * clampd(x_ * 0.1, -0.18, 0.18);
        steer = clampd(wrap(hdes - heading_) / 0.16, -1.0, 1.0);
        throttleFor(wantVy, sign);
        return;
    }

    double dist = targetY - y_;
    double wantVy;
    if (dist > 70.0) wantVy = 6.6;
    else if (dist > 36.0) wantVy = 4.0;
    else if (dist > 16.0) wantVy = 2.3;
    else if (dist > 0.0) wantVy = 1.25;
    else wantVy = -1.3;

    if (y_ > kMouth - 36.0 && (std::fabs(x_) > 1.6 || std::fabs(errH) > 0.28)) wantVy = std::min(wantVy, 1.15);
    if (y_ > kMouth - 12.0 && std::fabs(x_) > kIn - 5.0) wantVy = std::min(wantVy, 0.9);

    double sign = (wantVy >= flo - 0.1) ? 1.0 : -1.0;
    double gain = (y_ > kMouth - 48.0) ? 0.16 : 0.09;
    double hdes = kNorth + sign * clampd(x_ * gain, -0.75, 0.75);
    if (std::fabs(wrap(hdes - heading_)) > 0.9) wantVy = std::min(wantVy, 2.2);
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
    sys_->rumble(0.3f, 0.16f, 160);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.55f, 0.25f, 180);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.42f, 120.f, 0.4f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.4f;
}

void Game::puff(Puff* ring, int& cursor, int n, double x, double y) {
    ring[cursor].x = x;
    ring[cursor].y = y;
    ring[cursor].life = 1;
    cursor = (cursor + 1) % n;
}

void Game::physics(double steer) {
    double rate = 1.35 + std::min(std::fabs(surge_), 8.0) * 0.06;
    heading_ = wrap(heading_ + steer * rate * kDt);
    double target = throttle_ >= 0 ? throttle_ * kAhead : throttle_ * kAstern;
    surge_ += (target - surge_) * (1.0 - std::exp(-2.6 * kDt));
    surge_ = clampd(surge_, -kAstern, kAhead);

    const double flo = grade();
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
        sys_->apu.noiseBurst(0.22f, 170.f, 0.1f);
        thumpT_ = 0.24f;
        shake_ = std::max(shake_, 0.3f);
    };

    const Wall walls[] = {
        {-400, -kCurb, -400, kMouth, false},
        {kCurb, 400, -400, kMouth, false},
        {-400, 400, -400, kSouth, false},
        {-400, -kOut, kMouth - 0.4, kEnd + 16, false},
        {kOut, 400, kMouth - 0.4, kEnd + 16, false},
        {-kOut, -kIn, kMouth, kHead + 1.6, true},
        {kIn, kOut, kMouth, kHead + 1.6, true},
    };

    bool boomHit = false;
    for (int iter = 0; iter < 3; iter++) {
        double px[14], py[14];
        int n = 0;
        const double c = std::cos(heading_), s = std::sin(heading_);
        const double dx = x_ + kLead * c;
        const double dy = y_ + kLead * s;
        auto add = [&](double ox, double oy, double f, double b) {
            px[n] = ox + f * c + b * s;
            py[n] = oy + f * s - b * c;
            n++;
        };
        const double tf[5] = {kNose, -kTail, 1.5, -2.0, 0};
        const double tb[5] = {0, 0, kBeam, kBeam, 0};
        for (int i = 0; i < 5; i++) {
            add(x_, y_, tf[i], tb[i]);
            if (tb[i] != 0) add(x_, y_, tf[i], -tb[i]);
        }
        const double df[4] = {kDL, -kDL, kDL, -kDL};
        const double db[4] = {kDW, kDW, -kDW, -kDW};
        for (int i = 0; i < 4; i++) add(dx, dy, df[i], db[i]);

        double x0 = x_, y0 = y_;
        for (int i = 0; i < n; i++) {
            for (const Wall& w : walls) {
                if (px[i] < w.x0 || px[i] > w.x1 || py[i] < w.y0 || py[i] > w.y1) continue;
                double dl = px[i] - w.x0, dr = w.x1 - px[i], dbm = py[i] - w.y0, dtp = w.y1 - py[i];
                if (dl <= dr && dl <= dbm && dl <= dtp) x_ -= dl + 0.05;
                else if (dr <= dbm && dr <= dtp) x_ += dr + 0.05;
                else if (dbm <= dtp) y_ -= dbm + 0.05;
                else y_ += dtp + 0.05;
                if (w.boom) boomHit = true;
            }
        }
        double mx = x_ - x0, my = y_ - y0;
        double md = std::hypot(mx, my);
        if (md > 2.0) {
            x_ = x0 + mx / md * 2.0;
            y_ = y0 + my / md * 2.0;
        }
    }

    if (boomHit) {
        if (hitSpd > kBreak) {
            fail("broke the boom");
            return;
        }
        surge_ *= 0.6;
        thud();
    }

    double vy = std::sin(heading_) * surge_ + flo;
    double px[10], py[10];
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
        add(x_, y_, kNose, 0);
        add(x_, y_, -kTail, 0);
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
        if (px[i] < -kOut - 0.2 || px[i] > kOut + 0.2) continue;
        if (py[i] > kHead && py[i] < kEnd + 0.3 && py[i] > worst) {
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
            y_ -= worst - (kHead - 0.1);
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
        horn(0.18f);
        blip(620.f);
    }
    phase_ = in ? (hold_ > 0.05 ? 2 : 1) : 0;

    const double c = std::cos(heading_), s = std::sin(heading_);
    exhaustT_ -= float(kDt);
    if (exhaustT_ <= 0 && (std::fabs(throttle_) > 0.08 || std::fabs(surge_) > 0.6)) {
        exhaustT_ = 0.1f;
        puff(exhaust_, exhaustCursor_, 8, x_ - c * 4.6, y_ - s * 4.6);
    }
    for (Puff& p : exhaust_)
        if (p.life > 0) {
            p.life -= kDt * 0.5;
            p.y += kDt * 0.4;
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
    float rumble = mode_ == Mode::Run ? 0.012f + float(std::fabs(surge_) * 0.0005) : 0.006f;
    sys_->apu.noise(rumble, 280.f + float(std::fabs(surge_)) * 10.f, false);
    if (hornT_ > 0) {
        hornT_ -= float(kDt);
        float v = hornT_ > 0.05f ? 0.06f : std::max(0.f, hornT_) * 1.0f;
        sys_->apu.tone(0, 196.f, v);
        if (tone1_ <= 0) sys_->apu.tone(1, 294.f, v * 0.35f);
    } else if (tone0_ <= 0 && chimeN_ == 0) {
        sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && (std::fabs(throttle_) > 0.03 || std::fabs(surge_) > 0.4)) {
        float wob = 0.78f + 0.22f * std::sin(float(t_) * (9.f + float(std::fabs(throttle_)) * 14.f));
        float vol = (0.012f + float(std::fabs(throttle_)) * 0.024f) * wob;
        float f = 48.f + float(std::fabs(throttle_)) * 36.f;
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
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.15f;
            chimeT_ = 0.12f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - float(kDt) * 1.7f);
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
            blip(380.f);
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
    else if (hold_ > 0.02) sys.setLight(200, 150, 30);
    else if (mode_ == Mode::Run) sys.setLight(40, 80, 140);
    camera();
    audio();
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = 0.f;
        camY_ = 112.f;
        zoom_ = 0.95f;
        return;
    }
    const double c = std::cos(heading_), s = std::sin(heading_);
    float lead = 14.f;
    if (y_ > kMouth - 20.0) lead = 7.f;
    if (hold_ > 0.02 || mode_ == Mode::Win || mode_ == Mode::Fail) lead = 5.f;
    float tx = float(x_ + c * lead);
    float ty = float(y_ + s * lead + kLead * 0.3);
    float tz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        tx = 0.f;
        ty = float((kMouth + kHead) * 0.5);
        tz = 1.9f;
    }
    float k = 1.f - std::exp(-float(kDt) * 4.4f);
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
        jx = std::sin(float(t_) * 51.f) * shake_ * 3.5f;
        jy = std::cos(float(t_) * 37.f) * shake_ * 2.6f;
    }
    float invZ = 1.f / std::max(zoom_, 0.25f);
    camX_ -= jx * invZ;
    camY_ += jy * invZ;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) * invZ;
        float half = float(kCurb);
        if (wy >= float(kMouth) && wy < float(kEnd + 1.5)) half = float(kIn);
        else if (wy >= float(kEnd + 1.5)) half = 0.5f;
        uint16_t near = gs::rgb4(3, 4, 3);
        uint16_t far = gs::rgb4(2, 3, 2);
        float u = std::clamp((wy + 10.f) / 230.f, 0.f, 1.f);
        v.lineBackdrop[y] = lerpC(near, far, u);
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f + (0.f - camX_) * zoom_;
        r.hw = std::max(2.f, half * zoom_);
        r.v = wy * 18.f;
        r.pal = uint8_t(PAL_ROAD);
        r.band = (int(std::floor(wy * 0.08f)) & 3) == 0 ? 1 : 0;
        r.style = 1;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }

    const float postMin = mode_ == Mode::Title ? 7.f : 0.f;
    const int gatePal = hold_ > 0.02 ? PAL_WIN : PAL_MARK;
    for (double y = 6; y <= 220; y += 16) {
        place(art_.walk, -(kCurb + 3.2), y, 10.f, PAL_WALK, 0, false);
        place(art_.walk, kCurb + 3.2, y, 10.f, PAL_WALK, 0, true);
    }
    place(art_.shelter, -32, 48, 9.f, PAL_WALK, mode_ == Mode::Title ? 8.f : 0);
    place(art_.shelter, 33, 96, 9.f, PAL_WALK, 0);
    const double lamps[] = {28, 78, 124, 176};
    for (double y : lamps) {
        place(art_.lamp, -(kCurb + 0.4), y, 6.2f, PAL_LAMP, 0);
        place(art_.lamp, kCurb + 0.4, y, 6.2f, PAL_LAMP, 0);
    }

    const double armX = (kIn + kOut) * 0.5;
    for (double y = kMouth + 2.5; y < kHead - 1.0; y += 6.2) {
        place(art_.girderV, -armX, y, 5.6f, PAL_BOOM, postMin);
        place(art_.girderV, armX, y, 5.6f, PAL_BOOM, postMin);
    }
    for (double x = -kIn + 2.4; x <= kIn - 1.6; x += 5.6)
        place(art_.girderH, x, kHead + 1.4, 1.8f, PAL_BOOM, postMin);
    place(art_.post, -kIn, kMouth, 7.f, gatePal, postMin);
    place(art_.post, kIn, kMouth, 7.f, gatePal, postMin);
    place(art_.post, -kIn, kHead, 7.f, PAL_END, postMin);
    place(art_.post, kIn, kHead, 7.f, PAL_END, postMin);
    worldRect(art_.stripe, 0, kEnd, kIn * 2.0 - 1.2, 1.1, PAL_END);

    int flap = int(t_ * 2.6) & 1;
    place(art_.bird[flap], -8 + std::sin(t_ * 0.4) * 10, 90, 2.6f, PAL_BIRD, mode_ == Mode::Title ? 6.f : 0);

    for (const Puff& p : exhaust_) {
        if (p.life <= 0) continue;
        float h = (1.6f + float(1.0 - p.life) * 2.0f) * (zoom_ / kPlayZoom);
        float sx = 160.f + float(p.x - camX_) * zoom_;
        float sy = 112.f - float(p.y - camY_) * zoom_;
        spr(art_.exhaust, sx, sy, std::max(2.f, h), PAL_EXHAUST, false, false);
    }

    const double c = std::cos(heading_), s = std::sin(heading_);
    const double dx = x_ + kLead * c;
    const double dy = y_ + kLead * s;
    for (int i = 1; i <= 3; i++) {
        double u = i / 4.0;
        double lx = x_ + c * (kNose + (kLead - kNose - kDL) * u);
        double ly = y_ + s * (kNose + (kLead - kNose - kDL) * u);
        place(art_.hitch, lx, ly, 1.05f, PAL_DRIVE, 0);
    }

    int fi = frameOf(heading_);
    auto blitCraft = [&](const gs::Mipped& m, double wx, double wy, int pal) {
        float bh = float(m.h) / float(kPx) * zoom_;
        if (mode_ == Mode::Title) bh = std::max(bh, 20.f);
        float sx = 160.f + float(wx - camX_) * zoom_;
        float sy = 112.f - float(wy - camY_) * zoom_;
        spr(art_.shade, sx + 2.f, sy + 3.f, bh * 0.5f, pal, false, true);
        spr(m, sx, sy, bh, pal, false, false);
    };
    blitCraft(art_.drive[fi], dx, dy, PAL_DRIVE);
    blitCraft(art_.bus[fi], x_, y_, PAL_BUS);

    camX_ += jx * invZ;
    camY_ -= jy * invZ;

    auto banner = [&](const gs::Mipped& m, float y, int pal) { spr(m, 160.f, y, float(m.h), pal, false, false); };
    if (mode_ == Mode::Title) banner(art_.title, 18.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Win) banner(art_.delivered, 22.f, PAL_WIN);
    else if (mode_ == Mode::Fail) {
        if (!std::strcmp(why_, "missed the end")) banner(art_.missed, 24.f, PAL_ALERT);
        else if (!std::strcmp(why_, "stopped short of the boom")) banner(art_.shortOf, 24.f, PAL_ALERT);
        else if (!std::strcmp(why_, "broke the boom")) banner(art_.broke, 24.f, PAL_ALERT);
        else if (!std::strcmp(why_, "the drive is not on the boom")) banner(art_.offBoom, 24.f, PAL_ALERT);
    }

    if (mode_ == Mode::Title) {
        hudC(21, "DELIVER THE DRIVE TO THE BOOM", PAL_BANNER);
        hudC(22, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        hudC(23, "THE GRADE ROLLS YOU TO THE END", PAL_HUD);
        if ((int(t_ * 2.0) & 1) == 0) hudC(25, "START", PAL_WIN);
        else hudC(25, "ARROWS STEER   UP AHEAD   DOWN BRAKE", PAL_HUD);
        hudC(26, "HOLD THE DRIVE INSIDE THE BOOM", PAL_HUD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
        return;
    }

    hud(1, 0, "S3 BUS BOOM", PAL_BANNER);
    char buf[48];
    std::snprintf(buf, sizeof buf, "LEG %02d", int(std::max(0.0, kLeg - race_)));
    hud(32, 0, buf, race_ > kLeg - 12 ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Run) {
        if (hold_ > 0.02) hudC(26, "HOLD", PAL_WIN);
        else if (driveInside()) hudC(26, "ON THE BOOM", PAL_WIN);
        else if (y_ + kLead > kMouth - 8) hudC(26, "EASE TO THE END", PAL_MARK);
        else hudC(26, "TAKE THE DRIVE NORTH", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(25, "THE DRIVE IS ON THE BOOM", PAL_WIN);
        hudC(26, "THE LEG IS MADE", PAL_BANNER);
    } else if (mode_ == Mode::Fail) {
        hudC(25, why_, PAL_ALERT);
        hudC(26, "THE LEG IS LOST", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(26, "START TO ROLL", PAL_HUD);
    }
}

}  // namespace busboom
