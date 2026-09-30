#include "game/heli.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace helilock {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHx = 16.f;
constexpr float kHy = 9.f;
constexpr float kCruise = 58.f;
constexpr float kPadY = 44.f;
constexpr float kLo0 = 340.f;
constexpr float kLo1 = 364.f;
constexpr float kHi0 = 700.f;
constexpr float kHi1 = 724.f;
constexpr float kHoldX = 520.f;
constexpr float kEndL = 908.f;
constexpr float kEndR = 984.f;
constexpr float kMiss = 1036.f;
constexpr float kLimit = 48.f;
constexpr float kSlot = 46.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
bool overlap(float a0, float a1, float b0, float b1) { return a1 > b0 && a0 < b1; }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.reset();
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.clear();
    sys.vdp.setFogColor(gs::rgb4(6, 9, 12));
    sys.apu.setMaster(0.42f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
}

void Game::begin() {
    phase_ = Phase::Approach;
    x_ = 96.f;
    y_ = kCruise;
    vx_ = vy_ = 0;
    lo_ = 1.f;
    hi_ = 0.f;
    fill_ = 0.f;
    hold_ = 0;
    time_ = 0;
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

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    over_ = true;
    won_ = false;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    vx_ *= 0.15f;
    vy_ *= 0.15f;
    sys_->apu.noiseBurst(0.35f, 800.f, 0.22f);
    sys_->apu.tone(1, 80.f, 0.22f);
    beep_ = 0.28f;
}

void Game::finish() {
    if (mode_ != Mode::Run) return;
    over_ = true;
    won_ = true;
    mode_ = Mode::Win;
    why_ = "passed the lock";
    chime_ = 0;
    chimeT_ = 0;
}

void Game::pilot(float& thrust, float& climb) {
    const gs::Pad& pad = sys_->pad;
    thrust = 0;
    climb = 0;
    if (pad.down(gs::BTN_RIGHT)) thrust += 1.f;
    if (pad.down(gs::BTN_LEFT)) thrust -= 1.f;
    if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C)) climb += 1.f;
    if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B)) climb -= 1.f;
    if (std::fabs(pad.axisX) > 0.18f) thrust = pad.axisX;
    if (std::fabs(pad.axisY) > 0.18f) climb = pad.axisY;
    if (!bot_) return;

    float tx = kHoldX;
    float ty = kCruise;
    if (phase_ == Phase::Leave) {
        if (x_ < kHi1 + 70.f) {
            tx = kHi1 + 160.f;
            ty = kCruise;
        } else {
            tx = (kEndL + kEndR) * 0.5f;
            ty = kPadY;
        }
    }
    thrust = clampf((tx - x_) * 0.022f - vx_ * 0.11f, -1.f, 1.f);
    climb = clampf((ty - y_) * 0.09f - vy_ * 0.16f, -1.f, 1.f);
}

void Game::step(float thrust, float climb) {
    time_ += kDt;
    vx_ += thrust * 46.f * kDt;
    vy_ += climb * 62.f * kDt;
    vx_ *= std::exp(-kDt * 0.85f);
    vy_ *= std::exp(-kDt * 1.7f);
    vx_ = clampf(vx_, -42.f, 56.f);
    vy_ = clampf(vy_, -40.f, 40.f);
    x_ += vx_ * kDt;
    y_ += vy_ * kDt;

    if (phase_ == Phase::Shut) {
        lo_ = std::max(0.f, lo_ - kDt * 0.5f);
        if (lo_ <= 0.f) phase_ = Phase::Fill;
    } else if (phase_ == Phase::Fill) {
        fill_ = std::min(1.f, fill_ + kDt / 2.2f);
        if (fill_ >= 1.f) {
            phase_ = Phase::Open;
            blip(360.f);
        }
    } else if (phase_ == Phase::Open) {
        hi_ = std::min(1.f, hi_ + kDt * 0.48f);
        if (hi_ >= 1.f) phase_ = Phase::Leave;
    }

    if (phase_ == Phase::Approach) {
        bool in = x_ > kLo1 + kHx + 24.f && x_ < kHi0 - kHx - 30.f;
        if (in && std::fabs(vx_) < 8.f && std::fabs(y_ - kCruise) < 16.f) {
            phase_ = Phase::Shut;
            blip(180.f);
        }
    }

    auto gateHit = [&](float gx0, float gx1, float open) {
        if (!overlap(x_ - kHx, x_ + kHx, gx0, gx1)) return false;
        float half = open * kSlot;
        float top = y_ + kHy;
        float bot = y_ - kHy;
        return bot < kCruise - half || top > kCruise + half;
    };
    if (gateHit(kLo0, kLo1, lo_)) {
        fail("scraped the lower gate");
        return;
    }
    if (gateHit(kHi0, kHi1, hi_)) {
        fail("scraped the upper gate");
        return;
    }

    bool inChamber = x_ > kLo1 && x_ < kHi0;
    if (inChamber && y_ + kHy > kCruise + kSlot + 18.f) {
        fail("scraped the lock wall");
        return;
    }
    if (y_ - kHy < 6.f) {
        fail("ditched in the lock");
        return;
    }
    if (x_ > kMiss) {
        fail("missed the end");
        return;
    }
    if (time_ > kLimit) {
        fail("missed the end");
        return;
    }

    bool onEnd = phase_ == Phase::Leave && x_ > kEndL + 8.f && x_ < kEndR - 8.f &&
                 std::fabs(y_ - kPadY) < 10.f && std::fabs(vx_) < 10.f && std::fabs(vy_) < 10.f;
    if (onEnd) hold_ += kDt;
    else hold_ = 0;
    if (hold_ > 0.35f) finish();
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
            float thrust = 0, climb = 0;
            pilot(thrust, climb);
            step(thrust, climb);
        }
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            mode_ = Mode::Title;
            over_ = false;
        }
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
    rotor_ = int((time_ + 1.f) * 22.f) % 3;
    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        int stepN = int(chimeT_ / 0.16f);
        if (stepN != chime_ && stepN < 4) {
            chime_ = stepN;
            sys.apu.tone(2, notes[stepN], 0.16f);
        }
        if (stepN >= 6) {
            sys.apu.tone(2, 0, 0);
            chime_ = -1;
        }
    }
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::quad(float x0, float y0, float x1, float y1, const gs::Mipped& m, int pal) {
    if (m.h < 1) return;
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);
    float jx = (shake_ > 0.f) ? std::sin(time_ * 80.f) * shake_ * 3.f : 0.f;
    auto sx = [&](float x) { return (x - cam_) + 110.f + jx; };
    auto sy = [&](float alt) { return 176.f - alt * 1.35f; };
    float px0 = sx(x0), px1 = sx(x1), py0 = sy(y1), py1 = sy(y0);
    if (px1 < -30.f || px0 > gs::SCREEN_W + 30.f || py1 < -30.f || py0 > gs::SCREEN_H + 30.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(px1 - px0), 1L, 500L));
    s.h = int16_t(std::clamp(std::lround(py1 - py0), 1L, 500L));
    s.x = int16_t(std::lround(px0));
    s.y = int16_t(std::lround(py0));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    for (const char* p = s; *p; ++p) {
        char ch = *p;
        if (ch < 32 || ch > 126) {
            x += 6.f * scale;
            continue;
        }
        const gs::Mipped& m = art_.glyph[ch - 32];
        float h = std::max(7.f * scale, 1.f);
        spr(m, x + h * 0.45f, y, h, pal);
        x += (ch == ' ' ? 4.6f : 6.0f) * scale;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    float waterAlt = 10.f + fill_ * 16.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = int(4 + 6 * (1.f - u));
        int g = int(7 + 5 * (1.f - u));
        int b = int(12 + 3 * (1.f - u));
        float waterLine = 176.f - waterAlt * 1.35f;
        if (y > waterLine) {
            r = 2;
            g = 5 + int(3 * fill_);
            b = 10;
        }
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = uint8_t(y > 190 ? 2 : 0);
        vdp.road[y].on = false;
    }

    float base = cam_ - 140.f;
    for (int i = 0; i < 8; i++) {
        float wx = base + i * 70.f;
        quad(wx, 0.f, wx + 68.f, waterAlt + 8.f, art_.water, PAL_WORLD);
    }
    spr(art_.cloud, 70.f + std::sin(time_ * 0.2f) * 8.f, 28.f, 18.f, PAL_FX);
    spr(art_.cloud, 210.f, 42.f, 14.f, PAL_FX);

    quad(kLo1, kCruise + kSlot + 8.f, kHi0, kCruise + kSlot + 28.f, art_.wall, PAL_GATE);

    auto leaf = [&](float gx0, float gx1, float open) {
        float half = open * kSlot;
        float mid = (gx0 + gx1) * 0.5f;
        float top0 = kCruise + half;
        float bot1 = kCruise - half;
        if (140.f > top0 + 2.f) quad(gx0, top0, gx1, 140.f, art_.gate, PAL_GATE);
        if (bot1 > 0.f) quad(gx0, 0.f, gx1, bot1, art_.gate, PAL_GATE);
        (void)mid;
    };
    leaf(kLo0, kLo1, lo_);
    leaf(kHi0, kHi1, hi_);

    quad(40.f, 18.f, 160.f, 30.f, art_.pad, PAL_WORLD);
    quad(kEndL, 22.f, kEndR, 36.f, art_.pad, PAL_WORLD);

    float jx = (shake_ > 0.f) ? std::sin(time_ * 80.f) * shake_ * 3.f : 0.f;
    float sx = (x_ - cam_) + 110.f + jx;
    float sy = 176.f - y_ * 1.35f;
    bool faceL = vx_ < -6.f;
    spr(art_.body, sx, sy, 36.f, PAL_SHIP, faceL);
    spr(art_.rotor[rotor_], sx + (faceL ? -4.f : 4.f), sy - 16.f, 12.f, PAL_SHIP, faceL);

    if (mode_ == Mode::Title) {
        text("S3 HELILOCK", 78, 64, 2.1f, PAL_HUD);
        text("PASS THE LOCK", 86, 90, 1.3f, PAL_HUD);
        text("DO NOT SCRAPE A GATE", 58, 108, 1.15f, PAL_HUD);
        text("ARROWS FLY   ENTER START", 46, 130, 1.05f, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 120, 90, 2.f, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        text("LOCK CLEAR", 96, 36, 1.8f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(why_, 48, 32, 1.2f, PAL_HUD);
    } else {
        const char* tag = "APPROACH";
        if (phase_ == Phase::Shut) tag = "LOWER GATE";
        else if (phase_ == Phase::Fill) tag = "HOLD";
        else if (phase_ == Phase::Open) tag = "UPPER GATE";
        else if (phase_ == Phase::Leave) tag = "MAKE THE END";
        text(tag, 8, 14, 1.05f, PAL_HUD);
    }
}

}  // namespace helilock
