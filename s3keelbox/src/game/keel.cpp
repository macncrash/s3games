#include "keel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace keelbox {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kPi = 3.141592653589793;
constexpr double kTau = 6.283185307179586;
constexpr double kBoxL = -9.0;
constexpr double kBoxR = 9.0;
constexpr double kBoxB = 148.0;
constexpr double kBoxT = 196.0;
constexpr double kEnd = 214.0;
constexpr double kBank = 26.0;
constexpr double kSouth = 8.0;
constexpr double kHoldY = 170.0;
constexpr double kHalfW = 1.35;
constexpr double kHalfL = 5.55;
constexpr double kMargin = 0.25;
constexpr double kStop = 0.42;
constexpr double kHoldNeed = 0.65;
constexpr double kOutNeed = 1.15;
constexpr double kMaxDrive = 8.4;
constexpr double kEase = 0.46;
constexpr double kFill = 1.25;
constexpr double kStartX = 1.2;
constexpr double kStartY = 28.0;
constexpr double kStartH = 1.48;
constexpr float kPlayZoom = 3.15f;
constexpr float kArtScale = 6.4f;
constexpr int kFrames = 12;

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
    int i = int(std::lround(u / kTau * kFrames)) % kFrames;
    if (i < 0) i += kFrames;
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

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    surge_ = 0.4;
    sheet_ = 0.15;
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
    zoom_ = 1.05f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = float(x_);
    camY_ = float(y_);
    zoom_ = kPlayZoom;
    blip(520.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.14f, 0.16f, 0.08f);
    t_ = 0;
    if (bot_) startRun();
    else showTitle();
}

void Game::slewSheet(double want) {
    want = clampd(want, 0.0, 1.0);
    double step = kDt * 0.85;
    double d = want - sheet_;
    if (std::fabs(d) <= step) sheet_ = want;
    else sheet_ += std::copysign(step, d);
}

void Game::controls(double& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    if (p.down(gs::BTN_LEFT)) steer += 1;
    if (p.down(gs::BTN_RIGHT)) steer -= 1;
    if (std::fabs(p.axisX) > 0.18f) steer = clampd(double(-p.axisX), -1.0, 1.0);
    double want = sheet_;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C)) want = 1.0;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X)) want = 0.0;
    if (p.accel > 0.08f) want = std::max(want, double(p.accel));
    if (p.brake > 0.08f) want = std::min(want, 1.0 - double(p.brake));
    if (!(p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) ||
          p.down(gs::BTN_X) || p.accel > 0.08f || p.brake > 0.08f)) {
        return;
    }
    slewSheet(want);
}

void Game::pilot(double& steer) {
    const double north = kPi * 0.5;
    const bool in = hullInside();
    double glide = std::max(0.0, surge_) / kEase;
    double aim = kHoldY;
    bool spill = in || y_ + glide >= aim - 0.4 || y_ > kBoxB + 6.0;
    phase_ = in ? 1 : (spill ? 2 : 0);
    double wantSheet = 0;
    if (!spill) {
        double dist = aim - y_;
        double wantV = dist > 55.0 ? 7.6 : dist > 24.0 ? 4.6 : 2.6;
        double drive = std::max(0.55, std::sin(heading_));
        wantSheet = clampd(wantV / (kMaxDrive * drive), 0.0, 1.0);
    }
    slewSheet(wantSheet);
    double hdes = north + clampd(x_ * 0.09, -0.5, 0.5);
    if (in) hdes = north + clampd(x_ * 0.05, -0.16, 0.16);
    steer = clampd(wrap(hdes - heading_) / 0.28, -1.0, 1.0);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.045f);
    tone0_ = std::max(tone0_, 0.06f);
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
    sys_->rumble(0.28f, 0.12f, 160);
    sys_->setLight(40, 170, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.55f, 0.25f, 180);
    sys_->setLight(170, 40, 28);
    sys_->apu.noiseBurst(0.4f, 120.f, 0.4f);
    sys_->apu.tone(0, 90.f, 0.05f);
    tone0_ = 0.4f;
}

void Game::puff(double x, double y) {
    wake_[wakeCursor_].x = x;
    wake_[wakeCursor_].y = y;
    wake_[wakeCursor_].life = 1;
    wakeCursor_ = (wakeCursor_ + 1) % 16;
}

void Game::physics(double steer) {
    double rate = 0.95 + std::min(std::fabs(surge_), 8.0) * 0.06;
    heading_ = wrap(heading_ + steer * rate * kDt);
    // Wind is aft on a run north. Sheet fills the sail; easing it lets the hull coast.
    double drive = std::max(0.0, std::sin(heading_));
    double target = sheet_ * drive * kMaxDrive;
    double lam = target + 0.2 < surge_ ? kEase : kFill;
    surge_ += (target - surge_) * (1.0 - std::exp(-lam * kDt));
    surge_ = clampd(surge_, -0.4, kMaxDrive);
    double c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * surge_ * kDt;
    y_ += s * surge_ * kDt;
    if (!std::isfinite(x_) || !std::isfinite(y_) || !std::isfinite(surge_)) {
        fail("missed the end");
        return;
    }

    auto thud = [&]() {
        if (thumpT_ > 0) return;
        sys_->apu.noiseBurst(0.22f, 180.f, 0.1f);
        thumpT_ = 0.25f;
        shake_ = std::max(shake_, 0.35f);
    };
    Ext e = extents();
    if (e.minX < -kBank) {
        x_ += -kBank - e.minX;
        surge_ *= 0.55;
        thud();
    }
    e = extents();
    if (e.maxX > kBank) {
        x_ += kBank - e.maxX;
        surge_ *= 0.55;
        thud();
    }
    e = extents();
    if (e.minY < kSouth) {
        y_ += kSouth - e.minY;
        if (surge_ < 0) surge_ *= 0.4;
        thud();
    }
    e = extents();
    if (e.maxY > kEnd) {
        fail("missed the end");
        return;
    }

    bool in = hullInside();
    if (in && !announced_) {
        announced_ = true;
        blip(660.f);
    }
    wakeT_ -= float(kDt);
    if (wakeT_ <= 0 && std::fabs(surge_) > 1.4) {
        wakeT_ = 0.07f;
        puff(x_ - c * 5.2, y_ - s * 5.2);
    }
    for (Puff& p : wake_)
        if (p.life > 0) p.life -= kDt * 0.5;

    double g = std::fabs(surge_);
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
    float water = mode_ == Mode::Run ? 0.014f + float(std::fabs(surge_) * 0.0006) : 0.01f;
    sys_->apu.noise(water, 380.f + float(sheet_) * 80.f, false);
    if (mode_ == Mode::Run && sheet_ > 0.08) {
        float wob = 0.75f + 0.25f * std::sin(float(t_) * (6.f + float(sheet_) * 4.f));
        sys_->apu.tone(2, 180.f + float(sheet_) * 40.f, 0.012f * wob);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (tone0_ > 0) {
        tone0_ -= float(kDt);
        if (tone0_ <= 0 && chimeN_ == 0) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (thumpT_ > 0) thumpT_ -= float(kDt);
    if (chimeN_ > 0) {
        chimeT_ -= float(kDt);
        if (chimeT_ <= 0) {
            static const float notes[] = {392.f, 494.f, 587.3f, 784.f, 988.f};
            int n = std::min(chimeStep_, 4);
            sys_->apu.tone(0, notes[n], 0.05f);
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
            blip(420.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            double steer = 0;
            if (bot_) pilot(steer);
            else controls(steer);
            physics(steer);
            if (mode_ == Mode::Run && race_ > 75.0) fail("the leg ran out");
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    if (mode_ == Mode::Win) sys.setLight(40, 170, 80);
    else if (mode_ == Mode::Fail) sys.setLight(170, 40, 28);
    else if (hold_ > 0.02) sys.setLight(200, 170, 50);
    else if (mode_ == Mode::Run) sys.setLight(30, 110, 160);
    camera();
    audio();
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = 0.f;
        camY_ = 118.f;
        zoom_ = 1.05f;
        return;
    }
    float lead = 10.f;
    if (y_ > kBoxB - 20.0) lead = 3.f;
    if (hold_ > 0.02 || mode_ == Mode::Win || mode_ == Mode::Fail) lead = 0.f;
    float tx = float(x_ + std::cos(heading_) * lead * 0.25);
    float ty = float(y_ + std::sin(heading_) * lead);
    float tz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        tx = 0.f;
        ty = float((kBoxB + kBoxT) * 0.5);
        tz = 2.35f;
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
        jx = std::sin(float(t_) * 44.f) * shake_ * 3.f;
        jy = std::cos(float(t_) * 35.f) * shake_ * 2.f;
    }
    float invZ = 1.f / std::max(zoom_, 0.25f);
    camX_ -= jx * invZ;
    camY_ += jy * invZ;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) * invZ;
        uint16_t deep = gs::rgb4(1, 4, 8);
        uint16_t mid = gs::rgb4(2, 8, 11);
        float u = std::clamp((wy - 4.f) / 230.f, 0.f, 1.f);
        v.lineBackdrop[y] = lerpC(mid, deep, u);
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f + (0.f - camX_) * zoom_;
        float half = std::max(8.f, float(kBank + 6.0) * zoom_);
        r.hw = half;
        r.v = wy * 18.f + float(t_) * 14.f;
        r.pal = uint8_t(PAL_CH);
        r.band = (int(std::floor(wy * 0.15f + t_ * 0.7f)) & 1) ? 1 : 0;
        r.style = 2;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }

    worldRect(art_.hatch, 0, (kBoxB + kBoxT) * 0.5, kBoxR - kBoxL, kBoxT - kBoxB, PAL_MARK);
    for (double x : {-6.0, 0.0, 6.0}) {
        place(art_.hbar, x, kBoxB, 1.3f, PAL_MARK, 0);
        place(art_.hbar, x, kBoxT, 1.3f, PAL_MARK, 0);
    }
    for (double y : {160.0, 172.0, 184.0}) {
        place(art_.vbar, kBoxL, y, 6.f, PAL_MARK, 0);
        place(art_.vbar, kBoxR, y, 6.f, PAL_MARK, 0);
    }
    const int postPal = hold_ > 0.02 ? PAL_WIN : PAL_MARK;
    const double corners[4][2] = {{kBoxL, kBoxB}, {kBoxR, kBoxB}, {kBoxL, kBoxT}, {kBoxR, kBoxT}};
    for (const double* c : corners) place(art_.post, c[0], c[1], 6.5f, postPal, mode_ == Mode::Title ? 8.f : 0);

    place(art_.committee, kBoxR + 4.2, kEnd - 1.2, 7.5f, PAL_END, mode_ == Mode::Title ? 10.f : 0);
    place(art_.pin, kBoxL - 3.4, kEnd - 1.2, 5.4f, PAL_END, mode_ == Mode::Title ? 8.f : 0);
    place(art_.pin, 0, kEnd, 5.0f, PAL_ALERT, 0);

    for (double y = 18; y < 230; y += 16) {
        place(art_.reed, -(kBank + 2.2), y, 5.f, PAL_SHORE, 0);
        place(art_.reed, kBank + 2.2, y, 5.f, PAL_SHORE, 0, true);
    }
    for (double y = 40; y < 210; y += 36) {
        float spin = float(t_) * 1.4f + float(y) * 0.01f;
        place(art_.vane, -4.5 + std::sin(spin) * 0.4, y, 3.2f, PAL_WIND, 0);
    }

    int flap = int(t_ * 2.6) & 1;
    place(art_.gull[flap], -8 + std::sin(t_ * 0.33) * 10, 90 + std::cos(t_ * 0.2) * 6, 3.4f, PAL_BIRD, 0);
    place(art_.gull[1 - flap], 10 + std::cos(t_ * 0.27) * 8, 176, 3.0f, PAL_BIRD, 0);

    for (const Puff& p : wake_) {
        if (p.life <= 0) continue;
        float h = (1.4f + float(1.0 - p.life) * 1.8f) * zoom_;
        float sx = 160.f + float(p.x - camX_) * zoom_;
        float sy = 112.f - float(p.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false, false);
    }

    int fi = hullFrame();
    float bob = std::sin(float(t_) * 2.1f) * 0.25f;
    float bh = float(art_.boat[fi].h) / kArtScale * zoom_;
    if (mode_ == Mode::Title) bh = std::max(bh, 18.f);
    float bsx = 160.f + float(x_ - camX_) * zoom_;
    float bsy = 112.f - float(y_ + bob - camY_) * zoom_;
    spr(art_.shade, bsx + 2.f, bsy + 3.f, bh * 0.55f, PAL_BOAT, false, true);
    spr(art_.boat[fi], bsx, bsy, bh, PAL_BOAT, false, false);

    camX_ += jx * invZ;
    camY_ -= jy * invZ;

    auto banner = [&](const gs::Mipped& m, float y, int pal) { spr(m, 160.f, y, float(m.h), pal, false, false); };
    if (mode_ == Mode::Title) banner(art_.title, 20.f, PAL_BANNER);
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
        hudC(24, "WIND AFT  EASE THE SHEET TO GLIDE", PAL_HUD);
        if ((int(t_ * 2.0) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "ARROWS STEER   UP SHEET   DOWN EASE", PAL_HUD);
        return;
    }

    hud(1, 0, "S3 KEEL BOX", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "LEG %4.1fS", race_);
    hud(30, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        hudC(19, "ESC BACK TO THE LINE", PAL_HUD);
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
    std::snprintf(buf, sizeof buf, "END %3.0f", std::max(0.0, toEnd));
    hud(1, 1, buf, toEnd < 16 ? PAL_ALERT : PAL_MARK);
    std::snprintf(buf, sizeof buf, "SHEET %3d  SPD %4.1f", int(std::lround(sheet_ * 100.0)), std::fabs(surge_));
    hud(20, 1, buf, PAL_HUD);
    if (in && std::fabs(surge_) < kStop) {
        int n = std::clamp(int(hold_ / kHoldNeed * 5.0) + 1, 1, 5);
        std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
        hud(1, 2, buf, PAL_WIN);
    } else if (in) {
        hud(1, 2, "IN THE BOX  EASE AND HOLD", PAL_BANNER);
    } else if (e.maxY > kBoxT) {
        hud(1, 2, "PAST THE BOX  THE END IS AHEAD", PAL_ALERT);
    } else {
        hud(1, 2, "SAIL THE KEEL INTO THE BOX", PAL_HUD);
    }
    hud(1, 27, "ARROWS STEER  UP SHEET  DOWN EASE", PAL_HUD);
}

}  // namespace keelbox
