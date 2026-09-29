#include "lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ricklock {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kLen = 52.f;
constexpr float kWid = 16.f;
constexpr float kLo = 300.f;
constexpr float kHi = 620.f;
constexpr float kGateT = 16.f;
constexpr float kWall = 30.f;
constexpr float kHold = 455.f;
constexpr float kLimit = 48.f;
constexpr float kScale = 1.32f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (phase_ == Phase::Leave && x_ - kLen * 0.5f > kHi + 6.f) return 3;
    if (phase_ == Phase::Approach) return 1;
    return 2;
}

bool Game::bodyInside() const {
    const float c = std::cos(psi_), s = std::sin(psi_);
    const float nose = kLo + kGateT + 8.f;
    const float tail = kHi - kGateT - 8.f;
    for (int i = 0; i < 4; i++) {
        float ox = (i & 1) ? kLen * 0.5f : -kLen * 0.5f;
        float oy = (i & 2) ? kWid * 0.5f : -kWid * 0.5f;
        float px = x_ + ox * c - oy * s;
        if (px < nose || px > tail) return false;
    }
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.reset();
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.clear();
    sys.apu.setMaster(0.45f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
}

void Game::begin() {
    phase_ = Phase::Approach;
    x_ = 140.f;
    y_ = 0.f;
    psi_ = 0.f;
    v_ = 0.f;
    lo_ = 1.f;
    hi_ = 0.f;
    fill_ = 0.f;
    time_ = 0;
    cam_ = x_;
    shake_ = 0;
    spin_ = 0;
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

void Game::scrape(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    over_ = true;
    won_ = false;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    v_ *= 0.15f;
    sys_->apu.noiseBurst(0.42f, 800.f, 0.28f);
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

void Game::pilot(float& pedal, float& brake, float& steer) {
    const gs::Pad& pad = sys_->pad;
    pedal = brake = steer = 0;
    if (pad.down(gs::BTN_UP) || pad.accel > 0.2f || pad.down(gs::BTN_A)) pedal = 1.f;
    if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) brake = 1.f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (std::fabs(pad.axisX) > 0.2f) steer = pad.axisX;
    if (!bot_) return;

    steer = clampf(-y_ * 0.09f - psi_ * 2.4f, -1.f, 1.f);
    bool go = phase_ == Phase::Leave || (phase_ == Phase::Open && hi_ > 0.97f);
    if (go) {
        pedal = 1.f;
        brake = 0.f;
        return;
    }
    if (phase_ == Phase::Approach) {
        float err = kHold - x_;
        float want = clampf(err * 0.32f, 0.f, 20.f);
        if (v_ < want - 0.4f) pedal = 1.f;
        else if (v_ > want + 0.6f) brake = 1.f;
        return;
    }
    if (v_ > 1.2f) brake = 1.f;
    else if (v_ < -0.4f) pedal = 0.4f;
}

void Game::step(float pedal, float brake, float steer) {
    time_ += kDt;
    float turn = 1.05f + std::min(std::fabs(v_), 18.f) * 0.07f;
    psi_ += steer * turn * kDt;
    psi_ = clampf(psi_, -1.2f, 1.2f);
    v_ += pedal * 26.f * kDt;
    v_ -= brake * 42.f * kDt;
    v_ *= std::exp(-kDt * 0.7f);
    v_ = clampf(v_, -10.f, 36.f);
    x_ += std::cos(psi_) * v_ * kDt;
    y_ += std::sin(psi_) * v_ * kDt;
    spin_ += v_ * kDt;

    if (phase_ == Phase::Shut) {
        lo_ = std::max(0.f, lo_ - kDt * 0.48f);
        if (lo_ <= 0.f) phase_ = Phase::Rise;
    } else if (phase_ == Phase::Rise) {
        fill_ = std::min(1.f, fill_ + kDt / 2.4f);
        if (fill_ >= 1.f) {
            phase_ = Phase::Open;
            blip(360.f);
        }
    } else if (phase_ == Phase::Open) {
        hi_ = std::min(1.f, hi_ + kDt * 0.46f);
        if (hi_ >= 1.f) phase_ = Phase::Leave;
    }

    if (phase_ == Phase::Approach && bodyInside() && std::fabs(v_) < 6.f && std::fabs(y_) < 8.f &&
        std::fabs(psi_) < 0.35f) {
        phase_ = Phase::Shut;
        blip(190.f);
    }

    const float c = std::cos(psi_), s = std::sin(psi_);
    auto gateHit = [&](float gx0, float gx1, float open) {
        float gap = open * (kWall + 8.f);
        for (int ix = 0; ix < 3; ix++) {
            for (int iy = 0; iy < 3; iy++) {
                float ox = (ix - 1) * kLen * 0.5f;
                float oy = (iy - 1) * kWid * 0.5f;
                float px = x_ + ox * c - oy * s;
                float py = y_ + ox * s + oy * c;
                if (px < gx0 || px > gx1) continue;
                if (py > -gap && py < gap) continue;
                if (py < -kWall - 2.f || py > kWall + 2.f) continue;
                return true;
            }
        }
        return false;
    };
    if (gateHit(kLo, kLo + kGateT, lo_)) {
        scrape("scraped the lower gate");
        return;
    }
    if (gateHit(kHi - kGateT, kHi, hi_)) {
        scrape("scraped the upper gate");
        return;
    }
    if (std::fabs(y_) + kWid * 0.5f > kWall + 1.f) {
        scrape("scraped the lock wall");
        return;
    }
    if (time_ > kLimit) {
        scrape("took too long");
        return;
    }
    if (phase_ == Phase::Leave && x_ - kLen * 0.5f > kHi + 28.f) finish();
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
            float pedal = 0, brake = 0, steer = 0;
            pilot(pedal, brake, steer);
            step(pedal, brake, steer);
        }
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            mode_ = Mode::Title;
            over_ = false;
        }
    }

    float want = x_ - 16.f;
    cam_ += (want - cam_) * (1.f - std::exp(-kDt * 3.4f));
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.6f);
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    if (mode_ == Mode::Run && std::fabs(v_) > 4.f) {
        float tick = std::fmod(spin_, 7.f);
        if (tick < std::fabs(v_) * kDt) sys.apu.tone(3, 140.f + std::fabs(v_) * 2.f, 0.04f);
    }
    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        int step = int(chimeT_ / 0.16f);
        if (step != chime_ && step < 4) {
            chime_ = step;
            sys.apu.tone(2, notes[step], 0.16f);
        }
        if (step >= 6) {
            sys.apu.tone(2, 0, 0);
            chime_ = -1;
        }
    }
    draw();
}

void Game::blit(const gs::Mipped& m, float wx, float wy, float h, int pal) {
    if (m.h < 1 || h < 1.f) return;
    float jx = (shake_ > 0.f) ? std::sin(time_ * 90.f) * shake_ * 3.f : 0.f;
    float px = (wx - cam_) * kScale + 168.f + jx;
    float py = 112.f + wy * kScale;
    float ph = h * kScale;
    float pw = ph * float(m.w) / float(m.h);
    if (px + pw * 0.5f < -8.f || px - pw * 0.5f > gs::SCREEN_W + 8.f) return;
    if (py + ph * 0.5f < -8.f || py - ph * 0.5f > gs::SCREEN_H + 8.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(pw), 1L, 400L));
    s.h = int16_t(std::clamp(std::lround(ph), 1L, 400L));
    s.x = int16_t(std::lround(px - s.w * 0.5f));
    s.y = int16_t(std::lround(py - s.h * 0.5f));
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
    int water = 4 + int(fill_ * 4.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int deep = (y < 78 || y > 150) ? water : 2;
        vdp.lineBackdrop[y] = gs::rgb4(1, deep, deep + 4);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    if (mode_ == Mode::Title) blit(art_.title, cam_ + 8.f, -18.f, 22.f, PAL_GOLD);
    else if (mode_ == Mode::Win) blit(art_.clear, x_, y_ - 28.f, 20.f, PAL_WIN);
    else if (mode_ == Mode::Fail) blit(art_.fail, x_, y_ - 28.f, 18.f, PAL_ALERT);

    const float c = std::cos(psi_), s = std::sin(psi_);
    auto part = [&](const gs::Mipped& m, float ox, float oy, float h, int pal) {
        blit(m, x_ + ox * c - oy * s, y_ + ox * s + oy * c, h, pal);
    };
    part(art_.hood, -6.f, 0.f, 26.f, PAL_CAB);
    part(art_.seat, -8.f, 0.f, 14.f, PAL_CAB);
    part(art_.body, 2.f, 0.f, 16.f, PAL_CAB);
    part(art_.driver, 12.f, 0.f, 12.f, PAL_CAB);
    part(art_.wheel, 18.f, 0.f, 12.f, PAL_WHEEL);
    part(art_.wheel, -14.f, 9.f, 13.f, PAL_WHEEL);
    part(art_.wheel, -14.f, -9.f, 13.f, PAL_WHEEL);

    auto leaf = [&](float gx, float open, float sign) {
        float gap = open * (kWall + 10.f);
        float cy = sign < 0 ? (-kWall - gap) * 0.5f : (kWall + gap) * 0.5f;
        float hh = std::max(4.f, (kWall - gap) * 0.5f + 2.f);
        blit(art_.gate, gx + kGateT * 0.5f, cy, hh * 2.f, PAL_GATE);
    };
    leaf(kLo, lo_, -1.f);
    leaf(kLo, lo_, 1.f);
    leaf(kHi - kGateT, hi_, -1.f);
    leaf(kHi - kGateT, hi_, 1.f);

    float beamY = -kWall - 8.f - (1.f - fill_) * 10.f;
    blit(art_.beam, (kLo + kHi) * 0.5f, beamY, 8.f, PAL_GATE);

    for (float wx = std::floor((cam_ - 170.f) / 28.f) * 28.f; wx < cam_ + 200.f; wx += 28.f) {
        blit(art_.plank, wx + 14.f, 0.f, 18.f, PAL_WOOD);
        blit(art_.stone, wx + 14.f, -kWall - 12.f, 18.f, PAL_STONE);
        blit(art_.stone, wx + 14.f, kWall + 12.f, 18.f, PAL_STONE);
        blit(art_.grass, wx + 14.f, -kWall - 28.f, 14.f, PAL_BANK);
        blit(art_.grass, wx + 14.f, kWall + 28.f, 14.f, PAL_BANK);
    }
    blit(art_.house, kLo - 60.f, -kWall - 36.f, 32.f, PAL_HOUSE);
    blit(art_.house, kHi + 50.f, kWall + 34.f, 28.f, PAL_HOUSE);
    blit(art_.lamp, kLo + 6.f, -kWall + 2.f, 20.f, PAL_LAMP);
    blit(art_.lamp, kHi - 6.f, kWall - 2.f, 20.f, PAL_LAMP);

    if (mode_ == Mode::Title) {
        hudC(16, "PASS THE LOCK", PAL_HUD);
        hudC(18, "DO NOT SCRAPE A GATE", PAL_HUD);
        hudC(22, "START  OR  A", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        hudC(24, "PASSED WITHOUT A SCRAPE", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        hudC(24, why_, PAL_ALERT);
    } else {
        const char* hint = "PEDAL INTO THE LOCK";
        if (phase_ == Phase::Shut) hint = "LOWER GATE IS CLOSING";
        else if (phase_ == Phase::Rise) hint = "HOLD WHILE THE LOCK RISES";
        else if (phase_ == Phase::Open) hint = "UPPER GATE IS OPENING";
        else if (phase_ == Phase::Leave) hint = "LEAVE WITHOUT A SCRAPE";
        else if (x_ > kLo) hint = "STOP BETWEEN THE GATES";
        hudC(25, hint, PAL_HUD);
        hud(1, 26, "ARROWS PEDAL AND STEER", PAL_HUD);
    }
    hud(1, 0, "S3 RICKSHAW LOCK", PAL_GOLD);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", int(time_));
    hud(36, 0, buf, PAL_HUD);
    const char* tag = "LOW";
    if (phase_ == Phase::Rise) tag = "RISE";
    else if (phase_ == Phase::Open || phase_ == Phase::Leave || fill_ >= 1.f) tag = "HIGH";
    hud(17, 0, tag, phase_ == Phase::Rise ? PAL_WIN : PAL_HUD);
}

}  // namespace ricklock
