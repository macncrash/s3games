#include "skiff.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace skiffbox {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kPi = 3.141592653589793;
constexpr double kTau = 6.283185307179586;
constexpr double kBoxL = -29.2;
constexpr double kBoxR = 29.2;
constexpr double kBoxB = 150.0;
constexpr double kBoxT = 214.0;
constexpr double kRoadHalf = 26.0;
constexpr double kHoldX = 0.0;
constexpr double kHoldY = 182.0;
constexpr double kTideX = 0.85;
constexpr double kTideY = 2.55;
constexpr double kHalfW = 3.5;
constexpr double kHalfL = 7.6;
constexpr double kStopSpd = 1.05;
constexpr double kHoldNeed = 0.48;
constexpr double kCrew = 42.0;
constexpr double kStartX = -34.0;
constexpr double kStartY = 62.0;
constexpr double kStartH = 1.02;
constexpr float kTitleZoom = 0.55f;
constexpr float kTitleCamX = -4.f;
constexpr float kTitleCamY = 119.f;
constexpr float kPlayZoom = 1.62f;
constexpr float kBoatH = 20.f;
constexpr double kFaceX = 16.0;
constexpr double kFaceY = 250.0;

double wrap(double a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

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
    if (holdT_ > 0.05) return 3;
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

int Game::handFrame() const {
    double u = 1.0 - std::clamp(crew_ / kCrew, 0.0, 1.0);
    int i = int(u * 15.999) % 16;
    if (i < 0) i += 16;
    return i;
}

bool Game::hullInside() const {
    const double c = std::cos(heading_), s = std::sin(heading_);
    const double L = kBoxL + 0.3, R = kBoxR - 0.3, B = kBoxB + 0.3, T = kBoxT - 0.3;
    const double lx[2] = {-kHalfW, kHalfW};
    const double ly[2] = {-kHalfL, kHalfL};
    for (double x : lx) {
        for (double y : ly) {
            double wx = x_ + y * c + x * s;
            double wy = y_ + y * s - x * c;
            if (wx <= L || wx >= R || wy <= B || wy >= T) return false;
        }
    }
    return true;
}

double Game::groundSpeed() const {
    const double c = std::cos(heading_), s = std::sin(heading_);
    return std::hypot(c * speed_ + kTideX, s * speed_ + kTideY);
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 0;
    throttle_ = 0;
    phase_ = 0;
    raceTime_ = 0;
    crew_ = kCrew;
    holdT_ = 0;
    wakeT_ = 0;
    wakeCursor_ = 0;
    lastSec_ = int(kCrew);
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    report_[0] = 0;
    for (Wake& w : wakes_) w = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    zoom_ = kPlayZoom;
    camX_ = float(x_);
    camY_ = float(y_);
    blip(720.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.14f, 0.2f, 0.12f);
    begin();
    if (bot_) {
        mode_ = Mode::Run;
        zoom_ = kPlayZoom;
        camX_ = float(x_);
        camY_ = float(y_);
    } else {
        showTitle();
    }
}

void Game::controls(double& steer, double& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    if (p.down(gs::BTN_LEFT)) steer += 1;
    if (p.down(gs::BTN_RIGHT)) steer -= 1;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(double(-p.axisX), -1.0, 1.0);
    const bool up = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO);
    const bool down = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    if (up) throttle_ = std::min(1.0, throttle_ + kDt * 0.85);
    if (down) throttle_ = std::max(-1.0, throttle_ - kDt * 1.05);
    if (p.accel > 0.05f) throttle_ = std::min(1.0, throttle_ + p.accel * kDt * 1.2);
    if (p.brake > 0.05f) throttle_ = std::max(-1.0, throttle_ - p.brake * kDt * 1.2);
    throttle = throttle_;
}

void Game::pilot(double& steer, double& throttle) {
    if (phase_ == 0) {
        if (y_ > 124.0 && std::fabs(x_) < 5.5 && std::fabs(wrap(kPi * 0.5 - heading_)) < 0.30 && speed_ < 12.0) {
            phase_ = 1;
        } else {
            double cy = std::fabs(x_) > 5.0 ? y_ + 28.0 : std::max(y_ + 10.0, 132.0);
            double err = wrap(std::atan2(cy - y_, -x_) - heading_);
            steer = std::clamp(err / 0.34, -1.0, 1.0);
            double want = std::fabs(x_) > 8.0 ? 13.0 : 8.0;
            if (std::fabs(err) > 0.8) want = 3.5;
            if (y_ > 110.0) want = std::min(want, 7.0);
            throttle = std::clamp((want - speed_) * 0.25, -0.6, 0.85);
            return;
        }
    }
    if (phase_ == 1) {
        if (y_ > kHoldY - 6.0 && std::fabs(x_) < 6.0 && std::fabs(wrap(kPi * 0.5 - heading_)) < 0.35) {
            phase_ = 2;
        } else {
            double hdes = kPi * 0.5 + std::clamp(-x_ * 0.045, -0.28, 0.28);
            double err = wrap(hdes - heading_);
            steer = std::clamp(err / 0.25, -1.0, 1.0);
            double want = std::clamp((kHoldY - y_) * 0.22, 3.2, 8.0);
            throttle = std::clamp((want - speed_) * 0.3, -0.7, 0.7);
            return;
        }
    }
    double snh = std::sin(heading_);
    double wantVy = std::clamp((kHoldY - y_) * 1.6, -3.5, 3.5);
    double wantVx = std::clamp(-x_ * 1.4, -3.5, 3.5);
    double denom = std::fabs(snh) > 0.75 ? snh : (snh >= 0 ? 0.75 : -0.75);
    double spdCmd = (wantVy - kTideY) / denom;
    double hdes = kPi * 0.5;
    if (std::fabs(spdCmd) > 0.6) {
        double cDes = std::clamp((wantVx - kTideX) / spdCmd, -0.34, 0.34);
        hdes = std::acos(cDes);
    }
    double err = wrap(hdes - heading_);
    steer = std::clamp(err / 0.32, -1.0, 1.0);
    double cap = spdCmd >= 0 ? 22.0 : 12.0;
    throttle = std::clamp(spdCmd / cap, -1.0, 1.0);
}

void Game::thud() {
    if (thumpT_ > 0) return;
    sys_->apu.noiseBurst(0.28f, 240.f, 0.12f);
    thumpT_ = 0.28f;
}

void Game::physics(double steer, double throttle) {
    double rate = 1.75 + std::min(std::fabs(speed_), 16.0) * 0.035;
    heading_ = wrap(heading_ + steer * rate * kDt);
    double cap = throttle >= 0 ? 22.0 : 12.0;
    double target = throttle * cap;
    speed_ += (target - speed_) * (1.0 - std::exp(-2.8 * kDt));
    speed_ = std::clamp(speed_, -12.0, 24.0);
    double c = std::cos(heading_), s = std::sin(heading_);
    double vx = c * speed_ + kTideX;
    double vy = s * speed_ + kTideY;
    x_ += vx * kDt;
    y_ += vy * kDt;

    if (y_ < 28.0) {
        y_ = 28.0;
        if (vy < 0) speed_ *= 0.35;
        thud();
    }
    if (y_ > 236.0) {
        y_ = 236.0;
        if (vy > 0) speed_ *= 0.35;
        thud();
    }
    x_ = std::clamp(x_, -140.0, 140.0);
    if (x_ >= -44.0 && x_ <= -2.0 && y_ >= 24.0 && y_ <= 54.0) {
        y_ = 54.2;
        speed_ *= 0.45;
        thud();
    }
    const double posts[4][2] = {{-32.2, 146.2}, {32.2, 146.2}, {-32.2, 217.8}, {32.2, 217.8}};
    for (const double* p : posts) {
        double dx = x_ - p[0], dy = y_ - p[1];
        double d = std::hypot(dx, dy);
        if (d < 3.3 && d > 0.01) {
            x_ = p[0] + dx / d * 3.5;
            y_ = p[1] + dy / d * 3.5;
            speed_ *= 0.4;
            thud();
        }
    }

    wakeT_ -= kDt;
    double g = groundSpeed();
    if (wakeT_ <= 0 && g > 6.0) {
        wakeT_ = 0.07;
        Wake w;
        w.x = x_ - c * 8.0;
        w.y = y_ - s * 8.0;
        w.life = 1;
        wakes_[wakeCursor_] = w;
        wakeCursor_ = (wakeCursor_ + 1) % 20;
    }
    for (Wake& w : wakes_)
        if (w.life > 0) w.life -= kDt;

    if (hullInside() && g < kStopSpd) holdT_ += kDt;
    else holdT_ = 0;
    if (holdT_ >= kHoldNeed) win();
}

void Game::win() {
    if (won_) return;
    if (!hullInside()) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    std::snprintf(report_, sizeof report_,
                  "S3 SKIFF BOX  PASS  stopped inside the box before the other crew (%.1fs, %.1fs left)", raceTime_,
                  std::max(0.0, crew_));
    std::printf("%s\n", report_);
    std::fflush(stdout);
    chime(5);
    sys_->rumble(0.4f, 0.2f, 180);
}

void Game::fail() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    std::snprintf(report_, sizeof report_, "S3 SKIFF BOX  FAIL  the other crew took the box (%.1fs)", raceTime_);
    sys_->apu.noiseBurst(0.42f, 90.f, 0.5f);
    sys_->apu.tone(0, 82.f, 0.07f);
    tone0_ = 0.5f;
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.08f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 6);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio() {
    float water = mode_ == Mode::Run ? 0.016f + float(std::fabs(speed_) * 0.00045) : 0.01f;
    sys_->apu.noise(water, 540.f, false);
    if (mode_ == Mode::Run && (std::fabs(throttle_) > 0.04 || std::fabs(speed_) > 2.0)) {
        float wob = 0.62f + 0.38f * std::sin(float(t_) * (14.f + float(std::max(0.0, throttle_)) * 22.f));
        float vol = (0.012f + float(std::fabs(throttle_)) * 0.028f) * wob;
        sys_->apu.tone(2, 52.f + float(std::fabs(throttle_)) * 40.f + float(std::fabs(speed_)) * 0.4f, vol);
    } else if (tone0_ <= 0) {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (tone0_ > 0) {
        tone0_ -= float(kDt);
        if (tone0_ <= 0) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0) {
        tone1_ -= float(kDt);
        if (tone1_ <= 0) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (thumpT_ > 0) thumpT_ -= float(kDt);
    if (chimeN_ > 0) {
        chimeT_ -= float(kDt);
        if (chimeT_ <= 0) {
            static const float notes[] = {392.f, 523.25f, 659.25f, 784.f, 1046.5f};
            int n = std::min(chimeStep_, 4);
            sys_->apu.tone(0, notes[n], 0.055f);
            tone0_ = 0.14f;
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (mode_ == Mode::Run) {
        int sec = std::max(0, int(std::ceil(crew_ - 1e-4)));
        if (sec != lastSec_) {
            lastSec_ = sec;
            blip(sec <= 8 ? 920.f : 480.f);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(400.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            raceTime_ += kDt;
            crew_ -= kDt;
            double steer = 0, thr = throttle_;
            if (bot_) pilot(steer, thr);
            else controls(steer, thr);
            throttle_ = thr;
            physics(steer, thr);
            if (mode_ == Mode::Run && crew_ <= 0) fail();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    camera();
    audio();
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
        return;
    }
    float lead = mode_ == Mode::Run ? 12.f : 0.f;
    float gx = float(x_ + std::cos(heading_) * lead);
    float gy = float(y_ + std::sin(heading_) * lead);
    float k = 1.f - std::exp(-float(kDt) * 4.2f);
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (kPlayZoom - zoom_) * k;
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
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
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

void Game::place(const gs::Mipped& m, double wx, double wy, float worldH, int pal, float minPx) {
    float sx = 160.f + float(wx - camX_) * zoom_;
    float sy = 112.f - float(wy - camY_) * zoom_;
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    spr(m, sx, sy, h, pal, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.2f);
        uint16_t c;
        if (wy < 30.f) c = gs::rgb4(12, 10, 6);
        else if (wy > 236.f) c = gs::rgb4(10, 9, 5);
        else {
            float u = std::clamp((wy - 30.f) / 220.f, 0.f, 1.f);
            c = lerpC(gs::rgb4(3, 11, 12), gs::rgb4(1, 4, 7), u);
            float shimmer = 0.5f + 0.5f * std::sin(wy * 0.17f + float(t_) * 1.35f);
            if (shimmer > 0.94f) c = lerpC(c, gs::rgb4(10, 15, 14), 0.45f);
        }
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        if (wy >= float(kBoxB) && wy <= float(kBoxT)) {
            r.on = true;
            r.cx = 160.f + (0.f - camX_) * zoom_;
            r.hw = std::max(2.f, float(kRoadHalf) * zoom_);
            r.v = wy * 48.f;
            r.pal = uint8_t(PAL_BOX);
            r.band = (int(std::floor(wy * 0.4f)) & 1) ? 1 : 0;
            r.style = 1;
            r.left = gs::GROUND_DROP;
            r.right = gs::GROUND_DROP;
        } else {
            r.on = false;
        }
        v.B.hscroll[y] = int16_t(std::sin(y * 0.05f + float(t_) * 0.8f) * 4.f + float(t_) * 7.f);
        v.B.vscroll[y] = int16_t(float(t_) * 3.f);
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 12.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 100.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) banner(art_.crewTook, 160.f, 96.f, PAL_ALERT);
    else if (mode_ == Mode::Win) {
        banner(art_.inBox, 160.f, 78.f, PAL_WIN);
        banner(art_.ahead, 160.f, 108.f, PAL_WIN);
    }

    if (mode_ != Mode::Title) {
        float bsx = 160.f + float(kHoldX - camX_) * zoom_;
        float bsy = 112.f - float(kHoldY - camY_) * zoom_;
        if (bsx < 16.f || bsx > 304.f || bsy < 22.f || bsy > 202.f) {
            float dx = bsx - 160.f, dy = bsy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 132.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 80.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 12.f, PAL_POST, false);
        }
        auto chart = [&](double wx, double wy, float& sx, float& sy) {
            sx = 292.f + float(wx) * 0.16f;
            sy = 52.f - float(wy - 140.0) * 0.15f;
        };
        float sx, sy;
        chart(kBoxL, kBoxB, sx, sy);
        spr(art_.pin, sx, sy, 4.f, PAL_POST, false);
        chart(kBoxR, kBoxB, sx, sy);
        spr(art_.pin, sx, sy, 4.f, PAL_POST, false);
        chart(kBoxL, kBoxT, sx, sy);
        spr(art_.pin, sx, sy, 4.f, PAL_POST, false);
        chart(kBoxR, kBoxT, sx, sy);
        spr(art_.pin, sx, sy, 4.f, PAL_POST, false);
        chart(x_, y_, sx, sy);
        spr(art_.pin, sx, sy, 5.f, PAL_BANNER, false);
    }

    int hf = handFrame();
    float handMin = mode_ == Mode::Title ? 16.f : 0.f;
    float faceMin = mode_ == Mode::Title ? 18.f : 0.f;
    place(art_.hand[hf], kFaceX, kFaceY, 12.5f, PAL_CLOCK, handMin);
    place(art_.hand[(hf / 4) % 16], kFaceX, kFaceY, 8.f, PAL_CLOCK, handMin * 0.6f);
    place(art_.face, kFaceX, kFaceY, 14.f, PAL_CLOCK, faceMin);

    float bob = std::sin(float(t_) * 2.2f) * 0.7f;
    float bsx = 160.f + float(x_ - camX_) * zoom_;
    float bsy = 112.f - float(y_ - camY_) * zoom_ + bob;
    float boatH = kBoatH * zoom_;
    if (mode_ == Mode::Title) boatH = std::max(boatH, 18.f);
    const gs::Mipped& hull = art_.hull[hullFrame()];
    spr(hull, bsx, bsy, boatH, PAL_HULL, false);
    spr(hull, bsx + 3.f, bsy + 3.f, boatH, PAL_HULL, true);

    const double posts[4][2] = {{-32.2, 146.2}, {32.2, 146.2}, {-32.2, 217.8}, {32.2, 217.8}};
    float postMin = mode_ == Mode::Title ? 12.f : 0.f;
    for (const double* p : posts) place(art_.post, p[0], p[1], 12.f, PAL_POST, postMin);
    const double hx[3] = {-16, 0, 16};
    for (double x : hx) {
        place(art_.hdash, x, kBoxB, 2.4f, PAL_POST, 0);
        place(art_.hdash, x, kBoxT, 2.4f, PAL_POST, 0);
    }
    const double hy[3] = {164, 182, 200};
    for (double y : hy) {
        place(art_.vdash, kBoxL, y, 8.f, PAL_POST, 0);
        place(art_.vdash, kBoxR, y, 8.f, PAL_POST, 0);
    }

    place(art_.boxTag, 0, kBoxB - 8, 6.f, PAL_TAG, mode_ == Mode::Title ? 12.f : 0.f);
    place(art_.pier, -24, 43, 16.f, PAL_WOOD, mode_ == Mode::Title ? 18.f : 0.f);
    place(art_.shed, -30, 18, 12.f, PAL_WOOD, mode_ == Mode::Title ? 12.f : 0.f);
    place(art_.flag, -8, 52, 8.f, PAL_ALERT, mode_ == Mode::Title ? 10.f : 0.f);
    place(art_.tower, kFaceX, kFaceY - 10, 22.f, PAL_CLOCK, mode_ == Mode::Title ? 20.f : 0.f);
    place(art_.crewTag, kFaceX + 18, kFaceY - 2, 6.f, PAL_TAG, mode_ == Mode::Title ? 11.f : 0.f);
    const double tuft[4][2] = {{-20, 246}, {36, 248}, {8, 20}, {-48, 22}};
    for (const double* g : tuft) place(art_.tuft, g[0], g[1], 6.f, PAL_WIN, 0);

    int flap = int(t_ * 3.5) & 1;
    place(art_.gull[flap], -20 + std::sin(t_ * 0.31) * 36, 110 + std::cos(t_ * 0.22) * 10, 7.f, PAL_GULL,
          mode_ == Mode::Title ? 10.f : 0.f);
    place(art_.gull[1 - flap], 48 + std::cos(t_ * 0.27) * 28, 200 + std::sin(t_ * 0.2) * 8, 6.f, PAL_GULL, 0);

    for (const Wake& w : wakes_) {
        if (w.life <= 0) continue;
        float h = (2.5f + float(1.0 - w.life) * 4.f) * (zoom_ / kPlayZoom);
        float sx = 160.f + float(w.x - camX_) * zoom_;
        float sy = 112.f - float(w.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false);
    }
    if (groundSpeed() > 7.0) {
        place(art_.foam, x_ + std::cos(heading_) * 9.0, y_ + std::sin(heading_) * 9.0, 3.5f, PAL_FOAM, 0);
    }

    char buf[48];
    int left = std::max(0, int(std::ceil(crew_ - 1e-4)));
    if (mode_ == Mode::Title) {
        hudC(24, "STOP INSIDE THE BOX", PAL_HUD);
        hudC(25, "THE CLOCK IS THE OTHER CREW", PAL_BANNER);
        hudC(26, "WHEN IT RUNS OUT THEY TAKE THE BOX", PAL_ALERT);
        if ((int(t_ * 2.0) & 1) == 0) hudC(27, "START", PAL_WIN);
        else hudC(27, "ARROWS STEER   UP DRIVE   DOWN EASE", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 SKIFF BOX", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "CREW %d:%02d", left / 60, left % 60);
    hud(29, 0, buf, left <= 8 ? PAL_ALERT : PAL_TAG);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "%.1fS  CREW HAD %.1fS", raceTime_, std::max(0.0, crew_));
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "START RUNS IT AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(18, "THE OTHER CREW TOOK THE BOX", PAL_ALERT);
        if (!bot_) hudC(20, "START TRIES AGAIN", PAL_HUD);
        return;
    }
    double g = groundSpeed();
    bool in = hullInside();
    if (in && g < kStopSpd) {
        int n = std::clamp(int(holdT_ / kHoldNeed * 5.0) + 1, 1, 5);
        std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
        hud(1, 1, buf, PAL_WIN);
    } else if (in) {
        hud(1, 1, "IN THE BOX - EASE TO A STOP", PAL_BANNER);
    } else if (y_ > kBoxT) {
        hud(1, 1, "BACK INTO THE BOX", PAL_ALERT);
    } else {
        hud(1, 1, "TAKE THE SKIFF INTO THE BOX", PAL_HUD);
    }
    std::snprintf(buf, sizeof buf, "THR %+4d   SPD %4.1f", int(std::lround(throttle_ * 100.0)), g);
    hud(1, 2, buf, PAL_HUD);
    if (raceTime_ < 4.0) hud(1, 26, "TIDE SETS NORTH", PAL_TAG);
    else if (in) hud(1, 26, "WHOLE SKIFF INSIDE, THEN HOLD", PAL_HUD);
    else hud(1, 26, "STOP INSIDE BEFORE THEIR CLOCK", PAL_HUD);
    hud(1, 27, "ARROWS STEER   UP DRIVE   DOWN EASE", PAL_HUD);
}

}  // namespace skiffbox
