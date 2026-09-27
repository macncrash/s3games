#include "game/span.h"

#include <algorithm>
#include <cmath>

namespace spanpace {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 78.f;
constexpr float kZNear = 2.4f;
constexpr float kPpm = 92.f;
constexpr float kDeckHalf = 2.35f;
constexpr float kWalkerH = 1.82f;
constexpr float kZ0 = 17.4f;
constexpr float kThroughZ = 3.05f;
constexpr float kIntro = 0.7f;
constexpr float kPi = 3.14159265f;

constexpr float kMarkZ[4] = {0.f, 13.2f, 8.4f, 4.85f};
constexpr float kMarkLat[4] = {0.f, -0.42f, 0.36f, -0.08f};

float lerpf(float a, float b, float u) { return a + (b - a) * u; }

float smooth(float u) {
    u = std::clamp(u, 0.f, 1.f);
    return u * u * (3.f - 2.f * u);
}

uint16_t mixC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [](int c0, int c1, float u) { return int(std::lround(c0 + (c1 - c0) * u)); };
    return gs::rgb4(ch(ar, br, t), ch(ag, bg, t), ch(ab, bb, t));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Over) return 4;
    if (pace_ >= 3) return 2;
    return 1;
}

float Game::holdDur() const { return pace_ >= 3 ? 0.95f : 0.38f; }
float Game::strideDur() const { return pace_ >= 3 ? 1.05f : 0.58f; }

float Game::swayAt(float row) const {
    float sag = std::sin(row * 0.011f) * 6.f;
    float wind = std::sin(t_ * 0.85f + row * 0.02f) * 1.6f;
    return sag + wind;
}

int Game::fogFor(float z) const {
    float t = std::clamp((z - 6.f) / 16.f, 0.f, 1.f);
    return int(t * 10.f);
}

float Game::reach() const { return std::max(28.f, walkerH_ * 0.9f); }

Game::Proj Game::project(float lat, float z) const {
    Proj p;
    if (!(z > 0.5f)) return p;
    float span = float(gs::SCREEN_H) - kHorizon;
    float t = kZNear / z;
    p.ppm = kPpm * t;
    p.y = kHorizon + t * span;
    p.x = 160.f + swayAt(p.y - kHorizon) + lat * p.ppm;
    p.ok = true;
    return p;
}

void Game::resetPose() {
    pace_ = 0;
    phase_ = Phase::Intro;
    phaseT_ = 0.f;
    z_ = kZ0;
    lat_ = 0.f;
    fromZ_ = toZ_ = z_;
    fromLat_ = toLat_ = 0.f;
    step_ = 0.f;
    shot_ = false;
    shotPace_ = 0;
    fell_ = false;
    won_ = false;
    over_ = false;
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
    if (sys_) sys_->setLight(40, 90, 140);
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
    sys_->apu.noiseBurst(0.18f, n == 3 ? 640.f : 280.f, 0.07f);
    if (n == 3) {
        blip(440.f, 0.08f, 0.16f);
        sys_->setLight(40, 180, 120);
    } else {
        blip(160.f + float(n) * 40.f, 0.04f, 0.07f);
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
    Proj p = project(lat_, z_);
    if (!p.ok) return;
    walkerX_ = p.x;
    walkerH_ = std::clamp(kWalkerH * p.ppm, 4.f, 190.f);
    walkerFeet_ = p.y;
    float bob = 0.f;
    if (!fell_ && phase_ == Phase::Stride) bob = std::sin(step_ * kPi) * std::min(7.f, walkerH_ * 0.11f);
    else if (!fell_) bob = std::sin(t_ * 2.1f) * 0.8f;
    walkerFeet_ -= bob;
    walkerChest_ = walkerFeet_ - walkerH_ * 0.55f;
    walkerFog_ = fogFor(z_);
}

bool Game::aim() {
    if (bot_) {
        float dx = walkerX_ - sightX_;
        float maxStep = 1100.f * kDt;
        if (std::fabs(dx) <= maxStep) sightX_ = walkerX_;
        else sightX_ += std::copysign(maxStep, dx);
        sightX_ = std::clamp(sightX_, 4.f, 316.f);
        if (shot_ || pace_ < 3) return false;
        bool stepping = phase_ == Phase::Stride && phaseT_ > 0.12f && phaseT_ < strideDur() * 0.85f;
        if (!stepping) return false;
        return std::fabs(walkerX_ - sightX_) <= reach();
    }
    float dir = 0.f;
    if (sys_->pad.down(gs::BTN_LEFT)) dir -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) dir += 1.f;
    if (std::fabs(sys_->pad.axisX) > 0.22f) dir = sys_->pad.axisX;
    sightX_ += dir * 340.f * kDt;
    sightX_ = std::clamp(sightX_, 24.f, 296.f);
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
    sys_->apu.noiseBurst(0.45f, 180.f, 0.14f);
    sys_->rumble(0.3f, 0.7f, 80);
    sys_->setLight(30, 200, 140);
}

void Game::lose(const char* why) {
    won_ = false;
    over_ = true;
    fell_ = false;
    mode_ = Mode::Over;
    reason_ = why;
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 1.f;
    blip_ = 0.f;
    sys_->setLight(160, 30, 30);
}

void Game::resolveShot() {
    shot_ = true;
    shotPace_ = pace_;
    flash_ = 0.16f;
    flashX_ = sightX_;
    flashY_ = walkerChest_;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.65f, 1400.f, 0.16f);
    sys_->rumble(0.5f, 0.85f, 60);
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
        float u = smooth(std::min(1.f, phaseT_ / strideDur()));
        z_ = lerpf(fromZ_, toZ_, u);
        lat_ = lerpf(fromLat_, toLat_, u);
        step_ = u;
    }
    measure();
    if (aim() && !shot_) {
        resolveShot();
        if (mode_ != Mode::Play) return;
    }
    float dur = phase_ == Phase::Intro ? kIntro : phase_ == Phase::Hold ? holdDur() : strideDur();
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
        lose("CROSSED");
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
        if (fanT_ < 0.13f) return;
        fanT_ = 0.f;
        static const float good[] = {494.f, 622.f, 740.f, 988.f};
        static const float bad[] = {180.f, 140.f, 98.f};
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
    t_ += kDt;
    if (flash_ > 0.f) flash_ = std::max(0.f, flash_ - kDt);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 2.4f);

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
    if (!(h > 1.2f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -500, 500));
    s.y = int16_t(std::clamp(int(std::lround(feet ? cy - s.h : cy - s.h * 0.5f)), -500, 500));
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

void Game::drawDeck(float shx) {
    (void)shx;
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(1, 2, 6);
    uint16_t skyHor = gs::rgb4(8, 6, 8);
    if (mode_ == Mode::Over) skyHor = gs::rgb4(8, 2, 3);
    else if (mode_ == Mode::Victory) skyHor = gs::rgb4(4, 8, 8);
    v.setFogColor(skyHor);
    const float span = float(gs::SCREEN_H) - kHorizon;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            rd.on = false;
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = mixC(skyTop, skyHor, u * u);
            v.lineFog[y] = uint8_t(std::clamp(int(u * 4.f), 0, 8));
            continue;
        }
        float t = (float(y) + 0.5f - kHorizon) / span;
        t = std::max(t, 0.02f);
        float wz = kZNear / t;
        float row = float(y) - kHorizon;
        rd.on = true;
        rd.cx = 160.f + swayAt(row);
        rd.hw = kDeckHalf * kPpm * t;
        rd.v = wz * 1.6f + t_ * 0.4f;
        rd.pal = PAL_ROAD;
        rd.band = uint8_t((int(wz * 0.7f) & 1));
        rd.style = 1;
        rd.left = gs::GROUND_WATER;
        rd.right = gs::GROUND_WATER;
        v.lineBackdrop[y] = gs::rgb4(1, 3, 6);
        v.lineFog[y] = uint8_t(std::clamp(fogFor(wz), 0, 12));
    }
}

void Game::drawSky(float shx) {
    spr(art_.moon, 54.f + shx, 36.f, 22.f, PAL_SKY, false, 1, false, false);
    spr(art_.cloud, 190.f + std::sin(t_ * 0.12f) * 10.f + shx, 30.f, 14.f, PAL_SKY, false, 4, false, false);
    spr(art_.cloud, 260.f + std::sin(t_ * 0.09f) * 8.f + shx, 52.f, 10.f, PAL_SKY, true, 6, false, false);
    float gx = 40.f + std::fmod(t_ * 18.f, 360.f);
    spr(art_.gull[int(t_ * 5.f) & 1], gx + shx, 48.f + std::sin(t_ * 1.4f) * 4.f, 10.f, PAL_SKY, false, 2, false,
        false);
    spr(art_.gull[(int(t_ * 5.f) + 1) & 1], 300.f - std::fmod(t_ * 14.f, 200.f) + shx, 62.f, 8.f, PAL_SKY, true, 3,
        false, false);
}

void Game::drawSpan(float shx) {
    auto place = [&](float lat, float z, const gs::Mipped& m, float worldH, int pal, bool feet) {
        Proj p = project(lat, z);
        if (!p.ok) return p;
        float h = std::clamp(worldH * p.ppm, 3.f, 220.f);
        spr(m, p.x + shx, p.y, h, pal, lat > 0.f, fogFor(z), feet, false);
        return p;
    };

    Proj nearL = place(-kDeckHalf - 0.15f, 6.2f, art_.tower, 7.4f, PAL_STEEL, true);
    Proj nearR = place(kDeckHalf + 0.15f, 6.2f, art_.tower, 7.4f, PAL_STEEL, true);
    Proj farL = place(-kDeckHalf - 0.2f, 14.6f, art_.tower, 7.8f, PAL_STEEL, true);
    Proj farR = place(kDeckHalf + 0.2f, 14.6f, art_.tower, 7.8f, PAL_STEEL, true);

    auto cable = [&](Proj a, Proj b, float droop) {
        if (!a.ok || !b.ok) return;
        float ay = a.y - 7.4f * a.ppm * 0.92f;
        float by = b.y - 7.8f * b.ppm * 0.92f;
        for (int i = 0; i <= 14; i++) {
            float u = float(i) / 14.f;
            float x = lerpf(a.x, b.x, u);
            float y = lerpf(ay, by, u) + std::sin(u * kPi) * droop;
            spr(art_.link, x + shx, y, 5.f, PAL_CABLE, false, 2, false, false);
        }
    };
    cable(nearL, farL, 22.f);
    cable(nearR, farR, 22.f);

    for (int n = 1; n <= 3; n++) {
        float side = n == 2 ? 1.f : -1.f;
        place(side * (kDeckHalf - 0.35f), kMarkZ[n], art_.lamp, n == 3 ? 2.4f : 1.8f,
              (pace_ >= n && mode_ != Mode::Over) ? PAL_LIVE : PAL_GOLD, true);
    }
    place(-3.4f, 11.5f, art_.buoy, 1.1f, PAL_WATER, true);
    place(3.6f, 9.2f, art_.buoy, 0.9f, PAL_WATER, true);
    place(-3.1f, 5.4f, art_.buoy, 1.3f, PAL_WATER, true);

    for (int i = 0; i < 5; i++) {
        float z = 5.f + float(i) * 2.2f;
        place(-kDeckHalf + 0.05f, z, art_.rail, 0.45f, PAL_STEEL, true);
        place(kDeckHalf - 0.05f, z, art_.rail, 0.45f, PAL_STEEL, true);
    }

    if (fell_) {
        Proj p = project(lat_, z_);
        if (p.ok) {
            float h = std::clamp(0.7f * p.ppm, 6.f, 70.f);
            spr(art_.fallen, p.x + shx, p.y, h, PAL_COAT, false, walkerFog_, true, false);
        }
    } else {
        int fr = (phase_ == Phase::Stride && step_ > 0.15f && step_ < 0.85f) ? 1 : 0;
        spr(art_.coat[fr], walkerX_ + shx, walkerFeet_, walkerH_, PAL_COAT, lat_ > 0.f, walkerFog_, true, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 52.f) * 3.2f * std::min(shake_, 1.f);
    drawDeck(shx);

    const char* word = "SPAN PACE";
    int wordPal = PAL_GOLD;
    float wordSc = 0.95f;
    if (mode_ == Mode::Victory) {
        word = "HELD";
        wordPal = PAL_GOOD;
        wordSc = 1.25f;
    } else if (mode_ == Mode::Over) {
        word = reason_ && reason_[0] ? reason_ : "OVER";
        wordPal = PAL_ALERT;
        wordSc = 1.0f;
    } else if (mode_ == Mode::Pause) {
        word = "PAUSED";
        wordPal = PAL_GOLD;
    } else if (mode_ == Mode::Play) {
        if (pace_ <= 0) word = "WAIT";
        else if (pace_ == 1) word = "ONE";
        else if (pace_ == 2) word = "TWO";
        else {
            word = "FIRE";
            wordPal = PAL_LIVE;
            wordSc = 1.28f;
        }
    }
    text(word, 160.f + shx, 28.f, wordSc, wordPal);

    for (int i = 0; i < 3; i++) {
        int pal = PAL_STEEL;
        float s = 9.f;
        if (mode_ == Mode::Over) {
            if (i < pace_) pal = PAL_ALERT;
        } else if (i < pace_ || mode_ == Mode::Victory) {
            pal = (i == 2 && pace_ >= 3) ? PAL_LIVE : PAL_GOLD;
            if (i == 2 && pace_ >= 3) s = 14.f + std::sin(t_ * 8.f) * 1.1f;
        }
        spr(art_.pip, 136.f + float(i) * 24.f + shx, 50.f, s, pal, false, 0, false, false);
    }

    int beadPal = PAL_HOLD;
    if (mode_ == Mode::Victory || (mode_ == Mode::Play && pace_ >= 3)) beadPal = PAL_LIVE;
    if (mode_ == Mode::Over) beadPal = PAL_ALERT;
    if (mode_ != Mode::Pause) spr(art_.bead, sightX_ + shx, walkerChest_, 16.f, beadPal, false, 0, false, false);
    if (flash_ > 0.f) {
        spr(art_.flash, flashX_ + shx, flashY_, 16.f + (0.16f - flash_) * 60.f, PAL_FX, false, 0, false, false);
        spr(art_.dust, 160.f + shx, 188.f, 12.f, PAL_FX, false, 0, false, false);
    }

    drawSpan(shx);
    drawSky(shx);

    if (mode_ == Mode::Title) {
        hudC(21, "WAIT UNTIL THE THIRD PACE", PAL_GOLD);
        hudC(22, "THEN FIRE", PAL_GOOD);
        hudC(23, "MISS THAT AND THE WATCH IS OVER", PAL_TEXT);
        hudC(25, "ARROWS AIM    Z FIRES", PAL_TEXT);
        hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(23, "FIRED ON THE THIRD PACE", PAL_GOOD);
        hudC(24, "THE WATCH HOLDS", PAL_TEXT);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Over) {
        hudC(23, reason_, PAL_ALERT);
        hudC(24, "THE WATCH IS OVER", PAL_TEXT);
        hudC(26, "START RETRIES", PAL_TEXT);
    } else if (pace_ < 3) {
        hud(1, 1, "HOLD FIRE", PAL_ALERT);
        hudC(24, "WAIT UNTIL THE THIRD PACE", PAL_TEXT);
        hudC(25, "DO NOT FIRE", PAL_ALERT);
    } else {
        hud(1, 1, "THIRD PACE", PAL_GOOD);
        hudC(24, "FIRE", PAL_GOOD);
        hudC(25, "BEFORE THEY CROSS", PAL_TEXT);
    }
}

}  // namespace spanpace
