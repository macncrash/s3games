#include "game/plow.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "version.h"

namespace plowlock {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kLimit = 36.f;
constexpr float kLane = 16.f;
constexpr float kLo = 46.f;
constexpr float kHi = 112.f;
constexpr float kGateT = 7.f;
constexpr float kHoldY = 78.f;
constexpr float kEnd = 168.f;
constexpr float kHalfL = 5.4f;
constexpr float kHalfW = 3.15f;
constexpr float kGap = 9.2f;

float wrapPi(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

bool overlap(float a0, float a1, float b0, float b1) { return a0 < b1 && a1 > b0; }

}  // namespace

float Game::bowY() const { return y_ + std::cos(heading_) * kHalfL; }
float Game::sternY() const { return y_ - std::cos(heading_) * kHalfL; }

int Game::marker() const {
    if (mode_ == Mode::Win) return 4;
    if (mode_ != Mode::Run && mode_ != Mode::Pause) return 0;
    if (phase_ == Phase::Leave && sternY() > kHi + 8.f) return 3;
    if (phase_ == Phase::Shut || phase_ == Phase::Hold || phase_ == Phase::Open) return 2;
    if (phase_ == Phase::Approach || phase_ == Phase::Leave) return 1;
    return 0;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.08f);
    tone_ = 0.08f;
}

void Game::begin() {
    x_ = 0.f;
    y_ = 6.f;
    heading_ = 0.f;
    speed_ = 0.f;
    lo_ = 1.f;
    hi_ = 0.f;
    hold_ = 0.f;
    time_ = 0.f;
    won_ = false;
    over_ = false;
    why_ = "";
    phase_ = Phase::Approach;
    mode_ = Mode::Run;
    shake_ = 0.f;
    chime_ = -1;
    camX_ = 0.f;
    camY_ = 28.f;
    zoom_ = 2.15f;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.resize(64, 32);
    for (int y = 0; y < sys.vdp.B.h; y++)
        for (int x = 0; x < sys.vdp.B.w; x++) sys.vdp.B.set(x, y, gs::entry(art_.snowTile, PAL_SNOW));
    sys.vdp.setFogColor(gs::rgb4(9, 11, 13));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.06f, 0.12f, 0.05f);
    begin();
    if (!bot_) {
        mode_ = Mode::Title;
        zoom_ = 1.35f;
        camY_ = 86.f;
    }
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    over_ = true;
    won_ = false;
    why_ = why;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    speed_ *= 0.2f;
    sys_->apu.noiseBurst(0.4f, 880.f, 0.22f);
    sys_->apu.tone(1, 80.f, 0.18f);
    tone_ = 0.25f;
}

void Game::finish() {
    if (mode_ != Mode::Run) return;
    over_ = true;
    won_ = true;
    why_ = "clear";
    mode_ = Mode::Win;
    chime_ = 0;
    chimeT_ = 0;
}

void Game::pilot(float& gas, float& steer) {
    const gs::Pad& pad = sys_->pad;
    gas = 0.f;
    steer = 0.f;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C)) gas += 1.f;
    if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B)) gas -= 1.f;
    if (std::fabs(pad.axisX) > 0.18f) steer = pad.axisX;
    if (pad.accel > 0.2f) gas = std::max(gas, pad.accel);
    if (!bot_) return;

    const float aim = std::atan2(-x_, 14.f);
    steer = clampf(wrapPi(aim - heading_) * 2.5f, -1.f, 1.f);
    const bool gateClear = phase_ == Phase::Leave || (phase_ == Phase::Open && hi_ > 0.98f);
    if (gateClear) {
        gas = 0.92f;
        return;
    }
    if (phase_ == Phase::Approach) {
        const float dist = kHoldY - y_;
        if (dist > 10.f) gas = std::fabs(x_) < 4.f ? 0.72f : 0.28f;
        else gas = clampf(dist * 0.12f - speed_ * 0.45f, -1.f, 0.45f);
        return;
    }
    gas = clampf(-speed_ * 0.9f, -1.f, 0.f);
}

void Game::step(float gas, float steer) {
    time_ += kDt;
    speed_ += gas * 14.f * kDt;
    speed_ -= speed_ * 1.35f * kDt;
    speed_ = clampf(speed_, -4.f, 9.5f);
    const float turn = (0.4f + std::fabs(speed_) * 0.22f);
    heading_ = wrapPi(heading_ + steer * turn * kDt);
    x_ += std::sin(heading_) * speed_ * kDt;
    y_ += std::cos(heading_) * speed_ * kDt;

    if (phase_ == Phase::Shut) {
        lo_ = std::max(0.f, lo_ - kDt * 0.7f);
        if (lo_ <= 0.f) {
            phase_ = Phase::Hold;
            hold_ = 0.f;
            blip(196.f);
        }
    } else if (phase_ == Phase::Hold) {
        hold_ += kDt;
        if (hold_ >= 1.15f) {
            phase_ = Phase::Open;
            blip(262.f);
        }
    } else if (phase_ == Phase::Open) {
        hi_ = std::min(1.f, hi_ + kDt * 0.55f);
        if (hi_ >= 1.f) {
            phase_ = Phase::Leave;
            blip(330.f);
        }
    }

    if (phase_ == Phase::Approach) {
        const bool in = sternY() > kLo + kGateT + 3.f && bowY() < kHi - 6.f;
        if (in && std::fabs(speed_) < 2.4f && std::fabs(x_) < 5.f && std::fabs(heading_) < 0.45f) {
            phase_ = Phase::Shut;
            blip(160.f);
        }
    }

    const float c = std::cos(heading_);
    const float s = std::sin(heading_);
    float xs[4], ys[4];
    const float lx[4] = {-kHalfW, kHalfW, kHalfW, -kHalfW};
    const float ly[4] = {-kHalfL, -kHalfL, kHalfL, kHalfL};
    for (int i = 0; i < 4; i++) {
        xs[i] = x_ + lx[i] * c + ly[i] * s;
        ys[i] = y_ - lx[i] * s + ly[i] * c;
    }
    float x0 = xs[0], x1 = xs[0], y0 = ys[0], y1 = ys[0];
    for (int i = 1; i < 4; i++) {
        x0 = std::min(x0, xs[i]);
        x1 = std::max(x1, xs[i]);
        y0 = std::min(y0, ys[i]);
        y1 = std::max(y1, ys[i]);
    }

    auto gateHit = [&](float gy, float open) {
        if (!overlap(y0, y1, gy, gy + kGateT)) return false;
        const float gap = open * kGap;
        const bool left = x0 < -gap;
        const bool right = x1 > gap;
        return left || right;
    };
    if (gateHit(kLo, lo_)) {
        fail("scraped the lower gate");
        return;
    }
    if (gateHit(kHi, hi_)) {
        fail("scraped the upper gate");
        return;
    }
    if (time_ > kLimit) {
        fail("missed the end");
        return;
    }
    if (y_ < -30.f || std::fabs(x_) > kLane + 10.f) {
        fail("missed the end");
        return;
    }
    if (sternY() > kEnd) {
        if (phase_ == Phase::Leave) finish();
        else fail("missed the end");
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float gas = 0, steer = 0;
            pilot(gas, steer);
            step(gas, steer);
        }
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            begin();
            mode_ = Mode::Title;
            over_ = false;
            zoom_ = 1.35f;
            camY_ = 86.f;
        }
    }

    float wantY = y_ + 10.f;
    if (mode_ == Mode::Title) wantY = 86.f;
    camY_ += (wantY - camY_) * (1.f - std::exp(-kDt * 3.f));
    camX_ += (x_ * 0.25f - camX_) * (1.f - std::exp(-kDt * 2.f));
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.5f);
    if (tone_ > 0.f) {
        tone_ -= kDt;
        if (tone_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        int stepN = int(chimeT_ / 0.16f);
        if (stepN != chime_ && stepN < 4) {
            chime_ = stepN;
            sys.apu.tone(2, notes[stepN], 0.15f);
        }
        if (stepN >= 6) {
            sys.apu.tone(2, 0, 0);
            chime_ = -1;
        }
    }
    if (mode_ == Mode::Run && std::fabs(speed_) > 0.4f) sys.apu.tone(1, 70.f + std::fabs(speed_) * 6.f, 0.04f);
    draw();
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    float jx = shake_ > 0.f ? std::sin(time_ * 80.f) * shake_ * 3.f : 0.f;
    sx = (wx - camX_) * zoom_ + 160.f + jx;
    sy = 118.f - (wy - camY_) * zoom_;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * zoom_, pal);
}

void Game::quad(const gs::Mipped& m, float x0, float y0, float x1, float y1, int pal) {
    if (m.h < 1) return;
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);
    float sx0, sy0, sx1, sy1;
    worldToScreen(x0, y1, sx0, sy0);
    worldToScreen(x1, y0, sx1, sy1);
    if (sx1 < -40.f || sx0 > gs::SCREEN_W + 40.f || sy1 < -40.f || sy0 > gs::SCREEN_H + 40.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(sx1 - sx0), 1L, 500L));
    s.h = int16_t(std::clamp(std::lround(sy1 - sy0), 1L, 500L));
    s.x = int16_t(std::lround(sx0));
    s.y = int16_t(std::lround(sy0));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
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
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(7, 9, 12);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
        vdp.B.hscroll[y] = int16_t(camX_ * 0.4f);
        vdp.B.vscroll[y] = int16_t(-camY_ * 0.35f);
    }

    int frame = int(std::lround(heading_ / (kPi / 4.f)));
    frame = (frame % 8 + 8) % 8;
    place(art_.plow[frame], x_, y_, 13.f, PAL_PLOW);

    for (float yy = -20.f; yy < 200.f; yy += 18.f) {
        quad(art_.bank, -kLane - 14.f, yy, -kLane, yy + 16.f, PAL_BANK);
        quad(art_.bank, kLane, yy, kLane + 14.f, yy + 16.f, PAL_BANK);
    }

    auto leaf = [&](float gy, float open, float sign) {
        float gap = open * kGap;
        float inner = sign < 0.f ? -kLane - 1.f : gap;
        float outer = sign < 0.f ? -gap : kLane + 1.f;
        if (outer - inner < 1.2f) return;
        quad(art_.gate, inner, gy, outer, gy + kGateT, PAL_GATE);
    };
    leaf(kLo, lo_, -1.f);
    leaf(kLo, lo_, 1.f);
    leaf(kHi, hi_, -1.f);
    leaf(kHi, hi_, 1.f);

    place(art_.post, -kLane + 1.f, kLo - 2.f, 10.f, PAL_POST);
    place(art_.post, kLane - 1.f, kHi + kGateT + 2.f, 10.f, PAL_POST);
    place(art_.banner, 0.f, kEnd + 4.f, 8.f, PAL_END);
    quad(art_.gate, -kLane, kEnd, kLane, kEnd + 1.6f, PAL_END);

    if (mode_ == Mode::Title) {
        hudC(4, "S3 PLOW LOCK", PAL_HUD);
        hudC(16, "PASS THE LOCK", PAL_HUD);
        hudC(18, "DO NOT SCRAPE A GATE", PAL_HUD);
        hudC(20, "MISSING THE END FAILS THE LEG", PAL_HUD);
        hudC(23, "START", 2);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", 2);
    } else if (mode_ == Mode::Win) {
        hudC(22, "CLEAR OF BOTH GATES", 5);
        hudC(24, "THE LEG IS IN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(22, why_, 4);
        hudC(24, "THE LEG FAILS", 4);
    } else {
        const char* hint = "THREAD THE LOWER GATE";
        if (phase_ == Phase::Shut) hint = "LOWER GATE IS CLOSING";
        else if (phase_ == Phase::Hold) hint = "HOLD IN THE LOCK";
        else if (phase_ == Phase::Open) hint = "UPPER GATE IS OPENING";
        else if (phase_ == Phase::Leave) hint = "MAKE THE END OF THE LEG";
        else if (sternY() > kLo) hint = "STOP IN THE CHAMBER";
        hudC(25, hint, PAL_HUD);
        hud(1, 26, "ARROWS STEER AND DRIVE", PAL_HUD);
    }

    hud(1, 0, "S3 PLOW LOCK", 2);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", int(time_));
    hud(36, 0, buf, PAL_HUD);
    std::snprintf(buf, sizeof(buf), "%s", S3_VERSION_STRING);
    hud(39 - int(std::strlen(buf)), 27, buf, PAL_HUD);
}

}  // namespace plowlock
