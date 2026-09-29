#include "lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace mailvanlock {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kLen = 86.f;
constexpr float kBeam = 32.f;
constexpr float kLo = 340.f;
constexpr float kHi = 680.f;
constexpr float kGateT = 22.f;
constexpr float kWall = 48.f;
constexpr float kHoldX = 500.f;
constexpr float kLimit = 46.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

bool overlap(float a0, float a1, float b0, float b1) { return a1 > b0 && a0 < b1; }

}  // namespace

float Game::nose() const { return x_ + kLen * 0.5f; }
float Game::tail() const { return x_ - kLen * 0.5f; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (phase_ == Phase::Leave && tail() > kHi + 6.f) return 3;
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
    sys.apu.setMaster(0.45f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
}

void Game::begin() {
    phase_ = Phase::Approach;
    x_ = 150.f;
    y_ = 0.f;
    vx_ = vy_ = 0;
    lo_ = 1.f;
    hi_ = 0.f;
    hold_ = 0.f;
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

void Game::scrape(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    over_ = true;
    won_ = false;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    vx_ *= 0.15f;
    sys_->apu.noiseBurst(0.4f, 800.f, 0.22f);
    sys_->apu.tone(1, 80.f, 0.2f);
    beep_ = 0.3f;
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
    steer = clampf(-y_ * 0.11f - vy_ * 0.16f, -1.f, 1.f);
    bool go = phase_ == Phase::Leave || (phase_ == Phase::Open && hi_ > 0.97f);
    if (go) {
        thrust = 0.85f;
    } else if (phase_ == Phase::Approach) {
        float err = kHoldX - x_;
        if (x_ < kHoldX - 70.f) thrust = 0.75f;
        else thrust = clampf(err * 0.03f - vx_ * 0.18f, -1.f, 1.f);
    } else {
        thrust = clampf(-vx_ * 0.28f, -1.f, 1.f);
    }
}

void Game::step(float thrust, float steer) {
    time_ += kDt;
    vy_ += steer * 52.f * kDt;
    vy_ *= std::exp(-kDt * 2.8f);
    y_ += vy_ * kDt;
    vx_ += thrust * 38.f * kDt;
    vx_ *= std::exp(-kDt * 0.85f);
    vx_ = clampf(vx_, -28.f, 46.f);
    x_ += vx_ * kDt;

    if (phase_ == Phase::Shut) {
        lo_ = std::max(0.f, lo_ - kDt * 0.5f);
        if (lo_ <= 0.f) phase_ = Phase::Hold;
    } else if (phase_ == Phase::Hold) {
        hold_ = std::min(1.f, hold_ + kDt / 1.8f);
        if (hold_ >= 1.f) {
            phase_ = Phase::Open;
            blip(360.f);
        }
    } else if (phase_ == Phase::Open) {
        hi_ = std::min(1.f, hi_ + kDt * 0.48f);
        if (hi_ >= 1.f) phase_ = Phase::Leave;
    }

    if (phase_ == Phase::Approach) {
        bool in = tail() > kLo + kGateT + 10.f && nose() < kHi - kGateT - 16.f;
        if (in && std::fabs(vx_) < 5.f && std::fabs(y_) < 10.f) {
            phase_ = Phase::Shut;
            blip(160.f);
        }
    }

    float x0 = tail(), x1 = nose();
    float y0 = y_ - kBeam * 0.5f, y1 = y_ + kBeam * 0.5f;
    auto gateHit = [&](float gx0, float gx1, float open) {
        if (!overlap(x0, x1, gx0, gx1)) return false;
        float gap = open * (kWall + 8.f);
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
        scrape("scraped the lock wall");
        return;
    }
    if (time_ > kLimit) {
        scrape("took too long");
        return;
    }
    if (phase_ == Phase::Leave && tail() > kHi + 28.f) finish();
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
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.6f);
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
    draw();
}

void Game::quad(const gs::Mipped& m, float x0, float y0, float x1, float y1, int pal) {
    if (m.h < 1) return;
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);
    float jx = (shake_ > 0.f) ? std::sin(time_ * 90.f) * shake_ * 3.f : 0.f;
    auto sx = [&](float x) { return (x - cam_) + 150.f + jx; };
    auto sy = [&](float y) { return 112.f + y * 1.55f; };
    float px0 = sx(x0), px1 = sx(x1), py0 = sy(y0), py1 = sy(y1);
    if (px1 < -40.f || px0 > gs::SCREEN_W + 40.f || py1 < -40.f || py0 > gs::SCREEN_H + 40.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(px1 - px0), 1L, 400L));
    s.h = int16_t(std::clamp(std::lround(py1 - py0), 1L, 400L));
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
        int sky = 7 + (y < 40 ? 2 : 0);
        vdp.lineBackdrop[y] = gs::rgb4(sky, sky - 1, sky - 2);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    float bank = kWall + 16.f;
    for (float x = std::floor((cam_ - 200.f) / 40.f) * 40.f; x < cam_ + 240.f; x += 40.f) {
        bool chamber = x + 20.f > kLo && x < kHi;
        float top = chamber ? -bank : -(kWall + 64.f);
        float bot = chamber ? bank : (kWall + 64.f);
        quad(art_.road, x, top, x + 40.f, bot, PAL_ROAD);
        quad(art_.bank, x, top - 18.f, x + 40.f, top, PAL_BANK);
        quad(art_.bank, x, bot, x + 40.f, bot + 18.f, PAL_BANK);
    }

    mark(art_.office, kLo - 90.f, -(kWall + 52.f), 40.f, PAL_OFFICE);
    mark(art_.box, kLo - 40.f, kWall + 40.f, 22.f, PAL_POST);
    mark(art_.box, kHi + 36.f, -(kWall + 40.f), 22.f, PAL_POST);
    mark(art_.tree, kHi + 70.f, kWall + 50.f, 30.f, PAL_BANK);
    mark(art_.tree, kLo + 40.f, -(kWall + 56.f), 26.f, PAL_BANK);
    mark(art_.lamp, kLo + 10.f, -kWall + 2.f, 24.f, PAL_POST);
    mark(art_.lamp, kHi - 10.f, kWall - 2.f, 24.f, PAL_POST);
    mark(art_.sack, kHoldX - 20.f, kWall - 8.f, 12.f, PAL_MAIL);
    mark(art_.sack, kHoldX + 18.f, -(kWall - 8.f), 12.f, PAL_MAIL);

    auto leaf = [&](float gx, float open, float sign) {
        float gap = open * (kWall + 8.f);
        if (sign < 0) quad(art_.gate, gx, -kWall - 2.f, gx + kGateT, -gap, PAL_GATE);
        else quad(art_.gate, gx, gap, gx + kGateT, kWall + 2.f, PAL_GATE);
    };
    leaf(kLo, lo_, -1.f);
    leaf(kLo, lo_, 1.f);
    leaf(kHi - kGateT, hi_, -1.f);
    leaf(kHi - kGateT, hi_, 1.f);

    mark(art_.van, x_, y_, 28.f, PAL_VAN);

    if (mode_ == Mode::Title) {
        mark(art_.title, cam_ + 8.f, -6.f, 22.f, PAL_GOLD);
        hudC(16, "PASS THE LOCK", PAL_HUD);
        hudC(18, "DO NOT SCRAPE A GATE", PAL_HUD);
        hudC(22, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        mark(art_.clear, x_, y_ - 34.f, 22.f, PAL_WIN);
        hudC(24, "CLEAR OF BOTH GATES", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        mark(art_.fail, x_, y_ - 34.f, 20.f, PAL_ALERT);
        hudC(24, why_, PAL_ALERT);
    } else {
        const char* hint = "DRIVE INTO THE LOCK";
        if (phase_ == Phase::Shut) hint = "LOWER GATE IS CLOSING";
        else if (phase_ == Phase::Hold) hint = "HOLD WHILE THE LOCK TURNS";
        else if (phase_ == Phase::Open) hint = "UPPER GATE IS OPENING";
        else if (phase_ == Phase::Leave) hint = "LEAVE WITHOUT A SCRAPE";
        else if (tail() > kLo) hint = "STOP IN THE CHAMBER";
        hudC(25, hint, PAL_HUD);
        hud(1, 26, "ARROWS  AHEAD AND ACROSS", PAL_HUD);
    }

    char buf[48];
    std::snprintf(buf, sizeof(buf), "S3 MAILVAN LOCK");
    hud(1, 0, buf, PAL_GOLD);
    std::snprintf(buf, sizeof(buf), "%d", int(time_));
    hud(36, 0, buf, PAL_HUD);
    const char* tag = "OPEN";
    if (phase_ == Phase::Shut || phase_ == Phase::Hold) tag = "SHUT";
    else if (phase_ == Phase::Open) tag = "RISE";
    else if (phase_ == Phase::Leave) tag = "CLEAR";
    hud(16, 0, tag, phase_ == Phase::Hold ? PAL_WIN : PAL_HUD);
}

}  // namespace mailvanlock
