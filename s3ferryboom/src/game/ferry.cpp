#include "game/ferry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace fboom {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kPi = 3.141592653589793;
constexpr double kTau = 6.283185307179586;
constexpr double kNorth = kPi * 0.5;

constexpr double kWall = 26.0;
constexpr double kStickIn = 11.2;
constexpr double kStickOut = 15.0;
constexpr double kMouth = 108.0;
constexpr double kHead = 136.0;
constexpr double kEnd = 148.0;
constexpr double kFit = 0.55;

constexpr double kHL = 7.2;
constexpr double kHW = 3.1;
constexpr double kDL = 5.4;
constexpr double kDW = 2.6;
constexpr double kLead = 14.2;
constexpr double kNestY = 122.0;

constexpr double kFlow = 1.55;
constexpr double kAhead = 7.6;
constexpr double kAstern = 6.4;
constexpr double kHoldNeed = 0.55;
constexpr double kTide = 90.0;
constexpr double kBreak = 7.2;

constexpr double kStartX = 4.0;
constexpr double kStartY = 24.0;
constexpr double kStartH = 1.35;
constexpr float kPlayZoom = 3.1f;

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
    return b.minX >= -kStickIn + kFit && b.maxX <= kStickIn - kFit && b.minY >= kMouth + kFit && b.maxY <= kHead - kFit;
}

double Game::flood() const {
    const double c = std::cos(heading_), s = std::sin(heading_);
    const double dy = y_ + kLead * s;
    const double dx = x_ + kLead * c;
    double cross = 0.85 * std::sin(t_ * 0.37);
    (void)cross;
    double along = kFlow;
    if (dy > kMouth - 4.0 && std::fabs(dx) < kStickIn - 0.4) {
        double u = clampd((dy - (kMouth - 4.0)) / 14.0, 0.0, 1.0);
        u = u * u * (3.0 - 2.0 * u);
        along *= (1.0 - 0.9 * u);
    }
    return along;
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    surge_ = 0;
    sway_ = 0;
    throttle_ = 0;
    hold_ = 0;
    clock_ = kTide;
    clock0_ = kTide;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    chimeStep_ = 0;
    hornT_ = 0;
    shake_ = 0;
    foamN_ = 0;
    why_[0] = 0;
    for (Puff& p : foam_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 0;
    camY_ = 96.f;
    zoom_ = 1.15f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = float(x_);
    camY_ = float(y_);
    zoom_ = kPlayZoom;
    blip(480.f);
    hornT_ = 0.32f;
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

void Game::controls(double& steer, double& bow) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    bow = 0;
    if (p.down(gs::BTN_LEFT)) steer += 1;
    if (p.down(gs::BTN_RIGHT)) steer -= 1;
    if (std::fabs(p.axisX) > 0.18f) steer = clampd(double(-p.axisX), -1.0, 1.0);
    if (p.down(gs::BTN_Z)) bow += 1;
    if (p.down(gs::BTN_X)) bow -= 1;
    const bool ahead = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C);
    const bool astern = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B);
    if (ahead && !astern) throttle_ = std::min(1.0, throttle_ + kDt * 1.5);
    else if (astern && !ahead) throttle_ = std::max(-1.0, throttle_ - kDt * 1.5);
    else throttle_ *= std::exp(-1.6 * kDt);
    if (p.pressed(gs::BTN_Y)) hornT_ = 0.18f;
}

void Game::pilot(double& steer, double& bow) {
    const double nestFerry = kNestY - kLead;
    const Bounds d = driveBounds();
    const double flo = flood();
    const bool in = driveInside();

    auto throttleFor = [&](double wantVy) {
        double snh = std::sin(heading_);
        if (std::fabs(snh) < 0.55) snh = std::copysign(0.55, snh > 0 || heading_ > 0 ? 1.0 : -1.0);
        double surgeCmd = (wantVy - flo) / snh;
        double cap = surgeCmd >= 0 ? kAhead : kAstern;
        throttle_ = clampd(surgeCmd / cap, -1.0, 1.0);
    };

    // Positive heading error from north points the bow west, which sheds +x.
    auto aim = [&](double gain, double cap) {
        double hdes = kNorth + clampd(x_ * gain, -cap, cap);
        steer = clampd(wrap(hdes - heading_) / 0.22, -1.0, 1.0);
    };

    if (d.maxY > kHead - 5.0) {
        aim(0.08, 0.3);
        bow = clampd(-x_ * 0.4, -1.0, 1.0);
        throttleFor(flo - 2.0);
        return;
    }

    if (in && std::fabs(x_) < 2.2) {
        aim(0.04, 0.12);
        bow = std::fabs(x_) < 0.35 ? 0 : clampd(-x_ * 0.45, -0.5, 0.5);
        double want = flo + clampd((nestFerry - y_) * 0.45, -0.35, 0.55);
        throttleFor(want);
        return;
    }

    // Too wide for the mouth: drop back and square up before the sticks.
    if (y_ > kMouth - 18.0 && std::fabs(x_) > 2.4) {
        aim(0.22, 0.85);
        bow = clampd(-x_ * 0.45, -1.0, 1.0);
        throttleFor(flo - 2.4);
        return;
    }

    double dist = nestFerry - y_;
    double wantVy = flo + (dist > 50 ? 4.8 : dist > 22 ? 2.6 : dist > 8 ? 1.3 : 0.45);
    if (std::fabs(wrap(kNorth - heading_)) > 0.55) wantVy = flo + 0.8;
    aim(y_ > kMouth - 46 ? 0.18 : 0.1, 0.65);
    bow = (y_ > kMouth - 40) ? clampd(-x_ * 0.3, -0.8, 0.8) : 0;
    throttleFor(wantVy);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone0_ = 0.08f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    std::snprintf(why_, sizeof why_, "delivered");
    chimeN_ = 5;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.4f, 120.f, 0.4f);
}

void Game::physics(double steer, double bow) {
    double rate = 1.05 + std::min(std::fabs(surge_), 8.0) * 0.04;
    heading_ = wrap(heading_ + steer * rate * kDt);
    double target = throttle_ >= 0 ? throttle_ * kAhead : throttle_ * kAstern;
    surge_ += (target - surge_) * (1.0 - std::exp(-2.4 * kDt));
    surge_ = clampd(surge_, -kAstern, kAhead);
    sway_ += (bow * 3.4 - sway_) * (1.0 - std::exp(-3.2 * kDt));

    const double flo = flood();
    const double cross = 0.35 * std::sin(t_ * 0.41);
    const double c0 = std::cos(heading_), s0 = std::sin(heading_);
    // Bow thruster pushes to port when bow > 0 (local +X is starboard? heading 0 is east).
    // Port is left of the bow: direction (-sin, cos) when bow positive... 
    // If x is too far right, pilot sets bow negative? I set bow = -x, so negative bow when x>0.
    // Negative bow should move the hull left (negative x). Use sway along (-sin, cos) so +sway is port.
    x_ += (c0 * surge_ - s0 * sway_ + cross) * kDt;
    y_ += (s0 * surge_ + c0 * sway_ + flo) * kDt;

    const double hitSpd = std::hypot(c0 * surge_ - s0 * sway_, s0 * surge_ + c0 * sway_ + flo);

    const Box walls[] = {
        {-400, -kWall, -40, kMouth, false},
        {kWall, 400, -40, kMouth, false},
        {-400, -kStickOut, kMouth - 0.2, kEnd + 8, false},
        {kStickOut, 400, kMouth - 0.2, kEnd + 8, false},
        {-kStickOut, -kStickIn, kMouth, kHead + 1.5, true},
        {kStickIn, kStickOut, kMouth, kHead + 1.5, true},
    };

    bool boomHit = false;
    for (int iter = 0; iter < 3; iter++) {
        const double c = std::cos(heading_), s = std::sin(heading_);
        const double dx = x_ + kLead * c;
        const double dy = y_ + kLead * s;
        double px[12], py[12];
        int n = 0;
        auto add = [&](double ox, double oy, double f, double b) {
            px[n] = ox + f * c + b * s;
            py[n] = oy + f * s - b * c;
            n++;
        };
        add(x_, y_, kHL, 0);
        add(x_, y_, -kHL, 0);
        add(x_, y_, 0, kHW);
        add(x_, y_, 0, -kHW);
        add(dx, dy, kDL, kDW);
        add(dx, dy, kDL, -kDW);
        add(dx, dy, -kDL, kDW);
        add(dx, dy, -kDL, -kDW);
        add(dx, dy, kDL, 0);
        add(dx, dy, -kDL, 0);

        double x0 = x_, y0 = y_;
        for (int i = 0; i < n; i++) {
            for (const Box& w : walls) {
                if (px[i] < w.x0 || px[i] > w.x1 || py[i] < w.y0 || py[i] > w.y1) continue;
                double dl = px[i] - w.x0, dr = w.x1 - px[i], db = py[i] - w.y0, dt = w.y1 - py[i];
                if (dl <= dr && dl <= db && dl <= dt) x_ -= dl + 0.05;
                else if (dr <= db && dr <= dt) x_ += dr + 0.05;
                else if (db <= dt) y_ -= db + 0.05;
                else y_ += dt + 0.05;
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
    if (boomHit) surge_ *= 0.7;

    {
        const double c = std::cos(heading_), s = std::sin(heading_);
        const double dx = x_ + kLead * c;
        const double dy = y_ + kLead * s;
        const double pts[4][2] = {{kDL, kDW}, {kDL, -kDW}, {-kDL, kDW}, {-kDL, -kDW}};
        for (auto& pt : pts) {
            double px = dx + pt[0] * c + pt[1] * s;
            double py = dy + pt[0] * s - pt[1] * c;
            if (py > kEnd) {
                fail("missed the end");
                return;
            }
            if (py > kHead + 1.2 && std::fabs(px) < kStickOut) {
                if ((s * surge_ + flo) > kBreak) {
                    fail("broke the boom");
                    return;
                }
                y_ -= py - (kHead - 0.2);
                surge_ *= 0.55;
            }
        }
    }

    if (clock_ <= 0) {
        fail("the tide ran out");
        return;
    }

    const bool in = driveInside();
    if (in && std::fabs(surge_) < 0.95 && std::fabs(sway_) < 1.15) {
        hold_ += kDt;
        if (hold_ >= kHoldNeed) win();
    } else {
        hold_ = 0;
    }

    if (std::fabs(surge_) > 1.4) {
        foam_[foamN_].x = x_ - c0 * 6.5;
        foam_[foamN_].y = y_ - s0 * 6.5;
        foam_[foamN_].life = 1;
        foamN_ = (foamN_ + 1) % 10;
    }
    for (Puff& p : foam_)
        if (p.life > 0) p.life -= kDt * 0.6;
}

void Game::audio() {
    if (hornT_ > 0) {
        hornT_ -= float(kDt);
        sys_->apu.tone(0, 92.f, 0.08f);
    } else if (mode_ == Mode::Run) {
        float eng = 48.f + float(std::fabs(surge_)) * 7.f;
        sys_->apu.tone(0, eng, 0.03f + float(std::fabs(throttle_)) * 0.02f);
    } else {
        sys_->apu.tone(0, 0, 0);
    }
    if (tone0_ > 0) tone0_ -= float(kDt);
    else if (chimeN_ == 0) sys_->apu.tone(1, 0, 0);
    if (chimeN_ > 0) {
        chimeT_ -= float(kDt);
        if (chimeT_ <= 0) {
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f, 784.f};
            sys_->apu.tone(1, notes[chimeStep_ % 5], 0.06f);
            chimeStep_++;
            chimeT_ = 0.16f;
            if (chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    t_ += kDt;
    if (mode_ == Mode::Title) {
        const gs::Pad& p = sys.pad;
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) startRun();
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        if (sys.pad.pressed(gs::BTN_B)) showTitle();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            double steer = 0, bow = 0;
            if (bot_) pilot(steer, bow);
            else controls(steer, bow);
            clock_ -= kDt;
            physics(steer, bow);
        }
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
        showTitle();
    }
    audio();
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s) return;
    for (int i = 0; s[i] && col + i < 40; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c < 32 || c > 127) c = ' ';
        sys_->vdp.HUD.set(col + i, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s && s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -20 || cy + h * 0.5f < -20 || cx - w * 0.5f > gs::SCREEN_W + 20 || cy - h * 0.5f > gs::SCREEN_H + 20)
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

void Game::place(const gs::Mipped& m, double wx, double wy, float worldH, int pal) {
    float sx = 160.f + (float(wx) - camX_) * zoom_;
    float sy = 112.f - (float(wy) - camY_) * zoom_;
    spr(m, sx, sy, worldH * zoom_, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shakeX = 0, shakeY = 0;
    if (shake_ > 0) {
        shakeX = std::sin(t_ * 40.f) * shake_ * 3.f;
        shakeY = std::cos(t_ * 33.f) * shake_ * 2.f;
        shake_ *= 0.9f;
        if (shake_ < 0.05f) shake_ = 0;
    }
    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float tx = float(x_), ty = float(y_ + 6);
        camX_ += (tx - camX_) * 0.08f;
        camY_ += (ty - camY_) * 0.08f;
        zoom_ += (kPlayZoom - zoom_) * 0.06f;
    }
    camX_ += shakeX * 0.15f;
    camY_ += shakeY * 0.15f;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.2f);
        float u = std::clamp((wy + 10.f) / 150.f, 0.f, 1.f);
        uint16_t water = lerpC(gs::rgb4(1, 4, 9), gs::rgb4(2, 8, 12), u);
        float sh = 0.5f + 0.5f * std::sin(y * 0.09f + float(t_) * 1.6f);
        water = lerpC(water, gs::rgb4(8, 13, 15), sh * 0.1f);
        v.lineBackdrop[y] = water;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) spr(art_.title, 160.f, 36.f, float(art_.title.h), PAL_BANNER);
    else if (mode_ == Mode::Win) spr(art_.made, 160.f, 48.f, float(art_.made.h), PAL_WIN);
    else if (mode_ == Mode::Fail) spr(art_.missed, 160.f, 48.f, float(art_.missed.h), PAL_ALERT);
    else if (mode_ == Mode::Pause) spr(art_.paused, 160.f, 48.f, float(art_.paused.h), PAL_BANNER);

    for (double yy = 8; yy < kMouth; yy += 14) {
        place(art_.bank, -kWall - 3.5, yy, 12.f, PAL_BANK);
        place(art_.bank, kWall + 3.5, yy, 12.f, PAL_BANK);
    }
    for (double yy = kMouth + 4; yy < kHead; yy += 7.5) {
        place(art_.post, -kStickOut + 1.2, yy, 6.5f, PAL_BOOM);
        place(art_.post, kStickOut - 1.2, yy, 6.5f, PAL_BOOM);
        place(art_.post, -kStickIn - 0.4, yy, 5.2f, PAL_BOOM);
        place(art_.post, kStickIn + 0.4, yy, 5.2f, PAL_BOOM);
    }
    for (double xx = -kStickOut + 4; xx <= kStickOut - 4; xx += 8)
        place(art_.head, xx, kHead + 1.2, 3.4f, PAL_BOOM);

    for (const Puff& p : foam_)
        if (p.life > 0.05) place(art_.foam, p.x, p.y, 1.4f + float(p.life), PAL_FOAM);

    const double c = std::cos(heading_), s = std::sin(heading_);
    int fr = frameOf(heading_);
    place(art_.drive[fr], x_ + kLead * c, y_ + kLead * s, 9.2f, PAL_DRIVE);
    float sx = 160.f + (float(x_) - camX_) * zoom_;
    float sy = 112.f - (float(y_) - camY_) * zoom_ + std::sin(float(t_) * 2.f) * 0.6f;
    float hh = 16.f * zoom_;
    spr(art_.hull[fr], sx + 2.f, sy + 3.f, hh, PAL_FERRY, false, true);
    spr(art_.hull[fr], sx, sy, hh, PAL_FERRY);

    for (int i = 0; i < 2; i++) {
        float gx = 40.f + i * 90.f + std::sin(float(t_) * 0.5f + i) * 20.f;
        float gy = 30.f + std::cos(float(t_) * 0.4f + i) * 8.f;
        spr(art_.gull[(int(t_ * 4 + i) & 1)], gx, gy, 10.f, PAL_GULL);
    }

    char buf[64];
    int left = std::max(0, int(clock_));
    if (mode_ == Mode::Title) {
        hudC(16, "DELIVER THE DRIVE TO THE BOOM", PAL_BANNER);
        hudC(18, "MISSING THE END FAILS THE LEG", PAL_DIM);
        hudC(21, "ARROWS  ENGINE AND RUDDER", PAL_HUD);
        hudC(22, "Z BOW PORT    X BOW STARBOARD", PAL_HUD);
        if ((int(t_ * 2) & 1) == 0) hudC(25, "RETURN", PAL_WIN);
        return;
    }
    hud(1, 0, "S3 FERRY BOOM", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "TIDE %02d:%02d", left / 60, left % 60);
    hud(28, 0, buf, left < 16 ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Win) {
        hudC(16, "THE DRIVE IS ON THE BOOM", PAL_WIN);
        if (!bot_) hudC(18, "RETURN SAILS AGAIN", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_[0] ? why_ : "MISSED", PAL_ALERT);
        if (!bot_) hudC(18, "RETURN TRIES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(16, "RETURN CONTINUES", PAL_HUD);
        return;
    }
    if (hold_ > 0.02) hud(1, 26, "HOLD HER IN THE BOOM", PAL_WIN);
    else if (driveInside()) hud(1, 26, "EASE OFF", PAL_BANNER);
    else if (y_ > kMouth - 20) hud(1, 26, "NOSE THE DRIVE IN", PAL_HUD);
    else hud(1, 26, "THE BOOM IS AHEAD", PAL_HUD);
}

}  // namespace fboom
