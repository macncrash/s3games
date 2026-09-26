#include "kilo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace kilo {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kPi = 3.141592653589793;
constexpr double kTau = 6.283185307179586;
constexpr double kHalfL = 2.58;
constexpr double kHalfW = 0.96;
constexpr double kBank = 15.05;
constexpr double kMaxSpd = 16.4;
constexpr double kCurrent = 0.22;
constexpr double kCrew = 96.0;
constexpr double kBoatH = 8.4;
constexpr float kTitleZoom = 2.25f;
constexpr float kTitleCamX = 1.2f;
constexpr float kTitleCamY = 40.6f;
constexpr float kPlayZoom = 3.9f;
constexpr float kLead = 18.f;

struct Mark {
    double y, x;
};

struct WheelDef {
    double y, x, r, sway, phase;
    int kind;  // 0 mill, 1 paddle
};

// Flats hold a safe lane through each wheel. Transitions sit in the gaps.
constexpr Mark kMarks[] = {
    {0, 0},     {34, 0},    {56, -7.2}, {100, -7.2}, {140, 7.2}, {186, 7.2}, {224, -7.2}, {272, -7.2},
    {312, 0},   {358, 0},   {396, 7.2}, {444, 7.2},  {482, -7.2}, {530, -7.2}, {568, 0},   {616, 0},
    {654, 7.2}, {702, 7.2}, {740, -7.2}, {788, -7.2}, {826, 0},   {874, 0},    {912, 7.2}, {960, 7.2},
    {1004, 2.2}, {1100, 0},
};

// Channel wheels bite the centre line. Bank wheels leave a gate between them.
constexpr WheelDef kWheels[] = {
    {78, 6.15, 5.0, 0.70, 0.2, 0},   {164, -6.15, 5.0, 0.70, 1.0, 0},  {250, 6.00, 4.85, 0.65, 0.4, 1},
    {336, -12.35, 4.60, 0.25, 0.1, 0}, {336, 12.35, 4.60, 0.25, 1.6, 0}, {422, -6.15, 5.0, 0.70, 0.8, 0},
    {508, 6.05, 4.90, 0.65, 1.3, 1}, {594, -12.35, 4.60, 0.25, 0.5, 0}, {594, 12.35, 4.60, 0.25, 2.1, 0},
    {680, -6.10, 4.95, 0.65, 0.6, 0}, {766, 6.15, 5.0, 0.70, 1.5, 1},  {852, -12.35, 4.60, 0.25, 0.9, 0},
    {852, 12.35, 4.60, 0.25, 1.9, 0}, {938, -6.00, 4.80, 0.60, 0.3, 0},
};

constexpr int kMarkN = int(sizeof kMarks / sizeof kMarks[0]);
constexpr int kWheelN = int(sizeof kWheels / sizeof kWheels[0]);

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
    if (meters_ >= 820) return 3;
    if (meters_ >= 240) return 2;
    return 1;
}

double Game::laneAt(double y) const {
    if (y <= kMarks[0].y) return kMarks[0].x;
    for (int i = 1; i < kMarkN; i++) {
        if (y <= kMarks[i].y) {
            double den = kMarks[i].y - kMarks[i - 1].y;
            double u = den > 1e-6 ? (y - kMarks[i - 1].y) / den : 0.0;
            u = u * u * (3.0 - 2.0 * u);
            return kMarks[i - 1].x + (kMarks[i].x - kMarks[i - 1].x) * u;
        }
    }
    return kMarks[kMarkN - 1].x;
}

void Game::sample(double lx, double ly, double& wx, double& wy) const {
    const double c = std::cos(heading_), s = std::sin(heading_);
    wx = x_ + ly * c + lx * s;
    wy = y_ + ly * s - lx * c;
}

double Game::bowY() const {
    double wx, wy;
    sample(0, kHalfL, wx, wy);
    return wy;
}

int Game::wheelHit() const {
    const double lx[3] = {-kHalfW, 0, kHalfW};
    const double ly[3] = {-kHalfL, 0, kHalfL};
    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& w = kWheels[i];
        if (std::fabs(y_ - w.y) > w.r + kHalfL + 1.2) continue;
        double wx = w.x + std::sin(t_ * 0.9 + w.phase) * w.sway;
        if (std::fabs(x_ - wx) > w.r + kHalfL + 1.2) continue;
        const double lim = w.r * w.r;
        for (double x : lx) {
            for (double y : ly) {
                double sx, sy;
                sample(x, y, sx, sy);
                double dx = sx - wx, dy = sy - w.y;
                if (dx * dx + dy * dy < lim) return i + 1;
            }
        }
    }
    return 0;
}

bool Game::beached() const {
    const double lx[3] = {-kHalfW, 0, kHalfW};
    const double ly[3] = {-kHalfL, 0, kHalfL};
    for (double x : lx) {
        for (double y : ly) {
            double sx, sy;
            sample(x, y, sx, sy);
            if (std::fabs(sx) > kBank) return true;
        }
    }
    return false;
}

void Game::begin() {
    x_ = 0;
    y_ = -kHalfL;
    heading_ = kPi * 0.5;
    speed_ = 0;
    throttle_ = 0;
    raceTime_ = 0;
    crew_ = kCrew;
    meters_ = 0;
    cleared_ = 0;
    danger_ = false;
    shake_ = 0;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    chimeStep_ = 0;
    wakeCursor_ = 0;
    wakeT_ = 0;
    lastSec_ = int(std::ceil(kCrew));
    why_[0] = 0;
    report_[0] = 0;
    failPal_ = PAL_ALERT;
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
    blip(680.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.16f, 0.22f, 0.1f);
    begin();
    if (bot_) {
        mode_ = Mode::Run;
        zoom_ = kPlayZoom;
        camX_ = float(x_);
        camY_ = float(y_ + 8.0);
    } else {
        showTitle();
    }
}

void Game::controls(double& steer, double& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    if (p.down(gs::BTN_LEFT)) steer += 1;
    if (p.down(gs::BTN_RIGHT)) steer -= 1;
    if (std::fabs(p.axisX) > 0.15f) steer = std::clamp(double(-p.axisX), -1.0, 1.0);
    const bool up = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) ||
                    p.accel > 0.2f;
    const bool down = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.2f;
    if (up) throttle_ = std::min(1.0, throttle_ + kDt * 0.9);
    if (down) throttle_ = std::max(-0.45, throttle_ - kDt * 1.15);
    if (!up && !down) throttle_ += (0.1 - throttle_) * (1.0 - std::exp(-0.8 * kDt));
    if (p.accel > 0.25f) throttle_ = std::max(throttle_, double(p.accel));
    throttle = throttle_;
}

void Game::pilot(double& steer, double& throttle) {
    const double here = laneAt(y_);
    const double tx = here * 0.62 + laneAt(y_ + 11.0) * 0.38;
    const double xerr = tx - x_;
    const double hdes = kPi * 0.5 - std::clamp(xerr * 0.075, -0.48, 0.48);
    const double err = wrap(hdes - heading_);
    steer = std::clamp(err / 0.26, -1.0, 1.0);
    if (x_ > 11.0) steer = std::max(steer, 0.4);
    if (x_ < -11.0) steer = std::min(steer, -0.4);
    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& w = kWheels[i];
        double wx = w.x + std::sin(t_ * 0.9 + w.phase) * w.sway;
        double dx = x_ - wx;
        double dy = y_ - w.y;
        double d = std::hypot(dx, dy);
        double lim = w.r + 3.1;
        if (d < lim && dy > -5.0 && dy < w.r + 7.0) {
            double push = dx >= 0 ? -1.0 : 1.0;
            steer += push * std::min(1.1, (lim - d) / 2.0);
        }
    }
    steer = std::clamp(steer, -1.0, 1.0);
    double want = 13.4;
    if (std::fabs(err) > 0.5) want = 9.0;
    if (std::fabs(x_ - here) > 3.2) want = 9.2;
    double cmd = speed_ > want + 1.0 ? want - 2.0 : want;
    throttle = std::clamp(cmd / kMaxSpd, -0.3, 1.0);
}

void Game::win() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    meters_ = 1000;
    std::snprintf(report_, sizeof report_,
                  "S3 SKIFF KILO  PASS  finished the kilometer clean  1000 m  wheels untouched  (%.1fs, %.1fs left on "
                  "the crew clock)",
                  raceTime_, std::max(0.0, crew_));
    std::printf("%s\n", report_);
    std::fflush(stdout);
    chime(5);
    sys_->rumble(0.35f, 0.22f, 160);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    shake_ = 8;
    std::snprintf(why_, sizeof why_, "%s", why);
    failPal_ = PAL_ALERT;
    std::snprintf(report_, sizeof report_, "S3 SKIFF KILO  FAIL  %s at %d m", why, meters_);
    std::printf("%s\n", report_);
    std::fflush(stdout);
    sys_->apu.noiseBurst(0.4f, 110.f, 0.45f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.45f;
}

void Game::physics(double steer, double throttle) {
    const double rate = 2.05 - std::min(std::fabs(speed_), 14.0) * 0.055;
    heading_ = wrap(heading_ + steer * rate * kDt);
    const double cap = throttle >= 0 ? kMaxSpd : 6.5;
    const double target = throttle * cap;
    speed_ += (target - speed_) * (1.0 - std::exp(-2.6 * kDt));
    speed_ = std::clamp(speed_, -7.0, kMaxSpd + 0.4);
    const double c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * kDt;
    y_ += (s * speed_ + kCurrent) * kDt;
    if (y_ < -kHalfL) {
        y_ = -kHalfL;
        if (s * speed_ < 0) speed_ *= 0.35;
    }

    meters_ = int(std::clamp(bowY(), 0.0, 1000.0));
    danger_ = false;
    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& w = kWheels[i];
        double dy = w.y - y_;
        if (dy < -3.0 || dy > 18.0) continue;
        double wx = w.x + std::sin(t_ * 0.9 + w.phase) * w.sway;
        if (std::fabs(x_ - wx) < w.r + 4.0) danger_ = true;
    }

    if (beached()) {
        fail("left the fairway");
        return;
    }
    if (int hit = wheelHit()) {
        fail(kWheels[hit - 1].kind ? "touched a paddle wheel" : "touched a mill wheel");
        return;
    }
    if (bowY() >= 1000.0) {
        win();
        return;
    }

    while (cleared_ < kWheelN && y_ > kWheels[cleared_].y + kWheels[cleared_].r + 0.4) {
        if (cleared_ == 0 || std::fabs(kWheels[cleared_].y - kWheels[cleared_ - 1].y) > 2.0) blip(620.f);
        cleared_++;
    }

    double g = std::hypot(c * speed_, s * speed_ + kCurrent);
    wakeT_ -= kDt;
    if (wakeT_ <= 0 && g > 4.0 && mode_ == Mode::Run) {
        wakeT_ = 0.08;
        Wake w;
        w.x = x_ - c * 3.2;
        w.y = y_ - s * 3.2;
        w.life = 1;
        wakes_[wakeCursor_] = w;
        wakeCursor_ = (wakeCursor_ + 1) % 16;
    }
    for (Wake& w : wakes_)
        if (w.life > 0) w.life -= kDt;
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.045f);
    tone1_ = 0.07f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 6);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio() {
    float water = mode_ == Mode::Run ? 0.014f + float(std::fabs(speed_) * 0.0008) : 0.008f;
    sys_->apu.noise(water, 480.f, false);
    if (mode_ == Mode::Run && (std::fabs(throttle_) > 0.05 || std::fabs(speed_) > 2.0)) {
        float wob = 0.6f + 0.4f * std::sin(float(t_) * (9.f + float(std::fabs(throttle_)) * 16.f));
        float vol = (0.012f + float(std::fabs(throttle_)) * 0.03f) * wob;
        sys_->apu.tone(2, 46.f + float(std::fabs(throttle_)) * 48.f, vol);
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
    if (chimeN_ > 0) {
        chimeT_ -= float(kDt);
        if (chimeT_ <= 0) {
            static const float notes[] = {349.2f, 440.f, 523.25f, 698.5f, 880.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
            tone0_ = 0.16f;
            chimeT_ = 0.13f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (mode_ == Mode::Run) {
        int sec = std::max(0, int(std::ceil(crew_ - 1e-4)));
        if (sec != lastSec_) {
            lastSec_ = sec;
            blip(sec <= 10 ? 880.f : 440.f);
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
            if (mode_ == Mode::Run && crew_ <= 0.0) fail("the other crew took the kilometer");
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
    float lead = mode_ == Mode::Run ? kLead : 8.f;
    float gx = float(x_ + std::cos(heading_) * lead);
    float gy = float(y_ + std::sin(heading_) * lead);
    float k = 1.f - std::exp(-float(kDt) * 4.4f);
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (kPlayZoom - zoom_) * k;
    if (shake_ > 0) {
        camX_ += std::sin(float(t_) * 46.f) * float(shake_) * 0.3f;
        shake_--;
    }
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
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, double wx, double wy, float worldH, int pal, bool flip) {
    float sx = 160.f + float(wx - camX_) * zoom_;
    float sy = 112.f - float(wy - camY_) * zoom_;
    spr(m, sx, sy, worldH * zoom_, pal, flip, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.2f);
        float u = std::clamp(wy / 1000.f, 0.f, 1.f);
        uint16_t c = lerpC(gs::rgb4(1, 7, 9), gs::rgb4(2, 9, 7), u);
        float shimmer = std::sin(wy * 0.31f + float(t_) * 1.5f);
        if (shimmer > 0.93f) c = lerpC(c, gs::rgb4(9, 14, 13), 0.4f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 78.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const bool beach = why_[0] == 'l';
        const bool wheel = why_[0] == 't' && why_[1] == 'o';
        banner(beach ? art_.beached : (wheel ? art_.touched : art_.took), 160.f, 86.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.kilo, 160.f, 74.f, PAL_WIN);
        banner(art_.clean, 160.f, 100.f, PAL_WIN);
    }

    int hf = int(std::lround(std::fmod(heading_ < 0 ? heading_ + kTau : heading_, kTau) / kTau * 16.0)) % 16;
    if (hf < 0) hf += 16;
    float bob = std::sin(float(t_) * 2.3f) * 0.8f;
    float bsx = 160.f + float(x_ - camX_) * zoom_;
    float bsy = 112.f - float(y_ - camY_) * zoom_ + bob;
    float boatH = float(kBoatH) * zoom_;
    if (mode_ == Mode::Title) boatH = std::max(boatH, 16.f);
    const gs::Mipped& hull = art_.hull[hf];
    spr(hull, bsx, bsy, boatH, PAL_HULL, false, false);
    spr(hull, bsx + 3.f, bsy + 3.5f, boatH, PAL_HULL, false, true);

    const double y0 = camY_ - 70.0;
    const double y1 = camY_ + 70.0;
    for (double y = std::floor(y0 / 10.0) * 10.0; y < y1; y += 10.0) {
        place(art_.bank, -33.5, y, 14.f, PAL_BANK, true);
        place(art_.bank, 33.5, y, 14.f, PAL_BANK, false);
        place(art_.bank, -62.0, y, 14.f, PAL_BANK, true);
        place(art_.bank, 62.0, y, 14.f, PAL_BANK, false);
    }
    for (double y = std::floor(y0 / 18.0) * 18.0; y < y1; y += 18.0) {
        place(art_.reed, -16.4, y, 3.4f, PAL_BANK, false);
        place(art_.reed, 16.6, y + 6.0, 3.1f, PAL_BANK, false);
    }

    place(art_.dock, 0, -7.5, 4.2f, PAL_HOUSE, false);
    place(art_.clock, -22.5, 16, 13.f, PAL_CLOCK, false);
    int hand = int(std::fmod((1.0 - std::clamp(crew_ / kCrew, 0.0, 1.0)) * 8.0, 8.0));
    if (hand < 0) hand += 8;
    place(art_.hand[hand], -22.5, 16.6, 4.6f, PAL_CLOCK, false);
    place(art_.crewTag, -22.5, 24.5, 3.2f, PAL_TAG, false);

    const double posts[][2] = {{250, -17.2}, {500, 17.4}, {750, -17.2}, {1000, 0}};
    const gs::Mipped* tags[] = {&art_.m250, &art_.m500, &art_.m750, &art_.m1000};
    for (int i = 0; i < 4; i++) {
        place(art_.post, posts[i][1], posts[i][0], 6.5f, PAL_SPAR, false);
        place(*tags[i], posts[i][1] + (posts[i][1] < 0 ? -4.2 : 4.2), posts[i][0] + 1.5, 2.6f, PAL_TAG, false);
    }
    place(art_.line, 0, 1000, 1.35f, PAL_BUOY, false);
    for (int i = -3; i <= 3; i++) place(art_.buoy, i * 4.2, 1002.5, 2.0f, PAL_BUOY, false);
    place(art_.hull[12], 1.4, 1014, kBoatH * 0.92f, PAL_CREW, false);

    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& w = kWheels[i];
        if (w.y < y0 - 8 || w.y > y1 + 8) continue;
        double wx = w.x + std::sin(t_ * 0.9 + w.phase) * w.sway;
        double spin = t_ * 0.7 * (w.x >= 0 ? 1.0 : -1.0) + w.phase;
        spin -= std::floor(spin);
        int fr = int(spin * 8.0) % 8;
        if (fr < 0) fr += 8;
        float visual = float(w.r * 2.0 * (40.0 / 36.0));
        const gs::Mipped& img = w.kind ? art_.paddle[fr] : art_.mill[fr];
        place(img, wx, w.y, visual, w.kind ? PAL_PADDLE : PAL_MILL, false);
        if (std::fabs(w.x) > 10.0) place(art_.house, w.x > 0 ? 24.5 : -24.5, w.y, 12.f, PAL_HOUSE, w.x < 0);
    }

    int flap = int(t_ * 3.2) & 1;
    place(art_.gull[flap], -6.0 + std::sin(t_ * 0.6) * 2.0, 48, 3.0f, PAL_GULL, false);
    place(art_.gull[1 - flap], 9.0 + std::cos(t_ * 0.45) * 2.4, 210, 2.6f, PAL_GULL, false);
    place(art_.gull[flap], -4.0 + std::sin(t_ * 0.5 + 1.0) * 3.0, 640, 2.8f, PAL_GULL, false);

    for (const Wake& w : wakes_) {
        if (w.life <= 0) continue;
        float sx = 160.f + float(w.x - camX_) * zoom_;
        float sy = 112.f - float(w.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(3.f, (2.2f + float(1.0 - w.life) * 2.4f) * zoom_ / kPlayZoom), PAL_FOAM, false,
            false);
    }

    char buf[48];
    int left = std::max(0, int(std::ceil(crew_ - 1e-4)));
    if (mode_ == Mode::Title) {
        hudC(23, "FINISH THE KILOMETER", PAL_HUD);
        hudC(24, "DO NOT TOUCH A WHEEL", PAL_ALERT);
        hudC(25, "THE CLOCK IS THE OTHER CREW", PAL_BANNER);
        if ((int(t_ * 2.0) & 1) == 0) hudC(27, "START", PAL_WIN);
        else hudC(27, "ARROWS STEER   UP DRIVE   DOWN EASE", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 SKIFF KILO", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "CREW %d:%02d", left / 60, left % 60);
    hud(29, 0, buf, left <= 10 ? PAL_ALERT : PAL_TAG);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "%.1fS  CREW HAD %.1fS", raceTime_, std::max(0.0, crew_));
        hudC(16, buf, PAL_HUD);
        hudC(17, "1000 M  WHEELS UNTOUCHED", PAL_WIN);
        if (!bot_) hudC(19, "START RUNS IT AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
        std::snprintf(buf, sizeof buf, "%d M", meters_);
        hudC(17, buf, PAL_HUD);
        if (!bot_) hudC(19, "START TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "%d/1000 M", meters_);
    hud(1, 1, buf, meters_ >= 1000 ? PAL_WIN : PAL_HUD);
    if (danger_) hud(1, 2, "WHEEL AHEAD", PAL_ALERT);
    else hud(1, 2, "WHEELS CLEAN", PAL_WIN);
    std::snprintf(buf, sizeof buf, "SPD %4.1f", std::hypot(std::cos(heading_) * speed_, std::sin(heading_) * speed_ + kCurrent));
    hud(22, 2, buf, PAL_HUD);
    if (raceTime_ < 3.5) hud(1, 26, "KEEP THE SKIFF MOVING", PAL_TAG);
    else if (danger_) hud(1, 26, "PASS THE WHEEL, DO NOT TOUCH IT", PAL_ALERT);
    else hud(1, 26, "THE KILOMETER IS THEIRS IF YOU ARE SLOW", PAL_HUD);
    hud(1, 27, "ARROWS STEER   UP DRIVE   DOWN EASE", PAL_HUD);
}

}  // namespace kilo
