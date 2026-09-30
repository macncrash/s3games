#include "lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace metrolock {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kLen = 168.f;
constexpr float kBeam = 30.f;
constexpr float kLo = 390.f;
constexpr float kHi = 800.f;
constexpr float kGateT = 18.f;
constexpr float kWall = 50.f;
constexpr float kHoldX = 575.f;
constexpr float kLimit = 46.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
bool overlap(float a0, float a1, float b0, float b1) { return a1 > b0 && a0 < b1; }

}  // namespace

float Game::bow() const { return x_ + kLen * 0.5f; }
float Game::stern() const { return x_ - kLen * 0.5f; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (phase_ == Phase::Leave && stern() > kHi + 6.f) return 3;
    if (phase_ == Phase::Approach) return 1;
    return 2;
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
    x_ = 150.f;
    y_ = 0;
    cam_ = x_;
}

void Game::begin() {
    phase_ = Phase::Approach;
    x_ = 150.f;
    y_ = 4.f;
    vx_ = vy_ = 0;
    lo_ = 1.f;
    hi_ = 0.f;
    fill_ = 0.f;
    time_ = 0;
    cam_ = x_;
    shake_ = 0;
    why_ = "";
    over_ = false;
    won_ = false;
    chime_ = -1;
    mode_ = Mode::Run;
    blip(246.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.11f);
    beep_ = 0.07f;
}

void Game::scrape(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    over_ = true;
    won_ = false;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    vx_ *= 0.15f;
    sys_->apu.noiseBurst(0.38f, 780.f, 0.22f);
    sys_->apu.tone(1, 78.f, 0.18f);
    beep_ = 0.28f;
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
    if (pad.down(gs::BTN_UP) || pad.accel > 0.2f) thrust += 1.f;
    if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) thrust -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (std::fabs(pad.axisX) > 0.2f) steer = pad.axisX;
    if (!bot_) return;
    steer = clampf(-y_ * 0.11f - vy_ * 0.14f, -1.f, 1.f);
    bool go = phase_ == Phase::Leave || (phase_ == Phase::Open && hi_ > 0.97f);
    if (go) {
        thrust = 0.92f;
    } else if (phase_ == Phase::Approach) {
        float err = kHoldX - x_;
        if (x_ < kHoldX - 70.f) thrust = 0.82f;
        else thrust = clampf(err * 0.028f - vx_ * 0.16f, -1.f, 1.f);
    } else {
        thrust = clampf(-vx_ * 0.24f, -1.f, 1.f);
    }
}

void Game::step(float thrust, float steer) {
    time_ += kDt;
    vy_ += steer * 42.f * kDt;
    vy_ *= std::exp(-kDt * 2.6f);
    y_ += vy_ * kDt;
    vx_ += thrust * 34.f * kDt;
    vx_ *= std::exp(-kDt * 0.7f);
    vx_ = clampf(vx_, -32.f, 50.f);
    x_ += vx_ * kDt;

    if (phase_ == Phase::Shut) {
        lo_ = std::max(0.f, lo_ - kDt * 0.48f);
        if (lo_ <= 0.f) phase_ = Phase::Fill;
    } else if (phase_ == Phase::Fill) {
        fill_ = std::min(1.f, fill_ + kDt / 2.4f);
        if (fill_ >= 1.f) {
            phase_ = Phase::Open;
            blip(360.f);
        }
    } else if (phase_ == Phase::Open) {
        hi_ = std::min(1.f, hi_ + kDt * 0.46f);
        if (hi_ >= 1.f) phase_ = Phase::Leave;
    }

    if (phase_ == Phase::Approach) {
        bool in = stern() > kLo + kGateT + 10.f && bow() < kHi - kGateT - 20.f;
        if (in && std::fabs(vx_) < 5.5f && std::fabs(y_) < 12.f) {
            phase_ = Phase::Shut;
            blip(168.f);
        }
    }

    float x0 = stern(), x1 = bow();
    float y0 = y_ - kBeam * 0.5f, y1 = y_ + kBeam * 0.5f;
    auto gateHit = [&](float gx0, float gx1, float open) {
        if (!overlap(x0, x1, gx0, gx1)) return false;
        float gap = open * (kWall + 14.f);
        return y0 < -gap || y1 > gap;
    };
    if (gateHit(kLo, kLo + kGateT, lo_)) {
        scrape("scraped the lower gate");
        return;
    }
    if (gateHit(kHi - kGateT, kHi, hi_)) {
        scrape("scraped the upper gate");
        return;
    }
    if (overlap(x0, x1, kLo, kHi) && (y0 < -kWall || y1 > kWall)) {
        scrape("scraped the chamber wall");
        return;
    }
    if (time_ > kLimit) {
        scrape("took too long");
        return;
    }
    if (phase_ == Phase::Leave && stern() > kHi + 28.f) finish();
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
            float thrust = 0, steer = 0;
            pilot(thrust, steer);
            step(thrust, steer);
        }
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            mode_ = Mode::Title;
            over_ = false;
        }
    }

    float want = x_ - 16.f;
    cam_ += (want - cam_) * (1.f - std::exp(-kDt * 3.4f));
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.7f);
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {330.f, 415.f, 494.f, 659.f};
        int stepN = int(chimeT_ / 0.15f);
        if (stepN != chime_ && stepN < 4) {
            chime_ = stepN;
            sys.apu.tone(2, notes[stepN], 0.15f);
        }
        if (stepN >= 6) {
            sys.apu.tone(2, 0, 0);
            chime_ = -1;
        }
    }
    draw();
}

void Game::quad(const gs::Mipped& m, float x0, float y0, float x1, float y1, int pal) {
    if (m.h < 1) return;
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);
    float jx = (shake_ > 0.f) ? std::sin(time_ * 88.f) * shake_ * 3.f : 0.f;
    auto sx = [&](float x) { return (x - cam_) + 148.f + jx; };
    float lift = fill_ * 6.f;
    auto sy = [&](float y) { return 116.f - lift + y * 1.55f; };
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
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = 2 + (y > 48 && y < 176 ? 1 : 0);
        if (y > 90 && y < 140) g += int(fill_ * 2.f);
        vdp.lineBackdrop[y] = gs::rgb4(1, g, g + 2);
        vdp.lineFog[y] = uint8_t(y < 28 || y > 196 ? 4 : 0);
        vdp.road[y].on = false;
    }

    float bank = kWall + 10.f;
    for (float x = std::floor((cam_ - 200.f) / 36.f) * 36.f; x < cam_ + 240.f; x += 36.f) {
        bool chamber = x + 18.f > kLo && x < kHi;
        float top = chamber ? -bank : -(kWall + 62.f);
        float bot = chamber ? bank : (kWall + 62.f);
        quad(art_.concrete, x, top - 18.f, x + 36.f, top, PAL_CONCRETE);
        quad(art_.concrete, x, bot, x + 36.f, bot + 18.f, PAL_CONCRETE);
        quad(art_.tile, x, top - 30.f, x + 32.f, top - 14.f, PAL_TILE);
        quad(art_.tile, x, bot + 14.f, x + 32.f, bot + 30.f, PAL_TILE);
        quad(art_.sleeper, x + 4.f, -7.f, x + 30.f, 7.f, PAL_RAIL);
    }
    mark(art_.lamp, kLo + 14.f, -kWall + 2.f, 20.f, PAL_LAMP);
    mark(art_.lamp, kHi - 16.f, kWall - 2.f, 20.f, PAL_LAMP);
    mark(art_.signal, kLo - 28.f, -kWall + 8.f, 24.f, PAL_SIGNAL);
    mark(art_.signal, kHi + 22.f, kWall - 8.f, 24.f, PAL_SIGNAL);
    float drip = std::sin(time_ * 5.f) * 4.f;
    mark(art_.drip, kLo + 40.f + drip, -8.f, 6.f, PAL_WATER);
    mark(art_.drip, kHi - 30.f - drip, 10.f, 5.f, PAL_WATER);

    auto gateLeaf = [&](float gx, float open, float sign) {
        float gap = open * (kWall + 14.f);
        if (sign < 0) quad(art_.gate, gx, -kWall - 6.f, gx + kGateT, -gap, PAL_GATE);
        else quad(art_.gate, gx, gap, gx + kGateT, kWall + 6.f, PAL_GATE);
    };
    gateLeaf(kLo, lo_, -1.f);
    gateLeaf(kLo, lo_, 1.f);
    gateLeaf(kHi - kGateT, hi_, -1.f);
    gateLeaf(kHi - kGateT, hi_, 1.f);

    float noseH = 22.f;
    mark(art_.nose, bow() - 10.f, y_, noseH, PAL_CAR);
    float carH = 20.f;
    mark(art_.car, x_ - 18.f, y_, carH, PAL_CAR);
    mark(art_.car, stern() + 28.f, y_, carH, PAL_CAR);

    if (mode_ == Mode::Title) {
        mark(art_.title, cam_ + 8.f, -6.f, 26.f, PAL_GOLD);
        hudC(16, "PASS THE LOCK", PAL_HUD);
        hudC(18, "DO NOT SCRAPE A GATE", PAL_HUD);
        hudC(22, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        mark(art_.clear, x_, y_ - 34.f, 24.f, PAL_WIN);
        hudC(24, "CLEAR OF BOTH GATES", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        mark(art_.fail, x_, y_ - 34.f, 20.f, PAL_ALERT);
        hudC(24, why_, PAL_ALERT);
    } else {
        const char* hint = "ROLL INTO THE LOCK";
        if (phase_ == Phase::Shut) hint = "LOWER GATE IS CLOSING";
        else if (phase_ == Phase::Fill) hint = "HOLD WHILE THE LOCK FILLS";
        else if (phase_ == Phase::Open) hint = "UPPER GATE IS OPENING";
        else if (phase_ == Phase::Leave) hint = "LEAVE WITHOUT A SCRAPE";
        else if (stern() > kLo) hint = "STOP CENTRED IN THE CHAMBER";
        hudC(25, hint, PAL_HUD);
        hud(1, 26, "ARROWS  DRIVE AND STEER", PAL_HUD);
    }

    hud(1, 0, "S3 METRO LOCK", PAL_GOLD);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", int(time_));
    hud(36, 0, buf, PAL_HUD);
    const char* tag = "LOW";
    if (phase_ == Phase::Fill) tag = "RISE";
    else if (phase_ == Phase::Open || phase_ == Phase::Leave || fill_ >= 1.f) tag = "HIGH";
    hud(16, 0, tag, phase_ == Phase::Fill ? PAL_WIN : PAL_HUD);
}

}  // namespace metrolock
