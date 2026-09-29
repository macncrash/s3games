#include "game/pace.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace redoubtpace {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSlitL = 70.f;
constexpr float kSlitR = 250.f;
constexpr float kHold = 0.38f;
constexpr float kStride = 0.95f;
constexpr float kWindow = 0.85f;
constexpr float kMark[3] = {78.f, 148.f, 214.f};

float smooth(float u) {
    u = std::clamp(u, 0.f, 1.f);
    return u * u * (3.f - 2.f * u);
}

int snap(float v) { return int(std::lround(std::clamp(v, -400.f, 2000.f))); }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Over) return 4;
    if (pace_ >= 3) return 2;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    sys.setLight(50, 40, 20);
    if (bot_) beginWatch();
}

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_TURBO);
}

bool Game::firePressed() {
    const gs::Pad& p = sys_->pad;
    bool trig = p.accel > 0.55f;
    bool edge = trig && !trigWas_;
    trigWas_ = trig;
    return edge || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_X) ||
           p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_TURBO);
}

void Game::beginWatch() {
    mode_ = Mode::Play;
    phase_ = Phase::Hold;
    pace_ = 0;
    shot_ = false;
    downed_ = false;
    over_ = false;
    won_ = false;
    shotPace_ = 0;
    reason_ = "";
    phaseT_ = 0.f;
    walkerX_ = 28.f;
    sightX_ = 160.f;
    flash_ = 0.f;
    fanStep_ = -1;
    dirRight_ = true;
    sys_->setLight(40, 70, 30);
    sys_->apu.tone(0, 196.f, 0.05f);
    blip_ = 0.08f;
}

void Game::beginPace(int n) {
    pace_ = n;
    phase_ = Phase::Hold;
    phaseT_ = 0.f;
    dirRight_ = true;
    fromX_ = (n <= 1) ? 28.f : kMark[n - 2];
    toX_ = kMark[std::min(n, 3) - 1];
    walkerX_ = fromX_;
}

void Game::win() {
    won_ = true;
    over_ = true;
    downed_ = true;
    mode_ = Mode::Victory;
    reason_ = "FIRED ON THE THIRD PACE";
    fanGood_ = true;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->apu.noiseBurst(0.45f, 160.f, 0.14f);
    sys_->rumble(0.3f, 0.7f, 90);
    sys_->setLight(40, 180, 60);
}

void Game::lose(const char* why) {
    won_ = false;
    over_ = true;
    downed_ = false;
    mode_ = Mode::Over;
    reason_ = why;
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->apu.tone(0, 110.f, 0.07f);
    sys_->rumble(0.5f, 0.15f, 140);
    sys_->setLight(160, 30, 16);
}

void Game::resolveShot() {
    shot_ = true;
    shotPace_ = std::max(1, pace_);
    flash_ = 0.16f;
    sys_->apu.noiseBurst(0.65f, 900.f, 0.16f);
    sys_->rumble(0.45f, 0.85f, 80);
    if (pace_ != 3 || phase_ != Phase::Stride) {
        lose("TOO SOON");
        return;
    }
    if (std::fabs(sightX_ - walkerX_) <= 28.f) win();
    else lose("MISSED THE PACE");
}

bool Game::wantFire() {
    if (bot_) {
        float dx = walkerX_ - sightX_;
        float step = 640.f * kDt;
        if (std::fabs(dx) <= step) sightX_ = walkerX_;
        else sightX_ += std::copysign(step, dx);
        sightX_ = std::clamp(sightX_, kSlitL, kSlitR);
        if (shot_ || pace_ != 3 || phase_ != Phase::Stride) return false;
        float u = phaseT_ / kStride;
        return u > 0.55f && std::fabs(sightX_ - walkerX_) <= 10.f;
    }
    float dir = 0.f;
    if (sys_->pad.down(gs::BTN_LEFT)) dir -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) dir += 1.f;
    if (std::fabs(sys_->pad.axisX) > 0.2f) dir = sys_->pad.axisX;
    sightX_ = std::clamp(sightX_ + dir * 200.f * kDt, kSlitL, kSlitR);
    return firePressed();
}

void Game::updatePlay() {
    phaseT_ += kDt;
    if (pace_ == 0 && phaseT_ >= 0.40f) {
        beginPace(1);
        return;
    }
    if (phase_ == Phase::Stride && pace_ > 0) {
        float dur = pace_ >= 3 ? kStride + kWindow : kStride;
        float u = smooth(std::min(1.f, phaseT_ / kStride));
        walkerX_ = fromX_ + (toX_ - fromX_) * u;
        if (wantFire() && !shot_) {
            resolveShot();
            return;
        }
        if (phaseT_ >= dur) {
            if (pace_ >= 3) {
                lose("THE WATCH IS OVER");
                return;
            }
            beginPace(pace_ + 1);
        }
        return;
    }
    if (wantFire() && !shot_ && pace_ > 0) {
        resolveShot();
        return;
    }
    if (pace_ > 0 && phase_ == Phase::Hold && phaseT_ >= kHold) {
        phase_ = Phase::Stride;
        phaseT_ = 0.f;
        sys_->apu.noiseBurst(0.10f, 180.f, 0.04f);
    }
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (fanStep_ < 0) return;
    fanT_ += kDt;
    if (fanT_ < 0.14f) return;
    fanT_ = 0.f;
    static const float good[] = {330.f, 440.f, 554.f, 659.f};
    static const float bad[] = {174.f, 146.f, 110.f};
    const float* notes = fanGood_ ? good : bad;
    int n = fanGood_ ? 4 : 3;
    if (fanStep_ >= n) {
        sys_->apu.tone(0, 0.f, 0.f);
        fanStep_ = -1;
        return;
    }
    sys_->apu.tone(0, notes[fanStep_], 0.08f);
    blip_ = 0.12f;
    fanStep_++;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    flash_ = std::max(0.f, flash_ - kDt);
    if (mode_ == Mode::Title) {
        if (startPressed()) beginWatch();
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (sys.pad.pressed(gs::BTN_START) && !bot_) mode_ = Mode::Pause;
        else updatePlay();
    }
    draw();
    serviceAudio();
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(snap(w), 1, 2000));
    s.h = int16_t(std::clamp(snap(h), 1, 2000));
    s.x = int16_t(snap(cx - s.w * 0.5f));
    s.y = int16_t(snap(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (!(h > 1.f) || !(w > 1.f) || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(snap(w), 1, 2000));
    s.h = int16_t(std::clamp(snap(h), 1, 2000));
    s.x = int16_t(snap(cx - s.w * 0.5f));
    s.y = int16_t(snap(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.A.clear();
    v.B.clear();
    v.HUD.clear();
    v.hudEnabled = true;
    v.setFogColor(gs::rgb4(2, 2, 3));
    uint16_t top = gs::rgb4(1, 1, 4);
    uint16_t hor = gs::rgb4(4, 5, 3);
    if (mode_ == Mode::Over) hor = gs::rgb4(6, 2, 2);
    else if (mode_ == Mode::Victory) hor = gs::rgb4(2, 5, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        float u = float(y) / float(gs::SCREEN_H);
        int r0 = (top >> 8) & 15, g0 = (top >> 4) & 15, b0 = top & 15;
        int r1 = (hor >> 8) & 15, g1 = (hor >> 4) & 15, b1 = hor & 15;
        v.lineBackdrop[y] = gs::rgb4(int(r0 + (r1 - r0) * u), int(g0 + (g1 - g0) * u), int(b0 + (b1 - b0) * u));
        v.lineFog[y] = 0;
    }

    int step = int(t_ * 5.f) & 1;
    if (downed_) spr(art_.fallen, walkerX_, 168.f, 28.f, PAL_FIGURE, false, true);
    else if (mode_ != Mode::Title) spr(art_.raider[step], walkerX_, 168.f, 70.f, PAL_FIGURE, !dirRight_, true);

    for (int i = 0; i < 3; i++) {
        int pal = (pace_ == i + 1 && mode_ == Mode::Play) ? PAL_AMBER : PAL_WOOD;
        if (pace_ >= 3 && i == 2 && mode_ != Mode::Over) pal = PAL_GOOD;
        spr(art_.stake, kMark[i], 176.f, 40.f, pal, false, true);
    }

    spr(art_.flag, 36.f, 78.f, 52.f, PAL_FLAG, false, true);
    spr(art_.gabion, 58.f, 150.f, 58.f, PAL_EARTH, false, true);
    spr(art_.gabion, 262.f, 150.f, 58.f, PAL_EARTH, true, true);
    sprBox(art_.parapet, 160.f, 196.f, 280.f, 48.f, PAL_EARTH);
    spr(art_.gabion, 110.f, 188.f, 36.f, PAL_EARTH, false, true);
    spr(art_.gabion, 210.f, 188.f, 36.f, PAL_EARTH, true, true);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        spr(art_.bead, sightX_, 118.f, 16.f, pace_ >= 3 ? PAL_GOOD : PAL_AMBER, false, false);
    }
    if (flash_ > 0.f) spr(art_.flash, sightX_, 112.f, 40.f, PAL_FX, false, false);

    hudC(1, "S3 REDOUBT PACE", PAL_AMBER);
    if (mode_ == Mode::Title) {
        hudC(12, "WAIT FOR THE THIRD PACE", PAL_TEXT);
        hudC(14, "THEN FIRE", PAL_AMBER);
        hudC(24, "START", PAL_TEXT);
        hud(30, 26, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        hudC(14, "PAUSED", PAL_AMBER);
    } else if (mode_ == Mode::Victory) {
        hudC(3, "THE REDOUBT HOLDS", PAL_GOOD);
        hudC(22, "FIRED ON THE THIRD PACE", PAL_GOOD);
    } else if (mode_ == Mode::Over) {
        hudC(3, "THE WATCH IS OVER", PAL_ALERT);
        hudC(22, reason_, PAL_ALERT);
    } else {
        if (pace_ <= 0) hudC(3, "HOLD", PAL_AMBER);
        else if (pace_ < 3) hudC(3, "PACE " + std::to_string(pace_), PAL_AMBER);
        else hudC(3, "THIRD PACE", PAL_GOOD);
        hudC(25, "AIM   FIRE", PAL_TEXT);
    }
}

}  // namespace redoubtpace
