#include "tug.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tugbox {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kPi = 3.141592653589793;
constexpr double kTau = 6.283185307179586;
constexpr double kBoxL = -11.5;
constexpr double kBoxR = 11.5;
constexpr double kBoxB = 176.0;
constexpr double kBoxT = 214.0;
constexpr double kEnd = 226.0;
constexpr double kWall = 22.0;
constexpr double kSouth = 16.0;
constexpr double kHoldY = 195.0;
constexpr double kHalfW = 3.6;
constexpr double kHalfL = 8.3;
constexpr double kMargin = 0.2;
constexpr double kStop = 0.62;
constexpr double kHoldNeed = 0.55;
constexpr double kOutNeed = 1.25;
constexpr double kCurrent = 3.15;
constexpr double kAhead = 13.0;
constexpr double kAstern = 8.5;
constexpr double kStartX = 10.0;
constexpr double kStartY = 46.0;
constexpr double kStartH = 1.05;
constexpr float kPlayZoom = 2.48f;
constexpr float kArtScale = 5.1f;

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

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.08) return 3;
    if (hullInside()) return 2;
    return 1;
}

int Game::hullFrame() const {
    double u = std::fmod(heading_, kTau);
    if (u < 0) u += kTau;
    int i = int(std::lround(u / kTau * 16.0)) % 16;
    if (i < 0) i += 16;
    return i;
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

bool Game::hullInside() const {
    Ext e = extents();
    return e.minX >= kBoxL + kMargin && e.maxX <= kBoxR - kMargin && e.minY >= kBoxB + kMargin &&
           e.maxY <= kBoxT - kMargin;
}

double Game::groundSpeed() const {
    double vx = std::cos(heading_) * surge_;
    double vy = std::sin(heading_) * surge_ + kCurrent;
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
    camY_ = 132.f;
    zoom_ = 1.02f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = float(x_);
    camY_ = float(y_);
    zoom_ = kPlayZoom;
    horn(0.42f);
    blip(640.f);
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
    const bool ahead = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A);
    const bool astern = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    if (ahead) throttle_ = std::min(1.0, throttle_ + kDt * 0.72);
    if (astern) throttle_ = std::max(-1.0, throttle_ - kDt * 0.62);
    if (p.accel > 0.05f) throttle_ = std::min(1.0, throttle_ + p.accel * kDt * 0.9);
    if (p.brake > 0.05f) throttle_ = std::max(-1.0, throttle_ - p.brake * kDt * 0.8);
    if (p.down(gs::BTN_TURBO)) horn(0.12f);
}

void Game::pilot(double& steer) {
    const double north = kPi * 0.5;
    const Ext e = extents();
    const bool in = hullInside();
    if (e.maxY > kEnd - 6.0) {
        phase_ = 2;
        steer = clampd(wrap(north - heading_) / 0.25, -1.0, 1.0);
        throttle_ = -1;
        return;
    }
    if (in && std::fabs(x_) < 5.4 && std::fabs(wrap(north - heading_)) < 0.42) {
        phase_ = 1;
        double snh = std::sin(heading_);
        if (std::fabs(snh) < 0.72) snh = snh >= 0 ? 0.72 : -0.72;
        double wantVy = clampd((kHoldY - y_) * 0.45, -0.40, 0.40);
        double surgeCmd = (wantVy - kCurrent) / snh;
        // Astern, the bow swings the other way: keep the sign with the screw.
        double sign = surge_ >= -0.2 ? 1.0 : -1.0;
        double hdes = north + sign * clampd(x_ * 0.045, -0.11, 0.11);
        steer = clampd(wrap(hdes - heading_) / 0.22, -1.0, 1.0);
        throttle_ = clampd(surgeCmd / (surgeCmd >= 0 ? kAhead : kAstern), -1.0, 1.0);
        return;
    }
    phase_ = 0;
    double dist = kHoldY - y_;
    double wantVy;
    if (y_ > kBoxT - kHalfL && !in) wantVy = -2.1;
    else if (dist > 40.0) wantVy = 8.0;
    else if (dist > 16.0) wantVy = 3.1 + (dist - 16.0) * 0.20;
    else wantVy = clampd(dist * 0.22, -2.0, 2.6);
    if (!in && y_ < kHoldY && e.maxY < kBoxT - 0.4) wantVy = std::max(wantVy, 1.15);
    double sign = surge_ >= -0.25 ? 1.0 : -1.0;
    double hdes = north + sign * clampd(x_ * 0.075, -0.62, 0.62);
    if (std::fabs(wrap(hdes - heading_)) > 0.75) wantVy = std::min(wantVy, 3.4);
    steer = clampd(wrap(hdes - heading_) / 0.24, -1.0, 1.0);
    double snh = std::sin(heading_);
    if (std::fabs(snh) < 0.45) snh = snh >= 0 ? 0.45 : -0.45;
    double surgeCmd = (wantVy - kCurrent) / snh;
    throttle_ = clampd(surgeCmd / (surgeCmd >= 0 ? kAhead : kAstern), -1.0, 1.0);
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
    std::snprintf(why_, sizeof why_, "stopped inside the box");
    chime(5);
    sys_->rumble(0.35f, 0.16f, 180);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.6f, 0.3f, 200);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.46f, 140.f, 0.42f);
    sys_->apu.tone(0, 78.f, 0.06f);
    tone0_ = 0.45f;
}

void Game::puff(Puff* ring, int& cursor, int n, double x, double y) {
    ring[cursor].x = x;
    ring[cursor].y = y;
    ring[cursor].life = 1;
    cursor = (cursor + 1) % n;
}

void Game::physics(double steer) {
    double rate = 1.05 + std::min(std::fabs(surge_), 12.0) * 0.055;
    heading_ = wrap(heading_ + steer * rate * kDt);
    double target = throttle_ >= 0 ? throttle_ * kAhead : throttle_ * kAstern;
    surge_ += (target - surge_) * (1.0 - std::exp(-2.6 * kDt));
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
        sys_->apu.noiseBurst(0.26f, 210.f, 0.12f);
        thumpT_ = 0.28f;
        shake_ = std::max(shake_, 0.4f);
    };
    Ext e = extents();
    if (e.minX < -kWall) {
        x_ += -kWall - e.minX;
        surge_ *= 0.42;
        thud();
    }
    e = extents();
    if (e.maxX > kWall) {
        x_ += kWall - e.maxX;
        surge_ *= 0.42;
        thud();
    }
    e = extents();
    if (e.minY < kSouth) {
        y_ += kSouth - e.minY;
        if (s * surge_ + kCurrent < 0) surge_ *= 0.45;
        thud();
    }
    e = extents();
    // Crossing the end line fails the leg. Close to the box is still not the end.
    if (e.maxY > kEnd) {
        fail("missed the end");
        return;
    }

    double g = groundSpeed();
    bool in = hullInside();
    if (in && !announced_) {
        announced_ = true;
        horn(0.28f);
    }
    wakeT_ -= float(kDt);
    smokeT_ -= float(kDt);
    if (wakeT_ <= 0 && g > 1.6) {
        wakeT_ = 0.06f;
        puff(wake_, wakeCursor_, 18, x_ - c * 7.4, y_ - s * 7.4);
    }
    if (smokeT_ <= 0 && (std::fabs(throttle_) > 0.08 || std::fabs(surge_) > 0.8)) {
        smokeT_ = 0.09f;
        puff(smoke_, smokeCursor_, 10, x_ - c * 3.6, y_ - s * 3.6);
    }
    for (Puff& p : wake_)
        if (p.life > 0) p.life -= kDt * 0.55;
    for (Puff& p : smoke_)
        if (p.life > 0) {
            p.life -= kDt * 0.4;
            p.y += kDt * 1.6;
            p.x += kDt * 0.4;
        }

    if (g < kStop && in) {
        outT_ = 0;
        hold_ += kDt;
        if (hold_ >= kHoldNeed) win();
    } else if (g < kStop) {
        hold_ = 0;
        outT_ += kDt;
        if (outT_ >= kOutNeed) fail(e.maxY < kBoxB ? "stopped short of the box" : "stopped outside the box");
    } else {
        hold_ = 0;
        outT_ = 0;
    }
}

void Game::audio() {
    float water = mode_ == Mode::Run ? 0.016f + float(std::fabs(surge_) * 0.0005) : 0.011f;
    sys_->apu.noise(water, 460.f + float(std::fabs(surge_)) * 12.f, false);
    if (hornT_ > 0) {
        hornT_ -= float(kDt);
        float v = hornT_ > 0.06f ? 0.075f : std::max(0.f, hornT_) * 1.1f;
        sys_->apu.tone(0, 110.f, v);
        if (tone1_ <= 0) sys_->apu.tone(1, 166.f, v * 0.5f);
    } else if (tone0_ <= 0 && chimeN_ == 0) {
        sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && (std::fabs(throttle_) > 0.03 || std::fabs(surge_) > 0.6)) {
        float wob = 0.72f + 0.28f * std::sin(float(t_) * (10.f + float(std::fabs(throttle_)) * 16.f));
        float vol = (0.011f + float(std::fabs(throttle_)) * 0.028f) * wob;
        float f = 46.f + float(std::fabs(throttle_)) * 34.f + float(std::fabs(surge_)) * 0.7f;
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
            static const float notes[] = {349.2f, 440.f, 523.25f, 698.5f, 880.f};
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
            blip(420.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            double steer = 0;
            if (bot_) pilot(steer);
            else controls(steer);
            physics(steer);
            if (mode_ == Mode::Run && race_ > 80.0) fail("the leg ran out");
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
        camY_ = 132.f;
        zoom_ = 1.02f;
        return;
    }
    float lead = 14.f;
    if (y_ > kBoxB - 28.0) lead = 5.f;
    if (hold_ > 0.02 || mode_ == Mode::Win || mode_ == Mode::Fail) lead = 0.f;
    float tx = float(x_ + std::cos(heading_) * lead);
    float ty = float(y_ + std::sin(heading_) * lead);
    float tz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        tx = 0.f;
        ty = float((kBoxB + kBoxT) * 0.5);
        tz = 2.12f;
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
        jx = std::sin(float(t_) * 46.f) * shake_ * 4.f;
        jy = std::cos(float(t_) * 37.f) * shake_ * 3.f;
    }
    float invZ = 1.f / std::max(zoom_, 0.25f);
    camX_ -= jx * invZ;
    camY_ += jy * invZ;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) * invZ;
        bool water = wy >= float(kSouth - 1.0) && wy <= float(kEnd + 16.0);
        uint16_t deep = gs::rgb4(1, 4, 8);
        uint16_t mid = gs::rgb4(2, 8, 12);
        float u = std::clamp((wy - 10.f) / 240.f, 0.f, 1.f);
        v.lineBackdrop[y] = lerpC(mid, deep, u);
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f + (0.f - camX_) * zoom_;
        r.hw = water ? std::max(3.f, float(kWall) * zoom_) : 0.55f;
        r.v = wy * 34.f + float(t_) * 26.f;
        r.pal = uint8_t(PAL_CH);
        r.band = (int(std::floor(wy * 0.22f + t_ * 1.3f)) & 1) ? 1 : 0;
        r.style = water ? 2 : 0;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }

    const float postMin = mode_ == Mode::Title ? 9.f : 0.f;
    const int postPal = hold_ > 0.02 ? PAL_WIN : PAL_MARK;
    worldRect(art_.hatch, 0, (kBoxB + kBoxT) * 0.5, kBoxR - kBoxL, kBoxT - kBoxB, PAL_MARK);
    const double hx[] = {-9.0, -3.0, 3.0, 9.0};
    for (double x : hx) {
        place(art_.hbar, x, kBoxB, 1.7f, PAL_MARK, 0);
        place(art_.hbar, x, kBoxT, 1.7f, PAL_MARK, 0);
    }
    const double hy[] = {184.0, 195.0, 206.0};
    for (double y : hy) {
        place(art_.vbar, kBoxL, y, 7.f, PAL_MARK, 0);
        place(art_.vbar, kBoxR, y, 7.f, PAL_MARK, 0);
    }
    const double corners[4][2] = {{kBoxL, kBoxB}, {kBoxR, kBoxB}, {kBoxL, kBoxT}, {kBoxR, kBoxT}};
    for (const double* c : corners) place(art_.post, c[0], c[1], 8.f, postPal, postMin);

    worldRect(art_.boom, 0, kEnd + 0.7, kWall * 2.0 - 3.0, 1.5, PAL_END);
    for (double x = -18; x <= 18.1; x += 9) place(art_.buoyR, x, kEnd + 0.7, 5.2f, PAL_END, mode_ == Mode::Title ? 8.f : 0);

    worldRect(art_.bulk, 0, kSouth - 3.2, kWall * 2.0 + 6.0, 5.5, PAL_QUAY);
    worldRect(art_.bulk, 0, kEnd + 20.0, kWall * 2.0 + 10.0, 6.0, PAL_QUAY);

    for (double y = 10; y <= 250; y += 11) {
        place(art_.quay, -(kWall + 4.2), y, 12.f, PAL_QUAY, 0, false);
        place(art_.quay, kWall + 4.2, y, 12.f, PAL_QUAY, 0, true);
    }
    const double sheds[][2] = {{-34, 28}, {35, 62}, {-33, 118}, {34, 168}, {-35, 248}};
    for (const double* s : sheds) place(art_.shed, s[0], s[1], 11.f, PAL_QUAY, mode_ == Mode::Title ? 10.f : 0);
    const double lamps[] = {36, 88, 148, 198};
    for (double y : lamps) {
        place(art_.lamp, -(kWall + 0.4), y, 7.f, PAL_LAMP, 0);
        place(art_.lamp, kWall + 0.4, y, 7.f, PAL_LAMP, 0);
    }
    const double marks[] = {72, 112, 148};
    for (double y : marks) {
        place(art_.buoyG, -20.2, y, 4.6f, PAL_END, 0);
        place(art_.buoyR, 20.2, y, 4.6f, PAL_END, 0);
    }

    int flap = int(t_ * 3.2) & 1;
    place(art_.gull[flap], -14 + std::sin(t_ * 0.37) * 18, 96 + std::cos(t_ * 0.21) * 8, 4.2f, PAL_GULL,
          mode_ == Mode::Title ? 8.f : 0);
    place(art_.gull[1 - flap], 18 + std::cos(t_ * 0.29) * 12, 188 + std::sin(t_ * 0.19) * 6, 3.6f, PAL_GULL, 0);

    for (const Puff& p : wake_) {
        if (p.life <= 0) continue;
        float h = (1.8f + float(1.0 - p.life) * 2.4f) * zoom_;
        float sx = 160.f + float(p.x - camX_) * zoom_;
        float sy = 112.f - float(p.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false, false);
    }
    for (const Puff& p : smoke_) {
        if (p.life <= 0) continue;
        float h = (2.2f + float(1.0 - p.life) * 3.2f) * (zoom_ / kPlayZoom);
        float sx = 160.f + float(p.x - camX_) * zoom_;
        float sy = 112.f - float(p.y - camY_) * zoom_;
        spr(art_.smoke, sx, sy, std::max(2.f, h), PAL_SMOKE, false, false);
    }
    if (groundSpeed() > 2.2 && mode_ != Mode::Title) {
        double c = std::cos(heading_), s = std::sin(heading_);
        place(art_.foam, x_ + c * 8.0, y_ + s * 8.0, 2.4f, PAL_FOAM, 0);
    }

    int fi = hullFrame();
    float bob = std::sin(float(t_) * 2.3f + float(x_) * 0.05f) * 0.45f;
    float bh = float(art_.tug[fi].h) / kArtScale * zoom_;
    if (mode_ == Mode::Title) bh = std::max(bh, 16.f);
    float bsx = 160.f + float(x_ - camX_) * zoom_;
    float bsy = 112.f - float(y_ - camY_) * zoom_ + bob * zoom_ * 0.15f;
    spr(art_.shade, bsx + 3.f, bsy + 4.f, bh * 0.72f, PAL_TUG, false, true);
    spr(art_.tug[fi], bsx, bsy, bh, PAL_TUG, false, false);

    if (mode_ != Mode::Title) {
        float ax = 160.f + (0.f - camX_) * zoom_;
        float ay = 112.f - float((kBoxB + kBoxT) * 0.5 - camY_) * zoom_;
        bool off = ax < 14.f || ax > 306.f || ay < 16.f || ay > 208.f;
        if (off) {
            float dx = ax - 160.f, dy = ay - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 136.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 84.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 11.f, PAL_MARK, false, false);
        }
        auto chart = [&](double wx, double wy, float h, int pal) {
            float sx = 292.f + float(wx) * 0.82f;
            float sy = 78.f - float(wy - 130.0) * 0.30f;
            spr(art_.pin, sx, sy, h, pal, false, false);
        };
        chart(kBoxL, kBoxB, 3.5f, PAL_MARK);
        chart(kBoxR, kBoxB, 3.5f, PAL_MARK);
        chart(kBoxL, kBoxT, 3.5f, PAL_MARK);
        chart(kBoxR, kBoxT, 3.5f, PAL_MARK);
        chart(-kWall + 1, kEnd, 3.5f, PAL_ALERT);
        chart(kWall - 1, kEnd, 3.5f, PAL_ALERT);
        chart(x_, y_, 5.f, PAL_BANNER);
    }

    camX_ += jx * invZ;
    camY_ -= jy * invZ;

    auto banner = [&](const gs::Mipped& m, float y, int pal) { spr(m, 160.f, y, float(m.h), pal, false, false); };
    if (mode_ == Mode::Title) banner(art_.title, 18.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Win) banner(art_.stopped, 28.f, PAL_WIN);
    else if (mode_ == Mode::Fail) {
        if (!std::strcmp(why_, "missed the end")) banner(art_.missed, 30.f, PAL_ALERT);
        else if (!std::strcmp(why_, "stopped short of the box")) banner(art_.stoppedShort, 30.f, PAL_ALERT);
        else if (std::strcmp(why_, "the leg ran out") != 0) banner(art_.outside, 30.f, PAL_ALERT);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(22, "STOP INSIDE THE BOX", PAL_BANNER);
        hudC(23, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        hudC(24, "A FLOOD SETS YOU TOWARD THE END", PAL_HUD);
        if ((int(t_ * 2.0) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "ARROWS STEER   UP AHEAD   DOWN ASTERN", PAL_HUD);
        hudC(27, "SPACE HORN", PAL_HUD);
        return;
    }

    hud(1, 0, "S3 TUGBOAT BOX", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "LEG %4.1fS", race_);
    hud(30, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        hudC(19, "ESC BACK TO THE DOCK", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(15, "INSIDE THE BOX", PAL_WIN);
        hudC(16, "THE LEG IS MADE", PAL_BANNER);
        std::snprintf(buf, sizeof buf, "%.1fS", race_);
        hudC(18, buf, PAL_HUD);
        if (!bot_) hudC(20, "START RUNS THE LEG AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, "THE LEG FAILS", PAL_ALERT);
        if (!std::strcmp(why_, "missed the end")) hudC(17, "YOU CROSSED THE END", PAL_HUD);
        else if (!std::strcmp(why_, "stopped short of the box")) hudC(17, "SHORT OF THE BOX", PAL_HUD);
        else if (!std::strcmp(why_, "the leg ran out")) hudC(17, "THE LEG RAN OUT", PAL_HUD);
        else hudC(17, "CLOSE TO THE BOX IS STILL OUTSIDE", PAL_HUD);
        if (!bot_) hudC(19, "START TRIES THE LEG AGAIN", PAL_HUD);
        return;
    }

    Ext e = extents();
    double toEnd = kEnd - e.maxY;
    bool in = hullInside();
    double g = groundSpeed();
    std::snprintf(buf, sizeof buf, "END %3.0f", std::max(0.0, toEnd));
    hud(1, 1, buf, toEnd < 18 ? PAL_ALERT : PAL_MARK);
    std::snprintf(buf, sizeof buf, "ENG %+4d  SPD %4.1f", int(std::lround(throttle_ * 100.0)), g);
    hud(22, 1, buf, PAL_HUD);
    if (in && g < kStop) {
        int n = std::clamp(int(hold_ / kHoldNeed * 5.0) + 1, 1, 5);
        std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
        hud(1, 2, buf, PAL_WIN);
    } else if (in) {
        hud(1, 2, "IN THE BOX  BACK DOWN AND HOLD", PAL_BANNER);
    } else if (e.maxY > kBoxT) {
        hud(1, 2, "PAST THE BOX  THE END IS AHEAD", PAL_ALERT);
    } else {
        hud(1, 2, "BRING THE TUG INTO THE BOX", PAL_HUD);
    }
    if (race_ < 5.0) hud(1, 26, "FLOOD ASTERN  ABOUT ENG -35 HOLDS", PAL_MARK);
    else if (in) hud(1, 26, "WHOLE TUG INSIDE, THEN HOLD STILL", PAL_HUD);
    else hud(1, 26, "MISSING THE END FAILS THE LEG", PAL_ALERT);
    hud(1, 27, "ARROWS STEER  UP AHEAD  DOWN ASTERN", PAL_HUD);
}

}  // namespace tugbox
