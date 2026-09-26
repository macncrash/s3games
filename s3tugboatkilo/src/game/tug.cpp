#include "game/tug.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tugkilo {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kPi = 3.141592653589793;
constexpr double kTau = 6.283185307179586;
constexpr double kHalfL = 4.55;
constexpr double kHalfW = 1.75;
constexpr double kBank = 16.4;
constexpr double kMaxSpd = 11.2;
constexpr double kCurrent = 0.30;
constexpr double kLeg = 155.0;
constexpr double kFinish = 1000.0;
constexpr float kBoatH = 9.6f;
constexpr float kTitleZoom = 2.8f;
constexpr float kTitleCamX = 1.5f;
constexpr float kTitleCamY = 10.f;
constexpr float kPlayZoom = 3.65f;
constexpr float kLead = 16.f;

struct Mark {
    double y, x;
};

struct WheelDef {
    double y, x, r, sway, phase;
    int kind;
};

struct Scenic {
    double y, x, r;
};

constexpr Mark kMarks[] = {
    {0, 0},     {36, 0},     {112, -8.1}, {205, -8.1}, {280, 8.1},  {372, 8.1}, {448, 0},
    {528, 0},   {604, -7.3}, {700, -7.3}, {776, 7.5},  {868, 7.5},  {944, 0},   {1020, 0},
    {1120, 0},
};

constexpr WheelDef kWheels[] = {
    {168, 3.4, 4.55, 0.0, 0.2, 0},   {338, -3.5, 4.60, 0.30, 1.1, 1}, {482, -9.05, 4.30, 0.0, 0.4, 0},
    {482, 9.05, 4.30, 0.0, 2.2, 0},  {652, 4.15, 4.45, 0.35, 0.7, 2}, {822, -3.7, 4.40, 0.25, 1.4, 1},
};

constexpr Scenic kScenic[] = {
    {18, 20.2, 2.5}, {64, -20.4, 2.2}, {210, 21.0, 2.6}, {360, -20.8, 2.3},
    {540, 20.5, 2.5}, {730, -21.2, 2.4}, {900, 20.6, 2.3},
};

constexpr int kMarkN = int(sizeof kMarks / sizeof kMarks[0]);
constexpr int kWheelN = int(sizeof kWheels / sizeof kWheels[0]);
constexpr int kScenicN = int(sizeof kScenic / sizeof kScenic[0]);

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

const char* wheelWhy(int kind) {
    if (kind == 1) return "touched a paddle wheel";
    if (kind == 2) return "touched a drum wheel";
    return "touched a mill wheel";
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (meters_ >= 800) return 3;
    if (meters_ >= 220) return 2;
    return 1;
}

int Game::hullFrame() const {
    double a = heading_;
    while (a < 0) a += kTau;
    while (a >= kTau) a -= kTau;
    int hf = int(std::floor(a / kTau * 16.0 + 0.5)) % 16;
    if (hf < 0) hf += 16;
    return hf > 15 ? 0 : hf;
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

double Game::wheelX(int i) const {
    const WheelDef& w = kWheels[i];
    return w.x + std::sin(t_ * 0.70 + w.phase) * w.sway;
}

void Game::sample(double lf, double lr, double& wx, double& wy) const {
    const double c = std::cos(heading_), s = std::sin(heading_);
    wx = x_ + lf * c + lr * s;
    wy = y_ + lf * s - lr * c;
}

void Game::begin() {
    x_ = 0;
    y_ = -kHalfL;
    heading_ = kPi * 0.5;
    speed_ = 0;
    engine_ = 0;
    steerF_ = 0;
    throttle_ = 0;
    race_ = 0;
    meters_ = 0;
    cleared_ = 0;
    danger_ = false;
    shake_ = 0;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    chimeStep_ = 0;
    wakeCursor_ = 0;
    smokeCursor_ = 0;
    smokeT_ = 0;
    wakeT_ = 0;
    why_[0] = 0;
    for (Puff& p : wake_) p = {};
    for (Puff& p : smoke_) p = {};
    camX_ = float(x_);
    camY_ = float(y_ + 10.0);
    zoom_ = kPlayZoom;
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
    blip(520.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.16f, 0.20f, 0.10f);
    begin();
    if (bot_) mode_ = Mode::Run;
    else showTitle();
}

void Game::controls(double& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    if (p.down(gs::BTN_LEFT)) steer += 1;
    if (p.down(gs::BTN_RIGHT)) steer -= 1;
    if (std::fabs(p.axisX) > 0.18f) steer = clampd(double(-p.axisX), -1.0, 1.0);
    const bool up = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) ||
                    p.accel > 0.2f;
    const bool down = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.2f;
    double cmd = 0.16;
    if (up) cmd = 1.0;
    if (down) cmd = -0.6;
    if (std::fabs(p.axisY) > 0.22f) cmd = p.axisY > 0 ? double(p.axisY) : double(p.axisY) * 0.6;
    if (p.accel > 0.25f) cmd = std::max(cmd, double(p.accel));
    throttle_ = clampd(cmd, -0.7, 1.0);
    if (p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z)) horn(0.42f);
}

void Game::pilot(double& steer) {
    const double here = laneAt(y_);
    const double tx = laneAt(y_) * 0.25 + laneAt(y_ + 26.0) * 0.75;
    const double xerr = tx - x_;
    const double xnow = laneAt(y_ + 8.0) - x_;
    const double hdes = kPi * 0.5 - clampd(xerr * 0.062 + xnow * 0.045, -0.62, 0.62);
    const double err = wrap(hdes - heading_);
    steer = clampd(err / 0.20, -1.0, 1.0);
    if (x_ > 12.0) steer = std::max(steer, 0.7);
    if (x_ < -12.0) steer = std::min(steer, -0.7);
    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& w = kWheels[i];
        double wx = wheelX(i);
        double dx = x_ - wx;
        double dy = y_ - w.y;
        double d = std::hypot(dx, dy);
        double lim = w.r + kHalfW + 2.6;
        if (d < lim && dy > -3.0 && dy < w.r + 10.0) {
            double push = dx >= 0.0 ? -1.0 : 1.0;
            steer += push * std::min(0.9, (lim - d) / 2.0);
        }
    }
    steer = clampd(steer, -1.0, 1.0);
    double want = 9.6;
    if (std::fabs(err) > 0.5) want = 7.0;
    if (std::fabs(x_ - here) > 2.8) want = 6.6;
    double cmd = speed_ < want + 0.4 ? want / kMaxSpd : (want - 1.5) / kMaxSpd;
    throttle_ = clampd(cmd, -0.2, 1.0);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    meters_ = 1000;
    std::snprintf(why_, sizeof why_, "wheels untouched");
    std::printf("S3 TUGBOAT KILO  MADE  finished the kilometer  1000 m  wheels untouched  the leg is made  (%.1f s)\n",
                race_);
    std::fflush(stdout);
    chime(5);
    sys_->rumble(0.32f, 0.18f, 160);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    shake_ = 10;
    std::snprintf(why_, sizeof why_, "%s", why);
    std::printf("S3 TUGBOAT KILO  FAIL  %s at %d m\n", why, meters_);
    std::fflush(stdout);
    sys_->rumble(0.55f, 0.3f, 180);
    sys_->setLight(180, 30, 20);
    sys_->apu.noiseBurst(0.45f, 140.f, 0.4f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.4f;
}

void Game::physics(double steer) {
    engine_ += (throttle_ - engine_) * (1.0 - std::exp(-1.5 * kDt));
    double target = engine_ >= 0 ? engine_ * kMaxSpd : engine_ * 4.2;
    speed_ += (target - speed_) * (1.0 - std::exp(-0.7 * kDt));
    speed_ = clampd(speed_, -5.0, kMaxSpd + 0.3);
    steerF_ += (steer - steerF_) * (1.0 - std::exp(-3.4 * kDt));
    double auth = clampd(std::fabs(speed_) / 2.8, 0.42, 1.0);
    heading_ = wrap(heading_ + steerF_ * auth * 0.95 * kDt);
    const double c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * kDt;
    y_ += (s * speed_ + kCurrent) * kDt;
    if (y_ < -kHalfL) {
        y_ = -kHalfL;
        if (s * speed_ < 0) speed_ *= 0.2;
    }

    const double pts[][2] = {
        {kHalfL, 0},          {-kHalfL, 0},         {3.3, -kHalfW}, {3.3, kHalfW},
        {0, -kHalfW},         {0, kHalfW},          {-3.4, -kHalfW}, {-3.4, kHalfW},
        {1.6, -kHalfW * 0.8}, {1.6, kHalfW * 0.8},
    };
    constexpr int kPtN = 10;

    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& w = kWheels[i];
        double wx = wheelX(i);
        if (std::fabs(y_ - w.y) > w.r + kHalfL + 1.5) continue;
        if (std::fabs(x_ - wx) > w.r + kHalfL + 1.5) continue;
        const double lim = w.r * w.r;
        for (int p = 0; p < kPtN; p++) {
            double sx, sy;
            sample(pts[p][0], pts[p][1], sx, sy);
            double dx = sx - wx, dy = sy - w.y;
            if (dx * dx + dy * dy < lim) {
                fail(wheelWhy(w.kind));
                return;
            }
        }
    }
    for (int p = 0; p < kPtN; p++) {
        double sx, sy;
        sample(pts[p][0], pts[p][1], sx, sy);
        if (std::fabs(sx) > kBank) {
            fail("left the cut");
            return;
        }
    }

    double bowX, bowY;
    sample(kHalfL, 0, bowX, bowY);
    meters_ = int(std::clamp(bowY, 0.0, kFinish));
    if (bowY >= kFinish) {
        if (std::fabs(x_) <= 4.8 && std::fabs(bowX) <= 5.2) win();
        else fail("missed the end");
        return;
    }

    danger_ = false;
    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& w = kWheels[i];
        double dy = w.y - y_;
        if (dy < -4.0 || dy > 26.0) continue;
        double wx = wheelX(i);
        if (std::fabs(x_ - wx) < w.r + 5.5) danger_ = true;
    }
    while (cleared_ < kWheelN && y_ > kWheels[cleared_].y + kWheels[cleared_].r + 0.4) {
        if (cleared_ == 0 || std::fabs(kWheels[cleared_].y - kWheels[cleared_ - 1].y) > 2.0) blip(640.f);
        cleared_++;
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.07f;
}

void Game::horn(float seconds) { hornT_ = seconds; }

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 6);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::cosmetics() {
    const double c = std::cos(heading_), s = std::sin(heading_);
    const bool alive = mode_ == Mode::Run || mode_ == Mode::Title || mode_ == Mode::Win;
    smokeT_ -= kDt;
    if (smokeT_ <= 0 && alive) {
        smokeT_ = mode_ == Mode::Title ? 0.18 : 0.11;
        double sx = x_ + (-3.35) * c;
        double sy = y_ + (-3.35) * s;
        smoke_[smokeCursor_] = {sx, sy, 1};
        smokeCursor_ = (smokeCursor_ + 1) % 10;
    }
    for (Puff& p : smoke_)
        if (p.life > 0) {
            p.life -= kDt * 0.55;
            p.y += 0.4 * kDt;
        }
    double g = std::hypot(c * speed_, s * speed_ + kCurrent);
    wakeT_ -= kDt;
    if (wakeT_ <= 0 && mode_ == Mode::Run && g > 2.4) {
        wakeT_ = 0.07;
        wake_[wakeCursor_] = {x_ - c * 4.3, y_ - s * 4.3, 1};
        wakeCursor_ = (wakeCursor_ + 1) % 14;
    }
    for (Puff& p : wake_)
        if (p.life > 0) p.life -= kDt * 0.7;
}

void Game::audio() {
    float water = mode_ == Mode::Run ? 0.016f + float(std::fabs(speed_) * 0.0012) : 0.01f;
    sys_->apu.noise(water, 520.f, false);
    if (hornT_ > 0 && chimeN_ == 0) {
        hornT_ -= float(kDt);
        float f = std::fmod(hornT_, 0.22f) < 0.11f ? 196.f : 146.f;
        sys_->apu.tone(0, f, 0.07f);
        tone0_ = 0.08f;
    } else if (chimeN_ > 0) {
        chimeT_ -= float(kDt);
        if (chimeT_ <= 0) {
            static const float notes[] = {392.f, 494.f, 587.f, 784.f, 988.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.055f);
            tone0_ = 0.16f;
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (tone0_ > 0) {
        tone0_ -= float(kDt);
        if (tone0_ <= 0) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0) {
        tone1_ -= float(kDt);
        if (tone1_ <= 0) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && std::fabs(engine_) > 0.04) {
        float wob = 0.72f + 0.28f * std::sin(float(t_) * (8.f + float(std::fabs(engine_)) * 14.f));
        float vol = (0.018f + float(std::fabs(engine_)) * 0.04f) * wob;
        sys_->apu.tone(2, 42.f + float(std::fabs(engine_)) * 36.f, vol);
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(2, 46.f, 0.012f);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (mode_ == Mode::Run) {
        if (danger_) sys_->setLight(170, 50, 24);
        else sys_->setLight(30, 80, 120);
    } else if (mode_ == Mode::Title) {
        sys_->setLight(170, 120, 40);
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
            blip(360.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            double steer = 0;
            if (bot_) pilot(steer);
            else controls(steer);
            physics(steer);
            if (mode_ == Mode::Run && race_ > kLeg) fail("missed the end");
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    cosmetics();
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
    float k = 1.f - std::exp(-float(kDt) * 4.2f);
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (kPlayZoom - zoom_) * k;
    if (shake_ > 0) {
        camX_ += std::sin(float(t_) * 46.f) * float(shake_) * 0.05f;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -12 || cy + h * 0.5f < -12 || cx - w * 0.5f > gs::SCREEN_W + 12 ||
        cy - h * 0.5f > gs::SCREEN_H + 12)
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

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    if (cx + w * 0.5f < -8 || cy + h * 0.5f < -8 || cx - w * 0.5f > gs::SCREEN_W + 8 ||
        cy - h * 0.5f > gs::SCREEN_H + 8)
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

void Game::place(const gs::Mipped& m, double wx, double wy, float worldH, int pal) {
    float sx = 160.f + float(wx - camX_) * zoom_;
    float sy = 112.f - float(wy - camY_) * zoom_;
    spr(m, sx, sy, worldH * zoom_, pal, false);
}

void Game::worldBox(const gs::Mipped& m, double wx, double wy, double ww, double wh, int pal) {
    float sx = 160.f + float(wx - camX_) * zoom_;
    float sy = 112.f - float(wy - camY_) * zoom_;
    sprBox(m, sx, sy, float(ww) * zoom_, float(wh) * zoom_, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    uint16_t deep = gs::rgb4(1, 4, 5);
    uint16_t far = gs::rgb4(2, 5, 4);
    if (mode_ == Mode::Fail) {
        deep = lerpC(deep, gs::rgb4(8, 2, 2), 0.35f);
        far = lerpC(far, gs::rgb4(8, 3, 2), 0.35f);
    } else if (mode_ == Mode::Win) {
        deep = lerpC(deep, gs::rgb4(2, 7, 4), 0.28f);
        far = lerpC(far, gs::rgb4(3, 8, 4), 0.28f);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) / std::max(zoom_, 0.2f);
        float u = std::clamp(wy / 1000.f, 0.f, 1.f);
        uint16_t c = lerpC(deep, far, u);
        float shimmer = std::sin(wy * 0.37f + float(t_) * 1.4f);
        if (shimmer > 0.94f) c = lerpC(c, gs::rgb4(8, 10, 7), 0.35f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.setFogColor(gs::rgb4(2, 4, 4));

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 28.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 36.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const bool missed = why_[0] == 'm';
        const bool cut = why_[0] == 'l';
        banner(missed ? art_.missed : (cut ? art_.cut : art_.touched), 160.f, 28.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.made, 160.f, 24.f, PAL_WIN);
        banner(art_.clean, 160.f, 48.f, PAL_WIN);
    }

    int hf = hullFrame();
    float bob = std::sin(float(t_) * 2.2f) * 0.7f;
    float bsx = 160.f + float(x_ - camX_) * zoom_;
    float bsy = 112.f - float(y_ - camY_) * zoom_ + bob;
    float boatH = kBoatH * zoom_;
    spr(art_.tug[hf], bsx + 3.f, bsy + 4.f, boatH, PAL_TUG, true);
    spr(art_.tug[hf], bsx, bsy, boatH, PAL_TUG, false);

    const double y0 = camY_ - 150.0 / std::max(double(zoom_), 0.4);
    const double y1 = camY_ + 150.0 / std::max(double(zoom_), 0.4);
    auto label = [&](const gs::Mipped& m, double wx, double wy) {
        place(m, wx, wy, 14.f / std::max(zoom_, 0.5f), PAL_TAG);
    };

    for (double y = std::floor(y0 / 16.0) * 16.0; y < y1; y += 16.0) {
        place(art_.quay, -20.0, y, 16.f, PAL_QUAY);
        place(art_.quay, 20.0, y, 16.f, PAL_QUAY);
        place(art_.quay, -30.5, y + 4.0, 16.f, PAL_QUAY);
        place(art_.quay, 30.5, y + 4.0, 16.f, PAL_QUAY);
        place(art_.quay, -41.0, y, 16.f, PAL_QUAY);
        place(art_.quay, 41.0, y, 16.f, PAL_QUAY);
    }
    for (double y = std::floor(y0 / 22.0) * 22.0; y < y1; y += 22.0) {
        if (y < 16.0 || y > 990.0) continue;
        place(art_.spar, laneAt(y), y, 2.15f, PAL_GATE);
    }

    place(art_.house, -26.0, -2.0, 11.f, PAL_QUAY);
    place(art_.barge, 24.0, 90.0, 6.2f, PAL_QUAY);
    place(art_.barge, -24.0, 300.0, 6.2f, PAL_QUAY);
    place(art_.crane, 28.0, 430.0, 8.f, PAL_QUAY);
    place(art_.barge, 24.0, 610.0, 6.2f, PAL_QUAY);
    place(art_.crane, -28.0, 740.0, 8.f, PAL_QUAY);
    place(art_.barge, -24.0, 860.0, 6.2f, PAL_QUAY);

    const double posts[][2] = {{250, 17.8}, {500, -17.8}, {750, 17.8}};
    const gs::Mipped* tags[] = {&art_.m250, &art_.m500, &art_.m750};
    for (int i = 0; i < 3; i++) {
        place(art_.post, posts[i][1], posts[i][0], 6.2f, PAL_GATE);
        label(*tags[i], posts[i][1] + (posts[i][1] < 0 ? -3.2 : 3.2), posts[i][0] + 1.2);
    }
    place(art_.post, -6.15, 1000, 7.4f, PAL_GATE);
    place(art_.post, 6.15, 1000, 7.4f, PAL_GATE);
    worldBox(art_.line, 0, 1000, 10.6, 0.85, PAL_GATE);
    for (int i = -2; i <= 2; i++) place(art_.buoy, i * 2.3, 1000.6, 1.8f, PAL_GATE);
    label(art_.endTag, 0, 1008);
    label(art_.m1000, 9.4, 1001.2);

    for (int i = 0; i < kScenicN; i++) {
        const Scenic& s = kScenic[i];
        if (s.y < y0 - 6 || s.y > y1 + 6) continue;
        double spin = std::fmod(t_ * 0.35 + s.y * 0.01, 1.0);
        if (spin < 0) spin += 1;
        int fr = int(spin * 8.0) % 8;
        place(art_.mill[fr], s.x, s.y, float(s.r * 2.0 * 1.18), PAL_MILL);
    }
    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& w = kWheels[i];
        if (w.y < y0 - 8 || w.y > y1 + 8) continue;
        double wx = wheelX(i);
        double spin = t_ * (w.kind == 1 ? 1.15 : 0.72) + w.phase;
        spin -= std::floor(spin);
        int fr = int(spin * 8.0) % 8;
        if (fr < 0) fr += 8;
        const gs::Mipped& img = w.kind == 1 ? art_.paddle[fr] : (w.kind == 2 ? art_.drum[fr] : art_.mill[fr]);
        int pal = w.kind == 1 ? PAL_PADDLE : (w.kind == 2 ? PAL_DRUM : PAL_MILL);
        place(img, wx, w.y, float(w.r * 2.0 * (52.0 / 44.0)), pal);
        double sign = w.x >= 0 ? 1.0 : -1.0;
        place(art_.pier, w.x + sign * (w.r + 3.5), w.y, 2.3f, PAL_QUAY);
        if (w.kind == 0) place(art_.house, sign * 26.5, w.y, 11.f, PAL_QUAY);
        else if (w.kind == 1) place(art_.barge, sign * 24.5, w.y + 8.0, 6.f, PAL_QUAY);
        else place(art_.crane, sign * 27.0, w.y, 8.f, PAL_QUAY);
    }

    int flap = int(t_ * 3.0) & 1;
    place(art_.gull[flap], -10.0 + std::sin(t_ * 0.7) * 2.2, 40.0, 2.4f, PAL_GULL);
    place(art_.gull[1 - flap], 8.0 + std::cos(t_ * 0.5) * 2.0, 190.0, 2.2f, PAL_GULL);
    place(art_.gull[flap], 4.0 + std::sin(t_ * 0.55 + 1.2) * 2.6, 620.0, 2.3f, PAL_GULL);

    for (const Puff& p : wake_) {
        if (p.life <= 0) continue;
        float sx = 160.f + float(p.x - camX_) * zoom_;
        float sy = 112.f - float(p.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, (2.0f + float(1.0 - p.life) * 2.2f) * zoom_ / kPlayZoom * 3.2f, PAL_FOAM, false);
    }
    for (const Puff& p : smoke_) {
        if (p.life <= 0) continue;
        float sx = 160.f + float(p.x - camX_) * zoom_;
        float sy = 112.f - float(p.y - camY_) * zoom_;
        spr(art_.smoke, sx, sy, (2.4f + float(1.0 - p.life) * 2.8f) * zoom_ / kPlayZoom * 3.4f, PAL_SMOKE, false);
    }

    char buf[48];
    int left = std::max(0, int(std::ceil(kLeg - race_ - 1e-4)));
    if (mode_ == Mode::Title) {
        hudC(22, "FINISH THE KILOMETER", PAL_HUD);
        hudC(23, "DO NOT TOUCH A WHEEL", PAL_ALERT);
        hudC(24, "MISSING THE END FAILS THE LEG", PAL_BANNER);
        hudC(25, "GREEN SPARS MARK THE LANE", PAL_WIN);
        if ((int(t_ * 2.0) & 1) == 0) hudC(27, "START", PAL_TAG);
        else hudC(27, "ARROWS STEER  UP AHEAD  DOWN ASTERN", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 TUGBOAT KILO", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "LEG %d:%02d", left / 60, left % 60);
    hud(30, 0, buf, left <= 20 ? PAL_ALERT : PAL_TAG);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        hudC(19, "ESC TO THE TITLE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "%.1f S", race_);
        hudC(16, buf, PAL_HUD);
        hudC(17, "1000 M  WHEELS UNTOUCHED", PAL_WIN);
        hudC(18, "THE LEG IS MADE", PAL_TAG);
        if (!bot_) hudC(20, "START RUNS IT AGAIN", PAL_HUD);
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
    hud(1, 1, buf, PAL_HUD);
    if (danger_) hud(1, 2, "WHEEL AHEAD", PAL_ALERT);
    else hud(1, 2, "WHEELS CLEAN", PAL_WIN);
    std::snprintf(buf, sizeof buf, "ENG %3.0f", std::max(0.0, engine_) * 100.0);
    hud(28, 1, buf, PAL_HUD);
    double g = std::hypot(std::cos(heading_) * speed_, std::sin(heading_) * speed_ + kCurrent);
    std::snprintf(buf, sizeof buf, "SPD %4.1f", g);
    hud(28, 2, buf, PAL_TAG);
    if (meters_ >= 900) hud(1, 26, "MAKE THE GATE OR MISS THE LEG", PAL_ALERT);
    else if (danger_) hud(1, 26, "PASS THE WHEEL, DO NOT TOUCH IT", PAL_ALERT);
    else if (race_ < 4.0) hud(1, 26, "FOLLOW THE SPARS TO THE END", PAL_TAG);
    else hud(1, 26, "THE KILOMETER ENDS AT THE GATE", PAL_HUD);
    hud(1, 27, "ARROWS STEER  UP AHEAD  DOWN ASTERN", PAL_HUD);
}

}  // namespace tugkilo
