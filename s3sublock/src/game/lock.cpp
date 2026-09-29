#include "lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace sublock {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kLen = 72.f;
constexpr float kBeam = 20.f;
constexpr float kLo = 400.f;
constexpr float kHi = 860.f;
constexpr float kGateT = 22.f;
constexpr float kWall = 46.f;
constexpr float kHoldX = 630.f;
constexpr float kCrew = 40.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
bool overlap(float a0, float a1, float b0, float b1) { return a1 > b0 && a0 < b1; }

}  // namespace

float Game::bow() const { return x_ + kLen * 0.5f; }
float Game::stern() const { return x_ - kLen * 0.5f; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (phase_ == Phase::Leave && stern() > kHi + 8.f) return 3;
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
    phase_ = Phase::Approach;
    over_ = false;
    won_ = false;
    x_ = 190.f;
    y_ = 0;
    vx_ = vy_ = 0;
    lo_ = 1.f;
    hi_ = 0.f;
    fill_ = 0;
    crew_ = 0;
    cam_ = x_;
    time_ = 0;
}

void Game::begin() {
    phase_ = Phase::Approach;
    x_ = 190.f;
    y_ = 0;
    vx_ = vy_ = 0;
    lo_ = 1.f;
    hi_ = 0.f;
    fill_ = 0;
    crew_ = 0;
    time_ = 0;
    cam_ = x_;
    shake_ = 0;
    why_ = "";
    over_ = false;
    won_ = false;
    chime_ = -1;
    prop_ = 0;
    mode_ = Mode::Run;
    blip(196.f);
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
    sys_->apu.noiseBurst(0.45f, 700.f, 0.28f);
    sys_->apu.tone(1, 70.f, 0.22f);
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

void Game::pilot(float& thrust, float& ballast) {
    const gs::Pad& pad = sys_->pad;
    thrust = 0;
    ballast = 0;
    if (pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_C) || pad.accel > 0.25f) thrust += 1.f;
    if (pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_X) || pad.brake > 0.25f) thrust -= 1.f;
    if (pad.down(gs::BTN_UP)) ballast -= 1.f;
    if (pad.down(gs::BTN_DOWN)) ballast += 1.f;
    if (std::fabs(pad.axisY) > 0.25f) ballast = -pad.axisY;
    if (!bot_) return;
    ballast = clampf(-y_ * 0.1f - vy_ * 0.16f, -1.f, 1.f);
    bool go = phase_ == Phase::Leave || (phase_ == Phase::Open && hi_ > 0.97f);
    if (go) {
        thrust = 1.f;
        ballast = clampf(-y_ * 0.14f - vy_ * 0.2f, -1.f, 1.f);
    } else if (phase_ == Phase::Approach) {
        float err = kHoldX - x_;
        if (x_ < kHoldX - 70.f) thrust = 1.f;
        else thrust = clampf(err * 0.03f - vx_ * 0.16f, -1.f, 1.f);
    } else {
        thrust = clampf(-vx_ * 0.25f, -1.f, 1.f);
    }
}

void Game::step(float thrust, float ballast) {
    time_ += kDt;
    crew_ = time_;
    prop_ += std::fabs(thrust) * 0.4f + 0.05f;
    vy_ += ballast * 38.f * kDt;
    vy_ *= std::exp(-kDt * 2.8f);
    y_ += vy_ * kDt;
    vx_ += thrust * 46.f * kDt;
    vx_ *= std::exp(-kDt * 0.55f);
    vx_ = clampf(vx_, -28.f, 62.f);
    x_ += vx_ * kDt;

    if (phase_ == Phase::Shut) {
        lo_ = std::max(0.f, lo_ - kDt * 0.55f);
        if (lo_ <= 0.f) phase_ = Phase::Fill;
    } else if (phase_ == Phase::Fill) {
        fill_ = std::min(1.f, fill_ + kDt / 2.1f);
        if (fill_ >= 1.f) {
            phase_ = Phase::Open;
            blip(310.f);
        }
    } else if (phase_ == Phase::Open) {
        hi_ = std::min(1.f, hi_ + kDt * 0.5f);
        if (hi_ >= 1.f) phase_ = Phase::Leave;
    }

    if (phase_ == Phase::Approach) {
        bool in = stern() > kLo + kGateT + 10.f && bow() < kHi - kGateT - 16.f;
        if (in && std::fabs(vx_) < 7.f && std::fabs(y_) < 12.f && std::fabs(vy_) < 10.f) {
            phase_ = Phase::Shut;
            blip(160.f);
        }
    }

    float x0 = stern(), x1 = bow();
    float y0 = y_ - kBeam * 0.5f, y1 = y_ + kBeam * 0.5f;
    auto gateHit = [&](float gx0, float gx1, float open) {
        if (!overlap(x0, x1, gx0, gx1)) return false;
        float gap = open * (kWall + 6.f);
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
    float wide = kWall + 70.f;
    if (y0 < -wide || y1 > wide) {
        scrape("scraped the channel");
        return;
    }
    if (crew_ > kCrew) {
        scrape("the other crew took the lock");
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
            float thrust = 0, ballast = 0;
            pilot(thrust, ballast);
            step(thrust, ballast);
        }
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            mode_ = Mode::Title;
            over_ = false;
        }
    }

    float want = x_ - 30.f;
    cam_ += (want - cam_) * (1.f - std::exp(-kDt * 3.f));
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
        static const float notes[] = {330.f, 392.f, 494.f, 659.f};
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
    float jx = (shake_ > 0.f) ? std::sin(time_ * 80.f) * shake_ * 3.f : 0.f;
    auto sx = [&](float x) { return (x - cam_) + 150.f + jx; };
    auto sy = [&](float y) { return 116.f + y * 1.55f; };
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
        int g = 3 + y / 48;
        int b = 7 + y / 36;
        if (fill_ > 0.05f && y > 70 && y < 168) b = std::min(14, b + int(fill_ * 3.f));
        vdp.lineBackdrop[y] = gs::rgb4(1, std::min(g, 8), std::min(b, 14));
        vdp.lineFog[y] = uint8_t(y > 190 ? (y - 190) / 3 : 0);
        vdp.road[y].on = false;
    }

    // Foreground first: earlier sprites sit on top.
    if (mode_ == Mode::Title) mark(art_.title, cam_ + 8.f, -18.f, 22.f, PAL_GOLD);
    else if (mode_ == Mode::Win) mark(art_.clear, x_, y_ - 28.f, 18.f, PAL_WIN);
    else if (mode_ == Mode::Fail) mark(art_.fail, x_, y_ - 28.f, 16.f, PAL_ALERT);

    float spin = std::sin(prop_ * 9.f);
    mark(art_.prop, stern() - 6.f, y_ + spin * 3.f, 10.f, PAL_SUB);
    mark(art_.sub, x_, y_, 26.f, PAL_SUB);

    float crewX = 140.f + (crew_ / kCrew) * (kHi + 40.f - 140.f);
    if (mode_ != Mode::Title) mark(art_.sub, crewX, -kWall - 58.f, 14.f, PAL_RIVAL);

    for (int i = 0; i < 5; i++) {
        float bx = x_ - 40.f - i * 18.f + std::fmod(time_ * 14.f + i * 11.f, 30.f);
        float by = y_ + std::sin(time_ * 3.f + i) * 8.f - 6.f;
        mark(art_.bub, bx, by, 4.f + (i & 1), PAL_BUB);
    }

    auto leaf = [&](float gx, float open, float sign) {
        float gap = open * (kWall + 6.f);
        if (sign < 0) quad(art_.gate, gx, -kWall - 8.f, gx + kGateT, -gap, PAL_GATE);
        else quad(art_.gate, gx, gap, gx + kGateT, kWall + 8.f, PAL_GATE);
    };
    leaf(kLo, lo_, -1.f);
    leaf(kLo, lo_, 1.f);
    leaf(kHi - kGateT, hi_, -1.f);
    leaf(kHi - kGateT, hi_, 1.f);

    mark(art_.lamp, kLo - 6.f, -kWall + 2.f, 18.f, PAL_LAMP);
    mark(art_.lamp, kHi + 6.f, -kWall + 2.f, 18.f, PAL_LAMP);

    float bank = kWall + 10.f;
    for (float x = std::floor((cam_ - 200.f) / 40.f) * 40.f; x < cam_ + 240.f; x += 40.f) {
        bool chamber = x + 20.f > kLo && x < kHi;
        float top = chamber ? -bank : -(kWall + 78.f);
        float bot = chamber ? bank : (kWall + 78.f);
        quad(art_.rock, x, top - 20.f, x + 40.f, top, PAL_ROCK);
        quad(art_.rock, x, bot, x + 40.f, bot + 20.f, PAL_ROCK);
        if (!chamber && int(x) % 80 == 0) mark(art_.kelp, x + 12.f, bot - 8.f, 22.f, PAL_KELP);
    }

    if (mode_ == Mode::Title) {
        hudC(16, "PASS THE LOCK", PAL_HUD);
        hudC(18, "DO NOT SCRAPE A GATE", PAL_HUD);
        hudC(20, "THE OTHER CREW IS THE CLOCK", PAL_GOLD);
        hudC(23, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        hudC(24, "CLEAR OF BOTH GATES", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        hudC(24, why_, PAL_ALERT);
    } else {
        const char* hint = "HOLD THE CENTRE LINE";
        if (phase_ == Phase::Shut) hint = "LOWER GATE IS CLOSING";
        else if (phase_ == Phase::Fill) hint = "HOLD FOR THE FLOOD";
        else if (phase_ == Phase::Open) hint = "UPPER GATE IS OPENING";
        else if (phase_ == Phase::Leave) hint = "LEAVE BEFORE THEIR CREW";
        else if (stern() > kLo) hint = "STOP IN THE CHAMBER";
        hudC(25, hint, PAL_HUD);
        hud(1, 26, "ARROWS  THRUST AND BALLAST", PAL_HUD);
    }

    hud(1, 0, "S3 SUB LOCK", PAL_GOLD);
    char buf[40];
    int left = std::max(0, int(std::ceil(kCrew - crew_)));
    std::snprintf(buf, sizeof(buf), "CREW %02d", left);
    hud(30, 0, buf, left < 10 ? PAL_ALERT : PAL_HUD);
    const char* tag = "LOW";
    if (phase_ == Phase::Fill) tag = "FLOOD";
    else if (phase_ == Phase::Open || phase_ == Phase::Leave || fill_ >= 1.f) tag = "HIGH";
    hud(15, 0, tag, phase_ == Phase::Fill ? PAL_WIN : PAL_HUD);
}

}  // namespace sublock
