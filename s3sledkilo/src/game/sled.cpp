#include "game/sled.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace sledkilo {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kPi = 3.141592653589793;
constexpr double kTau = 6.283185307179586;
constexpr double kHalfL = 2.12;
constexpr double kHalfW = 0.70;
constexpr double kBank = 11.0;
constexpr double kGrade = 2.05;
constexpr double kDrag = 0.24;
constexpr double kKick = 2.45;
constexpr double kKickGap = 0.30;
constexpr double kBrake = 8.6;
constexpr double kMax = 13.7;
constexpr double kCrew0 = 98.0;
constexpr float kSledH = 7.8f;
constexpr float kPlayZoom = 4.25f;
constexpr float kTitleZoom = 5.15f;
constexpr float kTitleCamX = 0.15f;
constexpr float kTitleCamY = -0.55f;
constexpr float kLead = 15.f;
constexpr float kWheelBmp = 42.f;
constexpr float kWheelDraw = 36.f;

struct Mark {
    double y, x;
};

enum Kind { HAY = 0, CART = 1, BIKE = 2, DRAY = 3, LOOSE = 4 };

struct Rig {
    double y, x;
    int kind;
    float sway, phase;
};

struct Geom {
    int wheels;
    double axle, track, radius;
    int style;
    int body;
    double halfL, halfW;
    float drawH;
};

// Flats hold the safe lane beside each rig. Transitions sit in the gaps.
constexpr Mark kMarks[] = {
    {0, 0},     {40, 0},     {78, 6.1},   {168, 6.1},  {210, -6.2}, {300, -6.2}, {342, 5.4},
    {432, 5.4}, {474, -5.6}, {564, -5.6}, {608, 0},    {700, 0},    {744, 5.7},  {832, 5.7},
    {878, 0},   {1020, 0},   {1140, 0},
};

constexpr Rig kRigs[] = {
    {118, -3.35, HAY, 0.16f, 0.4f},   {150, -8.35, LOOSE, 0.55f, 1.3f}, {252, 3.20, CART, 0.20f, 0.8f},
    {386, -2.15, DRAY, 0.14f, 0.2f},  {500, 0.25, BIKE, 0.32f, 1.1f},    {500, 2.15, BIKE, 0.28f, 2.4f},
    {534, 4.85, CART, 0.16f, 0.6f},   {650, -6.85, HAY, 0.08f, 0.3f},    {650, 6.85, HAY, 0.08f, 1.8f},
    {788, -2.45, HAY, 0.14f, 1.2f},
};

constexpr int kMarkN = int(sizeof kMarks / sizeof kMarks[0]);
constexpr int kRigN = int(sizeof kRigs / sizeof kRigs[0]);
constexpr double kPosts[] = {250, 500, 750};

const char* kHit[] = {
    "touched a wagon wheel", "touched a cart wheel", "touched a bicycle wheel", "touched a dray wheel",
    "touched a loose wheel",
};

Geom geomOf(int kind) {
    switch (kind) {
    case HAY: return {4, 4.05, 2.05, 1.68, 0, 0, 3.40, 2.20, 6.8f};
    case CART: return {4, 2.65, 1.62, 1.38, 1, 1, 2.30, 1.55, 4.6f};
    case BIKE: return {2, 1.45, 0.0, 0.80, 2, 2, 1.55, 0.55, 3.15f};
    case DRAY: return {4, 4.70, 2.25, 1.78, 3, 3, 4.15, 2.35, 8.3f};
    default: return {1, 0, 0, 1.24, 0, -1, 0, 0, 0};
    }
}

double wrap(double a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float wheelH(double r) { return float(r * 2.0 * (double(kWheelBmp) / double(kWheelDraw))); }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (meters_ >= 820) return 3;
    if (meters_ >= 180) return 2;
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

double Game::frontY() const {
    double wx, wy;
    sample(0, kHalfL, wx, wy);
    return wy;
}

double shiftedX(const Rig& r, double t) { return r.x + std::sin(t * 0.58 + r.phase) * double(r.sway); }

const char* Game::hitWhat() const {
    const double lx[3] = {-kHalfW, 0, kHalfW};
    const double ly[3] = {-kHalfL, 0, kHalfL};
    double sx[9], sy[9];
    int n = 0;
    for (double x : lx)
        for (double y : ly) sample(x, y, sx[n], sy[n]), ++n;

    for (int i = 0; i < kRigN; i++) {
        const Rig& r = kRigs[i];
        const Geom g = geomOf(r.kind);
        const double rx = shiftedX(r, t_);
        if (std::fabs(y_ - r.y) > g.axle + g.radius + kHalfL + 2.0 && g.wheels > 1) continue;
        if (g.halfW > 0) {
            for (int p = 0; p < 9; p++) {
                if (std::fabs(sx[p] - rx) < g.halfW && std::fabs(sy[p] - r.y) < g.halfL) return kHit[r.kind];
            }
        }
        double wxs[4], wys[4];
        int wn = 0;
        if (g.wheels == 1) {
            wxs[0] = rx;
            wys[0] = r.y;
            wn = 1;
        } else if (g.wheels == 2) {
            wxs[0] = wxs[1] = rx;
            wys[0] = r.y - g.axle;
            wys[1] = r.y + g.axle;
            wn = 2;
        } else {
            const double ax[2] = {-g.axle, g.axle};
            const double tr[2] = {-g.track, g.track};
            for (double a : ax)
                for (double t : tr) {
                    wxs[wn] = rx + t;
                    wys[wn] = r.y + a;
                    ++wn;
                }
        }
        const double lim = (g.radius + 0.06) * (g.radius + 0.06);
        for (int w = 0; w < wn; w++) {
            if (std::fabs(y_ - wys[w]) > g.radius + kHalfL + 0.4) continue;
            for (int p = 0; p < 9; p++) {
                double dx = sx[p] - wxs[w], dy = sy[p] - wys[w];
                if (dx * dx + dy * dy < lim) return kHit[r.kind];
            }
        }
    }
    return nullptr;
}

bool Game::offSnow() const {
    const double lx[3] = {-kHalfW, 0, kHalfW};
    const double ly[3] = {-kHalfL, 0, kHalfL};
    for (double x : lx)
        for (double y : ly) {
            double sx, sy;
            sample(x, y, sx, sy);
            if (std::fabs(sx) > kBank) return true;
        }
    return false;
}

void Game::begin() {
    x_ = 0;
    y_ = -kHalfL - 0.55;
    heading_ = kPi * 0.5;
    speed_ = 0;
    steer_ = 0;
    kickCd_ = 0;
    puffT_ = 0;
    raceTime_ = 0;
    crew_ = kCrew0;
    meters_ = 0;
    post_ = 0;
    danger_ = false;
    shake_ = 0;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    chimeStep_ = 0;
    puffCursor_ = 0;
    lastSec_ = int(std::ceil(kCrew0));
    why_[0] = 0;
    report_[0] = 0;
    for (Puff& p : puffs_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
    zoom_ = kTitleZoom;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    blip(520.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.74f);
    sys.apu.setEcho(0.12f, 0.18f, 0.08f);
    begin();
    if (bot_) {
        mode_ = Mode::Run;
        camX_ = float(x_);
        camY_ = float(y_ + 8.0);
        zoom_ = kPlayZoom;
    } else {
        showTitle();
    }
}

void Game::controls(double& steer, bool& kick, bool& brake) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    if (p.down(gs::BTN_LEFT)) steer += 1;
    if (p.down(gs::BTN_RIGHT)) steer -= 1;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(double(-p.axisX), -1.0, 1.0);
    kick = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) || p.accel > 0.28f;
    brake = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.28f;
}

void Game::pilot(double& steer, bool& kick, bool& brake) {
    const double look = 12.0 + speed_ * 0.72;
    const double here = laneAt(y_);
    const double tx = here * 0.32 + laneAt(y_ + look) * 0.68;
    const double xerr = tx - x_;
    const double hdes = kPi * 0.5 - std::clamp(xerr * 0.085, -0.8, 0.8);
    const double err = wrap(hdes - heading_);
    steer = std::clamp(err / 0.28, -1.0, 1.0);

    for (int i = 0; i < kRigN; i++) {
        const Rig& r = kRigs[i];
        const Geom g = geomOf(r.kind);
        const double rx = shiftedX(r, t_);
        double wxs[4], wys[4];
        int wn = 0;
        if (g.wheels == 1) {
            wxs[0] = rx;
            wys[0] = r.y;
            wn = 1;
        } else if (g.wheels == 2) {
            wxs[0] = wxs[1] = rx;
            wys[0] = r.y - g.axle;
            wys[1] = r.y + g.axle;
            wn = 2;
        } else {
            const double ax[2] = {-g.axle, g.axle};
            const double tr[2] = {-g.track, g.track};
            for (double a : ax)
                for (double t : tr) {
                    wxs[wn] = rx + t;
                    wys[wn] = r.y + a;
                    ++wn;
                }
        }
        for (int w = 0; w < wn; w++) {
            double dx = x_ - wxs[w];
            double dy = wys[w] - y_;
            if (dy < -2.0 || dy > 11.0) continue;
            double d = std::hypot(dx, dy);
            double lim = g.radius + 2.7;
            if (d < lim) {
                double push = (lim - d) / lim;
                steer += dx >= 0 ? -push * 1.2 : push * 1.2;
            }
        }
    }
    if (x_ > 8.3) steer = std::max(steer, 0.85);
    if (x_ < -8.3) steer = std::min(steer, -0.85);
    steer = std::clamp(steer, -1.0, 1.0);

    double want = 13.5;
    if (std::fabs(err) > 0.42) want = 10.6;
    if (std::fabs(x_ - here) > 2.6) want = 10.0;
    if (std::fabs(x_) > 7.4) want = 9.2;
    if (y_ > 610.0 && y_ < 705.0) want = std::min(want, 12.0);
    kick = speed_ < want - 0.15;
    brake = speed_ > want + 1.15;
}

void Game::win() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    meters_ = 1000;
    std::snprintf(report_, sizeof report_,
                  "S3 SLED KILO  PASS  finished the kilometer clean  1000 m  wheels untouched  (%.1fs, %.1fs left on "
                  "the crew clock)",
                  raceTime_, std::max(0.0, crew_));
    std::printf("%s\n", report_);
    std::fflush(stdout);
    chime(5);
    sys_->rumble(0.4f, 0.22f, 180);
    sys_->setLight(40, 210, 90);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    shake_ = 8;
    std::snprintf(why_, sizeof why_, "%s", why);
    std::snprintf(report_, sizeof report_, "S3 SLED KILO  FAIL  %s at %d m", why, meters_);
    std::printf("%s\n", report_);
    std::fflush(stdout);
    sys_->apu.noiseBurst(0.42f, 90.f, 0.4f);
    sys_->apu.tone(0, 64.f, 0.06f);
    tone0_ = 0.4f;
    sys_->rumble(0.75f, 0.4f, 220);
    sys_->setLight(220, 40, 24);
}

void Game::physics(double steerCmd, bool kick, bool brake) {
    const double follow = 1.0 - std::exp(-9.0 * kDt);
    steer_ += (steerCmd - steer_) * follow;
    if (x_ > 8.7) steer_ = std::max(steer_, 0.95);
    if (x_ < -8.7) steer_ = std::min(steer_, -0.95);

    const double rate = 2.75 - std::min(speed_, 14.0) * 0.09;
    heading_ = wrap(heading_ + steer_ * rate * kDt);
    const double hy = std::sin(heading_);
    double accel = hy * kGrade - kDrag * speed_;
    if (brake) accel -= kBrake;
    speed_ += accel * kDt;
    if (kick && !brake && kickCd_ <= 0 && speed_ < kMax) {
        speed_ = std::min(kMax, speed_ + kKick);
        kickCd_ = kKickGap;
        blip(180.f + float(std::min(speed_, 14.0)) * 8.f);
        double px, py;
        sample((int(t_ * 8) & 1) ? 0.4 : -0.4, -kHalfL * 0.2, px, py);
        puffs_[puffCursor_].x = px;
        puffs_[puffCursor_].y = py;
        puffs_[puffCursor_].life = 0.45;
        puffCursor_ = (puffCursor_ + 1) % 10;
    }
    if (kickCd_ > 0) kickCd_ -= kDt;
    speed_ = std::clamp(speed_, 0.0, kMax);

    const double c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * kDt;
    y_ += s * speed_ * kDt;
    if (y_ < -kHalfL - 1.2) {
        y_ = -kHalfL - 1.2;
        if (s < 0) speed_ *= 0.4;
    }

    meters_ = int(std::clamp(frontY(), 0.0, 1000.0));
    danger_ = false;
    for (int i = 0; i < kRigN; i++) {
        const Rig& r = kRigs[i];
        double dy = r.y - y_;
        if (dy < -4 || dy > 16) continue;
        if (std::fabs(x_ - shiftedX(r, t_)) < geomOf(r.kind).radius + geomOf(r.kind).track + 3.2) danger_ = true;
    }

    if (const char* why = hitWhat()) {
        fail(why);
        return;
    }
    if (offSnow()) {
        fail("left the snow");
        return;
    }
    if (frontY() >= 1000.0) {
        win();
        return;
    }
    if (crew_ <= 0.0) {
        fail("the other crew took the kilometer");
        return;
    }
    while (post_ < 3 && frontY() >= kPosts[post_]) {
        post_++;
        chime(2);
    }

    puffT_ -= kDt;
    if (puffT_ <= 0 && speed_ > 7.5 && mode_ == Mode::Run) {
        puffT_ = 0.07;
        double px, py;
        sample(0, -kHalfL * 0.85, px, py);
        puffs_[puffCursor_] = {px, py, 0.55};
        puffCursor_ = (puffCursor_ + 1) % 10;
    }
    for (Puff& p : puffs_)
        if (p.life > 0) p.life -= kDt;
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.04f);
    tone1_ = 0.07f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 6);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio() {
    if (mode_ == Mode::Run && speed_ > 1.5) {
        sys_->apu.noise(0.012f + float(speed_) * 0.0011f, 520.f + float(speed_) * 18.f, false);
        float wob = 0.65f + 0.35f * std::sin(float(t_) * (8.f + float(speed_) * 0.4f));
        sys_->apu.tone(2, 70.f + float(speed_) * 7.5f, (0.012f + float(speed_) * 0.0016f) * wob);
    } else {
        sys_->apu.noise(mode_ == Mode::Title ? 0.008f : 0.004f, 380.f, false);
        if (tone0_ <= 0) sys_->apu.tone(2, 0.f, 0.f);
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
            static const float notes[] = {392.f, 494.f, 587.3f, 784.f, 988.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
            tone0_ = 0.16f;
            chimeT_ = 0.12f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (mode_ == Mode::Run) {
        int sec = std::max(0, int(std::ceil(crew_ - 1e-4)));
        if (sec != lastSec_) {
            lastSec_ = sec;
            if (sec <= 12) blip(sec <= 5 ? 880.f : 520.f);
        }
    }
    if (mode_ == Mode::Run) {
        int left = std::max(0, int(std::ceil(crew_ - 1e-4)));
        if (left <= 12) sys_->setLight(220, 48, 28);
        else sys_->setLight(170, 200, 255);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(360.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            raceTime_ += kDt;
            crew_ -= kDt;
            double steer = 0;
            bool kick = false, brake = false;
            if (bot_) pilot(steer, kick, brake);
            else controls(steer, kick, brake);
            physics(steer, kick, brake);
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
    float k = 1.f - std::exp(-float(kDt) * 4.2f);
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (kPlayZoom - zoom_) * k;
    if (shake_ > 0) {
        camX_ += std::sin(float(t_) * 48.f) * float(shake_) * 0.22f;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow, int fog) {
    if (h < 1.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (shadow) {
        cx += 3.f;
        cy += 4.f;
    }
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
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;

    const float z = std::max(zoom_, 0.8f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) / z;
        gs::RoadLine& road = v.road[y];
        road.on = true;
        road.cx = 160.f + (0.f - camX_) * z;
        road.hw = float(kBank) * z;
        road.v = wy * 34.f;
        road.pal = PAL_ROAD;
        road.band = (int(std::floor(wy / 7.0)) & 1) ? 1 : 0;
        road.style = gs::ROAD_SNOW;
        road.left = gs::GROUND_SNOWWALL;
        road.right = gs::GROUND_SNOWWALL;
        float ahead = (90.f - float(y)) / 112.f;
        v.lineFog[y] = ahead > 0.62f ? uint8_t(std::min(3.f, (ahead - 0.62f) * 8.f)) : 0;
        v.lineBackdrop[y] = gs::rgb4(9, 11, 14);
    }

    struct Stamp {
        float sy, sx, h;
        const gs::Mipped* img;
        int pal;
        int fog;
        bool shade;
    };
    std::vector<Stamp> stamps;
    stamps.reserve(128);
    auto queue = [&](const gs::Mipped& m, double wx, double wy, float worldH, int pal, bool shade) {
        float sx = 160.f + float(wx - camX_) * z;
        float sy = 112.f - float(wy - camY_) * z;
        float h = worldH * z;
        if (h < 1.5f) return;
        if (sx < -80 || sx > gs::SCREEN_W + 80 || sy < -80 || sy > gs::SCREEN_H + 80) return;
        int fog = sy < 26.f ? 6 : sy < 52.f ? 3 : 0;
        stamps.push_back({sy, sx, h, &m, pal, fog, shade});
    };

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 18.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped& msg = why_[0] == 'l' ? art_.leftSnow : (why_[0] == 't' && why_[1] == 'h' ? art_.crewTook : art_.touched);
        banner(msg, 160.f, 78.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.kilometer, 160.f, 64.f, PAL_WIN);
        banner(art_.clean, 160.f, 90.f, PAL_WIN);
    }

    const double y0 = camY_ - 36.0;
    const double y1 = camY_ + 40.0;
    for (double ty = std::floor(y0 / 24.0) * 24.0; ty < y1; ty += 24.0) {
        int row = int(std::floor(ty));
        float j = float((row * 13) % 5) - 2.f;
        queue(art_.pine, -15.6 + j * 0.15, ty, 7.2f, PAL_PINE, false);
        queue(art_.pine, 16.2 - j * 0.12, ty + 12.0, 6.6f, PAL_PINE, false);
        if ((row / 24) % 3 == 0) queue(art_.cabin, (row & 32) ? 17.4 : -17.6, ty + 6.0, 6.4f, PAL_PINE, true);
    }
    for (double ty = std::floor(y0 / 40.0) * 40.0; ty < y1; ty += 40.0) {
        queue(art_.lamp, -12.35, ty + 8.0, 3.6f, PAL_POST, false);
        queue(art_.lamp, 12.45, ty + 28.0, 3.6f, PAL_POST, false);
    }

    queue(art_.startLine, 0, 0, 0.55f, PAL_POST, false);
    queue(art_.ribbon, 0, 1000, 1.15f, PAL_POST, false);

    int hand = 0;
    if (mode_ == Mode::Title) hand = int(t_ * 0.65) & 7;
    else hand = std::min(7, int((1.0 - std::clamp(crew_ / kCrew0, 0.0, 1.0)) * 8.0));
    queue(art_.clock[hand], -14.6, 10.0, 8.2f, PAL_CLOCK, true);
    queue(art_.crewTag, -16.4, 16.5, 2.5f, PAL_TAG, false);
    queue(art_.clock[hand], 8.4, 1014, 7.4f, PAL_CLOCK, true);
    queue(art_.crewTag, 8.4, 1021, 2.4f, PAL_TAG, false);

    const double posts[][2] = {{250, -10.4}, {500, 10.4}, {750, -10.4}, {1000, 10.4}};
    const gs::Mipped* tags[] = {&art_.m250, &art_.m500, &art_.m750, &art_.m1000};
    for (int i = 0; i < 4; i++) {
        queue(art_.post, posts[i][1], posts[i][0], 5.2f, PAL_POST, false);
        double tagX = posts[i][1] < 0 ? -15.2 : 15.2;
        queue(*tags[i], tagX, posts[i][0] + 1.2, 2.6f, PAL_TAG, false);
    }

    for (int i = 0; i < kRigN; i++) {
        const Rig& r = kRigs[i];
        if (r.y < y0 - 8 || r.y > y1 + 8) continue;
        const Geom g = geomOf(r.kind);
        const double rx = shiftedX(r, t_);
        const gs::Mipped* body = nullptr;
        int pal = PAL_WAGON;
        if (g.body == 0) body = &art_.hay;
        else if (g.body == 1) body = &art_.cart;
        else if (g.body == 2) {
            body = &art_.bike;
            pal = PAL_BIKE;
        } else if (g.body == 3) body = &art_.dray;
        if (body) queue(*body, rx, r.y, g.drawH, pal, true);

        double spin = t_ * (r.kind == BIKE ? 1.6 : r.kind == LOOSE ? 2.4 : 0.35) + r.phase;
        spin -= std::floor(spin);
        int fr = int(spin * 8.0) % 8;
        if (fr < 0) fr += 8;
        const gs::Mipped& wh = art_.wheel[g.style][fr];
        float hh = wheelH(g.radius);
        auto wheelAt = [&](double wx, double wy) { queue(wh, wx, wy, hh, pal, false); };
        if (g.wheels == 1) wheelAt(rx, r.y);
        else if (g.wheels == 2) {
            wheelAt(rx, r.y - g.axle);
            wheelAt(rx, r.y + g.axle);
        } else {
            wheelAt(rx - g.track, r.y - g.axle);
            wheelAt(rx + g.track, r.y - g.axle);
            wheelAt(rx - g.track, r.y + g.axle);
            wheelAt(rx + g.track, r.y + g.axle);
        }
    }

    int hf = int(std::lround(std::fmod(heading_ < 0 ? heading_ + kTau : heading_, kTau) / kTau * 16.0)) % 16;
    if (hf < 0) hf += 16;
    queue(art_.sled[hf], x_, y_, kSledH, PAL_SLED, true);
    queue(art_.sled[12], -1.1, 1026.0, kSledH * 0.96f, PAL_CREW, true);

    int flap = int(t_ * 3.0) & 1;
    queue(art_.bird[flap], -4.0 + std::sin(t_ * 0.7) * 3.0, 46, 2.2f, PAL_POST, false);
    queue(art_.bird[1 - flap], 6.0 + std::cos(t_ * 0.5) * 2.5, 430, 2.0f, PAL_POST, false);
    queue(art_.bird[flap], 2.0 + std::sin(t_ * 0.6 + 1.2) * 3.0, 910, 2.1f, PAL_POST, false);

    for (const Puff& p : puffs_) {
        if (p.life <= 0) continue;
        queue(art_.spray, p.x, p.y, 1.3f + float(0.55 - p.life) * 2.2f, PAL_SPRAY, false);
    }
    for (int i = 0; i < 10; i++) {
        double fy = camY_ + std::fmod(i * 8.5 + t_ * (5.0 + i * 0.17), 46.0) - 20.0;
        double fx = camX_ + std::sin(t_ * 0.35 + i * 1.7) * 14.0;
        queue(art_.dot, fx, fy, 0.42f, PAL_SPRAY, false);
    }

    std::sort(stamps.begin(), stamps.end(), [](const Stamp& a, const Stamp& b) { return a.sy > b.sy; });
    for (const Stamp& s : stamps) {
        spr(*s.img, s.sx, s.sy, s.h, s.pal, false, s.fog);
        if (s.shade) spr(*s.img, s.sx, s.sy, s.h, s.pal, true, s.fog);
    }

    char buf[56];
    int left = std::max(0, int(std::ceil(crew_ - 1e-4)));
    if (mode_ == Mode::Title) {
        hudC(22, "FINISH THE KILOMETER", PAL_HUD);
        hudC(23, "DO NOT TOUCH A WHEEL", PAL_ALERT);
        hudC(24, "THE CLOCK IS THE OTHER CREW", PAL_BANNER);
        if ((int(t_ * 2.0) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "ARROWS STEER   UP KICK   DOWN BRAKE", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 SLED KILO", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "CREW %d:%02d", left / 60, left % 60);
    hud(28, 0, buf, left <= 12 ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(16, "START CONTINUES", PAL_HUD);
        hudC(17, "ESC BACK TO THE SLED", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "%.1fS  CREW HAD %.1fS", raceTime_, std::max(0.0, crew_));
        hudC(15, buf, PAL_HUD);
        hudC(16, "1000 M  WHEELS UNTOUCHED", PAL_WIN);
        if (!bot_) hudC(18, "START RUNS IT AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(15, why_, PAL_ALERT);
        std::snprintf(buf, sizeof buf, "%d M", meters_);
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "START TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "%d/1000 M", meters_);
    hud(1, 1, buf, PAL_HUD);
    if (danger_) hud(1, 2, "WHEEL AHEAD", PAL_ALERT);
    else hud(1, 2, "WHEELS CLEAN", PAL_WIN);
    std::snprintf(buf, sizeof buf, "M/S %4.1f", speed_);
    hud(30, 2, buf, PAL_HUD);
    if (raceTime_ < 3.2) hud(1, 26, "KICK THE SLED UP TO SPEED", PAL_BANNER);
    else if (danger_) hud(1, 26, "PASS THE WHEEL, DO NOT TOUCH IT", PAL_ALERT);
    else hud(1, 26, "THE KILOMETER IS THEIRS IF YOU ARE SLOW", PAL_HUD);
    hud(1, 27, "ARROWS STEER   UP KICK   DOWN BRAKE", PAL_HUD);
}

}  // namespace sledkilo
