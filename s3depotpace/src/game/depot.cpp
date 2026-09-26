#include "game/depot.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace depotpace {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 88.f;
constexpr float kZNear = 2.4f;
constexpr float kPpm = 76.f;
constexpr float kRoadHalf = 3.25f;
constexpr float kWalkerH = 1.78f;
constexpr float kZ0 = 16.8f;
constexpr float kLineZ = 4.15f;
constexpr float kThroughZ = 3.25f;
constexpr float kIntro = 0.62f;
constexpr float kPi = 3.14159265f;

constexpr float kMarkZ[4] = {0.f, 12.2f, 8.0f, 5.15f};
constexpr float kMarkLat[4] = {0.f, -1.25f, 1.45f, 0.08f};

struct Prop {
    float z, lat, h;
    int kind;
};

constexpr Prop kProps[] = {
    {15.8f, -4.5f, 2.35f, 0}, {14.4f, 4.7f, 2.15f, 0}, {16.6f, 3.1f, 1.85f, 1},
    {11.4f, -4.8f, 1.05f, 2}, {9.6f, 5.0f, 0.95f, 2},  {13.2f, -3.7f, 0.72f, 3},
    {10.6f, -4.55f, 1.55f, 4}, {9.1f, 4.7f, 1.45f, 4},
};

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

float Game::holdDur() const { return pace_ >= 3 ? 0.72f : 0.40f; }
float Game::strideDur() const { return pace_ >= 3 ? 0.95f : 0.55f; }

float Game::bendAt(float row) const { return std::sin(row * 0.026f) * 20.f; }

int Game::fogFor(float z) const {
    float t = std::clamp((z - 6.f) / 16.f, 0.f, 1.f);
    return int(t * 12.f);
}

float Game::reach() const { return std::max(24.f, walkerH_ * 0.72f); }

int Game::signalPal() const {
    if (mode_ == Mode::Over) return PAL_ALERT;
    if (mode_ == Mode::Victory || (mode_ == Mode::Play && pace_ >= 3)) return PAL_LIVE;
    return PAL_GOLD;
}

Game::Proj Game::project(float lat, float z) const {
    Proj p;
    if (!(z > 0.55f)) return p;
    float span = float(gs::SCREEN_H) - kHorizon;
    float t = kZNear / z;
    p.ppm = kPpm * t;
    p.y = kHorizon + t * span;
    p.x = 160.f + bendAt(p.y - kHorizon) + lat * p.ppm;
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
    measure();
    sightX_ = walkerX_;
    if (sys_) sys_->setLight(200, 120, 30);
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
    puff_ = 0.26f;
    sys_->apu.noiseBurst(0.22f, n == 3 ? 740.f : 360.f, 0.08f);
    if (n == 3) {
        blip(523.f, 0.08f, 0.20f);
        sys_->setLight(40, 190, 70);
    } else {
        blip(196.f + float(n) * 54.f, 0.045f, 0.08f);
        sys_->setLight(200, 120, 30);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    t_ = 0.f;
    resetPose();
    mode_ = Mode::Title;
    sys.setLight(200, 110, 30);
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
    walkerH_ = std::clamp(kWalkerH * p.ppm, 4.f, 180.f);
    walkerFeet_ = p.y;
    float bob = 0.f;
    if (!fell_ && phase_ == Phase::Stride) bob = std::sin(step_ * kPi) * std::min(6.f, walkerH_ * 0.12f);
    else if (!fell_) bob = std::sin(t_ * 2.2f) * 1.1f;
    walkerFeet_ -= bob;
    walkerChest_ = walkerFeet_ - walkerH_ * 0.58f;
    walkerFog_ = fogFor(z_);
}

bool Game::aim() {
    if (bot_) {
        float dx = walkerX_ - sightX_;
        float maxStep = 800.f * kDt;
        if (std::fabs(dx) <= maxStep) sightX_ = walkerX_;
        else sightX_ += std::copysign(maxStep, dx);
        sightX_ = std::clamp(sightX_, 24.f, 296.f);
        if (shot_ || pace_ != 3) return false;
        if (phase_ == Phase::Hold && phaseT_ < 0.20f) return false;
        if (std::fabs(walkerX_ - sightX_) > 10.f) sightX_ = std::clamp(walkerX_, 24.f, 296.f);
        return std::fabs(walkerX_ - sightX_) <= 16.f;
    }
    float dir = 0.f;
    if (sys_->pad.down(gs::BTN_LEFT)) dir -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) dir += 1.f;
    if (std::fabs(sys_->pad.axisX) > 0.22f) dir = sys_->pad.axisX;
    sightX_ += dir * 340.f * kDt;
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
    fanT_ = 0.f;
    blip_ = 0.f;
    sys_->apu.noiseBurst(0.5f, 180.f, 0.16f);
    sys_->rumble(0.35f, 0.75f, 90);
    sys_->setLight(40, 200, 80);
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
    sys_->apu.tone(0, 146.f, 0.07f);
    sys_->rumble(0.55f, 0.2f, 140);
    sys_->setLight(210, 30, 24);
}

void Game::resolveShot() {
    shot_ = true;
    shotPace_ = pace_;
    flash_ = 0.16f;
    flashX_ = sightX_;
    flashY_ = walkerChest_;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.62f, 1500.f, 0.16f);
    sys_->rumble(0.45f, 0.85f, 70);
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
        sys_->apu.noiseBurst(0.16f, 280.f, 0.05f);
        return;
    }
    z_ = toZ_;
    lat_ = toLat_;
    if (pace_ >= 3) {
        z_ = kThroughZ;
        lat_ = 0.f;
        lose("INTO THE SHED");
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
        if (fanT_ < 0.14f) return;
        fanT_ = 0.f;
        static const float good[] = {392.f, 523.f, 659.f, 784.f};
        static const float bad[] = {220.f, 174.f, 130.f};
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
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.8f);
    if (puff_ > 0.f) puff_ = std::max(0.f, puff_ - kDt);

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

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog) {
    if (!(h > 1.f) || !(w > 1.f) || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(snap(w), 1, 2000));
    s.h = int16_t(std::clamp(snap(h), 1, 2000));
    s.x = int16_t(snap(cx - s.w * 0.5f));
    s.y = int16_t(snap(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
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

void Game::drawRoad(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(2, 2, 4);
    uint16_t skyHor = gs::rgb4(11, 6, 3);
    if (mode_ == Mode::Over) skyHor = gs::rgb4(9, 3, 2);
    else if (mode_ == Mode::Victory) skyHor = gs::rgb4(7, 8, 5);
    v.setFogColor(mode_ == Mode::Over ? gs::rgb4(7, 3, 3) : gs::rgb4(6, 4, 4));
    const float span = float(gs::SCREEN_H) - kHorizon;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            rd.on = false;
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = mixC(skyTop, skyHor, u * u);
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) + 0.5f - kHorizon) / span;
        t = std::max(t, 0.016f);
        float wz = kZNear / t;
        float row = float(y) - kHorizon;
        rd.on = true;
        rd.cx = 160.f + bendAt(row) + shx;
        rd.hw = std::max(4.f, kRoadHalf * kPpm * t);
        rd.v = wz * 34.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = gs::ROAD_RUTS;
        rd.band = (int(std::floor(wz * 0.22f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((wz - 9.f) / 20.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 11.f);
        v.lineBackdrop[y] = gs::rgb4(2, 2, 2);
    }
    v.roadTime = int(t_ * 30.f);
}

void Game::drawShed(float shx) {
    int lampPal = signalPal();
    float bob = std::sin(t_ * 3.1f) * 1.4f;
    spr(art_.lamp, 68.f + shx, 112.f + bob, 18.f, lampPal, false, 0, false, false);
    spr(art_.lamp, 252.f + shx, 116.f - bob, 18.f, lampPal, true, 0, false, false);
    spr(art_.clock[int(t_ * 2.f) & 1], 160.f + shx, 14.f, 22.f, PAL_GOLD, false, 0, false, false);
    spr(art_.sign, 160.f + shx, 34.f, 20.f, PAL_GOLD, false, 0, false, false);
    const float bars[] = {40.f, 76.f, 112.f, 208.f, 244.f, 280.f};
    for (float x : bars) spr(art_.girder, x + shx, 32.f, 16.f, PAL_METAL, false, 0, false, false);
    spr(art_.shed, 26.f + shx, 224.f, 136.f, PAL_BRICK, false, 0, true, false);
    spr(art_.shed, 294.f + shx, 224.f, 136.f, PAL_BRICK, true, 0, true, false);
}

void Game::drawWorld(float shx) {
    struct Item {
        float z;
        int kind;
        int id;
    };
    Item items[32];
    int n = 0;
    items[n++] = {z_ - 0.02f, 0, 0};
    items[n++] = {kLineZ, 1, 0};
    for (int i = 0; i < 3; i++) {
        items[n++] = {kMarkZ[i + 1], 2, i};
        items[n++] = {kMarkZ[i + 1], 2, i + 3};
    }
    for (int i = 0; i < int(sizeof kProps / sizeof kProps[0]); i++) items[n++] = {kProps[i].z, 3, i};

    std::sort(items, items + n, [](const Item& a, const Item& b) {
        if (a.z != b.z) return a.z < b.z;
        return a.kind < b.kind;
    });

    for (int i = 0; i < n; i++) {
        const Item& it = items[i];
        if (it.kind == 0) {
            int fr = 0;
            if (phase_ == Phase::Stride) fr = step_ < 0.5f ? 0 : 1;
            const gs::Mipped& body = fell_ ? art_.fallen : art_.walk[fr];
            float h = fell_ ? walkerH_ * 0.48f : walkerH_;
            spr(art_.shadow, walkerX_ + shx, walkerFeet_ + 2.f, h * (fell_ ? 0.72f : 0.30f), PAL_FX, false, 0, false,
                true);
            spr(body, walkerX_ + shx, fell_ ? walkerFeet_ - 1.f : walkerFeet_, h, PAL_FIGURE, false, walkerFog_, true,
                false);
            if (puff_ > 0.f && !fell_) {
                float u = 1.f - puff_ / 0.26f;
                spr(art_.dust, walkerX_ + shx, walkerFeet_ - u * 7.f, 8.f + u * 10.f, PAL_FX, false, walkerFog_, false,
                    false);
            }
            continue;
        }
        if (it.kind == 1) {
            Proj p = project(0.f, kLineZ);
            if (!p.ok) continue;
            float w = std::min(280.f, kRoadHalf * 1.85f * p.ppm);
            sprBox(art_.stripe, p.x + shx, p.y, w, std::max(3.f, p.ppm * 0.12f), signalPal(), fogFor(kLineZ));
            continue;
        }
        if (it.kind == 2) {
            int which = it.id % 3;
            float side = it.id < 3 ? -1.f : 1.f;
            float cz = kMarkZ[which + 1];
            Proj p = project(side * (kRoadHalf + 0.55f), cz);
            if (!p.ok) continue;
            bool third = which == 2;
            float bh = std::clamp((third ? 1.85f : 1.2f) * p.ppm, 5.f, 96.f);
            spr(art_.crate[which], p.x + shx, p.y, bh, PAL_WOOD, side > 0, fogFor(cz), true, false);
            continue;
        }
        const Prop& pr = kProps[it.id];
        Proj p = project(pr.lat, pr.z);
        if (!p.ok) continue;
        float h = std::clamp(pr.h * p.ppm, 4.f, 130.f);
        int fog = fogFor(pr.z);
        if (pr.kind == 0) {
            spr(art_.boxcar, p.x + shx, p.y, h, PAL_BRICK, pr.lat > 0, fog, true, false);
        } else if (pr.kind == 1) {
            spr(art_.loco, p.x + shx, p.y, h, PAL_METAL, false, fog, true, false);
        } else if (pr.kind == 2) {
            spr(art_.drum, p.x + shx, p.y, h, PAL_METAL, pr.lat < 0, fog, true, false);
        } else if (pr.kind == 3) {
            spr(art_.sack, p.x + shx, p.y, h, PAL_WOOD, false, fog, true, false);
        } else {
            spr(art_.post, p.x + shx, p.y, h, PAL_METAL, false, fog, true, false);
            spr(art_.lamp, p.x + shx, p.y - h, h * 0.32f, PAL_GOLD, false, fog, false, false);
        }
    }
}

void Game::drawSky(float shx) {
    float drift = std::sin(t_ * 0.7f) * 8.f;
    spr(art_.steam, 108.f + drift + shx, 22.f, 16.f, PAL_SOOT, false, 2, false, false);
    spr(art_.steam, 128.f + drift * 0.6f + shx, 16.f + std::sin(t_ * 1.1f) * 2.f, 12.f, PAL_SOOT, true, 4, false,
        false);
    spr(art_.stack, 96.f + shx, kHorizon - 1.f, 72.f, PAL_SOOT, false, 8, true, false);
    spr(art_.tower, 228.f + shx, kHorizon - 1.f, 50.f, PAL_SOOT, false, 9, true, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 47.f) * 3.2f * std::min(shake_, 1.f);
    drawRoad(shx);

    const char* word = "DEPOT PACE";
    int wordPal = PAL_GOLD;
    float wordSc = 1.0f;
    if (mode_ == Mode::Victory) {
        word = "HELD";
        wordPal = PAL_GOOD;
        wordSc = 1.3f;
    } else if (mode_ == Mode::Over) {
        word = "SHED";
        if (reason_ && std::strcmp(reason_, "TOO SOON") == 0) word = "TOO SOON";
        else if (reason_ && std::strcmp(reason_, "MISSED") == 0) word = "MISSED";
        wordPal = PAL_ALERT;
        wordSc = 1.05f;
    } else if (mode_ == Mode::Pause) {
        word = "PAUSED";
    } else if (mode_ == Mode::Play) {
        if (pace_ <= 0) word = "WAIT";
        else if (pace_ == 1) word = "ONE";
        else if (pace_ == 2) word = "TWO";
        else {
            word = "FIRE";
            wordPal = PAL_LIVE;
            wordSc = 1.35f;
        }
    }
    text(word, 160.f + shx, 58.f, wordSc, wordPal);

    for (int i = 0; i < 3; i++) {
        int pal = PAL_METAL;
        float s = 11.f;
        if (mode_ == Mode::Over) {
            if (i < pace_) pal = PAL_ALERT;
        } else if (i < pace_ || mode_ == Mode::Victory) {
            pal = (i == 2 && (pace_ >= 3 || mode_ == Mode::Victory)) ? PAL_LIVE : PAL_GOLD;
            if (i == 2 && pace_ >= 3 && mode_ != Mode::Over) s = 15.f + std::sin(t_ * 8.f) * 1.4f;
        }
        spr(art_.pip, 136.f + float(i) * 24.f + shx, 82.f, s, pal, false, 0, false, false);
    }

    int beadPal = signalPal();
    if (mode_ == Mode::Play && pace_ < 3) beadPal = PAL_HOLD;
    if (mode_ != Mode::Pause) spr(art_.bead, sightX_ + shx, walkerChest_, 16.f, beadPal, false, 0, false, false);
    if (flash_ > 0.f) {
        spr(art_.flash, flashX_ + shx, flashY_, 18.f + (0.16f - flash_) * 48.f, PAL_FX, false, 0, false, false);
        float gunX = 160.f + (sightX_ - 160.f) * 0.35f;
        spr(art_.dust, gunX + shx, 176.f, 14.f, PAL_FX, false, 0, false, false);
    }

    float gunX = 160.f + (sightX_ - 160.f) * 0.35f;
    spr(art_.rifle, gunX + shx, 230.f, 62.f, PAL_METAL, false, 0, true, false);
    drawShed(shx);
    drawWorld(shx);
    drawSky(shx);

    if (mode_ == Mode::Title) {
        hudC(20, "YOU HAVE THE DEPOT", PAL_TEXT);
        hudC(21, "WAIT UNTIL THE THIRD PACE", PAL_GOLD);
        hudC(22, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        hudC(24, "ARROWS AIM    Z FIRES", PAL_TEXT);
        hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(22, "FIRED ON THE THIRD PACE", PAL_GOOD);
        hudC(23, "THE DEPOT HOLDS", PAL_TEXT);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Over) {
        hudC(22, reason_ ? reason_ : "LOST", PAL_ALERT);
        hudC(23, "ANYTHING ELSE IS A LOSS", PAL_TEXT);
        hudC(26, "START RETRIES", PAL_TEXT);
    } else if (pace_ < 3) {
        hud(1, 1, "HOLD FIRE", PAL_ALERT);
        hud(36, 1, pace_ <= 0 ? "0/3" : pace_ == 1 ? "1/3" : "2/3", PAL_GOLD);
        hudC(23, "WAIT UNTIL THE THIRD PACE", PAL_TEXT);
        hudC(24, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
    } else {
        hud(1, 1, "THIRD PACE", PAL_GOOD);
        hud(36, 1, "3/3", PAL_GOOD);
        hudC(23, "FIRE", PAL_GOOD);
        hudC(24, "BEFORE THE SHED", PAL_TEXT);
    }
}

}  // namespace depotpace
