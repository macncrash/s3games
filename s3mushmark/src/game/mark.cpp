#include "game/mark.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace mushmark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPpm = 6.4f;
constexpr float kDrive = 3.35f;
constexpr float kCheck = 5.1f;
constexpr float kDrag = 0.62f;
constexpr float kGrade = 0.72f;
constexpr float kStop = 0.16f;
constexpr float kHold = 0.55f;
constexpr float kMark = 46.f;
constexpr float kHalf = 1.35f;
constexpr float kPaint = 2.15f;
constexpr float kTrail = 168.f;
constexpr float kCam = 118.f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.42f);
    sys.apu.setEcho(0.08f, 0.18f, 0.08f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    x_ = 0;
    v_ = 0;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.12f) return 3;
    if (onPaint()) return 2;
    return 1;
}

bool Game::onPaint() const { return std::fabs(x_ - kMark) <= kPaint - kHalf * 0.15f; }

bool Game::pastMark() const { return x_ > kMark + kPaint + 0.35f; }

void Game::begin() {
    x_ = 0;
    v_ = 0;
    phase_ = 0.05f;
    hold_ = 0;
    race_ = 0;
    won_ = false;
    over_ = false;
    why_[0] = 0;
    report_[0] = 0;
    mode_ = Mode::Run;
    sys_->apu.tone(0, 220.f, 0.05f);
}

void Game::fail(const char* why) {
    std::snprintf(why_, sizeof why_, "%s", why);
    std::snprintf(report_, sizeof report_, "S3 MUSHMARK  FAIL  %s  x %.1f  spd %.2f  (%.1f s)", why_, x_, v_, race_);
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    sys_->apu.noiseBurst(0.32f, 700.f, 0.2f);
}

void Game::settleWin() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    std::snprintf(report_, sizeof report_, "S3 MUSHMARK  PASS  set down on the mark  x %.1f  spd %.2f  (%.1f s)", x_,
                  v_, race_);
    sys_->apu.tone(0, 523.f, 0.12f);
    sys_->apu.tone(1, 659.f, 0.08f);
}

void Game::controls(bool& drive, bool& check) {
    gs::Pad& pad = sys_->pad;
    drive = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A);
    check = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_X) || pad.down(gs::BTN_B);
    if (drive && check) check = false;
}

void Game::pilot(bool& drive, bool& check) {
    float err = kMark - x_;
    float want;
    if (err > 20.f) want = 4.15f;
    else if (err > 10.f) want = 2.35f;
    else if (err > 4.2f) want = 1.15f;
    else if (err > 1.4f) want = 0.42f;
    else want = std::clamp(err * 0.55f, -0.25f, 0.28f);
    if (x_ > kMark + kPaint * 0.45f) want = std::min(want, -0.35f);

    if (std::fabs(err) < 0.42f && std::fabs(v_) < 0.22f) {
        if (v_ > 0.05f) check = true;
        else if (v_ < -0.05f) drive = true;
        else if (err > 0.12f) drive = true;
        else if (err < -0.12f) check = true;
        return;
    }
    if (v_ < want - 0.07f) drive = true;
    else if (v_ > want + 0.05f) check = true;
}

void Game::physics(bool drive, bool check) {
    float a = (drive ? kDrive : 0.f) - (check ? kCheck : 0.f) - kDrag * v_ + kGrade;
    v_ += a * kDt;
    if (v_ > 5.4f) v_ = 5.4f;
    if (v_ < -2.4f) v_ = -2.4f;
    x_ += v_ * kDt;
    if (x_ < -6.f) {
        x_ = -6.f;
        if (v_ < 0) v_ = 0;
    }
    float rate = 0.3f + std::fabs(v_) * 0.5f;
    if (drive) rate += 0.7f;
    phase_ += rate * kDt;
    if (phase_ >= 1.f) phase_ -= 1.f;
    if (drive && !wasDrive_) sys_->apu.noiseBurst(0.08f, 380.f, 0.035f);
    wasDrive_ = drive;

    race_ += kDt;
    if (pastMark()) {
        fail("past the mark");
        return;
    }
    if (race_ >= limit_) {
        fail("the rival team took the mark");
        return;
    }
    bool quiet = onPaint() && std::fabs(v_) < kStop;
    if (quiet) hold_ += kDt;
    else hold_ = 0;
    if (hold_ >= kHold) settleWin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    tick_++;
    t_ += kDt;
    gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
        } else {
            bool drive = false, check = false;
            if (bot_) pilot(drive, check);
            else controls(drive, check);
            physics(drive, check);
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        v_ *= 0.9f;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    }
    if (mode_ != Mode::Run) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
    draw();
}

float Game::sx(float wx, float par) const { return kCam + (wx - x_) * kPpm * par; }

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
        if (y < 88) {
            int r = 5 + y / 28;
            int g = 7 + y / 20;
            int b = 13 - y / 22;
            if (b < 8) b = 8;
            c = gs::rgb4(r, g, b);
        } else if (y < 146) {
            c = gs::rgb4(12, 13, 14);
        } else {
            int d = (y - 146) / 16;
            c = gs::rgb4(13 - std::min(d, 2), 14 - std::min(d, 2), 15 - std::min(d, 1));
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

    for (int i = -2; i < 8; i++) {
        float base = std::floor(x_ * 0.28f / 22.f) * 22.f;
        float wx = base + float(i) * 22.f;
        spr(art_.pine, sx(wx, 0.28f), 102.f, 54.f, PAL_PINE);
    }
    for (int i = -1; i < 14; i++) {
        float base = std::floor(x_ / 6.5f) * 6.5f;
        float wx = base + float(i) * 6.5f;
        spr(art_.drift, sx(wx, 0.92f), kTrail - 16.f, 11.f, PAL_SNOW);
    }

    float paintL = kMark - kPaint;
    float paintR = kMark + kPaint;
    float pw = (paintR - paintL) * kPpm;
    float ph = pw * float(art_.paint.h) / float(art_.paint.w);
    spr(art_.paint, sx(kMark), kTrail - 4.f, ph, PAL_MARK);
    spr(art_.stake, sx(paintL), kTrail - 18.f, 34.f, PAL_MARK);
    spr(art_.stake, sx(paintR), kTrail - 18.f, 34.f, PAL_MARK);
    spr(art_.flag, sx(paintR) + 10.f, kTrail - 36.f, 18.f, PAL_RIVAL);
    spr(art_.wordMark, sx(kMark), kTrail - 28.f, 12.f, PAL_GOLD);

    int fi = int(phase_ * 3.f) % 3;
    if (fi < 0) fi = 0;
    float bob = (fi == 1) ? 1.6f : 0.f;
    spr(art_.dog[fi], kCam + 58.f, kTrail - 22.f - bob, 18.f, PAL_TEAM);
    spr(art_.dog[(fi + 1) % 3], kCam + 38.f, kTrail - 20.f - (1.6f - bob), 17.f, PAL_TEAM);
    spr(art_.dog[(fi + 2) % 3], kCam + 18.f, kTrail - 21.f - bob, 17.f, PAL_TEAM);
    spr(art_.sled, kCam - 6.f, kTrail - 14.f, 24.f, PAL_TEAM);
    spr(art_.musher, kCam - 16.f, kTrail - 32.f, 30.f, PAL_TEAM);

    char buf[72];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 MUSHMARK", PAL_HUD);
        hudC(6, "SET DOWN ON THE MARK", 3);
        hudC(8, "PAST THE FAR STAKE MISSES IT", 4);
        hudC(15, "RIGHT OR C DRIVES THE TEAM", 5);
        hudC(16, "LEFT OR X CHECKS THE RUNNERS", 5);
        hudC(20, "THE RIVAL CLOCK IS ON THE STAKE", 2);
        hudC(23, "START TO MUSH", 3);
        hud(1, 26, S3_VERSION_STRING, 6);
    } else {
        hud(1, 0, "ONE JOB", PAL_HUD);
        std::snprintf(buf, sizeof buf, "RIVAL %4.1f", std::max(0.f, limit_ - race_));
        hud(26, 0, buf, rivalLeft() < 8.f ? 4 : PAL_HUD);
        std::snprintf(buf, sizeof buf, "PACE %4.1f", std::fabs(v_) * 2.2f);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "MARK %+5.1f", kMark - x_);
        hud(26, 1, buf, onPaint() ? 3 : 5);
        if (mode_ == Mode::Run) {
            if (hold_ > 0.05f) hudC(25, "HOLD THE SLED ON THE PAINT", 3);
            else if (onPaint()) hudC(25, "SET DOWN", 3);
            else if (kMark - x_ < 8.f && kMark - x_ > 0.f) hudC(25, "THE MARK IS UNDER THE NOSE", 2);
            else hudC(25, "BRING THE SLED ONTO THE MARK", PAL_HUD);
        } else if (mode_ == Mode::Win) {
            hudC(23, "SET DOWN ON THE MARK", 3);
            hudC(25, "THE MUSH IS YOURS", 2);
        } else if (mode_ == Mode::Fail) {
            hudC(23, why_, 4);
            hudC(25, "START TO TRY THE MARK AGAIN", PAL_HUD);
        }
    }
}

}  // namespace mushmark
