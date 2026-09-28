#include "game/scull.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace scull {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHalf = 3.6f;
constexpr float kPpm = 6.6f;
constexpr float kDrive = 2.55f;
constexpr float kCheck = 3.35f;
constexpr float kDrag = 0.42f;
constexpr float kCurrent = 0.62f;
constexpr float kStop = 0.14f;
constexpr float kMark = 54.f;
constexpr float kAlong = 0.48f;
constexpr float kHoldNeed = 0.85f;
constexpr float kEnd = 64.f;
constexpr float kClock = 42.f;
constexpr float kWater = 156.f;

float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

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
    made_ = 0;
    x_ = 0;
    v_ = 0;
    if (bot_) beginRun();
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.08f) return 3;
    if (std::fabs(x_ - kMark) <= kAlong) return 2;
    return 1;
}

void Game::beginRun() {
    x_ = 4.f;
    v_ = 0.2f;
    phase_ = 0.15f;
    hold_ = 0;
    race_ = 0;
    splash_ = 0;
    made_ = 0;
    won_ = false;
    over_ = false;
    why_ = nullptr;
    mode_ = Mode::Row;
    blip(220.f);
}

void Game::blip(float freq) {
    tone_ = freq;
    toneT_ = 0.1f;
}

void Game::bot(bool& drive, bool& check) const {
    float err = kMark - x_;
    float bow = (x_ + kHalf) - kEnd;
    if (bow > -1.4f) {
        check = true;
        return;
    }
    if (std::fabs(err) < 0.55f && std::fabs(v_) < 0.85f) {
        int period = 12;
        int on = 3;
        bool pulse = int(tick_ % period) < on;
        if (v_ > 0.08f || err < -0.12f) check = true;
        else if (v_ < -0.06f || err > 0.16f) drive = true;
        else if (pulse && err >= -0.05f) drive = true;
        return;
    }
    float want;
    if (err > 18.f) want = 3.4f;
    else if (err > 8.f) want = 1.85f;
    else if (err > 2.2f) want = 0.85f;
    else want = clampf(err * 0.72f, -0.55f, 0.7f);
    if (v_ < want - 0.06f) drive = true;
    else if (v_ > want + 0.05f) check = true;
}

void Game::win() {
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    made_ = 1;
    hold_ = kHoldNeed;
    blip(660.f);
}

void Game::fail(const char* why) {
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    blip(90.f);
}

void Game::update(float dt, bool drive, bool check) {
    float a = (drive ? kDrive : 0.f) - (check ? kCheck : 0.f) - kDrag * v_ + kCurrent;
    v_ += a * dt;
    v_ = clampf(v_, -2.4f, 4.2f);
    x_ += v_ * dt;
    if (x_ < 0.f) {
        x_ = 0.f;
        if (v_ < 0.f) v_ = 0.f;
    }

    float rate = drive ? 1.6f : check ? 1.15f : 0.28f;
    phase_ += rate * dt;
    if (phase_ >= 1.f) phase_ -= 1.f;
    if (drive && !wasDrive_) {
        sys_->apu.noiseBurst(0.1f, 680.f, 0.04f);
        splash_ = 0.32f;
    }
    wasDrive_ = drive;
    if (splash_ > 0.f) splash_ -= dt;

    race_ += dt;
    bool on = std::fabs(x_ - kMark) <= kAlong && std::fabs(v_) <= kStop;
    if (on) hold_ += dt;
    else hold_ = 0.f;
    if (hold_ >= kHoldNeed) {
        win();
        return;
    }
    if (x_ + kHalf > kEnd) {
        fail("PAST THE END");
        return;
    }
    if (race_ >= kClock) fail("THE OTHER CREW TOOK THE MARK");
}

void Game::audio() {
    if (toneT_ > 0.f) {
        sys_->apu.tone(0, tone_, 0.12f);
        toneT_ -= kDt;
    } else if (mode_ == Mode::Win) {
        float notes[] = {523.f, 659.f, 784.f, 1046.f};
        int step = int(t_ * 6.f) % 4;
        sys_->apu.tone(0, notes[step], 0.08f);
    } else {
        sys_->apu.tone(0, 0, 0);
    }
    float wash = mode_ == Mode::Row ? std::fabs(v_) * 0.035f : 0.f;
    sys_->apu.noise(wash, 0.35f, false);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    tick_++;
    t_ += kDt;
    gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) beginRun();
    } else if (mode_ == Mode::Row) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
        } else {
            bool drive = false, check = false;
            if (bot_) bot(drive, check);
            else {
                drive = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A);
                check = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_X) || pad.down(gs::BTN_B);
                if (drive && check) check = false;
            }
            update(kDt, drive, check);
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        v_ *= 0.92f;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    }
    audio();
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

    int oi = std::clamp(int(std::lround((0.5f - 0.5f * std::cos(phase_ * 6.2831853f)) * 8.f)), 0, 8);
    spr(art_.oar[oi], 124.f, kWater - 20.f, 30.f, PAL_BOAT, false);
    spr(art_.rower, 116.f, kWater - 24.f, 28.f, PAL_BOAT);
    spr(art_.hull, 118.f, kWater - 8.f, 22.f, PAL_BOAT);
    spr(art_.oar[oi], 124.f, kWater + 4.f, 34.f, PAL_BOAT, false);
    if (splash_ > 0.f) spr(art_.ripple, 136.f, kWater - 2.f, 10.f + splash_ * 8.f, PAL_FOAM);

    float paintL = kMark - kAlong;
    float paintR = kMark + kAlong;
    float wantW = (paintR - paintL) * kPpm;
    float ph = wantW * float(art_.paint.h) / float(art_.paint.w);
    spr(art_.paint, sx((paintL + paintR) * 0.5f), kWater + 2.f, std::max(ph, 10.f), PAL_MARK);
    spr(art_.buoy, sx(kMark), kWater - 16.f, 34.f, PAL_MARK);
    spr(art_.setWord, sx(kMark), kWater - 40.f, 14.f, PAL_MARK);
    spr(art_.buoy, sx(kEnd), kWater - 12.f, 28.f, PAL_WOOD);
    spr(art_.endWord, sx(kEnd), kWater - 34.f, 12.f, PAL_WOOD);

    for (int i = -1; i < 10; i++) {
        float base = std::floor(x_ / 9.f) * 9.f;
        float wx = base + float(i) * 9.f;
        spr(art_.reed, sx(wx, 0.85f), 132.f, 26.f, PAL_BANK);
    }
    for (int i = -2; i < 12; i++) {
        float base = std::floor((x_ + t_ * 1.6f) / 6.f) * 6.f;
        float wx = base + float(i) * 6.f - std::fmod(t_ * 1.6f, 6.f);
        spr(art_.ripple, sx(wx), kWater + 10.f + float((i * 3) & 7), 8.f, PAL_FOAM);
    }
    for (int i = -2; i < 8; i++) {
        float base = std::floor(x_ * 0.35f / 22.f) * 22.f;
        float wx = base + float(i) * 22.f;
        spr(art_.tree, sx(wx, 0.35f), 96.f, 52.f, PAL_BANK);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 SCULLMARK", PAL_HUD);
        hudC(6, "SET THE SCULL DOWN ON THE MARK", 2);
        hudC(8, "CLOSE TO THE MARK IS NOT ON IT", 4);
        hudC(16, "RIGHT OR C DRIVES THE STROKE", 5);
        hudC(17, "LEFT OR X CHECKS THE BLADES", 5);
        hudC(21, "START TO PUSH OFF", 3);
        hud(1, 26, S3_VERSION_STRING, 6);
    } else {
        float left = std::max(0.f, kClock - race_);
        std::snprintf(buf, sizeof buf, "CREW %4.1f", left);
        hud(1, 0, buf, left < 8.f ? 4 : PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPEED %4.1f", std::fabs(v_) * 1.94f);
        hud(16, 0, buf, PAL_HUD);
        float gap = x_ - kMark;
        std::snprintf(buf, sizeof buf, "MARK %+5.2f", -gap);
        hud(28, 0, buf, std::fabs(gap) <= kAlong ? 3 : PAL_HUD);
        if (mode_ == Mode::Row) {
            if (hold_ > 0.05f) hudC(25, "HOLD HER ON THE MARK", 3);
            else if (std::fabs(gap) < 2.4f) hudC(25, "THE PAINT IS UNDER YOU", 2);
            else if (v_ > 2.2f) hudC(25, "THE STREAM HAS YOU", 5);
            else hudC(25, "SET DOWN ON THE MARK", PAL_HUD);
            hudC(26, "PAST THE END THE LEG IS LOST", 4);
        } else if (mode_ == Mode::Win) {
            hudC(23, "SET DOWN ON THE MARK", 3);
            hudC(25, "BEFORE THE OTHER CREW", 2);
        } else {
            hudC(23, why_ ? why_ : "MISSED THE MARK", 4);
            hudC(25, "THE LEG IS LOST", 4);
            hudC(26, "START TO ROW AGAIN", PAL_HUD);
        }
    }
}

}  // namespace scull
