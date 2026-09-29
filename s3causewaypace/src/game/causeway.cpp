#include "game/causeway.h"

#include <algorithm>
#include <cmath>

namespace causewaypace {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 86.f;
constexpr float kZNear = 2.2f;
constexpr float kPpm = 100.f;
constexpr float kHalf = 1.85f;
constexpr float kWalkerH = 1.7f;
constexpr float kZ0 = 16.2f;
constexpr float kThroughZ = 2.65f;
constexpr float kIntro = 0.55f;
constexpr float kPi = 3.14159265f;

constexpr float kMarkZ[4] = {0.f, 12.4f, 8.05f, 4.35f};
constexpr float kMarkLat[4] = {0.f, 0.28f, -0.32f, 0.06f};

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

float Game::holdDur() const { return pace_ >= 3 ? 1.05f : 0.42f; }
float Game::strideDur() const { return pace_ >= 3 ? 1.15f : 0.62f; }

float Game::swayAt(float row) const {
    return std::sin(row * 0.008f) * 4.f + std::sin(t_ * 0.7f + row * 0.015f) * 1.2f;
}

int Game::fogFor(float z) const {
    float t = std::clamp((z - 5.f) / 14.f, 0.f, 1.f);
    return int(t * 9.f);
}

float Game::reach() const { return std::max(26.f, walkerH_ * 0.85f); }

Game::Proj Game::project(float lat, float z) const {
    Proj p;
    if (!(z > 0.45f)) return p;
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
    if (sys_) sys_->setLight(180, 90, 40);
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
    sys_->apu.noiseBurst(0.16f, n == 3 ? 520.f : 220.f, 0.06f);
    if (n == 3) {
        blip(392.f, 0.08f, 0.18f);
        sys_->setLight(40, 200, 110);
    } else {
        blip(130.f + float(n) * 36.f, 0.04f, 0.06f);
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
    if (!fell_ && phase_ == Phase::Stride) bob = std::sin(step_ * kPi) * std::min(6.f, walkerH_ * 0.1f);
    else if (!fell_) bob = std::sin(t_ * 1.8f) * 0.6f;
    walkerFeet_ -= bob;
    walkerChest_ = walkerFeet_ - walkerH_ * 0.52f;
    walkerFog_ = fogFor(z_);
}

bool Game::aim() {
    if (bot_) {
        float dx = walkerX_ - sightX_;
        float maxStep = 1200.f * kDt;
        if (std::fabs(dx) <= maxStep) sightX_ = walkerX_;
        else sightX_ += std::copysign(maxStep, dx);
        sightX_ = std::clamp(sightX_, 4.f, 316.f);
        if (shot_ || pace_ < 3) return false;
        bool stepping = phase_ == Phase::Stride && phaseT_ > 0.1f && phaseT_ < strideDur() * 0.82f;
        if (!stepping) return false;
        return std::fabs(walkerX_ - sightX_) <= reach();
    }
    float dir = 0.f;
    if (sys_->pad.down(gs::BTN_LEFT)) dir -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) dir += 1.f;
    if (std::fabs(sys_->pad.axisX) > 0.22f) dir = sys_->pad.axisX;
    sightX_ += dir * 320.f * kDt;
    sightX_ = std::clamp(sightX_, 20.f, 300.f);
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
    sys_->apu.noiseBurst(0.4f, 160.f, 0.12f);
    sys_->rumble(0.25f, 0.65f, 70);
    sys_->setLight(40, 210, 120);
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
    sys_->setLight(170, 30, 20);
}

void Game::resolveShot() {
    shot_ = true;
    shotPace_ = pace_;
    flash_ = 0.14f;
    flashX_ = sightX_;
    flashY_ = walkerChest_;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.6f, 1200.f, 0.14f);
    sys_->rumble(0.45f, 0.8f, 50);
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
        static const float good[] = {440.f, 554.f, 659.f, 880.f};
        static const float bad[] = {196.f, 147.f, 98.f};
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
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 2.2f);

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

void Game::drawDeck() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 18.f);
    uint16_t skyTop = gs::rgb4(2, 1, 4);
    uint16_t skyHor = gs::rgb4(12, 6, 3);
    if (mode_ == Mode::Over) skyHor = gs::rgb4(8, 2, 2);
    else if (mode_ == Mode::Victory) skyHor = gs::rgb4(4, 8, 7);
    v.setFogColor(skyHor);
    const float span = float(gs::SCREEN_H) - kHorizon;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            rd.on = false;
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = mixC(skyTop, skyHor, u * u);
            v.lineFog[y] = uint8_t(std::clamp(int(u * 3.f), 0, 6));
            continue;
        }
        float t = (float(y) + 0.5f - kHorizon) / span;
        t = std::max(t, 0.02f);
        float wz = kZNear / t;
        float row = float(y) - kHorizon;
        rd.on = true;
        rd.cx = 160.f + swayAt(row);
        rd.hw = kHalf * kPpm * t;
        rd.v = wz * 1.4f + t_ * 0.25f;
        rd.pal = PAL_ROAD;
        rd.band = uint8_t((int(wz * 0.55f) & 1));
        rd.style = 1;
        rd.left = gs::GROUND_WATER;
        rd.right = gs::GROUND_WATER;
        v.lineBackdrop[y] = gs::rgb4(1, 3, 5);
        v.lineFog[y] = uint8_t(std::clamp(fogFor(wz), 0, 11));
    }
}

void Game::drawSky(float shx) {
    spr(art_.sun, 248.f + shx, 58.f, 28.f, PAL_SKY, false, 1, false, false);
    float hx = 30.f + std::fmod(t_ * 12.f, 280.f);
    spr(art_.heron[int(t_ * 3.f) & 1], hx + shx, 42.f + std::sin(t_ * 1.1f) * 3.f, 12.f, PAL_SKY, false, 2, false,
        false);
}

void Game::drawCauseway(float shx) {
    auto place = [&](float lat, float z, const gs::Mipped& m, float worldH, int pal, bool feet) {
        Proj p = project(lat, z);
        if (!p.ok) return p;
        float h = std::clamp(worldH * p.ppm, 3.f, 200.f);
        spr(m, p.x + shx, p.y, h, pal, lat < 0.f, fogFor(z), feet, false);
        return p;
    };

    for (int i = 0; i < 4; i++) {
        float z = 4.8f + float(i) * 2.6f;
        place(-kHalf - 1.15f, z, art_.reed, 1.1f, PAL_TIDE, true);
        place(kHalf + 1.25f, z + 0.4f, art_.reed, 0.95f, PAL_TIDE, true);
    }
    place(-kHalf - 2.2f, 7.4f, art_.bank, 1.6f, PAL_STONE, true);
    place(kHalf + 2.4f, 10.2f, art_.bank, 1.4f, PAL_STONE, true);

    for (int n = 1; n <= 3; n++) {
        float side = (n == 2) ? 1.f : -1.f;
        int pal = (pace_ >= n && mode_ != Mode::Over) ? (n == 3 ? PAL_LIVE : PAL_GOLD) : PAL_POST;
        place(side * (kHalf - 0.22f), kMarkZ[n], art_.post, n == 3 ? 2.15f : 1.7f, pal, true);
    }

    if (fell_) {
        Proj p = project(lat_, z_);
        if (p.ok) {
            float h = std::clamp(0.62f * p.ppm, 6.f, 64.f);
            spr(art_.fallen, p.x + shx, p.y, h, PAL_COAT, false, walkerFog_, true, false);
        }
    } else {
        int fr = (phase_ == Phase::Stride && step_ > 0.18f && step_ < 0.82f) ? 1 : 0;
        spr(art_.coat[fr], walkerX_ + shx, walkerFeet_, walkerH_, PAL_COAT, lat_ < 0.f, walkerFog_, true, false);
        int lampPal = (pace_ >= 3 && mode_ != Mode::Over) ? PAL_LIVE : PAL_GOLD;
        spr(art_.lantern, walkerX_ + shx + walkerH_ * 0.22f, walkerFeet_ - walkerH_ * 0.42f, walkerH_ * 0.18f, lampPal,
            false, walkerFog_, false, false);
    }
    for (int i = 0; i < 3; i++) {
        float z = 5.2f + float(i) * 1.7f;
        place(-kHalf + 0.08f, z, art_.spray, 0.28f, PAL_TIDE, true);
        place(kHalf - 0.08f, z + 0.6f, art_.spray, 0.24f, PAL_TIDE, true);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 48.f) * 2.8f * std::min(shake_, 1.f);
    drawDeck();

    const char* word = "CAUSEWAY";
    int wordPal = PAL_GOLD;
    float wordSc = 0.9f;
    if (mode_ == Mode::Victory) {
        word = "HELD";
        wordPal = PAL_GOOD;
        wordSc = 1.3f;
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
            wordSc = 1.32f;
        }
    }
    text(word, 160.f + shx, 26.f, wordSc, wordPal);

    for (int i = 0; i < 3; i++) {
        int pal = PAL_STONE;
        float s = 8.f;
        if (mode_ == Mode::Over) {
            if (i < pace_) pal = PAL_ALERT;
        } else if (i < pace_ || mode_ == Mode::Victory) {
            pal = (i == 2 && pace_ >= 3) ? PAL_LIVE : PAL_GOLD;
            if (i == 2 && pace_ >= 3) s = 13.f + std::sin(t_ * 7.f) * 1.f;
        }
        spr(art_.pip, 132.f + float(i) * 28.f + shx, 48.f, s, pal, false, 0, false, false);
    }

    int beadPal = PAL_HOLD;
    if (mode_ == Mode::Victory || (mode_ == Mode::Play && pace_ >= 3)) beadPal = PAL_LIVE;
    if (mode_ == Mode::Over) beadPal = PAL_ALERT;
    if (mode_ != Mode::Pause) spr(art_.bead, sightX_ + shx, walkerChest_, 15.f, beadPal, false, 0, false, false);
    if (flash_ > 0.f)
        spr(art_.flash, flashX_ + shx, flashY_, 14.f + (0.14f - flash_) * 70.f, PAL_FX, false, 0, false, false);

    drawCauseway(shx);
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

}  // namespace causewaypace
