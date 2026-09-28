#include "game/pace.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace tower {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kYard = 148.f;
constexpr float kGallery = 118.f;
constexpr float kSpan = 54.f;
constexpr float kBody = 40.f;
constexpr float kIntro = 0.55f;
constexpr float kHit = 28.f;
constexpr float kPi = 3.14159265f;

constexpr float kEnd[4] = {0.f, -0.72f, 0.68f, 0.06f};

float lerpf(float a, float b, float u) { return a + (b - a) * u; }

float smooth(float u) {
    u = std::clamp(u, 0.f, 1.f);
    return u * u * (3.f - 2.f * u);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Over) return 4;
    if (pace_ >= 3) return 2;
    return 1;
}

float Game::holdDur() const { return pace_ >= 3 ? 0.82f : 0.48f; }
float Game::strideDur() const { return pace_ >= 3 ? 0.86f : 0.62f; }

void Game::resetPose() {
    pace_ = 0;
    phase_ = Phase::Intro;
    phaseT_ = 0.f;
    lat_ = 0.f;
    fromLat_ = toLat_ = 0.f;
    step_ = 0.f;
    shot_ = false;
    shotPace_ = 0;
    down_ = false;
    won_ = false;
    over_ = false;
    flash_ = 0.f;
    shake_ = 0.f;
    puff_ = 0.f;
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
}

void Game::beginPace(int n) {
    pace_ = n;
    phase_ = Phase::Hold;
    phaseT_ = 0.f;
    step_ = 0.f;
    fromLat_ = lat_;
    toLat_ = kEnd[n];
    puff_ = 0.22f;
    sys_->apu.noiseBurst(0.18f, n == 3 ? 620.f : 280.f, 0.07f);
    blip(n == 3 ? 698.f : 196.f + float(n) * 48.f, n == 3 ? 0.07f : 0.03f, n == 3 ? 0.16f : 0.06f);
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
    bool trig = p.accel > 0.6f;
    bool edge = trig && !trigWas_;
    trigWas_ = trig;
    return edge || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X) || p.pressed(gs::BTN_Y) ||
           p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_A);
}

void Game::measure() {
    walkerX_ = 160.f + lat_ * kSpan;
    walkerH_ = kBody;
    float bob = 0.f;
    if (phase_ == Phase::Stride) bob = std::sin(step_ * kPi) * 3.2f;
    else bob = std::sin(t_ * 2.2f) * 0.7f;
    walkerFeet_ = kGallery - bob;
    walkerChest_ = walkerFeet_ - walkerH_ * 0.55f;
}

bool Game::steer() {
    if (bot_) {
        float dx = walkerX_ - sightX_;
        float maxStep = 520.f * kDt;
        if (std::fabs(dx) <= maxStep) sightX_ = walkerX_;
        else sightX_ += std::copysign(maxStep, dx);
    } else {
        float dir = 0.f;
        if (sys_->pad.down(gs::BTN_LEFT)) dir -= 1.f;
        if (sys_->pad.down(gs::BTN_RIGHT)) dir += 1.f;
        if (std::fabs(sys_->pad.axisX) > 0.25f) dir = sys_->pad.axisX;
        sightX_ += dir * 260.f * kDt;
    }
    sightX_ = std::clamp(sightX_, 96.f, 224.f);
    if (bot_) {
        bool third = pace_ == 3 && phase_ == Phase::Hold;
        bool settled = phaseT_ > 0.20f;
        bool aligned = std::fabs(walkerX_ - sightX_) < 8.f;
        return third && settled && aligned && !shot_;
    }
    return firePressed();
}

void Game::win() {
    won_ = true;
    over_ = true;
    down_ = true;
    mode_ = Mode::Victory;
    reason_ = "FIRED ON THE THIRD PACE";
    fanGood_ = true;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->apu.tone(0, 523.f, 0.08f);
    sys_->apu.noiseBurst(0.45f, 180.f, 0.14f);
}

void Game::lose(const char* why) {
    won_ = false;
    over_ = true;
    down_ = false;
    mode_ = Mode::Over;
    reason_ = why;
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->apu.tone(0, 110.f, 0.07f);
}

void Game::resolveShot() {
    shot_ = true;
    shotPace_ = pace_;
    flash_ = 0.16f;
    flashX_ = sightX_;
    flashY_ = walkerChest_;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.55f, 1200.f, 0.14f);
    if (pace_ != 3) {
        lose("TOO SOON");
        return;
    }
    if (std::fabs(sightX_ - walkerX_) <= kHit) win();
    else lose("WIDE");
}

void Game::updatePlay() {
    t_ += kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.6f);
    if (flash_ > 0.f) flash_ = std::max(0.f, flash_ - kDt);
    if (puff_ > 0.f) puff_ = std::max(0.f, puff_ - kDt);

    phaseT_ += kDt;
    if (phase_ == Phase::Stride) {
        float u = smooth(std::min(1.f, phaseT_ / strideDur()));
        lat_ = lerpf(fromLat_, toLat_, u);
        step_ = u;
    }
    measure();
    if (steer() && !shot_) {
        resolveShot();
        if (mode_ != Mode::Play) return;
    }

    float dur = phase_ == Phase::Intro ? kIntro : phase_ == Phase::Hold ? holdDur() : strideDur();
    if (phaseT_ + 0.0001f < dur) return;
    if (phase_ == Phase::Intro) {
        beginPace(1);
        return;
    }
    if (phase_ == Phase::Hold) {
        phase_ = Phase::Stride;
        phaseT_ = 0.f;
        fromLat_ = lat_;
        return;
    }
    lat_ = toLat_;
    if (pace_ >= 3) {
        lose("INSIDE");
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
        if (fanT_ < 0.15f) return;
        fanT_ = 0.f;
        static const float good[] = {392.f, 523.f, 659.f, 784.f};
        static const float bad[] = {164.f, 123.f, 92.f};
        const float* notes = fanGood_ ? good : bad;
        int n = fanGood_ ? 4 : 3;
        if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.06f);
        else sys_->apu.tone(0, 0.f, 0.f);
        if (++fanStep_ > n + 2) fanStep_ = -1;
        return;
    }
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Play) t_ += kDt;
    if (mode_ != Mode::Play) {
        if (flash_ > 0.f) flash_ = std::max(0.f, flash_ - kDt);
        if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.6f);
    }

    if (mode_ == Mode::Title) {
        if (startPressed()) beginWatch();
        measure();
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
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) beginWatch();
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
    if (mode_ != Mode::Play) measure();
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) width += 12.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) {
            x += 12.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false, false);
        x += gw;
    }
}

void Game::drawYard(float shx) {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(mode_ == Mode::Over ? gs::rgb4(6, 2, 2) : gs::rgb4(4, 5, 7));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(kYard)) {
            v.road[y].on = false;
            float u = float(y) / kYard;
            int r = std::clamp(int(2.f + (1.f - u) * 2.f), 0, 15);
            int g = std::clamp(int(3.f + u * 3.f), 0, 15);
            int b = std::clamp(int(6.f + (1.f - u) * 5.f), 0, 15);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) + 0.5f - kYard) / (float(gs::SCREEN_H) - kYard);
        t = std::max(t, 0.04f);
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + shx * 0.2f;
        rd.hw = 70.f + t * 90.f;
        rd.v = (1.f / t) * 8.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = gs::ROAD_ROCKY;
        rd.band = (int(std::floor(rd.v)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        v.lineFog[y] = 0;
        v.lineBackdrop[y] = gs::rgb4(3, 4, 2);
    }
    v.roadTime = int(t_ * 20.f);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 40.f) * 2.4f * std::min(shake_, 1.f);
    drawYard(shx);

    float wave = std::sin(t_ * 1.6f) * 3.f;
    spr(art_.tower, 160.f + shx * 0.2f, 168.f, 150.f, PAL_STONE, false, 0, true, false);
    spr(art_.door, 160.f + shx * 0.2f, 158.f, 34.f, (pace_ >= 3 || mode_ == Mode::Victory) ? PAL_LIVE : PAL_WOOD, false, 0, true, false);
    spr(art_.stair, 196.f + shx * 0.2f, 150.f, 42.f, PAL_STONE, false, 0, true, false);
    for (int i = 0; i < 5; i++)
        spr(art_.merlon, 112.f + float(i) * 24.f + shx * 0.2f, 48.f, 16.f, PAL_STONE, false, 0, false, false);
    spr(art_.flag, 198.f + wave, 28.f, 18.f, PAL_FLAG, false, 0, false, false);

    const gs::Mipped& body = down_ ? art_.down : art_.step[phase_ == Phase::Stride && step_ >= 0.5f ? 1 : 0];
    float h = down_ ? walkerH_ * 0.42f : walkerH_;
    float y = down_ ? walkerFeet_ - 4.f : walkerFeet_;
    spr(art_.shadow, walkerX_ + shx, walkerFeet_ + 2.f, h * 0.18f, PAL_FX, false, 0, false, true);
    spr(body, walkerX_ + shx, y, h, PAL_COAT, lat_ < 0.f, 0, !down_, false);
    if (puff_ > 0.f && !down_) {
        float u = 1.f - puff_ / 0.22f;
        spr(art_.dust, walkerX_ + shx, walkerFeet_ - u * 3.f, 7.f + u * 5.f, PAL_FX, false, 0, false, false);
    }

    const char* banner = "TOWER";
    int bannerPal = PAL_AMBER;
    if (mode_ == Mode::Title) banner = "TOWER PACE";
    else if (mode_ == Mode::Victory) {
        banner = "DONE";
        bannerPal = PAL_GOOD;
    } else if (mode_ == Mode::Over) {
        banner = "LOST";
        bannerPal = PAL_ALERT;
        if (reason_ && std::string(reason_) == "TOO SOON") banner = "TOO SOON";
        else if (reason_ && std::string(reason_) == "WIDE") banner = "WIDE";
        else if (reason_ && std::string(reason_) == "INSIDE") banner = "INSIDE";
    } else if (mode_ == Mode::Pause) banner = "PAUSED";
    else if (pace_ == 0) banner = "WAIT";
    else if (pace_ == 1) banner = "ONE";
    else if (pace_ == 2) banner = "TWO";
    else {
        banner = "FIRE";
        bannerPal = PAL_LIVE;
    }
    text(banner, 160.f, 18.f, mode_ == Mode::Title ? 0.72f : 0.95f, bannerPal);

    for (int i = 0; i < 3; i++) {
        int pal = PAL_IRON;
        if (pace_ > i) pal = (i == 2) ? PAL_LIVE : PAL_AMBER;
        float s = (i == 2 && pace_ >= 3) ? 13.f : 8.f;
        spr(art_.pip, 140.f + float(i) * 20.f, 40.f, s, pal, false, 0, false, false);
    }

    if (mode_ == Mode::Play || flash_ > 0.f) {
        int beadPal = (pace_ >= 3 && mode_ == Mode::Play) || mode_ == Mode::Victory ? PAL_LIVE : PAL_SIGHT;
        if (mode_ == Mode::Play) spr(art_.bead, sightX_ + shx, walkerChest_, 14.f, beadPal, false, 0, false, false);
        if (flash_ > 0.f)
            spr(art_.flare, flashX_ + shx, flashY_, 16.f + (0.16f - flash_) * 40.f, PAL_FX, false, 0, false, false);
    }

    float gunX = 160.f + (sightX_ - 160.f) * 0.28f;
    spr(art_.rifle, gunX + shx, 228.f, 52.f, PAL_IRON, false, 0, true, false);

    if (mode_ == Mode::Title) {
        hudC(20, "ONE TOWER", PAL_TEXT);
        hudC(21, "WAIT UNTIL THE THIRD PACE", PAL_AMBER);
        hudC(22, "THEN FIRE", PAL_GOOD);
        hudC(24, "ARROWS AIM    C FIRES", PAL_TEXT);
        hudC(26, "START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(22, "FIRED ON THE THIRD PACE", PAL_GOOD);
        hudC(23, "THE TOWER IS DONE", PAL_TEXT);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Over) {
        hudC(22, reason_, PAL_ALERT);
        hudC(23, "THE TOWER IS NOT DONE", PAL_TEXT);
        hudC(26, "START RETRIES", PAL_TEXT);
    } else if (pace_ < 3) {
        hudC(0, "HOLD", PAL_AMBER);
        hudC(24, "WAIT UNTIL THE THIRD PACE", PAL_TEXT);
        hudC(25, "DO NOT FIRE", PAL_ALERT);
    } else {
        hudC(0, "THIRD PACE", PAL_GOOD);
        hudC(24, "FIRE", PAL_GOOD);
        hudC(25, "BEFORE THE DOOR", PAL_TEXT);
    }
}

}  // namespace tower
