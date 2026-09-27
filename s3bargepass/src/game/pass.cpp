#include "pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace bargepass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kExit = 1980.f;
constexpr float kHalfLen = 46.f;
constexpr float kBeam = 13.f;
constexpr float kClock = 36.f;
constexpr float kMargin = 2.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::centerAt(float x) const {
    return 28.f * std::sin(x * 0.0031f) + 14.f * std::sin(x * 0.0074f + 0.6f);
}

float Game::halfAt(float x) const {
    float n = x / kExit;
    float pinch = std::exp(-std::pow((n - 0.58f) / 0.15f, 2.f));
    return 86.f - pinch * 34.f;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (x_ > 1500.f) return 3;
    if (x_ > 780.f) return 2;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.reset();
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.clear();
    sys.apu.setMaster(0.42f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    x_ = 140.f;
    y_ = centerAt(x_);
    cam_ = x_;
    storm_ = kClock;
    time_ = 0;
}

void Game::begin() {
    x_ = 140.f;
    y_ = centerAt(x_);
    vx_ = vy_ = 0;
    time_ = 0;
    storm_ = kClock;
    cam_ = x_;
    shake_ = 0;
    why_ = "";
    over_ = false;
    won_ = false;
    chime_ = -1;
    mode_ = Mode::Run;
    blip(240.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.12f);
    beep_ = 0.08f;
}

void Game::wreck(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    over_ = true;
    won_ = false;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    vx_ *= 0.15f;
    sys_->apu.noiseBurst(0.45f, 800.f, 0.28f);
    sys_->apu.tone(1, 80.f, 0.22f);
    beep_ = 0.32f;
}

void Game::finish() {
    if (mode_ != Mode::Run) return;
    over_ = true;
    won_ = true;
    mode_ = Mode::Win;
    chime_ = 0;
    chimeT_ = 0;
    why_ = "clear";
}

void Game::pilot(float& thrust, float& steer) {
    const gs::Pad& pad = sys_->pad;
    thrust = 0;
    steer = 0;
    if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.accel > 0.2f) thrust += 1.f;
    if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) thrust -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (std::fabs(pad.axisX) > 0.2f) steer = pad.axisX;
    if (!bot_) return;
    float look = 120.f + vx_ * 0.55f;
    float ahead = x_ + look;
    float aim = centerAt(ahead);
    float slope = (centerAt(x_ + 24.f) - centerAt(x_)) / 24.f;
    float wantVy = slope * std::max(vx_, 20.f);
    float err = aim - y_;
    steer = clampf(err * 0.08f + (wantVy - vy_) * 0.06f, -1.f, 1.f);
    float room = halfAt(x_) - kBeam - std::fabs(y_ - centerAt(x_));
    thrust = room < 10.f ? 0.45f : 1.f;
}

void Game::stepRun(float thrust, float steer) {
    float drag = 1.15f;
    vx_ += (thrust * 92.f - vx_ * drag) * kDt;
    if (thrust < 0.f) vx_ += thrust * 40.f * kDt;
    vx_ = clampf(vx_, 0.f, 100.f);
    vy_ += (steer * 140.f - vy_ * 3.4f) * kDt;
    x_ += vx_ * kDt;
    y_ += vy_ * kDt;
    storm_ -= kDt;
    time_ += kDt;

    float worst = 99.f;
    for (float s = -1.f; s <= 1.f; s += 1.f) {
        float px = x_ + s * kHalfLen;
        float room = halfAt(px) - std::fabs(y_ - centerAt(px)) - kBeam;
        worst = std::min(worst, room);
    }
    if (worst < -kMargin) wreck("HULL ON THE ROCK");
    else if (storm_ <= 0.f) wreck("STORM CLOSED THE PASS");
    else if (x_ + kHalfLen >= kExit) finish();
}

void Game::frame(gs::System& sys) {
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        time_ += kDt;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || (bot_ && time_ > 0.2f)) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float thrust = 0, steer = 0;
            pilot(thrust, steer);
            stepRun(thrust, steer);
        }
    } else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
        time_ = 0;
        mode_ = Mode::Title;
        over_ = false;
    }

    float want = x_ - 30.f;
    cam_ += (want - cam_) * (1.f - std::exp(-kDt * 3.4f));
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.5f);
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        int step = int(chimeT_ / 0.15f);
        if (step != chime_ && step < 4) {
            chime_ = step;
            sys.apu.tone(2, notes[step], 0.15f);
        }
        if (step >= 6) {
            sys.apu.tone(2, 0, 0);
            chime_ = -1;
        }
    }
    if (mode_ == Mode::Run && storm_ < 8.f) sys.apu.noise(0.04f, 400.f + (8.f - storm_) * 40.f);
    else if (mode_ != Mode::Fail) sys.apu.noise(0, 0);
    draw();
}

void Game::quad(const gs::Mipped& m, float x0, float y0, float x1, float y1, int pal) {
    if (m.h < 1) return;
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);
    float jx = (shake_ > 0.f) ? std::sin(time_ * 90.f) * shake_ * 3.f : 0.f;
    auto sx = [&](float x) { return (x - cam_) + 70.f + jx; };
    auto sy = [&](float y) { return 112.f + y * 1.35f; };
    float px0 = sx(x0), px1 = sx(x1), py0 = sy(y0), py1 = sy(y1);
    if (px1 < -48.f || px0 > gs::SCREEN_W + 48.f || py1 < -48.f || py0 > gs::SCREEN_H + 48.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(px1 - px0), 1L, 420L));
    s.h = int16_t(std::clamp(std::lround(py1 - py0), 1L, 420L));
    s.x = int16_t(std::lround(px0));
    s.y = int16_t(std::lround(py0));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::mark(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (m.h < 1 || h < 1.f) return;
    float w = h * float(m.w) / float(m.h);
    quad(m, cx - w * 0.5f, cy - h * 0.5f, cx + w * 0.5f, cy + h * 0.5f, pal);
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

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float gloom = clampf(1.f - storm_ / kClock, 0.f, 1.f);
    if (mode_ == Mode::Title) gloom = 0.15f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r = int(3 + (1.f - gloom) * 3.f);
        int g = int(5 + (1.f - gloom) * 5.f - y * 0.01f);
        int b = int(6 + (1.f - gloom) * 6.f);
        vdp.lineBackdrop[y] = gs::rgb4(std::max(1, r), std::max(2, g), std::max(3, b));
        vdp.lineFog[y] = uint8_t(gloom > 0.55f ? int((gloom - 0.55f) * 10.f) : 0);
        vdp.road[y].on = false;
    }

    if (mode_ == Mode::Win) mark(art_.clear, x_, y_ - 28.f, 22.f, PAL_WIN);
    else if (mode_ == Mode::Fail) mark(art_.fail, x_, y_ - 28.f, 20.f, PAL_ALERT);
    else if (mode_ == Mode::Title) mark(art_.title, cam_ + 90.f, -6.f, 26.f, PAL_GOLD);

    int flakes = 8 + int(gloom * 16.f);
    for (int i = 0; i < flakes; i++) {
        float fx = std::fmod(cam_ * 0.4f + i * 47.f + time_ * (18.f + (i % 5) * 6.f), 360.f) - 40.f;
        float fy = std::fmod(i * 31.f + time_ * (30.f + i), 200.f) - 90.f;
        mark(art_.flake, cam_ + fx, fy, 4.f + (i & 1), PAL_STORM);
    }

    mark(art_.hull, x_, y_, 26.f, PAL_HULL);
    float bow = x_ + kHalfLen;
    float foam = std::sin(time_ * 7.f) * 2.f;
    quad(art_.foam, bow - 4.f, y_ - 7.f + foam * 0.1f, bow + 16.f, y_ + 7.f, PAL_FOAM);

    float rx = x_ - 210.f - time_ * 6.f;
    if (rx > 40.f) mark(art_.rival, rx, centerAt(rx) * 0.85f, 18.f, PAL_RIVAL);

    mark(art_.cabin, 60.f, centerAt(60.f) - halfAt(60.f) - 22.f, 24.f, PAL_GOLD);
    mark(art_.arch, kExit, centerAt(kExit) - halfAt(kExit) + 6.f, 48.f, PAL_ARCH);
    mark(art_.arch, kExit, centerAt(kExit) + halfAt(kExit) - 6.f, 48.f, PAL_ARCH);

    float x0 = std::floor((cam_ - 80.f) / 36.f) * 36.f;
    for (float x = x0; x < cam_ + 280.f; x += 36.f) {
        float c = centerAt(x + 18.f);
        float h = halfAt(x + 18.f);
        quad(art_.cliff, x, c - h - 46.f, x + 36.f, c - h + 4.f, PAL_CLIFF);
        quad(art_.cliff, x, c + h - 4.f, x + 36.f, c + h + 46.f, PAL_CLIFF);
        if (int(x) % 108 == 0) {
            mark(art_.pine, x + 10.f, c - h - 28.f, 30.f, PAL_PINE);
            mark(art_.pine, x + 22.f, c + h + 26.f, 26.f, PAL_PINE);
        }
    }

    if (mode_ == Mode::Title) {
        hudC(16, "CLEAR THE PASS", PAL_HUD);
        hudC(18, "BEFORE THE STORM CLOCK", PAL_HUD);
        hudC(22, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        hudC(24, "CLEAR OF THE PASS", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        hudC(24, why_, PAL_ALERT);
    } else {
        const char* hint = "KEEP OFF THE ROCK";
        if (x_ > 1500.f) hint = "THE MOUTH IS AHEAD";
        else if (x_ > 780.f) hint = "THE PASS NARROWS";
        hudC(25, hint, PAL_HUD);
        hud(1, 26, "ARROWS  STEER AND DRIVE", PAL_HUD);
    }

    hud(1, 0, "S3 BARGE PASS", PAL_GOLD);
    char buf[16];
    int show = mode_ == Mode::Title ? int(kClock) : std::max(0, int(std::ceil(storm_)));
    std::snprintf(buf, sizeof(buf), "%02d", show);
    hud(36, 0, buf, storm_ < 8.f && mode_ == Mode::Run ? PAL_ALERT : PAL_HUD);
}

}  // namespace bargepass
