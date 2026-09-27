#include "barge.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace bargeboom {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kPi = 3.141592653589793;
constexpr double kTau = 6.283185307179586;
constexpr double kNorth = kPi * 0.5;

constexpr double kWall = 24.0;
constexpr double kSouth = 12.0;
constexpr double kMouth = 150.0;
constexpr double kHead = 186.0;
constexpr double kEnd = 198.0;
constexpr double kPocket = 9.0;
constexpr double kCheek = 12.2;
constexpr double kFit = 0.55;

constexpr double kBL = 9.0;
constexpr double kBW = 4.4;
constexpr double kDL = 5.2;
constexpr double kDW = 3.15;
constexpr double kLead = 16.2;
constexpr double kNestDriveY = 167.0;

constexpr double kCurrent = 1.55;
constexpr double kAhead = 7.6;
constexpr double kAstern = 5.4;
constexpr double kStop = 0.62;
constexpr double kHoldNeed = 0.48;
constexpr double kOutNeed = 1.25;
constexpr double kBreak = 7.4;
constexpr double kCrew = 52.0;
constexpr double kPx = 3.6;

constexpr double kStartX = -5.2;
constexpr double kStartY = 30.0;
constexpr double kStartH = 1.18;
constexpr float kPlayZoom = 2.15f;

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
    return b.minX >= -kPocket + kFit && b.maxX <= kPocket - kFit && b.minY >= kMouth + kFit && b.maxY <= kHead - 2.4;
}

double Game::flood() const {
    const double c = std::cos(heading_), s = std::sin(heading_);
    const double dy = y_ + kLead * s;
    const double dx = x_ + kLead * c;
    if (dy > kMouth - 4.0 && std::fabs(dx) < kPocket - 0.4) {
        double u = clampd((dy - (kMouth - 4.0)) / 14.0, 0.0, 1.0);
        u = u * u * (3.0 - 2.0 * u);
        return kCurrent * (1.0 - 0.82 * u);
    }
    return kCurrent;
}

double Game::groundSpeed() const {
    const double vx = std::cos(heading_) * surge_;
    const double vy = std::sin(heading_) * surge_ + flood();
    return std::hypot(vx, vy);
}

double Game::crewY() const {
    double u = clampd(race_ / kCrew, 0.0, 1.0);
    return 24.0 + (kNestDriveY - 24.0) * u;
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
    wakeT_ = 0;
    hornT_ = 0;
    thumpT_ = 0;
    shake_ = 0;
    why_[0] = 0;
    for (Puff& p : wake_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 0;
    camY_ = 118.f;
    zoom_ = 0.86f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = float(x_);
    camY_ = float(y_);
    zoom_ = kPlayZoom;
    horn(0.32f);
    blip(440.f);
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
    if (ahead && !astern) throttle_ = std::min(1.0, throttle_ + kDt * 1.15);
    else if (astern && !ahead) throttle_ = std::max(-1.0, throttle_ - kDt * 1.15);
    else throttle_ *= std::exp(-1.35 * kDt);
    if (p.accel > 0.08f) throttle_ = std::min(1.0, throttle_ + p.accel * kDt * 1.0);
    if (p.brake > 0.08f) throttle_ = std::max(-1.0, throttle_ - p.brake * kDt * 1.0);
    if (p.down(gs::BTN_TURBO) || p.pressed(gs::BTN_Y)) horn(0.16f);
}

void Game::pilot(double& steer) {
    const double nestBargeY = kNestDriveY - kLead;
    const Bounds d = driveBounds();
    const double flo = flood();
    const bool in = driveInside();
    const bool lined = std::fabs(x_) < 1.4 && std::fabs(wrap(kNorth - heading_)) < 0.22;

    auto throttleFor = [&](double wantVy, double sign) {
        double snh = std::sin(heading_);
        if (std::fabs(snh) < 0.45) snh = std::copysign(0.45, snh);
        double surgeCmd = (wantVy - flo) / snh;
        if (sign < 0 && surgeCmd > 0) surgeCmd = 0;
        if (sign > 0 && surgeCmd < 0) surgeCmd = 0;
        double cap = surgeCmd >= 0 ? kAhead : kAstern;
        throttle_ = clampd(surgeCmd / cap, -1.0, 1.0);
    };

    if (d.maxY > kHead - 8.0) {
        double hdes = kNorth - clampd(x_ * 0.08, -0.35, 0.35);
        steer = clampd(wrap(hdes - heading_) / 0.22, -1.0, 1.0);
        throttleFor(-1.2, -1.0);
        return;
    }

    if (in && lined) {
        double wantVy = clampd((nestBargeY - y_) * 0.7, -0.7, 1.4);
        double sign = (wantVy >= flo - 0.1) ? 1.0 : -1.0;
        double hdes = kNorth + sign * clampd(x_ * 0.1, -0.18, 0.18);
        steer = clampd(wrap(hdes - heading_) / 0.18, -1.0, 1.0);
        throttleFor(wantVy, sign);
        return;
    }

    double dist = nestBargeY - y_;
    double wantVy;
    if (dist > 70.0) wantVy = 6.2;
    else if (dist > 36.0) wantVy = 3.6;
    else if (dist > 16.0) wantVy = 2.05;
    else if (dist > 0.4) wantVy = 1.15;
    else wantVy = -1.1;

    if (y_ > kMouth - 36.0 && (std::fabs(x_) > 1.5 || std::fabs(wrap(kNorth - heading_)) > 0.28)) wantVy = std::min(wantVy, 1.15);
    if (y_ > kMouth - 16.0 && std::fabs(x_) > kPocket - 5.0) wantVy = std::min(wantVy, 0.85);

    double sign = (wantVy >= flo - 0.1) ? 1.0 : -1.0;
    double gain = (y_ > kMouth - 48.0) ? 0.16 : 0.09;
    double hdes = kNorth + sign * clampd(x_ * gain, -0.7, 0.7);
    if (std::fabs(wrap(hdes - heading_)) > 0.9) wantVy = std::min(wantVy, 2.2);
    steer = clampd(wrap(hdes - heading_) / 0.22, -1.0, 1.0);
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
    sys_->setLight(40, 170, 60);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.55f, 0.24f, 180);
    sys_->setLight(170, 30, 20);
    sys_->apu.noiseBurst(0.42f, 120.f, 0.4f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.4f;
}

void Game::physics(double steer) {
    double rate = 0.85 + std::min(std::fabs(surge_), 8.0) * 0.035;
    heading_ = wrap(heading_ + steer * rate * kDt);
    double target = throttle_ >= 0 ? throttle_ * kAhead : throttle_ * kAstern;
    surge_ += (target - surge_) * (1.0 - std::exp(-1.7 * kDt));
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
        sys_->apu.noiseBurst(0.2f, 160.f, 0.1f);
        thumpT_ = 0.22f;
        shake_ = std::max(shake_, 0.3f);
    };

    const Box walls[] = {
        {-400, -kWall, -400, kMouth, false},
        {kWall, 400, -400, kMouth, false},
        {-400, 400, -400, kSouth, false},
        {-400, -kCheek, kMouth - 0.4, kEnd + 16, false},
        {kCheek, 400, kMouth - 0.4, kEnd + 16, false},
        {-kCheek, -kPocket, kMouth, kHead + 1.5, true},
        {kPocket, kCheek, kMouth, kHead + 1.5, true},
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
        const double tf[6] = {kBL, -kBL, 1.5, -1.5, 4.0, -4.0};
        const double tb[6] = {0, 0, kBW, -kBW, kBW * 0.7, -kBW * 0.7};
        for (int i = 0; i < 6; i++) add(x_, y_, tf[i], tb[i]);
        const double df[6] = {kDL, -kDL, kDL, -kDL, 0, 0};
        const double db[6] = {kDW, kDW, -kDW, -kDW, kDW, -kDW};
        for (int i = 0; i < 6; i++) add(dx, dy, df[i], db[i]);

        double x0 = x_, y0 = y_;
        for (int i = 0; i < n; i++) {
            for (const Box& w : walls) {
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
        if (md > 1.8) {
            x_ = x0 + mx / md * 1.8;
            y_ = y0 + my / md * 1.8;
        }
    }

    if (boomHit && hitSpd > kBreak) {
        fail("broke the boom");
        return;
    }
    if (boomHit) {
        surge_ *= 0.55;
        thud();
    }

    double vy = std::sin(heading_) * surge_ + flo;
    double tips[8][2];
    int n = 0;
    {
        const double c = std::cos(heading_), s = std::sin(heading_);
        const double dx = x_ + kLead * c;
        const double dy = y_ + kLead * s;
        auto add = [&](double ox, double oy, double f, double b) {
            tips[n][0] = ox + f * c + b * s;
            tips[n][1] = oy + f * s - b * c;
            n++;
        };
        add(x_, y_, kBL, 0);
        add(dx, dy, kDL, kDW);
        add(dx, dy, kDL, -kDW);
        add(dx, dy, -kDL, kDW);
        add(dx, dy, -kDL, -kDW);
    }
    double worst = -1e9;
    bool head = false;
    for (int i = 0; i < n; i++) {
        if (tips[i][0] < -kCheek - 0.2 || tips[i][0] > kCheek + 0.2) continue;
        if (tips[i][1] > kHead && tips[i][1] < kEnd + 0.5 && tips[i][1] > worst) {
            worst = tips[i][1];
            head = true;
        }
    }
    if (head) {
        if (vy > kBreak) {
            fail("broke the boom");
            return;
        }
        y_ -= worst - (kHead - 0.15);
        surge_ *= 0.45;
        thud();
    }
    for (int i = 0; i < n; i++) {
        if (tips[i][1] > kEnd) {
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
    wakeT_ -= float(kDt);
    if (wakeT_ <= 0 && g > 1.4) {
        wakeT_ = 0.08f;
        wake_[wakeCursor_].x = x_ - c * 7.4;
        wake_[wakeCursor_].y = y_ - s * 7.4;
        wake_[wakeCursor_].life = 1;
        wakeCursor_ = (wakeCursor_ + 1) % 18;
    }
    for (Puff& p : wake_)
        if (p.life > 0) p.life -= kDt * 0.5;

    if (g < kStop && in) {
        outT_ = 0;
        hold_ += kDt;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
    } else if (g < kStop) {
        hold_ = 0;
        outT_ += kDt;
        if (outT_ >= kOutNeed) {
            fail(db.maxY < kMouth ? "stopped short of the boom" : "the drive is not on the boom");
            return;
        }
    } else {
        hold_ = 0;
        outT_ = 0;
    }

    if (race_ >= kCrew) fail("the other crew made the boom");
}

void Game::audio() {
    float water = mode_ == Mode::Run ? 0.014f + float(std::fabs(surge_) * 0.0004) : 0.009f;
    sys_->apu.noise(water, 380.f + float(std::fabs(surge_)) * 8.f, false);
    if (hornT_ > 0) {
        hornT_ -= float(kDt);
        float v = hornT_ > 0.06f ? 0.06f : std::max(0.f, hornT_) * 0.9f;
        sys_->apu.tone(0, 82.f, v);
        if (tone1_ <= 0) sys_->apu.tone(1, 123.f, v * 0.4f);
    } else if (tone0_ <= 0 && chimeN_ == 0) {
        sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && (std::fabs(throttle_) > 0.03 || std::fabs(surge_) > 0.4)) {
        float wob = 0.78f + 0.22f * std::sin(float(t_) * (6.f + float(std::fabs(throttle_)) * 8.f));
        float vol = (0.01f + float(std::fabs(throttle_)) * 0.02f) * wob;
        sys_->apu.tone(2, 36.f + float(std::fabs(throttle_)) * 18.f, vol);
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
            static const float notes[] = {349.f, 440.f, 523.f, 698.f, 880.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
            tone0_ = 0.15f;
            chimeT_ = 0.12f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - float(kDt) * 1.5f);
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
            blip(360.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            double steer = 0;
            if (bot_) pilot(steer);
            else controls(steer);
            physics(steer);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    if (mode_ == Mode::Win) sys.setLight(40, 170, 60);
    else if (mode_ == Mode::Fail) sys.setLight(170, 30, 20);
    else if (hold_ > 0.02) sys.setLight(190, 150, 30);
    else if (mode_ == Mode::Run) sys.setLight(24, 80, 120);
    camera();
    audio();
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = 0.f;
        camY_ = 112.f;
        zoom_ = 0.86f;
        return;
    }
    const double c = std::cos(heading_), s = std::sin(heading_);
    float lead = 12.f;
    if (y_ > kMouth - 20.0) lead = 6.f;
    float tx = float(x_ + c * lead);
    float ty = float(y_ + s * lead + kLead * 0.25);
    float tz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        tx = 0.f;
        ty = float((kMouth + kHead) * 0.5);
        tz = 1.7f;
    }
    float k = 1.f - std::exp(-float(kDt) * 3.6f);
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
        jx = std::sin(float(t_) * 41.f) * shake_ * 3.5f;
        jy = std::cos(float(t_) * 33.f) * shake_ * 2.5f;
    }
    float invZ = 1.f / std::max(zoom_, 0.25f);
    camX_ -= jx * invZ;
    camY_ += jy * invZ;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) * invZ;
        float half = float(kWall);
        if (wy >= float(kMouth) && wy < float(kEnd)) half = float(kPocket);
        else if (wy >= float(kEnd)) half = 1.2f;
        uint16_t near = gs::rgb4(3, 6, 3);
        uint16_t far = gs::rgb4(2, 3, 2);
        float u = std::clamp((wy + 10.f) / 240.f, 0.f, 1.f);
        v.lineBackdrop[y] = lerpC(near, far, u);
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f + (0.f - camX_) * zoom_;
        r.hw = std::max(2.f, half * zoom_);
        r.v = wy * 28.f + float(t_) * 16.f;
        r.pal = uint8_t(PAL_CH);
        r.band = (int(std::floor(wy * 0.15f + t_ * 0.8f)) & 1) ? 1 : 0;
        r.style = 2;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }

    for (double y = 6; y <= 220; y += 16) {
        place(art_.reed, -(kWall + 2.8), y, 8.f, PAL_BANK, 0, false);
        place(art_.reed, kWall + 2.8, y, 8.f, PAL_BANK, 0, true);
    }
    const double lamps[] = {48, 96, 140};
    for (double y : lamps) {
        place(art_.lamp, -(kWall + 0.4), y, 5.5f, PAL_BANK, 0);
        place(art_.lamp, kWall + 0.4, y, 5.5f, PAL_BANK, 0);
    }

    const double cheek = (kPocket + kCheek) * 0.5;
    for (double y = kMouth + 2.5; y < kHead - 1.0; y += 6.2) {
        place(art_.timber, -cheek, y, 5.6f, PAL_BOOM, 0);
        place(art_.timber, cheek, y, 5.6f, PAL_BOOM, 0);
    }
    for (double x = -kPocket + 2.2; x <= kPocket - 1.6; x += 4.4) place(art_.pile, x, kHead + 1.2, 4.2f, PAL_BOOM, 0);
    place(art_.boomArm, -4.6, kHead + 3.4, 3.4f, PAL_BOOM, 0);
    place(art_.boomArm, 4.6, kHead + 3.4, 3.4f, PAL_BOOM, 0, true);
    place(art_.timber, -kPocket, kMouth, 7.f, hold_ > 0.02 ? PAL_WIN : PAL_MARK, 0);
    place(art_.timber, kPocket, kMouth, 7.f, hold_ > 0.02 ? PAL_WIN : PAL_MARK, 0);

    int flap = int(t_ * 2.4) & 1;
    place(art_.bird[flap], -8 + std::sin(t_ * 0.4) * 10, 70 + std::cos(t_ * 0.22) * 4, 2.8f, PAL_BIRD, 0);

    for (const Puff& p : wake_) {
        if (p.life <= 0) continue;
        float h = (1.4f + float(1.0 - p.life) * 2.0f) * zoom_;
        float sx = 160.f + float(p.x - camX_) * zoom_;
        float sy = 112.f - float(p.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false, false);
    }

    const double c = std::cos(heading_), s = std::sin(heading_);
    const double dx = x_ + kLead * c;
    const double dy = y_ + kLead * s;
    for (int i = 1; i <= 3; i++) {
        double u = i / 4.0;
        place(art_.chain, x_ + c * (kBL * 0.6 + (kLead - kBL) * u), y_ + s * (kBL * 0.6 + (kLead - kBL) * u), 1.0f,
              PAL_DRIVE, 0);
    }

    int fi = frameOf(heading_);
    float bob = std::sin(float(t_) * 1.7f) * 0.25f;
    auto blitCraft = [&](const gs::Mipped& m, double wx, double wy, int pal) {
        float bh = float(m.h) / float(kPx) * zoom_;
        if (mode_ == Mode::Title) bh = std::max(bh, 18.f);
        float sx = 160.f + float(wx - camX_) * zoom_;
        float sy = 112.f - float(wy - camY_) * zoom_ + bob * zoom_ * 0.1f;
        spr(art_.shade, sx + 2.f, sy + 3.f, bh * 0.55f, pal, false, true);
        spr(m, sx, sy, bh, pal, false, false);
    };

    if (mode_ != Mode::Win) {
        double cy = crewY();
        int cf = frameOf(kNorth);
        blitCraft(art_.barge[cf], 16.5, cy - kLead * 0.15, PAL_CREW);
        blitCraft(art_.drive[cf], 16.5, cy + kLead * 0.55, PAL_CREW);
    }
    blitCraft(art_.drive[fi], dx, dy, PAL_DRIVE);
    blitCraft(art_.barge[fi], x_, y_, PAL_BARGE);

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
        else if (!std::strcmp(why_, "the other crew made the boom")) banner(art_.crew, 24.f, PAL_ALERT);
        else if (!std::strcmp(why_, "the drive is not on the boom")) banner(art_.offBoom, 24.f, PAL_ALERT);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(21, "TAKE THE BARGE TO THE BOOM", PAL_BANNER);
        hudC(22, "DELIVER THE DRIVE BEFORE THE CREW", PAL_ALERT);
        hudC(23, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        if ((int(t_ * 2.0) & 1) == 0) hudC(25, "START", PAL_WIN);
        else hudC(25, "ARROWS STEER   UP AHEAD   DOWN ASTERN", PAL_HUD);
        hudC(26, "HOLD THE DRIVE INSIDE THE BOOM", PAL_HUD);
        return;
    }

    double left = std::max(0.0, kCrew - race_);
    hud(1, 0, "S3 BARGE BOOM", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "CREW %4.1f", left);
    hud(28, 0, buf, left < 12.0 ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(15, "THE DRIVE IS ON THE BOOM", PAL_WIN);
        hudC(16, "AHEAD OF THE OTHER CREW", PAL_BANNER);
        std::snprintf(buf, sizeof buf, "%.1fS", race_);
        hudC(18, buf, PAL_HUD);
        if (!bot_) hudC(20, "START RUNS IT AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, "THE LEG FAILS", PAL_ALERT);
        if (!std::strcmp(why_, "the other crew made the boom")) hudC(17, "THE OTHER CREW MADE THE BOOM", PAL_HUD);
        else if (!std::strcmp(why_, "stopped short of the boom")) hudC(17, "SHORT OF THE BOOM", PAL_HUD);
        else if (!std::strcmp(why_, "broke the boom")) hudC(17, "THE BOOM BROKE", PAL_HUD);
        else if (!std::strcmp(why_, "missed the end")) hudC(17, "YOU CROSSED THE END", PAL_HUD);
        else hudC(17, "THE DRIVE IS NOT ON THE BOOM", PAL_HUD);
        if (!bot_) hudC(19, "START TRIES AGAIN", PAL_HUD);
        return;
    }

    const Bounds db = driveBounds();
    const bool in = driveInside();
    std::snprintf(buf, sizeof buf, "BOOM %3.0f", std::max(0.0, kMouth - db.minY));
    hud(1, 1, buf, PAL_MARK);
    std::snprintf(buf, sizeof buf, "ENG %+4d", int(std::lround(throttle_ * 100.0)));
    hud(30, 1, buf, PAL_HUD);
    if (in && groundSpeed() < kStop) {
        int n = std::clamp(int(hold_ / kHoldNeed * 5.0) + 1, 1, 5);
        std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
        hud(1, 2, buf, PAL_WIN);
    } else if (in) {
        hud(1, 2, "DRIVE ON THE BOOM  HOLD STILL", PAL_BANNER);
    } else {
        hud(1, 2, "DELIVER THE DRIVE TO THE BOOM", PAL_HUD);
    }
    hud(1, 27, "THE CLOCK IS THE OTHER CREW", left < 12.0 ? PAL_ALERT : PAL_HUD);
}

}  // namespace bargeboom
