#include "game/wharf.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace wharfpace {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 78.f;
constexpr float kZNear = 2.2f;
constexpr float kPpm = 86.f;
constexpr float kDeckHalf = 1.85f;
constexpr float kWalkerH = 1.72f;
constexpr float kZ0 = 15.4f;
constexpr float kOffZ = 2.55f;
constexpr float kIntro = 0.55f;
constexpr float kPi = 3.14159265f;

constexpr float kMarkZ[4] = {0.f, 12.2f, 8.0f, 4.35f};
constexpr float kMarkLat[4] = {0.f, 0.22f, -0.28f, 0.08f};

struct Prop {
    float z, lat, h;
    int kind;  // 0 pile, 1 lamp, 2 crate, 3 coil
};

constexpr Prop kProps[] = {
    {14.8f, -2.15f, 2.4f, 0}, {11.6f, -2.2f, 2.4f, 0}, {8.4f, -2.1f, 2.4f, 0}, {5.4f, -2.15f, 2.4f, 0},
    {14.6f, 2.2f, 2.4f, 0},   {11.2f, 2.15f, 2.4f, 0}, {7.8f, 2.25f, 2.4f, 0}, {4.8f, 2.1f, 2.4f, 0},
    {13.2f, -1.15f, 1.15f, 1}, {9.6f, 1.05f, 0.55f, 2}, {6.4f, -0.95f, 0.32f, 3}, {10.8f, 2.6f, 0.7f, 1},
};

float lerpf(float a, float b, float u) { return a + (b - a) * u; }

float smooth(float u) {
    u = std::clamp(u, 0.f, 1.f);
    return u * u * (3.f - 2.f * u);
}

int snap(float v) {
    v = std::clamp(v, -400.f, 2000.f);
    return int(std::lround(v));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Over) return 4;
    if (pace_ >= 3) return 2;
    return 1;
}

float Game::holdDur() const { return pace_ >= 3 ? 0.78f : 0.38f; }
float Game::strideDur() const { return pace_ >= 3 ? 0.88f : 0.52f; }

float Game::swayAt(float row) const { return std::sin((row + t_ * 18.f) * 0.012f) * 6.f; }

int Game::fogFor(float z) const {
    float u = std::clamp((z - 5.f) / 14.f, 0.f, 1.f);
    return int(u * 11.f);
}

float Game::reach() const { return std::max(22.f, walkerH_ * 0.7f); }

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
    fromLat_ = toLat_ = lat_;
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
    if (sys_) sys_->setLight(90, 60, 30);
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
    sys_->apu.noiseBurst(0.16f, n == 3 ? 540.f : 220.f, 0.07f);
    if (n == 3) {
        blip(523.f, 0.08f, 0.18f);
        sys_->setLight(40, 170, 80);
    } else {
        blip(146.f + float(n) * 36.f, 0.04f, 0.07f);
        sys_->setLight(90, 60, 30);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    t_ = 0.f;
    resetPose();
    mode_ = Mode::Title;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.setLight(70, 50, 30);
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
    if (!fell_ && phase_ == Phase::Stride) bob = std::sin(step_ * kPi) * std::min(7.f, walkerH_ * 0.14f);
    else if (!fell_) bob = std::sin(t_ * 2.4f) * 0.8f;
    walkerFeet_ -= bob;
    walkerChest_ = walkerFeet_ - walkerH_ * 0.55f;
    walkerFog_ = fogFor(z_);
}

bool Game::aim() {
    if (bot_) {
        float dx = walkerX_ - sightX_;
        float maxStep = 720.f * kDt;
        if (std::fabs(dx) <= maxStep) sightX_ = walkerX_;
        else sightX_ += std::copysign(maxStep, dx);
        sightX_ = std::clamp(sightX_, 20.f, 300.f);
        if (shot_ || pace_ != 3) return false;
        if (phase_ != Phase::Hold || phaseT_ < 0.18f) return false;
        return std::fabs(walkerX_ - sightX_) <= 14.f;
    }
    float dir = 0.f;
    if (sys_->pad.down(gs::BTN_LEFT)) dir -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) dir += 1.f;
    if (std::fabs(sys_->pad.axisX) > 0.22f) dir = sys_->pad.axisX;
    sightX_ += dir * 320.f * kDt;
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
    fanT_ = 0.f;
    blip_ = 0.f;
    sys_->apu.noiseBurst(0.4f, 140.f, 0.14f);
    sys_->rumble(0.3f, 0.7f, 80);
    sys_->setLight(40, 180, 90);
}

void Game::lose(const char* why) {
    won_ = false;
    over_ = true;
    fell_ = false;
    mode_ = Mode::Over;
    reason_ = why;
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 0.f;
    blip_ = 0.f;
    sys_->apu.tone(0, 110.f, 0.06f);
    sys_->rumble(0.5f, 0.15f, 120);
    sys_->setLight(160, 28, 20);
}

void Game::resolveShot() {
    shot_ = true;
    shotPace_ = pace_;
    flash_ = 0.14f;
    flashX_ = sightX_;
    flashY_ = walkerChest_;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.55f, 1200.f, 0.14f);
    sys_->rumble(0.4f, 0.8f, 60);
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
    if (phaseT_ + 0.0001f < dur) return;
    if (phase_ == Phase::Intro) {
        beginPace(1);
        return;
    }
    if (phase_ == Phase::Hold) {
        phase_ = Phase::Stride;
        phaseT_ = 0.f;
        fromZ_ = z_;
        fromLat_ = lat_;
        sys_->apu.noiseBurst(0.12f, 200.f, 0.04f);
        return;
    }
    z_ = toZ_;
    lat_ = toLat_;
    if (pace_ >= 3) {
        z_ = kOffZ;
        lose("OFF THE WHARF");
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
        static const float good[] = {330.f, 415.f, 523.f, 659.f};
        static const float bad[] = {185.f, 147.f, 110.f};
        const float* notes = fanGood_ ? good : bad;
        int n = fanGood_ ? 4 : 3;
        if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
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
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.7f);

    if (mode_ == Mode::Title) {
        if (startPressed()) beginWatch();
        else {
            measure();
            sightX_ = walkerX_;
        }
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
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) beginWatch();
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
    if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) {
        if (sys.hasHome()) {
            sys.eject();
            return;
        }
        resetPose();
        mode_ = Mode::Title;
        measure();
        draw();
        serviceAudio();
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
    s.w = int16_t(std::clamp(snap(w), 1, 2000));
    s.h = int16_t(std::clamp(snap(h), 1, 2000));
    s.x = int16_t(snap(cx - s.w * 0.5f));
    s.y = int16_t(snap(feet ? cy - s.h : cy - s.h * 0.5f));
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

void Game::drawWharf(float shx) {
    gs::VDP& v = sys_->vdp;
    const float span = float(gs::SCREEN_H) - kHorizon;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float sky = y < int(kHorizon) ? float(y) / kHorizon : 1.f;
        int r = int(std::lround(2 + sky * 6));
        int g = int(std::lround(2 + sky * 4));
        int b = int(std::lround(5 + sky * 3));
        if (y >= int(kHorizon)) {
            float depth = float(y - int(kHorizon)) / span;
            r = int(std::lround(1 + (1.f - depth) * 2));
            g = int(std::lround(3 + depth * 2));
            b = int(std::lround(6 + (1.f - depth) * 2));
        }
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = y < 40 ? uint8_t(4) : 0;
        gs::RoadLine& rd = v.road[y];
        rd = gs::RoadLine{};
        if (y <= int(kHorizon) + 2) continue;
        float row = float(y) - kHorizon;
        float z = kZNear * span / std::max(1.f, row);
        float ppm = kPpm * (kZNear / z);
        rd.on = true;
        rd.cx = 160.f + shx + swayAt(row);
        rd.hw = std::max(8.f, kDeckHalf * ppm);
        rd.v = z * 38.f + t_ * 8.f;
        rd.pal = PAL_DECK;
        rd.band = (int(std::floor(z * 1.6f)) & 1) ? 1 : 0;
        rd.style = 0;
        rd.left = gs::GROUND_WATER;
        rd.right = gs::GROUND_WATER;
    }
    v.roadTime = int(t_ * 60.f);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 90.f) * shake_ * 4.f;
    drawWharf(shx);

    for (const Prop& p : kProps) {
        Proj q = project(p.lat, p.z);
        if (!q.ok || q.y < kHorizon) continue;
        float h = p.h * q.ppm;
        int fog = fogFor(p.z);
        if (p.kind == 0) spr(art_.pile, q.x + shx, q.y, h, PAL_PILE, false, fog, true);
        else if (p.kind == 1) spr(art_.lamp, q.x + shx, q.y, h, PAL_LAMP, false, fog, true);
        else if (p.kind == 2) spr(art_.crate, q.x + shx, q.y, h, PAL_CRATE, false, fog, true);
        else spr(art_.coil, q.x + shx, q.y, h, PAL_ROPE, false, fog, true);
    }

    int wing = int(t_ * 5.f) & 1;
    for (int i = 0; i < 3; i++) {
        float gx = 40.f + float(i) * 90.f + std::sin(t_ * 0.7f + float(i)) * 18.f;
        float gy = 28.f + float(i) * 10.f + std::sin(t_ * 1.4f + float(i) * 2.f) * 6.f;
        spr(art_.gull[wing], gx, gy, 10.f, PAL_GULL, i == 1, 2, false);
    }

    if (fell_) {
        spr(art_.downed, walkerX_ + shx, walkerFeet_, walkerH_ * 0.42f, PAL_COAT, false, walkerFog_, true);
        spr(art_.splash, walkerX_ + shx + 8.f, walkerFeet_ + 4.f, walkerH_ * 0.28f, PAL_FX, false, walkerFog_, false);
    } else {
        int stepFrame = (phase_ == Phase::Stride && step_ > 0.15f && step_ < 0.85f) ? 1 : 0;
        bool face = lat_ < 0.f;
        spr(art_.keeper[stepFrame], walkerX_ + shx, walkerFeet_, walkerH_, PAL_COAT, face, walkerFog_, true);
    }

    if (flash_ > 0.f) spr(art_.flash, flashX_ + shx, flashY_, 18.f + flash_ * 40.f, PAL_FX, false, 0, false);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int pal = pace_ >= 3 ? PAL_LIVE : PAL_LAMP;
        spr(art_.sight, sightX_, walkerChest_, 16.f, pal, false, 0, false);
    }

    int banner = mode_ == Mode::Victory ? PAL_LIVE : mode_ == Mode::Over ? PAL_ALERT : PAL_TEXT;
    if (mode_ == Mode::Title) {
        text("WHARF PACE", 160.f, 28.f, 1.f, PAL_LAMP);
        hudC(22, "WAIT FOR THE THIRD PACE", PAL_TEXT);
        hudC(24, "THEN FIRE", PAL_LAMP);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        hudC(3, "HELD", PAL_LAMP);
    } else if (mode_ == Mode::Victory) {
        text("THE WATCH HOLDS", 160.f, 26.f, 0.85f, PAL_LIVE);
        hudC(24, "FIRED ON THE THIRD PACE", PAL_LIVE);
    } else if (mode_ == Mode::Over) {
        text(reason_, 160.f, 26.f, 0.85f, PAL_ALERT);
        hudC(24, "THE WATCH IS OVER", PAL_ALERT);
    } else {
        hud(1, 1, pace_ >= 3 ? "FIRE" : "WAIT", pace_ >= 3 ? PAL_LIVE : PAL_LAMP);
        std::string mark = "PACE " + std::to_string(std::max(pace_, 1));
        if (pace_ == 0) mark = "PACE -";
        hud(32, 1, mark, banner);
        hudC(26, pace_ >= 3 ? "THIRD PACE" : "HOLD THE SHOT", pace_ >= 3 ? PAL_LIVE : PAL_TEXT);
    }
}

}  // namespace wharfpace
