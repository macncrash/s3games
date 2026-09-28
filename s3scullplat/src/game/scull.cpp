#include "game/scull.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace scull {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHalf = 3.6f;
constexpr float kPpm = 6.4f;
constexpr float kDrive = 2.35f;
constexpr float kCheck = 3.45f;
constexpr float kDrag = 0.32f;
constexpr float kStop = 0.24f;
constexpr float kWater = 156.f;
constexpr int kLegs = 3;

struct LegDef {
    float platL, platR, current;
};

constexpr LegDef kLeg[kLegs] = {
    {34.f, 52.f, 0.52f},
    {62.f, 76.f, 0.82f},
    {90.f, 101.f, 1.12f},
};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.45f);
    sys.apu.setEcho(0.12f, 0.25f, 0.12f);
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
    if (mode_ == Mode::Row) return 1;
    if (mode_ == Mode::Banner) return 2;
    if (mode_ == Mode::Miss) return 3;
    return 4;
}

void Game::beginLeg() {
    x_ = 0;
    v_ = 0;
    phase_ = 0.15f;
    settle_ = 0;
    mode_ = Mode::Row;
}

void Game::blip(float freq) { sys_->apu.tone(0, freq, 0.08f); }

void Game::bot(bool& drive, bool& check) const {
    const LegDef& L = kLeg[leg_];
    float lo = L.platL + kHalf;
    float hi = L.platR - kHalf;
    float aim = (lo + hi) * 0.5f;
    float err = aim - x_;
    float bowGap = L.platR - (x_ + kHalf);
    float want;
    if (err > 14.f) want = 3.55f;
    else if (err > 6.f) want = 2.05f;
    else if (err > 2.4f) want = 1.15f;
    else want = std::clamp(err * 0.7f, -0.45f, 0.85f);
    if (bowGap < 2.2f) want = std::min(want, std::max(0.f, bowGap) * 0.22f);

    if (std::fabs(err) < 0.55f && bowGap > 0.35f) {
        int period = 10;
        int on = std::max(1, int(std::lround(L.current / kDrive * float(period))));
        bool pulse = int(tick_ % period) < on;
        if (v_ > 0.16f || err < -0.22f) check = true;
        else if (v_ < -0.04f || err > 0.18f) drive = true;
        else if (pulse && err > -0.2f) drive = true;
        return;
    }
    if (v_ < want - 0.07f) drive = true;
    else if (v_ > want + 0.07f) check = true;
}

void Game::miss() {
    lives_--;
    v_ = 0;
    settle_ = 0;
    banner_ = 0;
    sys_->apu.noiseBurst(0.35f, 1400.f, 0.18f);
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

void Game::update(float dt, bool drive, bool check) {
    const LegDef& L = kLeg[leg_];
    float a = (drive ? kDrive : 0.f) - (check ? kCheck : 0.f) - kDrag * v_ - L.current;
    v_ += a * dt;
    if (v_ > 4.4f) v_ = 4.4f;
    if (v_ < -2.2f) v_ = -2.2f;
    x_ += v_ * dt;
    if (x_ < -6.f) {
        x_ = -6.f;
        if (v_ < 0) v_ = 0;
    }

    float rate = 0.35f;
    if (drive) rate = 1.55f;
    else if (check) rate = 1.15f;
    phase_ += rate * dt;
    if (phase_ >= 1.f) phase_ -= 1.f;
    if (drive && !wasDrive_) {
        sys_->apu.noiseBurst(0.12f, 700.f, 0.05f);
        splash_ = 0.35f;
    }
    wasDrive_ = drive;
    if (splash_ > 0) splash_ -= dt;

    if (x_ + kHalf > L.platR + 0.06f) {
        miss();
        return;
    }
    float lo = L.platL + kHalf;
    float hi = L.platR - kHalf;
    bool level = x_ >= lo && x_ <= hi && std::fabs(v_) < kStop;
    if (level) settle_ += dt;
    else settle_ = 0;
    if (settle_ > 0.5f) clearLeg();
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
    } else if (mode_ == Mode::Row) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
        } else {
            bool drive = false, check = false;
            if (bot_) bot(drive, check);
            else {
                drive = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_C);
                check = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_X);
                if (drive && check) check = false;
            }
            update(kDt, drive, check);
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
    if (mode_ != Mode::Row) {
        sys.apu.tone(0, 0, 0);
    }
    draw();
}

float Game::sx(float wx, float par) const { return 112.f + (wx - x_) * kPpm * par; }

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
        if (y < 78) {
            int r = 4 + y / 16;
            int g = 7 + y / 12;
            int b = 12 + (78 - y) / 20;
            c = gs::rgb4(r, g, b);
        } else if (y < 112) {
            c = gs::rgb4(3, 7, 3);
        } else if (y < 128) {
            c = gs::rgb4(5, 6, 3);
        } else if (y < 148) {
            c = gs::rgb4(3, 7, 8);
        } else {
            int d = (y - 148) / 16;
            c = gs::rgb4(2, 5 - std::min(d, 2), 7 - std::min(d, 3));
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

    int oi = std::clamp(int(std::lround((0.5f - 0.5f * std::cos(phase_ * 6.2831853f)) * 8.f)), 0, 8);
    float ox = 112.f + 6.f;
    float oy = kWater - 18.f;
    spr(art_.oar[oi], ox, oy - 2.f, 30.f, PAL_BOAT, false);
    spr(art_.rower, 112.f - 2.f, kWater - 24.f, 28.f, PAL_BOAT);
    spr(art_.hull, 112.f, kWater - 8.f, 22.f, PAL_BOAT);
    spr(art_.oar[oi], ox, oy + 6.f, 34.f, PAL_BOAT, false);
    if (splash_ > 0.f) spr(art_.ripple, 112.f + 16.f, kWater - 2.f, 10.f + splash_ * 8.f, PAL_FOAM);

    auto span = [&](float a, float b, const gs::Mipped& m) {
        if (b - a < 0.2f) return;
        float wantW = (b - a) * kPpm;
        float h = wantW * float(m.h) / float(m.w);
        spr(m, sx((a + b) * 0.5f), kWater - 4.f, h, PAL_WOOD);
    };
    span(L.platL, lo, art_.deck);
    span(lo, hi, art_.level);
    span(hi, L.platR, art_.deck);
    for (float p = L.platL + 1.5f; p < L.platR; p += 4.2f) spr(art_.pile, sx(p), kWater + 10.f, 36.f, PAL_WOOD);
    spr(art_.lip, sx(L.platR), kWater - 10.f, 30.f, PAL_MARK);
    spr(art_.endWord, sx(L.platR) - 18.f, kWater - 28.f, 14.f, PAL_MARK);
    spr(art_.levelWord, sx((lo + hi) * 0.5f), kWater - 26.f, 12.f, PAL_WOOD);

    for (int i = -1; i < 10; i++) {
        float base = std::floor(x_ / 9.f) * 9.f;
        float wx = base + float(i) * 9.f;
        spr(art_.reed, sx(wx, 0.85f), 132.f, 26.f, PAL_BANK);
    }
    for (int i = -2; i < 12; i++) {
        float base = std::floor((x_ + t_ * 1.4f) / 6.f) * 6.f;
        float wx = base + float(i) * 6.f - std::fmod(t_ * 1.4f, 6.f);
        spr(art_.ripple, sx(wx), kWater + 8.f + float((i * 3) & 7), 8.f, PAL_FOAM);
    }
    for (int i = -2; i < 8; i++) {
        float base = std::floor(x_ * 0.35f / 22.f) * 22.f;
        float wx = base + float(i) * 22.f;
        spr(art_.tree, sx(wx, 0.35f), 96.f, 52.f, PAL_BANK);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 SCULLPLAT", PAL_HUD);
        hudC(6, "STOP LEVEL WITH THE PLATFORM", 2);
        hudC(8, "MISS THE END AND THE LEG FAILS", 4);
        hudC(16, "RIGHT OR C DRIVES THE STROKE", 5);
        hudC(17, "LEFT OR X CHECKS THE BLADES", 5);
        hudC(21, "START TO PUSH OFF", 3);
        hud(1, 26, S3_VERSION_STRING, 6);
    } else {
        std::snprintf(buf, sizeof buf, "LEG %d/%d", std::min(leg_, kLegs - 1) + 1, kLegs);
        hud(1, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "MADE %d", made_);
        hud(12, 0, buf, 3);
        std::snprintf(buf, sizeof buf, "BLADES %d", std::max(lives_, 0));
        hud(28, 0, buf, lives_ <= 1 ? 4 : PAL_HUD);
        float kn = std::fabs(v_) * 1.94f;
        std::snprintf(buf, sizeof buf, "SPEED %4.1f", kn);
        hud(1, 1, buf, PAL_HUD);
        if (mode_ == Mode::Row) {
            float bowGap = L.platR - (x_ + kHalf);
            if (settle_ > 0.05f) hudC(25, "HOLD HER LEVEL", 3);
            else if (bowGap < 8.f && bowGap > 0.f) hudC(25, "THE END IS CLOSE", 2);
            else if (v_ < -0.2f) hudC(25, "THE STREAM HAS YOU", 5);
            else hudC(25, "BRING HER LEVEL ON THE STAGE", PAL_HUD);
            hudC(26, "PAST THE END FAILS THE LEG", 4);
        } else if (mode_ == Mode::Banner) {
            hudC(24, "LEG MADE", 3);
            hudC(26, "NEXT STAGE", PAL_HUD);
        } else if (mode_ == Mode::Miss) {
            hudC(24, "MISSED THE END", 4);
            hudC(26, "LEG FAILED", 4);
        } else if (mode_ == Mode::Victory) {
            hudC(23, "LEVEL AT EVERY STAGE", 3);
            hudC(25, "THE SCULL IS YOURS", 2);
        } else if (mode_ == Mode::Over) {
            hudC(24, "NO BLADES LEFT", 4);
            hudC(26, "START TO ROW AGAIN", PAL_HUD);
        }
    }
}

}  // namespace scull
