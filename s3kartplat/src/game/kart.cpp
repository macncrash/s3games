#include "game/kart.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace kart {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHalf = 2.2f;
constexpr float kPpm = 6.2f;
constexpr float kThrottle = 2.85f;
constexpr float kBrake = 4.15f;
constexpr float kDrag = 0.48f;
constexpr float kStop = 0.20f;
constexpr float kGround = 168.f;
constexpr int kLegs = 3;

struct LegDef {
    float platL, platR, grade;
};

constexpr LegDef kLeg[kLegs] = {
    {34.f, 54.f, 0.48f},
    {58.f, 74.f, 0.86f},
    {86.f, 98.f, 1.18f},
};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.45f);
    sys.apu.setEcho(0.08f, 0.18f, 0.08f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    leg_ = 0;
    lives_ = 3;
    made_ = 0;
    x_ = 0;
    v_ = 0;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Run) return 1;
    if (mode_ == Mode::Banner) return 2;
    if (mode_ == Mode::Miss) return 3;
    return 4;
}

void Game::beginLeg() {
    x_ = 0;
    v_ = 0;
    phase_ = 0.1f;
    settle_ = 0;
    mode_ = Mode::Run;
}

void Game::blip(float freq) { sys_->apu.tone(0, freq, 0.08f); }

void Game::bot(bool& throttle, bool& brake) const {
    const LegDef& L = kLeg[leg_];
    float lo = L.platL + kHalf;
    float hi = L.platR - kHalf;
    float aim = (lo + hi) * 0.5f;
    float err = aim - x_;
    float nose = L.platR - (x_ + kHalf);
    float want;
    if (err > 16.f) want = 3.6f;
    else if (err > 7.f) want = 2.05f;
    else if (err > 2.4f) want = 0.95f;
    else want = std::clamp(err * 0.64f - L.grade * 0.14f, -0.32f, 0.65f);
    if (nose < 2.4f) want = std::min(want, std::max(0.f, nose) * 0.16f);

    if (std::fabs(err) < 0.45f && nose > 0.35f) {
        if (v_ > 0.10f || err < -0.16f) brake = true;
        else if (v_ < -0.05f || err > 0.14f) throttle = true;
        else if ((tick_ % 12) < 3) throttle = true;
        return;
    }
    if (v_ < want - 0.06f) throttle = true;
    else if (v_ > want + 0.05f) brake = true;
}

void Game::miss() {
    lives_--;
    v_ = 0;
    settle_ = 0;
    banner_ = 0;
    sys_->apu.noiseBurst(0.38f, 700.f, 0.16f);
    if (lives_ <= 0) {
        mode_ = Mode::Over;
        over_ = true;
        won_ = false;
    } else {
        mode_ = Mode::Miss;
    }
}

void Game::clearLeg() {
    made_++;
    banner_ = 0;
    blip(520.f);
    if (leg_ + 1 >= kLegs) {
        won_ = true;
        mode_ = Mode::Victory;
    } else {
        mode_ = Mode::Banner;
    }
}

void Game::update(float dt, bool throttle, bool brake) {
    const LegDef& L = kLeg[leg_];
    float a = (throttle ? kThrottle : 0.f) - (brake ? kBrake : 0.f) - kDrag * v_ + L.grade;
    v_ += a * dt;
    if (v_ > 4.8f) v_ = 4.8f;
    if (v_ < -1.8f) v_ = -1.8f;
    x_ += v_ * dt;
    if (x_ < -6.f) {
        x_ = -6.f;
        if (v_ < 0) v_ = 0;
    }

    float rate = 0.2f + std::fabs(v_) * 0.7f;
    if (throttle) rate += 0.6f;
    phase_ += rate * dt;
    if (phase_ >= 1.f) phase_ -= 1.f;
    if (throttle && !wasThrottle_) sys_->apu.noiseBurst(0.08f, 280.f, 0.03f);
    wasThrottle_ = throttle;

    if (x_ + kHalf > L.platR + 0.05f) {
        miss();
        return;
    }
    float lo = L.platL + kHalf;
    float hi = L.platR - kHalf;
    bool level = x_ >= lo && x_ <= hi && std::fabs(v_) < kStop;
    if (level) settle_ += dt;
    else settle_ = 0;
    if (settle_ > 0.48f) clearLeg();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    tick_++;
    t_ += kDt;
    gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) {
            leg_ = 0;
            lives_ = 3;
            made_ = 0;
            won_ = false;
            over_ = false;
            beginLeg();
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
        } else {
            bool throttle = false, brake = false;
            if (bot_) bot(throttle, brake);
            else {
                throttle = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_C);
                brake = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_X);
                if (throttle && brake) brake = false;
            }
            update(kDt, throttle, brake);
        }
    } else if (mode_ == Mode::Banner) {
        banner_ += kDt;
        v_ *= 0.9f;
        if (banner_ > 1.15f) {
            leg_++;
            beginLeg();
        }
    } else if (mode_ == Mode::Miss) {
        banner_ += kDt;
        if (banner_ > 0.9f) beginLeg();
    } else if (mode_ == Mode::Victory) {
        banner_ += kDt;
        v_ *= 0.92f;
        if (banner_ > 1.4f) over_ = true;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
            mode_ = Mode::Title;
            over_ = false;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    }
    if (mode_ != Mode::Run) sys.apu.tone(0, 0, 0);
    draw();
}

float Game::sx(float wx, float par) const { return 118.f + (wx - x_) * kPpm * par; }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -8 || cx - w * 0.5f > gs::SCREEN_W + 8) return;
    if (cy + h * 0.5f < -8 || cy - h * 0.5f > gs::SCREEN_H + 8) return;
    gs::Sprite s;
    long sw = std::lround(w), sh = std::lround(h);
    s.w = int16_t(std::clamp(sw, 1L, 400L));
    s.h = int16_t(std::clamp(sh, 1L, 400L));
    s.x = int16_t(std::lround(cx - sw * 0.5f));
    s.y = int16_t(std::lround(cy - sh * 0.5f));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sky() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 100) {
            int b = 14 - y / 18;
            if (b < 8) b = 8;
            c = gs::rgb4(6 + y / 28, 9 + y / 24, b);
        } else if (y < 148) {
            c = gs::rgb4(8, 10, 8);
        } else {
            int d = (y - 148) / 16;
            c = gs::rgb4(5 - std::min(d, 2), 5 - std::min(d, 2), 6 - std::min(d, 2));
        }
        vdp.lineBackdrop[y] = c;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
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
    sky();

    const LegDef& L = kLeg[std::min(leg_, kLegs - 1)];
    float lo = L.platL + kHalf;
    float hi = L.platR - kHalf;

    for (int i = -2; i < 6; i++) {
        float base = std::floor(x_ * 0.28f / 30.f) * 30.f;
        float wx = base + float(i) * 30.f;
        spr(art_.stand, sx(wx, 0.28f), 96.f, 48.f, PAL_TREE);
    }
    for (int i = -1; i < 14; i++) {
        float base = std::floor(x_ / 6.f) * 6.f;
        float wx = base + float(i) * 6.f;
        spr(art_.deck, sx(wx, 1.f), kGround + 10.f, 8.f, PAL_TAR);
    }

    auto span = [&](float a, float b, const gs::Mipped& m, int pal) {
        if (b - a < 0.2f) return;
        float wantW = (b - a) * kPpm;
        float h = wantW * float(m.h) / float(m.w);
        spr(m, sx((a + b) * 0.5f), kGround - 6.f, h, pal);
    };
    span(L.platL, lo, art_.deck, PAL_PIT);
    span(lo, hi, art_.level, PAL_PIT);
    span(hi, L.platR, art_.deck, PAL_PIT);
    for (float p = L.platL + 1.5f; p < L.platR; p += 6.f) spr(art_.pylon, sx(p), kGround - 18.f, 22.f, PAL_END);
    spr(art_.pylon, sx(L.platR), kGround - 22.f, 32.f, PAL_END);
    spr(art_.endWord, sx(L.platR) - 18.f, kGround - 46.f, 14.f, PAL_END);
    spr(art_.levelWord, sx((lo + hi) * 0.5f), kGround - 40.f, 12.f, PAL_GOLD);

    int fi = int(phase_ * 4.f) & 3;
    spr(art_.body, 118.f, kGround - 22.f, 28.f, PAL_KART);
    spr(art_.driver, 118.f + 2.f, kGround - 36.f, 20.f, PAL_KART);
    spr(art_.wheel[fi], 118.f - 16.f, kGround - 10.f, 14.f, PAL_KART);
    spr(art_.wheel[(fi + 2) & 3], 118.f + 18.f, kGround - 10.f, 14.f, PAL_KART);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 KARTPLAT", PAL_HUD);
        hudC(6, "STOP LEVEL WITH THE PLATFORM", 2);
        hudC(8, "MISS THE END AND THE LEG FAILS", 4);
        hudC(16, "RIGHT OR C OPENS THE THROTTLE", 5);
        hudC(17, "LEFT OR X SETS THE BRAKE", 5);
        hudC(21, "START TO ROLL", 3);
        hud(1, 26, S3_VERSION_STRING, 6);
    } else {
        std::snprintf(buf, sizeof buf, "LEG %d/%d", std::min(leg_, kLegs - 1) + 1, kLegs);
        hud(1, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "MADE %d", made_);
        hud(12, 0, buf, 3);
        std::snprintf(buf, sizeof buf, "KART %d", std::max(lives_, 0));
        hud(28, 0, buf, lives_ <= 1 ? 4 : PAL_HUD);
        std::snprintf(buf, sizeof buf, "PACE %4.1f", std::fabs(v_) * 2.4f);
        hud(1, 1, buf, PAL_HUD);
        if (mode_ == Mode::Run) {
            float nose = L.platR - (x_ + kHalf);
            if (settle_ > 0.05f) hudC(25, "HOLD THE KART LEVEL", 3);
            else if (nose < 8.f && nose > 0.f) hudC(25, "THE END IS CLOSE", 2);
            else if (v_ > 2.6f) hudC(25, "THE GRADE HAS THE KART", 5);
            else hudC(25, "BRING THE KART LEVEL ON THE DECK", PAL_HUD);
            hudC(26, "PAST THE END FAILS THE LEG", 4);
        } else if (mode_ == Mode::Banner) {
            hudC(24, "LEG MADE", 3);
            hudC(26, "NEXT PLATFORM", PAL_HUD);
        } else if (mode_ == Mode::Miss) {
            hudC(24, "MISSED THE END", 4);
            hudC(26, "LEG FAILED", 4);
        } else if (mode_ == Mode::Victory) {
            hudC(23, "LEVEL AT EVERY PLATFORM", 3);
            hudC(25, "THE KART IS YOURS", 2);
        } else if (mode_ == Mode::Over) {
            hudC(24, "NO KART LEFT", 4);
            hudC(26, "START TO ROLL AGAIN", PAL_HUD);
        }
    }
}

}  // namespace kart
