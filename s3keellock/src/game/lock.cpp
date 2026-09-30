#include "lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace keellock {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kLen = 9.f;
constexpr float kBeam = 2.35f;
constexpr float kLo = 96.f;
constexpr float kHi = 188.f;
constexpr float kGateT = 5.5f;
constexpr float kWall = 4.15f;
constexpr float kHold = 142.f;
constexpr float kRival = 36.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::bow() const { return z_ + kLen * 0.5f; }
float Game::stern() const { return z_ - kLen * 0.5f; }
float Game::rivalLeft() const { return std::max(0.f, kRival - time_); }

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
    z_ = 28.f;
    x_ = 0.4f;
    camZ_ = z_ - 16.f;
}

void Game::begin() {
    phase_ = Phase::Approach;
    z_ = 28.f;
    x_ = 0.15f;
    heading_ = 0.f;
    speed_ = 0.f;
    lo_ = 1.f;
    hi_ = 0.f;
    fill_ = 0.f;
    time_ = 0.f;
    camZ_ = z_ - 16.f;
    shake_ = 0.f;
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
    speed_ *= 0.15f;
    sys_->apu.noiseBurst(0.4f, 880.f, 0.25f);
    sys_->apu.tone(1, 80.f, 0.22f);
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

    float aim = clampf(-x_ * 0.55f - heading_ * 0.8f, -1.f, 1.f);
    steer = aim;
    bool go = phase_ == Phase::Leave || (phase_ == Phase::Open && hi_ > 0.97f);
    if (go) {
        thrust = 1.f;
        return;
    }
    if (phase_ == Phase::Approach) {
        float err = kHold - z_;
        if (z_ < kHold - 28.f) thrust = 1.f;
        else thrust = clampf(err * 0.08f - speed_ * 0.35f, -1.f, 0.85f);
        return;
    }
    thrust = clampf(-speed_ * 0.5f, -1.f, 1.f);
}

void Game::step(float thrust, float steer) {
    time_ += kDt;
    heading_ += steer * 1.35f * kDt;
    heading_ = clampf(heading_, -1.1f, 1.1f);
    heading_ *= std::exp(-kDt * 0.35f);
    speed_ += thrust * 8.5f * kDt;
    speed_ *= std::exp(-kDt * 0.55f);
    speed_ = clampf(speed_, -4.f, 11.f);
    x_ += std::sin(heading_) * speed_ * kDt;
    z_ += std::cos(heading_) * speed_ * kDt;
    x_ = clampf(x_, -7.f, 7.f);

    if (phase_ == Phase::Shut) {
        lo_ = std::max(0.f, lo_ - kDt * 0.7f);
        if (lo_ <= 0.f) phase_ = Phase::Fill;
    } else if (phase_ == Phase::Fill) {
        fill_ = std::min(1.f, fill_ + kDt / 2.1f);
        if (fill_ >= 1.f) {
            phase_ = Phase::Open;
            blip(360.f);
        }
    } else if (phase_ == Phase::Open) {
        hi_ = std::min(1.f, hi_ + kDt * 0.62f);
        if (hi_ >= 1.f) phase_ = Phase::Leave;
    }

    if (phase_ == Phase::Approach) {
        bool in = stern() > kLo + kGateT + 2.f && bow() < kHi - kGateT - 4.f;
        if (in && speed_ < 1.3f && std::fabs(speed_) < 1.6f && std::fabs(x_) < 1.4f && std::fabs(heading_) < 0.35f) {
            phase_ = Phase::Shut;
            blip(170.f);
        }
    }

    auto cornerHits = [&](auto&& fn) {
        const float c = std::cos(heading_), s = std::sin(heading_);
        const float hx = kBeam * 0.5f, hz = kLen * 0.5f;
        const float ox[4] = {-hx, hx, hx, -hx};
        const float oz[4] = {-hz, -hz, hz, hz};
        for (int i = 0; i < 4; i++) {
            float wx = x_ + ox[i] * c - oz[i] * s;
            float wz = z_ + ox[i] * s + oz[i] * c;
            if (fn(wx, wz)) return true;
        }
        return false;
    };

    auto gateHit = [&](float gz0, float gz1, float open) {
        float gap = open * (kWall + 0.35f);
        return cornerHits([&](float wx, float wz) {
            if (wz < gz0 || wz > gz1) return false;
            return std::fabs(wx) > gap;
        });
    };
    if (gateHit(kLo, kLo + kGateT, lo_)) {
        scrape("scraped the lower gate");
        return;
    }
    if (gateHit(kHi - kGateT, kHi, hi_)) {
        scrape("scraped the upper gate");
        return;
    }
    bool inChamber = cornerHits([&](float wx, float wz) {
        if (wz < kLo || wz > kHi) return false;
        return std::fabs(wx) > kWall;
    });
    if (inChamber) {
        scrape("scraped the chamber wall");
        return;
    }
    if (time_ >= kRival) {
        scrape("the other crew cleared first");
        return;
    }
    if (phase_ == Phase::Leave && stern() > kHi + 10.f) finish();
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

    float want = z_ - 16.f;
    camZ_ += (want - camZ_) * (1.f - std::exp(-kDt * 4.f));
    if (mode_ == Mode::Title) camZ_ = z_ - 16.f;
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

bool Game::project(float wx, float wz, float& sx, float& sy, float& sc) const {
    float dz = wz - camZ_;
    if (dz < 3.2f) return false;
    sc = 340.f / dz;
    float jx = (shake_ > 0.f) ? std::sin(time_ * 90.f) * shake_ * 3.f : 0.f;
    sx = 160.f + wx * sc + jx;
    sy = 58.f + 1040.f / dz;
    return true;
}

void Game::stamp(const gs::Mipped& m, float x, float y, float w, float h, int pal) {
    if (m.h < 1 || w < 1.f || h < 1.f) return;
    if (x + w < -20.f || x > gs::SCREEN_W + 20.f || y + h < -20.f || y > gs::SCREEN_H + 30.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(w), 1L, 420L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 300L));
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::worldQuad(const gs::Mipped& m, float x0, float z0, float x1, float z1, float lift, int pal) {
    float sx0, sy0, sc0, sx1, sy1, sc1;
    if (!project(x0, z0, sx0, sy0, sc0)) return;
    if (!project(x1, z1, sx1, sy1, sc1)) return;
    float left = std::min(sx0, sx1);
    float right = std::max(sx0, sx1);
    float top = std::min(sy0, sy1) - lift * 0.5f * (sc0 + sc1);
    float bot = std::max(sy0, sy1);
    stamp(m, left, top, std::max(2.f, right - left), std::max(2.f, bot - top), pal);
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
        float t = y / float(gs::SCREEN_H);
        int g = 4 + int(t * 3.f);
        int b = 7 + int(t * 5.f);
        if (y < 58) {
            g = 6;
            b = 9;
        }
        int rise = int(fill_ * 2.f);
        vdp.lineBackdrop[y] = gs::rgb4(1, std::min(14, g + rise), std::min(15, b + rise));
        vdp.lineFog[y] = uint8_t(y < 70 ? (70 - y) / 6 : 0);
        vdp.road[y].on = false;
    }

    for (float z = camZ_ + 78.f; z > camZ_ + 4.f; z -= 8.f) {
        bool chamber = z > kLo && z < kHi;
        float bank = chamber ? kWall : kWall + 3.4f;
        worldQuad(art_.stone, -bank - 3.2f, z, -bank, z + 8.f, 10.f, PAL_STONE);
        worldQuad(art_.stone, bank, z, bank + 3.2f, z + 8.f, 10.f, PAL_STONE);
        if (!chamber && int(z) % 16 < 8) {
            worldQuad(art_.reed, -bank - 1.6f, z, -bank + 0.2f, z + 6.f, 16.f, PAL_BANK);
            worldQuad(art_.reed, bank - 0.2f, z, bank + 1.6f, z + 6.f, 16.f, PAL_BANK);
        }
    }
    worldQuad(art_.lamp, -kWall - 0.4f, kLo + 2.f, -kWall + 0.5f, kLo + 4.f, 22.f, PAL_GOLD);
    worldQuad(art_.lamp, kWall - 0.5f, kHi - 3.f, kWall + 0.4f, kHi - 1.f, 22.f, PAL_GOLD);

    auto leaf = [&](float gz, float open, float sign) {
        float gap = open * (kWall + 0.2f);
        float inner = sign < 0.f ? -kWall - 0.3f : gap;
        float outer = sign < 0.f ? -gap : kWall + 0.3f;
        if (outer < inner) std::swap(inner, outer);
        if (outer - inner < 0.15f) return;
        worldQuad(art_.gate, inner, gz, outer, gz + kGateT, 18.f, PAL_GATE);
    };
    leaf(kLo, lo_, -1.f);
    leaf(kLo, lo_, 1.f);
    leaf(kHi - kGateT, hi_, -1.f);
    leaf(kHi - kGateT, hi_, 1.f);

    float bsx, bsy, bsc;
    if (project(x_, z_, bsx, bsy, bsc)) {
        float bh = std::max(18.f, bsc * 7.2f);
        float bw = bh * 0.42f;
        float lean = heading_ * 18.f;
        stamp(art_.foam, bsx - bw * 0.7f, bsy + bh * 0.15f, bw * 1.3f, bh * 0.22f, PAL_FOAM);
        stamp(art_.sail, bsx - bw * 0.85f + lean, bsy - bh * 0.95f, bw * 1.7f, bh * 0.85f, PAL_SAIL);
        stamp(art_.hull, bsx - bw * 0.5f + lean * 0.3f, bsy - bh * 0.45f, bw, bh, PAL_HULL);
    }

    if (mode_ == Mode::Title) {
        float tsx, tsy, tsc;
        if (project(0.f, z_ + 26.f, tsx, tsy, tsc)) stamp(art_.title, tsx - 70.f, tsy - 18.f, 140.f, 28.f, PAL_GOLD);
        hudC(16, "TAKE THE KEEL", PAL_HUD);
        hudC(18, "PASS THE LOCK", PAL_HUD);
        hudC(20, "DO NOT SCRAPE A GATE", PAL_ALERT);
        hudC(23, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        hudC(16, "CLEAR OF BOTH GATES", PAL_WIN);
        hudC(18, "AHEAD OF THE OTHER CREW", PAL_GOLD);
    } else if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
    } else {
        const char* hint = "LINE THE KEEL AND ENTER";
        if (phase_ == Phase::Shut) hint = "LOWER GATE IS CLOSING";
        else if (phase_ == Phase::Fill) hint = "HOLD WHILE THE LOCK RISES";
        else if (phase_ == Phase::Open) hint = "UPPER GATE IS OPENING";
        else if (phase_ == Phase::Leave) hint = "LEAVE BEFORE THEY DO";
        else if (stern() > kLo) hint = "STOP IN THE CHAMBER";
        hudC(25, hint, PAL_HUD);
        hud(1, 26, "ARROWS  DRIVE AND STEER", PAL_HUD);
    }

    hud(1, 0, "S3 KEEL LOCK", PAL_GOLD);
    char buf[40];
    std::snprintf(buf, sizeof(buf), "CREW %d", int(std::ceil(rivalLeft())));
    hud(30, 0, buf, rivalLeft() < 8.f ? PAL_ALERT : PAL_CREW);
    const char* tag = "LOW";
    if (phase_ == Phase::Fill) tag = "RISE";
    else if (phase_ == Phase::Open || phase_ == Phase::Leave || fill_ >= 1.f) tag = "HIGH";
    if (mode_ != Mode::Title) hud(16, 0, tag, phase_ == Phase::Fill ? PAL_WIN : PAL_HUD);
}

}  // namespace keellock
