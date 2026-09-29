#include "game/mush.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace mush {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHalf = 2.4f;
constexpr float kPpm = 5.6f;
constexpr float kDrive = 2.55f;
constexpr float kCheck = 3.85f;
constexpr float kDrag = 0.42f;
constexpr float kStop = 0.22f;
constexpr float kTrail = 168.f;
constexpr int kLegs = 3;

struct LegDef {
    float platL, platR, grade;
};

constexpr LegDef kLeg[kLegs] = {
    {36.f, 58.f, 0.62f},
    {64.f, 80.f, 0.98f},
    {92.f, 104.f, 1.28f},
};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.45f);
    sys.apu.setEcho(0.10f, 0.22f, 0.10f);
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

void Game::bot(bool& drive, bool& check) const {
    const LegDef& L = kLeg[leg_];
    float lo = L.platL + kHalf;
    float hi = L.platR - kHalf;
    float aim = (lo + hi) * 0.5f;
    float err = aim - x_;
    float bowGap = L.platR - (x_ + kHalf);
    float want;
    if (err > 16.f) want = 3.7f;
    else if (err > 7.f) want = 2.15f;
    else if (err > 2.6f) want = 1.05f;
    else want = std::clamp(err * 0.62f - L.grade * 0.15f, -0.35f, 0.7f);
    if (bowGap < 2.6f) want = std::min(want, std::max(0.f, bowGap) * 0.18f);

    if (std::fabs(err) < 0.5f && bowGap > 0.4f) {
        if (v_ > 0.12f || err < -0.18f) check = true;
        else if (v_ < -0.06f || err > 0.16f) drive = true;
        else if ((tick_ % 12) < 3) drive = true;
        return;
    }
    if (v_ < want - 0.06f) drive = true;
    else if (v_ > want + 0.05f) check = true;
}

void Game::miss() {
    lives_--;
    v_ = 0;
    settle_ = 0;
    banner_ = 0;
    sys_->apu.noiseBurst(0.35f, 900.f, 0.18f);
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
    blip(480.f);
    if (leg_ + 1 >= kLegs) {
        won_ = true;
        mode_ = Mode::Victory;
    } else {
        mode_ = Mode::Banner;
    }
}

void Game::update(float dt, bool drive, bool check) {
    const LegDef& L = kLeg[leg_];
    float a = (drive ? kDrive : 0.f) - (check ? kCheck : 0.f) - kDrag * v_ + L.grade;
    v_ += a * dt;
    if (v_ > 4.6f) v_ = 4.6f;
    if (v_ < -2.0f) v_ = -2.0f;
    x_ += v_ * dt;
    if (x_ < -8.f) {
        x_ = -8.f;
        if (v_ < 0) v_ = 0;
    }

    float rate = 0.25f + std::fabs(v_) * 0.55f;
    if (drive) rate += 0.8f;
    phase_ += rate * dt;
    if (phase_ >= 1.f) phase_ -= 1.f;
    if (drive && !wasDrive_) sys_->apu.noiseBurst(0.10f, 420.f, 0.04f);
    wasDrive_ = drive;

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
    if (mode_ != Mode::Run) sys.apu.tone(0, 0, 0);
    draw();
}

float Game::sx(float wx, float par) const { return 108.f + (wx - x_) * kPpm * par; }

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
        if (y < 96) {
            int r = 6 + y / 22;
            int g = 8 + y / 18;
            int b = 12 + (40 - y / 6) / 8;
            if (b < 8) b = 8;
            if (b > 15) b = 15;
            c = gs::rgb4(r, g, b);
        } else if (y < 150) {
            c = gs::rgb4(11, 12, 13);
        } else {
            int d = (y - 150) / 14;
            c = gs::rgb4(12 - std::min(d, 3), 13 - std::min(d, 3), 14 - std::min(d, 2));
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

    for (int i = -2; i < 8; i++) {
        float base = std::floor(x_ * 0.32f / 24.f) * 24.f;
        float wx = base + float(i) * 24.f;
        spr(art_.pine, sx(wx, 0.32f), 108.f, 58.f, PAL_PINE);
    }
    for (int i = -1; i < 12; i++) {
        float base = std::floor(x_ / 7.f) * 7.f;
        float wx = base + float(i) * 7.f;
        spr(art_.drift, sx(wx, 0.9f), kTrail - 18.f, 12.f, PAL_SNOW);
    }

    auto span = [&](float a, float b, const gs::Mipped& m) {
        if (b - a < 0.2f) return;
        float wantW = (b - a) * kPpm;
        float h = wantW * float(m.h) / float(m.w);
        spr(m, sx((a + b) * 0.5f), kTrail - 8.f, h, PAL_WOOD);
    };
    span(L.platL, lo, art_.deck);
    span(lo, hi, art_.level);
    span(hi, L.platR, art_.deck);
    for (float p = L.platL + 2.f; p < L.platR; p += 5.5f) spr(art_.post, sx(p), kTrail + 4.f, 28.f, PAL_WOOD);
    spr(art_.post, sx(L.platR), kTrail - 6.f, 36.f, PAL_MARK);
    spr(art_.endWord, sx(L.platR) - 16.f, kTrail - 34.f, 14.f, PAL_MARK);
    spr(art_.levelWord, sx((lo + hi) * 0.5f), kTrail - 30.f, 12.f, PAL_GOLD);

    int fi = int(phase_ * 4.f) & 3;
    float bob = (fi & 1) ? 1.5f : 0.f;
    spr(art_.dog[fi], 108.f + 52.f, kTrail - 22.f - bob, 18.f, PAL_TEAM);
    spr(art_.dog[(fi + 1) & 3], 108.f + 34.f, kTrail - 20.f - (1.5f - bob), 18.f, PAL_TEAM);
    spr(art_.dog[(fi + 2) & 3], 108.f + 16.f, kTrail - 21.f - bob, 18.f, PAL_TEAM);
    spr(art_.sled, 108.f - 8.f, kTrail - 16.f, 26.f, PAL_TEAM);
    spr(art_.musher, 108.f - 14.f, kTrail - 32.f, 30.f, PAL_TEAM);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 MUSHPLAT", PAL_HUD);
        hudC(6, "STOP LEVEL WITH THE PLATFORM", 2);
        hudC(8, "MISS THE END AND THE LEG FAILS", 4);
        hudC(16, "RIGHT OR C DRIVES THE TEAM", 5);
        hudC(17, "LEFT OR X CHECKS THE RUNNERS", 5);
        hudC(21, "START TO MUSH", 3);
        hud(1, 26, S3_VERSION_STRING, 6);
    } else {
        std::snprintf(buf, sizeof buf, "LEG %d/%d", std::min(leg_, kLegs - 1) + 1, kLegs);
        hud(1, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "MADE %d", made_);
        hud(12, 0, buf, 3);
        std::snprintf(buf, sizeof buf, "TEAM %d", std::max(lives_, 0));
        hud(28, 0, buf, lives_ <= 1 ? 4 : PAL_HUD);
        std::snprintf(buf, sizeof buf, "PACE %4.1f", std::fabs(v_) * 2.1f);
        hud(1, 1, buf, PAL_HUD);
        if (mode_ == Mode::Run) {
            float bowGap = L.platR - (x_ + kHalf);
            if (settle_ > 0.05f) hudC(25, "HOLD THE TEAM LEVEL", 3);
            else if (bowGap < 8.f && bowGap > 0.f) hudC(25, "THE END IS CLOSE", 2);
            else if (v_ > 2.4f) hudC(25, "THE GRADE HAS THE RUNNERS", 5);
            else hudC(25, "BRING THE SLED LEVEL ON THE DECK", PAL_HUD);
            hudC(26, "PAST THE END FAILS THE LEG", 4);
        } else if (mode_ == Mode::Banner) {
            hudC(24, "LEG MADE", 3);
            hudC(26, "NEXT PLATFORM", PAL_HUD);
        } else if (mode_ == Mode::Miss) {
            hudC(24, "MISSED THE END", 4);
            hudC(26, "LEG FAILED", 4);
        } else if (mode_ == Mode::Victory) {
            hudC(23, "LEVEL AT EVERY PLATFORM", 3);
            hudC(25, "THE MUSH IS YOURS", 2);
        } else if (mode_ == Mode::Over) {
            hudC(24, "NO TEAM LEFT", 4);
            hudC(26, "START TO MUSH AGAIN", PAL_HUD);
        }
    }
}

}  // namespace mush
