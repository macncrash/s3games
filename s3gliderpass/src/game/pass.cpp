#include "game/pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace gliderpass {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float SADDLE_X = 206.f;
constexpr float END_X = 412.f;
constexpr float CLOCK = 36.f;
constexpr float NOSE = 4.55f;
constexpr float TAIL = 3.95f;
constexpr float TOP = 2.62f;
constexpr float BELLY = 0.66f;
constexpr float END_LO = 7.5f;
constexpr float END_HI = 13.5f;
constexpr float TRIM = 0.82f;
constexpr float NOSE_G = 5.6f;
constexpr float SPOIL_SINK = 3.5f;
constexpr float STALL_V = 12.2f;
constexpr float STALL_K = 0.66f;
constexpr float STORM_V = 13.6f;
constexpr float START_X = 26.f;
constexpr float START_H = 9.6f;
constexpr float START_V = 16.6f;

struct Knot {
    float x, g, c;
};
struct Mark {
    float x, h;
};

constexpr Knot kCourse[] = {
    {0, 2.15f, 48.f},   {40, 2.50f, 48.f},  {70, 4.20f, 46.f},   {110, 8.40f, 44.f}, {145, 12.20f, 38.f},
    {170, 14.50f, 32.5f}, {188, 15.80f, 29.2f}, {206, 16.70f, 26.8f}, {224, 15.60f, 29.6f}, {252, 11.00f, 40.f},
    {292, 6.40f, 46.f}, {340, 4.30f, 48.f}, {390, 3.70f, 48.f},  {430, 3.50f, 48.f},  {520, 3.30f, 48.f},
};
constexpr int kCourseN = int(sizeof kCourse / sizeof kCourse[0]);

constexpr Mark kLane[] = {
    {0, 9.5f},   {50, 10.4f}, {100, 13.2f}, {140, 16.4f}, {168, 18.6f}, {188, 19.6f},
    {206, 20.2f}, {226, 19.2f}, {260, 15.0f}, {310, 12.0f}, {360, 10.8f}, {412, 10.4f}, {480, 10.4f},
};
constexpr int kLaneN = int(sizeof kLane / sizeof kLane[0]);

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float smooth(const float* xs, const float* ys, int n, float x) {
    if (x <= xs[0]) return ys[0];
    for (int i = 1; i < n; i++) {
        if (x <= xs[i]) {
            float den = xs[i] - xs[i - 1];
            float u = den > 1e-4f ? (x - xs[i - 1]) / den : 0.f;
            u = u * u * (3.f - 2.f * u);
            return ys[i - 1] + (ys[i] - ys[i - 1]) * u;
        }
    }
    return ys[n - 1];
}

float groundAt(float x) {
    float xs[kCourseN], ys[kCourseN];
    for (int i = 0; i < kCourseN; i++) {
        xs[i] = kCourse[i].x;
        ys[i] = kCourse[i].g;
    }
    return smooth(xs, ys, kCourseN, x);
}

float ceilAt(float x) {
    float xs[kCourseN], ys[kCourseN];
    for (int i = 0; i < kCourseN; i++) {
        xs[i] = kCourse[i].x;
        ys[i] = kCourse[i].c;
    }
    return smooth(xs, ys, kCourseN, x);
}

float laneAt(float x) {
    float xs[kLaneN], ys[kLaneN];
    for (int i = 0; i < kLaneN; i++) {
        xs[i] = kLane[i].x;
        ys[i] = kLane[i].h;
    }
    return smooth(xs, ys, kLaneN, x);
}

float liftOf(float v) { return clampf((v - 7.f) / 11.f, 0.32f, 1.05f); }

float stallOf(float v) { return std::max(0.f, STALL_V - v) * STALL_K; }

// Windward lift on the climb, a downdraft in the notch, lee sink after the saddle.
float ridgeAir(float x) {
    if (x > 120.f && x < 188.f) {
        float u = (x - 120.f) / 68.f;
        return 0.55f * std::sin(u * 3.1415926f);
    }
    if (x >= 188.f && x <= 230.f) {
        float u = (x - 188.f) / 42.f;
        return -0.75f * std::sin(u * 3.1415926f);
    }
    if (x > 230.f && x < 280.f) {
        float u = (x - 230.f) / 50.f;
        return -0.32f * std::sin(u * 3.1415926f);
    }
    return 0.f;
}

float vyFrom(float nose, float spoil, float v, float air) {
    float lift = liftOf(v);
    return (nose * NOSE_G - TRIM) * lift - spoil * SPOIL_SINK - stallOf(v) + air;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch chimePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.12f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.2f, 0.55f, 0.28f, 0.f};
    p.op[1] = {2.f, 0.28f, 0.012f, 0.22f, 0.3f, 0.3f, 0.f};
    p.op[2] = {3.01f, 0.12f, 0.01f, 0.18f, 0.2f, 0.32f, 0.f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f, 0.f};
    p.vol = 0.2f;
    p.tone = 1800.f;
    p.echo = 0.28f;
    return p;
}

}  // namespace

float Game::stormLeft() const { return std::max(0.f, CLOCK - legT_); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 5;
    if (passOk_ && x_ > END_X - 55.f) return 4;
    if (passOk_) return 3;
    if (x_ > SADDLE_X - 32.f) return 2;
    return 1;
}

float Game::sx(float wx) const { return anchor_ + (wx - camX_) * zoom_ + shx_; }

float Game::sy(float wy) const { return 118.f - (wy - camH_) * zoom_ + shy_; }

int Game::wingFrame() const {
    if (att_ > 0.20f) return 0;
    if (att_ > 0.06f) return 1;
    if (att_ < -0.20f) return 4;
    if (att_ < -0.06f) return 3;
    return 2;
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.055f);
    beep_ = 0.07f;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    passOk_ = false;
    gate_ = false;
    endDone_ = false;
    why_ = "";
    note_ = "";
    noteT_ = 0;
    chime_ = -1;
    lastSec_ = -1;
    x_ = 156.f;
    h_ = 15.2f;
    v_ = 16.f;
    vy_ = 0.f;
    att_ = 0.04f;
    nose_ = 0;
    spoil_ = 0;
    stormX_ = -12.f;
    legT_ = 0;
    passT_ = 0;
    t_ = 0;
    shake_ = 0;
    snapCam_ = true;
    for (Puff& p : puffs_) p = {};
}

void Game::startRun() {
    mode_ = Mode::Fly;
    over_ = false;
    won_ = false;
    passOk_ = false;
    gate_ = false;
    endDone_ = false;
    why_ = "";
    note_ = "CLIMB FOR THE PASS";
    noteT_ = 2.2f;
    chime_ = -1;
    lastSec_ = -1;
    x_ = START_X;
    h_ = START_H;
    v_ = START_V;
    vy_ = -0.15f;
    att_ = 0.02f;
    nose_ = 0;
    spoil_ = 0;
    stormX_ = -14.f;
    t_ = 0;
    legT_ = 0;
    passT_ = 0;
    shake_ = 0;
    puffN_ = 0;
    snapCam_ = true;
    for (Puff& p : puffs_) p = {};
    blip(620.f);
}

void Game::win() {
    if (mode_ != Mode::Fly) return;
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "clear";
    note_ = "LEG CLEAR";
    chime_ = 0;
    chimeT_ = 0;
    sys_->rumble(0.22f, 0.08f, 160);
    sys_->setLight(40, 170, 90);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Fly) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    shake_ = 1.f;
    sys_->rumble(0.7f, 0.4f, 200);
    sys_->setLight(170, 30, 24);
    sys_->apu.noiseBurst(0.55f, 420.f, 0.32f);
    sys_->apu.tone(2, 0.f, 0.f);
}

void Game::pilot(float& nose, float& spoil) const {
    float lane = laneAt(x_ + 8.f);
    float floorH = groundAt(x_ + 12.f) + BELLY + 1.45f;
    floorH = std::max(floorH, groundAt(x_) + BELLY + 1.25f);
    float roof = ceilAt(x_ + 8.f);
    if (roof < 40.f) lane = std::min(lane, roof - TOP - 1.25f);
    roof = ceilAt(x_);
    if (roof < 40.f) lane = std::min(lane, roof - TOP - 1.05f);
    lane = std::max(lane, floorH);
    if (passOk_) lane = clampf(lane, END_LO + 0.6f, END_HI - 0.6f);

    float err = lane - h_;
    float vyWant = clampf(err * 0.9f - vy_ * 0.28f, -2.3f, 2.5f);
    float margin = h_ - BELLY - groundAt(x_ + 8.f);
    if (v_ < 14.4f && margin > 2.6f) vyWant = std::min(vyWant, -0.65f);
    if (v_ < 13.0f && margin > 2.2f) vyWant = std::min(vyWant, -1.5f);
    if (roof < 40.f) {
        float room = roof - TOP - h_;
        if (room < 2.4f) vyWant = std::min(vyWant, 0.2f);
        if (room < 1.2f) vyWant = std::min(vyWant, -0.45f);
    }
    if (passOk_ && END_X - x_ < 100.f) vyWant = clampf(err * 1.2f - vy_ * 0.4f, -2.1f, 2.1f);

    spoil = 0.f;
    if (v_ > 20.8f && err < 1.6f) spoil = 0.4f;
    if (err < -2.0f && margin > 2.8f) spoil = std::max(spoil, 0.62f);
    if (x_ > 186.f && x_ < 232.f) spoil = 0.f;

    float lift = std::max(liftOf(v_), 0.3f);
    float air = ridgeAir(x_);
    float need = vyWant + spoil * SPOIL_SINK + stallOf(v_) - air;
    nose = (need / lift + TRIM) / NOSE_G;
    nose = clampf(nose, -1.f, 1.f);
}

void Game::physics(float noseCmd, float spoilCmd) {
    if (bot_) {
        nose_ = noseCmd;
        spoil_ = spoilCmd;
    } else {
        nose_ += (noseCmd - nose_) * 0.32f;
        spoil_ += (spoilCmd - spoil_) * 0.28f;
    }
    nose_ = clampf(nose_, -1.f, 1.f);
    spoil_ = clampf(spoil_, 0.f, 1.f);
    legT_ += DT;
    t_ += DT;

    float air = ridgeAir(x_);
    float vyCmd = vyFrom(nose_, spoil_, v_, air);
    float gust = 0.28f * std::sin(x_ * 0.17f + 0.6f) * std::sin(legT_ * 0.85f);
    if (x_ > 190.f && x_ < 226.f) gust *= 0.35f;
    vy_ += (vyCmd - vy_) * std::min(1.f, 6.0f * DT);
    vy_ += gust * DT;
    v_ += (-0.045f + 0.38f * std::max(-vy_, 0.f) - 0.16f * std::max(vy_, 0.f) - spoil_ * 2.3f) * DT;
    if (x_ > 176.f && x_ < 236.f) v_ -= 0.35f * DT;
    v_ = clampf(v_, 0.f, 26.f);
    x_ += v_ * DT;
    h_ += vy_ * DT;
    stormX_ += STORM_V * DT;
    if (!std::isfinite(x_) || !std::isfinite(h_) || !std::isfinite(v_)) {
        fail("missed the end");
        return;
    }

    const float stations[] = {-TAIL, -TAIL * 0.45f, 0.f, NOSE * 0.45f, NOSE};
    for (float dx : stations) {
        float px = x_ + dx;
        float py = h_ + std::sin(att_) * dx;
        float g = groundAt(px);
        float c = ceilAt(px);
        float belly = (std::fabs(dx) > NOSE * 0.7f) ? 0.32f : BELLY;
        float crown = (dx < -TAIL * 0.55f) ? TOP : 1.72f;
        if (py - belly <= g) {
            fail("hit the ridge");
            return;
        }
        if (c < 42.f && py + crown >= c) {
            fail("missed the pass");
            return;
        }
    }
    if (h_ > 36.f) {
        fail("missed the end");
        return;
    }

    if (!gate_ && x_ >= SADDLE_X) {
        gate_ = true;
        float g = groundAt(SADDLE_X);
        float c = ceilAt(SADDLE_X);
        if (h_ - BELLY < g + 0.05f) {
            fail("hit the ridge");
            return;
        }
        if (h_ + TOP > c - 0.05f) {
            fail("missed the pass");
            return;
        }
        passOk_ = true;
        passT_ = legT_;
        note_ = "PASS CLEAR";
        noteT_ = 1.8f;
        blip(740.f);
    }

    if (stormX_ >= x_ - TAIL) {
        fail("taken by the storm");
        return;
    }

    if (!endDone_ && x_ + NOSE >= END_X) {
        endDone_ = true;
        if (!passOk_ || h_ < END_LO || h_ > END_HI) fail("missed the end");
        else win();
        return;
    }
    if (legT_ >= CLOCK) fail("the storm clock");
    else if (x_ > END_X + 24.f) fail("missed the end");
}

void Game::audio() {
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (chime_ >= 0) {
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        chimeT_ += DT;
        if (chimeT_ > 0.16f) {
            if (chime_ < 4) sys_->apu.keyOn(0, notes[chime_], 0.22f);
            else sys_->apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 8) chime_ = -1;
        }
    }
    if (mode_ != Mode::Fly) {
        if (mode_ != Mode::Win) sys_->apu.tone(2, 0.f, 0.f);
        if (mode_ != Mode::Fly) sys_->apu.noise(mode_ == Mode::Win ? 0.02f : 0.f, 900.f, false);
        return;
    }
    float wind = clampf((v_ - 8.f) / 16.f, 0.f, 1.f) * (spoil_ > 0.45f ? 0.08f : 0.045f);
    float mood = clampf(legT_ / CLOCK, 0.f, 1.f);
    sys_->apu.noise(wind + mood * 0.03f, 640.f + v_ * 30.f + mood * 400.f, false);
    varioT_ -= DT;
    if (varioT_ <= 0.f) {
        varioT_ = (vy_ > 0.4f) ? 0.18f : (vy_ < -0.85f) ? 0.1f : 0.32f;
        if (v_ < STALL_V + 0.5f) sys_->apu.tone(2, 170.f, 0.045f);
        else if (vy_ > 0.35f) sys_->apu.tone(2, 640.f + vy_ * 24.f, 0.02f);
        else if (vy_ < -0.75f) sys_->apu.tone(2, 320.f, 0.018f);
        else sys_->apu.tone(2, 0.f, 0.f);
    }
    int sec = int(legT_);
    int left = std::max(0, int(std::ceil(CLOCK - legT_ - 1e-3f)));
    if (sec != lastSec_) {
        lastSec_ = sec;
        if (left > 0 && left <= 8 && beep_ <= 0.f) blip(left <= 3 ? 920.f : 480.f);
    }
}

void Game::camera() {
    float wantZ = 8.15f, wantA = 96.f, wantX = x_ + 5.2f, wantH = h_ * 0.62f + 3.8f;
    if (mode_ == Mode::Title) {
        wantZ = 3.55f;
        wantA = 150.f;
        wantX = 188.f;
        wantH = 16.2f;
    } else if (mode_ == Mode::Win) {
        wantZ = 8.6f;
        wantX = x_ - 1.2f;
        wantA = 150.f;
        wantH = h_ * 0.7f + 3.2f;
    } else if (mode_ == Mode::Fail) {
        wantX = x_ + 1.f;
        wantA = 140.f;
    }
    if (snapCam_) {
        camX_ = wantX;
        camH_ = wantH;
        zoom_ = wantZ;
        anchor_ = wantA;
        snapCam_ = false;
    } else {
        float k = 1.f - std::exp(-7.2f * DT);
        camX_ += (wantX - camX_) * k;
        camH_ += (wantH - camH_) * k;
        zoom_ += (wantZ - zoom_) * k;
        anchor_ += (wantA - anchor_) * k;
    }
    if (shake_ > 0.f) {
        shake_ = std::max(0.f, shake_ - DT * 1.7f);
        shx_ = std::sin(t_ * 46.f) * shake_ * 5.f;
        shy_ = std::cos(t_ * 37.f) * shake_ * 3.f;
    } else {
        shx_ = shy_ = 0.f;
    }
}

void Game::sky(float mood) {
    uint16_t zen = lerpC(gs::rgb4(2, 4, 10), gs::rgb4(3, 3, 6), mood);
    uint16_t mid = lerpC(gs::rgb4(6, 9, 14), gs::rgb4(6, 6, 9), mood);
    uint16_t hor = lerpC(gs::rgb4(15, 11, 7), gs::rgb4(8, 7, 9), mood);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(12, 14, 9), 0.45f);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(10, 4, 4), 0.4f);
    bool flash = mode_ == Mode::Fly && mood > 0.35f && std::fmod(legT_, 6.5f) < 0.07f;
    if (flash) {
        zen = lerpC(zen, gs::rgb4(14, 14, 15), 0.7f);
        mid = lerpC(mid, gs::rgb4(13, 13, 15), 0.55f);
    }
    float horY = sy(std::max(0.f, camH_ - 2.f));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.5f ? lerpC(zen, mid, t / 0.5f) : lerpC(mid, hor, (t - 0.5f) / 0.5f);
        float df = std::fabs(float(y) - horY);
        sys_->vdp.lineFog[y] = uint8_t(clampf((4.5f + mood * 4.f) - df / 22.f, 0.f, 8.f));
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.setFogColor(lerpC(gs::rgb4(12, 10, 8), gs::rgb4(6, 6, 8), mood));
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip, int fog, bool shadow) {
    if (ht < 1.3f || m.h < 1) return;
    float w = ht * float(m.w) / float(std::max(1, m.h));
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -4000L, 4000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h * 0.5f)), -4000L, 4000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -8) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprAnchor(const gs::Mipped& m, float ax, float ay, float cx, float cy, float destH, int pal) {
    if (destH < 1.5f || m.h < 1) return;
    float sc = destH / float(m.h);
    float w = float(m.w) * sc;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(destH)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - ax * sc)), -4000L, 4000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - ay * sc)), -4000L, 4000L));
    if (s.x > gs::SCREEN_W + 12 || s.x + s.w < -12 || s.y > gs::SCREEN_H + 12 || s.y + s.h < -12) return;
    s.img = m.pick(destH);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::worldBand(float x0, float x1, float hTop, float hBot, const gs::Mipped& m, int pal, int fog) {
    float s0 = sx(x0), s1 = sx(x1);
    float y0 = sy(hTop), y1 = sy(hBot);
    float top = std::min(y0, y1);
    float bot = std::max(y0, y1);
    float w = std::fabs(s1 - s0) + 1.5f;
    float h = std::max(1.5f, bot - top);
    if (w < 1.2f || h < 1.2f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround((s0 + s1) * 0.5f - s.w * 0.5f)), -4000L, 4000L));
    s.y = int16_t(std::clamp(long(std::lround(top)), -4000L, 4000L));
    if (s.x > gs::SCREEN_W + 4 || s.x + s.w < -4 || s.y > gs::SCREEN_H + 4 || s.y + s.h < -4) return;
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::prop(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip, int fog) {
    spr(m, sx(wx), sy(wy), worldH * zoom_, pal, flip, fog);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !s[0]) return;
    const float adv = 18.f * scale;
    float left = x - float(std::strlen(s)) * adv * 0.5f;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + g.w * scale * 0.5f, y, std::max(8.f, g.h * scale), pal, false);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    camera();

    float mood = clampf(legT_ / CLOCK, 0.f, 1.f);
    float wall = stormX_;
    if (mode_ == Mode::Title) {
        mood = 0.46f + 0.06f * std::sin(t_ * 0.7f);
        wall = camX_ - 18.f;
    } else if (mode_ == Mode::Win) {
        mood *= 0.35f;
    } else if (mode_ == Mode::Fail) {
        mood = std::max(mood, 0.72f);
    }
    float gap = x_ - wall;
    if (mode_ == Mode::Fly) mood = std::max(mood, clampf(1.f - gap / 90.f, 0.f, 0.85f));
    sky(mood);

    const char* banner = nullptr;
    int bannerPal = PAL_HUD;
    if (mode_ == Mode::Title) banner = "GLIDER PASS";
    else if (mode_ == Mode::Pause) {
        banner = "PAUSED";
        bannerPal = PAL_AMBER;
    } else if (mode_ == Mode::Fail) {
        banner = (why_ && std::strstr(why_, "ridge")) ? "RIDGE" : (why_ && std::strstr(why_, "storm")) ? "STORM" : "MISSED";
        bannerPal = PAL_BAD;
    } else if (mode_ == Mode::Win) {
        banner = "CLEAR";
        bannerPal = PAL_GOOD;
    }
    if (banner) text(banner, 160.f, mode_ == Mode::Title ? 20.f : 24.f, mode_ == Mode::Title ? 1.05f : 1.15f, bannerPal);
    if (mode_ == Mode::Title) text("BEFORE THE STORM", 160.f, 46.f, 0.52f, PAL_AMBER);

    const Wing& wing = art_.wing[wingFrame()];
    float destH = float(wing.img.h) * (zoom_ / wing.ppm);
    sprAnchor(wing.img, wing.ax, wing.ay, sx(x_), sy(h_), destH, PAL_SHIP);

    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        prop(art_.puff, p.x, p.y, 0.55f + (1.f - p.life) * 0.7f, PAL_FX, false, int((1.f - p.life) * 8));
    }
    {
        float bed = groundAt(x_);
        float lift = clampf(h_ - bed, 0.f, 18.f);
        spr(art_.shadow, sx(x_), sy(bed) + 2.f, (16.f + lift) * 0.18f, PAL_FX, false, 0, true);
    }

    const float flags[] = {60.f, 110.f, 155.f, 206.f, 250.f, 310.f, 370.f, 412.f};
    for (float fx : flags) prop(art_.pennant, fx, laneAt(fx) + 0.35f, 1.5f, PAL_FLAG, false, 1);

    int gull = int(t_ * 5.f) & 1;
    prop(art_.gull[gull], 80.f + std::fmod(t_ * 6.f, 70.f), 18.f + std::sin(t_ * 0.8f), 1.15f, PAL_BIRD, false, 4);
    prop(art_.gull[1 - gull], 250.f + std::sin(t_ * 0.4f) * 12.f, 22.f, 1.05f, PAL_BIRD, true, 6);

    float topRib = END_HI + TOP + 0.25f;
    float botRib = END_LO - BELLY - 0.2f;
    float bedEnd = groundAt(END_X);
    worldBand(END_X - 0.28f, END_X + 0.28f, topRib + 1.3f, bedEnd, art_.post, PAL_TAPE);
    worldBand(END_X - 8.f, END_X + 8.f, topRib + 0.35f, topRib - 0.15f, art_.tape, PAL_TAPE);
    worldBand(END_X - 8.f, END_X + 8.f, botRib + 0.2f, botRib - 0.35f, art_.tape, PAL_TAPE);

    prop(art_.cairn, SADDLE_X - 2.4f, groundAt(SADDLE_X - 2.4f) + 1.15f, 2.3f, PAL_ROCK);
    prop(art_.hut, 96.f, groundAt(96.f) + 1.7f, 3.5f, PAL_HUT);
    prop(art_.sock, 34.f, groundAt(34.f) + 2.3f, 2.6f, PAL_FLAG);

    const float pines[] = {52.f, 78.f, 104.f, 132.f, 158.f, 242.f, 268.f, 304.f, 348.f, 388.f};
    for (float px : pines) {
        float g = groundAt(px);
        prop(art_.pine, px, g + 2.05f, 4.1f, PAL_PINE, px > 200.f, g > 12.f ? 2 : 0);
    }

    float left = camX_ - (anchor_ + 30.f) / zoom_;
    float right = camX_ + (gs::SCREEN_W - anchor_ + 40.f) / zoom_;
    float yBot = camH_ - 170.f / std::max(zoom_, 1.f);
    float yTop = camH_ + 150.f / std::max(zoom_, 1.f);
    float a = std::floor(left / 3.f) * 3.f;
    float b = right + 3.f;
    for (float wx = a; wx < b; wx += 3.f) {
        float g0 = groundAt(wx);
        float g1 = groundAt(wx + 3.f);
        float top = std::max(g0, g1);
        const gs::Mipped& cap = top > 12.f ? art_.snow : art_.grass;
        int capPal = top > 12.f ? PAL_SNOW : PAL_PINE;
        worldBand(wx, wx + 3.35f, top + 0.15f, top - 1.05f, cap, capPal);
    }
    worldBand(-8.f, 62.f, 2.35f, 1.55f, art_.tarn, PAL_WATER);
    for (float wx = a; wx < b; wx += 3.f) {
        float g0 = groundAt(wx);
        float g1 = groundAt(wx + 3.f);
        float top = std::max(g0, g1) - 0.7f;
        worldBand(wx, wx + 3.35f, top, std::min(top - 0.4f, yBot), art_.rock, PAL_ROCK, 1);
        float c0 = ceilAt(wx);
        float c1 = ceilAt(wx + 3.f);
        float bot = std::min(c0, c1);
        if (bot < 40.f) worldBand(wx, wx + 3.35f, std::max(bot + 0.4f, yTop), bot, art_.rock, PAL_ROCK, 3);
    }

    prop(art_.wall, wall, groundAt(wall) + 14.f, 28.f, PAL_STORM, false, 2);
    prop(art_.nimbus, 196.f, 30.5f, 7.5f, PAL_STORM, false, 2);
    prop(art_.nimbus, 228.f, 31.4f, 8.2f, PAL_STORM, true, 3);

    for (int i = 0; i < 4; i++) {
        float hx = std::fmod(40.f + float(i) * 90.f - camX_ * 0.28f, 380.f);
        if (hx < -50.f) hx += 380.f;
        spr(art_.peak, hx, 150.f, 34.f + float(i % 2) * 8.f, PAL_SKY, i & 1, 8);
    }
    spr(art_.sun, 270.f, 26.f, 18.f, PAL_SKY, false, int(mood * 10));
    for (int i = 0; i < 4; i++) {
        float cx = std::fmod(20.f + float(i) * 100.f - camX_ * 0.08f + t_ * (6.f + mood * 10.f), 460.f);
        if (cx < -40.f) cx += 460.f;
        spr(art_.cloud, cx, 22.f + float(i % 3) * 14.f, 14.f + float(i % 2) * 5.f, i > 1 ? PAL_STORM : PAL_SKY, i & 1,
            2 + int(mood * 4));
    }
    if (mood > 0.4f) {
        float rx = std::fmod(t_ * 140.f, 200.f);
        spr(art_.rain, rx, 70.f, 90.f, PAL_STORM, false, 6);
        spr(art_.rain, rx + 120.f, 90.f, 80.f, PAL_STORM, false, 7);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(16, "UP CLIMB    DOWN DIVE", PAL_HUD);
        hudC(18, "Z SPOILER", PAL_HUD);
        hudC(20, "CLEAR THE PASS", PAL_HUD);
        hudC(21, "BEFORE THE STORM CLOCK", PAL_AMBER);
        hudC(23, "MISSING THE END FAILS THE LEG", PAL_BAD);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START FLIES    ESC TITLE", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(23, why_ ? why_ : "", PAL_BAD);
        hudC(25, "START TRIES THE LEG AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(22, "CLEARED THE PASS", PAL_GOOD);
        hudC(23, "BEFORE THE STORM CLOCK", PAL_HUD);
        std::snprintf(buf, sizeof buf, "%.1f S", legT_);
        hudC(25, buf, PAL_AMBER);
    } else {
        std::snprintf(buf, sizeof buf, "ALT %4.1f", h_);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(13, 1, buf, v_ < STALL_V ? PAL_BAD : PAL_HUD);
        int left = std::max(0, int(std::ceil(CLOCK - legT_ - 1e-3f)));
        std::snprintf(buf, sizeof buf, "STORM %2d", left);
        hud(26, 1, buf, left <= 10 ? PAL_BAD : PAL_AMBER);
        if (v_ < STALL_V + 0.3f) hud(26, 2, "STALL", PAL_BAD);
        else if (spoil_ > 0.45f) hud(26, 2, "SPOILER", PAL_AMBER);

        const char* line = "CLIMB FOR THE PASS";
        int pal = PAL_HUD;
        if (!passOk_) {
            float dist = SADDLE_X - x_;
            if (dist < 36.f) {
                line = "THREAD THE NOTCH";
                pal = PAL_AMBER;
            } else {
                std::snprintf(buf, sizeof buf, "PASS  %3.0f M", std::max(0.f, dist));
                line = buf;
            }
        } else {
            float dist = END_X - (x_ + NOSE);
            if (dist < 46.f) {
                std::snprintf(buf, sizeof buf, "SLOT  %.0f TO %.0f", END_LO, END_HI);
                line = buf;
                pal = (h_ >= END_LO && h_ <= END_HI) ? PAL_GOOD : PAL_BAD;
            } else {
                std::snprintf(buf, sizeof buf, "END  %3.0f M", std::max(0.f, dist));
                line = buf;
                pal = PAL_AMBER;
            }
        }
        hudC(3, line, pal);
        if (noteT_ > 0.f && note_ && note_[0]) hudC(4, note_, PAL_GOOD);
        hudC(26, "UP DOWN FLY    Z SPOILER", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(8, 8, 10));
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.18f, 0.24f, 0.16f);
    sys.apu.setPatch(0, chimePatch());
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += DT;
        h_ = 15.2f + std::sin(t_ * 1.1f) * 0.35f;
        att_ = 0.05f + std::sin(t_ * 0.6f) * 0.05f;
        x_ = 156.f + std::sin(t_ * 0.35f) * 1.6f;
    }
    if (noteT_ > 0.f) noteT_ = std::max(0.f, noteT_ - DT);
    for (Puff& p : puffs_)
        if (p.life > 0.f) p.life = std::max(0.f, p.life - DT);
    audio();

    if (!bot_ && mode_ == Mode::Title) {
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
        return;
    }

    if (mode_ == Mode::Pause) {
        draw();
        if (pad.pressed(gs::BTN_START)) {
            blip(420.f);
            mode_ = Mode::Fly;
        } else if (pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (mode_ == Mode::Win) t_ += DT;
        draw();
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    float nose = 0.f, spoil = 0.f;
    if (bot_) {
        pilot(nose, spoil);
    } else {
        if (pad.down(gs::BTN_UP)) nose += 1.f;
        if (pad.down(gs::BTN_DOWN)) nose -= 1.f;
        if (std::fabs(pad.axisY) > 0.18f) nose = pad.axisY;
        nose = clampf(nose, -1.f, 1.f);
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_X) ||
            pad.down(gs::BTN_Z) || pad.down(gs::BTN_TURBO))
            spoil = 1.f;
        if (pad.accel > 0.08f) spoil = std::max(spoil, pad.accel);
        if (pad.brake > 0.08f) spoil = std::max(spoil, pad.brake);
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(400.f);
            draw();
            return;
        }
    }

    bool wasFly = mode_ == Mode::Fly;
    physics(nose, spoil);
    if (wasFly && (mode_ == Mode::Fly || mode_ == Mode::Win)) {
        float path = std::atan2(vy_, std::max(8.f, v_));
        float want = path * 0.8f + nose_ * 0.14f;
        att_ += (want - att_) * 0.28f;
        att_ = clampf(att_, -0.48f, 0.48f);
    }
    if (mode_ == Mode::Fly && spoil_ > 0.5f && (sys.frame % 5) == 0) {
        puffs_[puffN_ % 8] = {x_ - 1.2f, h_ - 0.2f, 0.4f};
        puffN_++;
    }
    if (mode_ == Mode::Fly) {
        int left = int(std::ceil(CLOCK - legT_));
        if (left <= 8) sys.setLight(170, 40, 30);
        else if (passOk_) sys.setLight(40, 140, 90);
        else if (x_ > SADDLE_X - 40.f) sys.setLight(150, 90, 40);
        else sys.setLight(40, 80, 160);
    }
    draw();
}

}  // namespace gliderpass
