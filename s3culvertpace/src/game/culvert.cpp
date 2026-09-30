#include "game/culvert.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace culvertpace {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kIntro = 0.55f;
constexpr float kPi = 3.1415926f;
constexpr float kMarkZ[4] = {0.f, 15.5f, 10.2f, 6.4f};
constexpr float kMarkLat[4] = {0.f, -0.55f, 0.42f, 0.05f};
constexpr float kStartZ = 22.f;
constexpr float kThroughZ = 4.4f;

float lerpf(float a, float b, float t) { return a + (b - a) * t; }
float smooth(float u) { return u * u * (3.f - 2.f * u); }

float holdDur(int pace) { return pace >= 3 ? 0.42f : 0.36f; }
float strideDur(int pace) { return pace >= 3 ? 0.72f : 0.48f; }

}  // namespace

int Game::marker() const {
    if (won_) return 3;
    if (over_ || mode_ == Mode::Over) return 4;
    if (mode_ == Mode::Title) return 0;
    if (pace_ >= 3 && !shot_) return 2;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::resetPose() {
    pace_ = 0;
    shotPace_ = 0;
    shot_ = false;
    fell_ = false;
    won_ = false;
    over_ = false;
    phase_ = Phase::Intro;
    phaseT_ = 0.f;
    step_ = 0.f;
    z_ = fromZ_ = toZ_ = kStartZ;
    lat_ = fromLat_ = toLat_ = 0.f;
    flash_ = 0.f;
    shake_ = 0.f;
    sightX_ = 160.f;
    reason_ = "";
    fanStep_ = -1;
    fanT_ = 0.f;
    blip_ = 0.f;
    trigWas_ = false;
}

void Game::beginWatch() {
    if (sys_) sys_->apu.silence();
    resetPose();
    mode_ = Mode::Play;
    measure();
    sightX_ = walkerX_;
    if (sys_) sys_->setLight(40, 70, 90);
}

void Game::beginPace(int n) {
    pace_ = n;
    phase_ = Phase::Hold;
    phaseT_ = 0.f;
    step_ = 0.f;
    fromZ_ = z_;
    fromLat_ = lat_;
    toZ_ = kMarkZ[n];
    toLat_ = kMarkLat[n];
    sys_->apu.noiseBurst(0.16f, n == 3 ? 180.f : 90.f, 0.08f);
    if (n == 3) {
        blip(392.f, 0.07f, 0.18f);
        sys_->setLight(30, 140, 70);
    } else {
        blip(110.f + float(n) * 28.f, 0.04f, 0.08f);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    t_ = 0.f;
    resetPose();
    mode_ = Mode::Title;
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

void Game::measure() {
    float ppm = 150.f / std::max(2.5f, z_);
    walkerX_ = 160.f + lat_ * ppm * 8.f;
    walkerH_ = std::clamp(240.f / z_, 6.f, 150.f);
    walkerFeet_ = 108.f + 340.f / z_;
    if (!fell_ && phase_ == Phase::Stride) walkerFeet_ -= std::sin(step_ * kPi) * std::min(6.f, walkerH_ * 0.12f);
    sightY_ = walkerFeet_ - walkerH_ * 0.55f;
    float near = std::clamp((14.f - z_) / 12.f, 0.f, 1.f);
    walkerFog_ = int((1.f - near) * 11.f);
}

float Game::reach() const { return std::max(14.f, walkerH_ * 0.55f); }

bool Game::aim() {
    if (bot_) {
        float dx = walkerX_ - sightX_;
        float maxStep = 900.f * kDt;
        if (std::fabs(dx) <= maxStep) sightX_ = walkerX_;
        else sightX_ += std::copysign(maxStep, dx);
        if (shot_ || pace_ < 3) return false;
        bool stepping = phase_ == Phase::Stride && phaseT_ > 0.18f;
        if (!stepping) return false;
        return std::fabs(walkerX_ - sightX_) <= reach();
    }
    float dir = 0.f;
    if (sys_->pad.down(gs::BTN_LEFT)) dir -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) dir += 1.f;
    if (std::fabs(sys_->pad.axisX) > 0.2f) dir = sys_->pad.axisX;
    sightX_ += dir * 280.f * kDt;
    sightX_ = std::clamp(sightX_, 28.f, 292.f);
    return firePressed();
}

void Game::win() {
    won_ = true;
    over_ = true;
    fell_ = true;
    mode_ = Mode::Victory;
    reason_ = "FIRED ON THE THIRD PACE";
    fanGood_ = true;
    fanStep_ = 0;
    fanT_ = 1.f;
    blip_ = 0.f;
    sys_->apu.noiseBurst(0.4f, 140.f, 0.18f);
    sys_->rumble(0.25f, 0.55f, 70);
    sys_->setLight(40, 160, 80);
}

void Game::lose(const char* why) {
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    reason_ = why;
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 1.f;
    blip_ = 0.f;
    sys_->setLight(120, 30, 20);
}

void Game::resolveShot() {
    shot_ = true;
    shotPace_ = pace_;
    flash_ = 0.14f;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.55f, 900.f, 0.12f);
    sys_->rumble(0.45f, 0.7f, 50);
    if (pace_ != 3) {
        lose("TOO SOON");
        return;
    }
    if (std::fabs(sightX_ - walkerX_) <= reach()) win();
    else lose("MISSED");
}

void Game::updatePlay() {
    phaseT_ += kDt;
    if (phase_ == Phase::Stride) {
        float u = smooth(std::min(1.f, phaseT_ / strideDur(pace_)));
        z_ = lerpf(fromZ_, toZ_, u);
        lat_ = lerpf(fromLat_, toLat_, u);
        step_ = u;
    }
    measure();
    if (aim() && !shot_) {
        resolveShot();
        if (mode_ != Mode::Play) return;
    }
    float dur = phase_ == Phase::Intro ? kIntro : phase_ == Phase::Hold ? holdDur(pace_) : strideDur(pace_);
    if (phaseT_ < dur) return;
    if (phase_ == Phase::Intro) {
        beginPace(1);
        return;
    }
    if (phase_ == Phase::Hold) {
        phase_ = Phase::Stride;
        phaseT_ = 0.f;
        step_ = 0.f;
        fromZ_ = z_;
        fromLat_ = lat_;
        return;
    }
    z_ = toZ_;
    lat_ = toLat_;
    if (pace_ >= 3) {
        z_ = kThroughZ;
        lat_ = 0.f;
        lose("THROUGH");
        return;
    }
    beginPace(pace_ + 1);
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    blip_ = hold;
}

void Game::serviceAudio() {
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ < 0.12f) return;
        fanT_ = 0.f;
        static const float good[] = {330.f, 392.f, 494.f, 659.f};
        static const float bad[] = {196.f, 146.f, 110.f};
        const float* notes = fanGood_ ? good : bad;
        int n = fanGood_ ? 4 : 3;
        if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.05f);
        else sys_->apu.tone(0, 0.f, 0.f);
        if (++fanStep_ > n + 2) fanStep_ = -1;
        return;
    }
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog, bool feet) {
    if (!(h > 1.2f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.img = m.pick(h);
    s.w = std::max(1, int(w));
    s.h = std::max(1, int(h));
    s.x = int(std::lround(cx - w * 0.5f));
    s.y = int(std::lround(feet ? cy - h : cy - h * 0.5f));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float sh = 0.f;
    if (shake_ > 0.f) sh = std::sin(t_ * 90.f) * shake_ * 3.f;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = y < 78 ? 0 : (y < 150 ? 1 : 2);
        uint16_t bg = sky == 0 ? gs::rgb4(1, 2, 4) : sky == 1 ? gs::rgb4(2, 3, 2) : gs::rgb4(1, 2, 2);
        v.lineBackdrop[y] = bg;
        v.lineFog[y] = uint8_t(y < 90 ? 2 : 0);
        gs::RoadLine line;
        if (y >= 132 && y < 214) {
            float tline = float(y - 132) / 82.f;
            line.on = true;
            line.cx = 160.f + sh;
            line.hw = 10.f + tline * 78.f;
            line.v = 80.f + (1.f - tline) * 520.f + t_ * 18.f;
            line.pal = PAL_ROAD;
            line.style = 2;
            line.band = (y & 8) ? 1 : 0;
            line.left = gs::GROUND_DROP;
            line.right = gs::GROUND_DROP;
        }
        v.road[y] = line;
    }

    // Far rings first so the near mouth sits on top (earlier sprites draw above).
    const float depths[] = {4.8f, 7.6f, 11.5f, 16.5f, 23.f};
    for (int i = 4; i >= 0; --i) {
        float z = depths[i];
        float h = 620.f / z;
        float cy = 100.f + 80.f / z;
        int fog = int(std::clamp((z - 6.f) * 0.7f, 0.f, 13.f));
        spr(art_.ring, 160.f + sh, cy, h, PAL_PIPE, fog, false);
    }

    const gs::Mipped& body = fell_ ? art_.fallen : art_.walker;
    float bh = fell_ ? walkerH_ * 0.62f : walkerH_;
    spr(body, walkerX_ + sh, walkerFeet_, bh, PAL_FIGURE, walkerFog_, true);

    for (int i = 0; i < 4; i++) {
        float phase = t_ * 0.7f + float(i) * 1.7f;
        float u = phase - std::floor(phase);
        float x = 148.f + float(i) * 9.f + std::sin(t_ + i) * 2.f;
        float y = 96.f + u * 70.f;
        spr(art_.drip, x + sh, y, 8.f + float(i), PAL_WATER, 2, false);
    }

    if (flash_ > 0.f) spr(art_.flash, sightX_ + sh, sightY_, 28.f + flash_ * 40.f, PAL_FLASH, 0, false);
    if (mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Title)
        spr(art_.sight, sightX_ + sh, sightY_, 18.f, PAL_SIGHT, 0, false);

    int textPal = PAL_TEXT;
    if (mode_ == Mode::Title) {
        hudC(3, "S3 CULVERT PACE", textPal);
        hudC(6, "ONE CULVERT", PAL_PIPE);
        hudC(9, "WAIT FOR THE THIRD PACE", textPal);
        hudC(11, "THEN FIRE", PAL_GOOD);
        hudC(24, "START", textPal);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "HELD", textPal);
    } else if (mode_ == Mode::Victory) {
        hudC(4, "THE CULVERT IS DONE", PAL_GOOD);
        hudC(7, "FIRED ON THE THIRD PACE", textPal);
    } else if (mode_ == Mode::Over) {
        hudC(4, reason_, PAL_ALERT);
        hudC(7, "THE CULVERT IS NOT DONE", textPal);
    } else {
        hud(1, 1, "CULVERT", textPal);
        if (pace_ <= 0) hud(28, 1, "WAIT", textPal);
        else if (pace_ < 3) hud(26, 1, pace_ == 1 ? "PACE 1" : "PACE 2", PAL_ALERT);
        else if (!shot_) hud(24, 1, "PACE 3  FIRE", PAL_GOOD);
        else hud(30, 1, "FIRE", textPal);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (flash_ > 0.f) flash_ = std::max(0.f, flash_ - kDt);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 2.6f);
    sys.vdp.roadTime = int(t_ * 18.f);

    if (mode_ == Mode::Title) {
        if (startPressed()) beginWatch();
        measure();
        if (mode_ == Mode::Title) sightX_ = walkerX_;
        draw();
        serviceAudio();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) {
            resetPose();
            mode_ = Mode::Title;
        }
        measure();
        draw();
        serviceAudio();
        return;
    }
    if (mode_ == Mode::Victory || mode_ == Mode::Over) {
        if (!bot_ && startPressed()) beginWatch();
        else if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) {
            resetPose();
            mode_ = Mode::Title;
        }
        measure();
        draw();
        serviceAudio();
        return;
    }
    if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        measure();
        draw();
        serviceAudio();
        return;
    }
    if (!bot_ && sys.pad.pressed(gs::BTN_MODE) && sys.hasHome()) {
        sys.eject();
        return;
    }
    updatePlay();
    measure();
    draw();
    serviceAudio();
}

}  // namespace culvertpace
