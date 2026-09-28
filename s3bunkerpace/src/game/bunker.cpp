#include "game/bunker.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace bunkerpace {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSlitL = 96.f;
constexpr float kSlitR = 224.f;
constexpr float kHold = 0.42f;
constexpr float kStride = 1.05f;
constexpr float kWindow = 0.70f;

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
    sys.setLight(40, 70, 40);
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
    walkerX_ = 48.f;
    sightX_ = 160.f;
    flash_ = 0.f;
    fanStep_ = -1;
    sys_->setLight(30, 90, 40);
    sys_->apu.tone(0, 220.f, 0.05f);
    blip_ = 0.08f;
}

void Game::beginPace(int n) {
    pace_ = n;
    phase_ = Phase::Hold;
    phaseT_ = 0.f;
    dirRight_ = (n != 2);
    fromX_ = dirRight_ ? 48.f : 272.f;
    if (n >= 3) toX_ = 160.f;
    else toX_ = dirRight_ ? 272.f : 48.f;
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
    sys_->apu.noiseBurst(0.45f, 180.f, 0.14f);
    sys_->rumble(0.3f, 0.7f, 90);
    sys_->setLight(40, 200, 70);
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
    sys_->apu.tone(0, 130.f, 0.07f);
    sys_->rumble(0.5f, 0.15f, 140);
    sys_->setLight(180, 30, 20);
}

void Game::resolveShot() {
    shot_ = true;
    shotPace_ = std::max(1, pace_);
    flash_ = 0.14f;
    sys_->apu.noiseBurst(0.6f, 1400.f, 0.14f);
    sys_->rumble(0.4f, 0.8f, 70);
    if (pace_ != 3 || phase_ != Phase::Stride) {
        lose("TOO SOON");
        return;
    }
    if (std::fabs(sightX_ - walkerX_) <= 26.f) win();
    else lose("MISSED");
}

bool Game::wantFire() {
    if (bot_) {
        float dx = walkerX_ - sightX_;
        float step = 720.f * kDt;
        if (std::fabs(dx) <= step) sightX_ = walkerX_;
        else sightX_ += std::copysign(step, dx);
        sightX_ = std::clamp(sightX_, kSlitL, kSlitR);
        if (shot_ || pace_ != 3 || phase_ != Phase::Stride) return false;
        float u = phaseT_ / kStride;
        return u > 0.72f && std::fabs(sightX_ - walkerX_) <= 8.f;
    }
    float dir = 0.f;
    if (sys_->pad.down(gs::BTN_LEFT)) dir -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) dir += 1.f;
    if (std::fabs(sys_->pad.axisX) > 0.2f) dir = sys_->pad.axisX;
    sightX_ = std::clamp(sightX_ + dir * 220.f * kDt, kSlitL, kSlitR);
    return firePressed();
}

void Game::updatePlay() {
    phaseT_ += kDt;
    if (pace_ == 0 && phaseT_ >= 0.45f) {
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
        sys_->apu.noiseBurst(0.12f, 240.f, 0.04f);
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
    static const float good[] = {392.f, 494.f, 587.f, 784.f};
    static const float bad[] = {196.f, 164.f, 130.f};
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
    v.setFogColor(gs::rgb4(1, 2, 3));
    uint16_t top = gs::rgb4(1, 1, 3);
    uint16_t hor = gs::rgb4(3, 4, 6);
    if (mode_ == Mode::Over) hor = gs::rgb4(6, 2, 2);
    else if (mode_ == Mode::Victory) hor = gs::rgb4(2, 5, 3);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        float u = float(y) / float(gs::SCREEN_H);
        int r0 = (top >> 8) & 15, g0 = (top >> 4) & 15, b0 = top & 15;
        int r1 = (hor >> 8) & 15, g1 = (hor >> 4) & 15, b1 = hor & 15;
        v.lineBackdrop[y] = gs::rgb4(int(r0 + (r1 - r0) * u), int(g0 + (g1 - g0) * u), int(b0 + (b1 - b0) * u));
        v.lineFog[y] = 0;
    }

    int step = int(t_ * 4.f) & 1;
    bool flip = !dirRight_;
    if (downed_) spr(art_.downed, walkerX_, 168.f, 36.f, PAL_FIGURE, false, true);
    else if (mode_ != Mode::Title) spr(art_.sentry[step], walkerX_, 176.f, 72.f, PAL_FIGURE, flip, true);

    spr(art_.lamp, 118.f, 52.f, 18.f, PAL_FX, false, false);
    spr(art_.lamp, 202.f, 52.f, 18.f, PAL_FX, false, false);
    sprBox(art_.pillar, 48.f, 112.f, 100.f, 224.f, PAL_CONCRETE);
    sprBox(art_.pillar, 272.f, 112.f, 100.f, 224.f, PAL_CONCRETE);
    sprBox(art_.lintel, 160.f, 28.f, 140.f, 28.f, PAL_CONCRETE);
    spr(art_.bag, 108.f, 188.f, 28.f, PAL_BAG, false, false);
    spr(art_.bag, 148.f, 196.f, 32.f, PAL_BAG, true, false);
    spr(art_.bag, 188.f, 190.f, 30.f, PAL_BAG, false, false);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        spr(art_.bead, sightX_, 120.f, 14.f, pace_ >= 3 ? PAL_GOOD : PAL_AMBER, false, false);
    }
    if (flash_ > 0.f) spr(art_.flash, sightX_, 118.f, 36.f, PAL_FX, false, false);

    hudC(1, "S3 BUNKER PACE", PAL_AMBER);
    if (mode_ == Mode::Title) {
        hudC(12, "WAIT FOR THE THIRD PACE", PAL_TEXT);
        hudC(14, "THEN FIRE", PAL_AMBER);
        hudC(24, "START", PAL_TEXT);
        hud(30, 26, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        hudC(14, "PAUSED", PAL_AMBER);
    } else if (mode_ == Mode::Victory) {
        hudC(3, "THE BUNKER HOLDS", PAL_GOOD);
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

}  // namespace bunkerpace
