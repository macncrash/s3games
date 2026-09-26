#include "game/pace.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace pace {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 90.f;
constexpr float kZNear = 2.35f;
constexpr float kPpm = 74.f;
constexpr float kRoadHalf = 3.2f;
constexpr float kWalkerH = 1.82f;
constexpr float kZ0 = 16.2f;
constexpr float kLine = 4.12f;
constexpr float kThroughZ = 3.15f;
constexpr float kIntro = 0.70f;
constexpr float kHit = 28.f;
constexpr float kPi = 3.14159265f;

struct Mark {
    float z, lat;
};
constexpr Mark kEnd[4] = {
    {0.f, 0.f},
    {11.6f, -1.15f},
    {7.8f, 2.05f},
    {4.45f, 0.12f},
};

struct Prop {
    float z, lat, h;
    int kind;
};

constexpr Prop kProps[] = {
    {6.6f, -5.4f, 3.1f, 0}, {6.6f, 5.4f, 2.8f, 0}, {10.4f, 5.5f, 3.3f, 0},
    {10.4f, -5.3f, 2.9f, 0}, {14.8f, -5.2f, 3.2f, 0}, {14.8f, 5.2f, 3.0f, 0},
};

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

float Game::holdDur() const { return pace_ >= 3 ? 0.70f : 0.46f; }
float Game::strideDur() const { return pace_ >= 3 ? 0.88f : 0.64f; }

int Game::fogFor(float z) const {
    float t = std::clamp((z - 5.f) / 16.f, 0.f, 1.f);
    return int(t * 12.f);
}

Game::Proj Game::project(float lat, float z) const {
    Proj p;
    if (!(z > 0.45f)) return p;
    float t = kZNear / z;
    p.ppm = kPpm * t;
    p.y = kHorizon + t * (float(gs::SCREEN_H) - kHorizon);
    p.x = 160.f + lat * p.ppm;
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
}

void Game::beginPace(int n) {
    pace_ = n;
    phase_ = Phase::Hold;
    phaseT_ = 0.f;
    step_ = 0.f;
    fromZ_ = z_;
    fromLat_ = lat_;
    toZ_ = kEnd[n].z;
    toLat_ = kEnd[n].lat;
    puff_ = 0.28f;
    sys_->apu.noiseBurst(0.28f, n == 3 ? 900.f : 520.f, 0.1f);
    if (n == 3) blip(880.f, 0.07f, 0.16f);
    else blip(220.f + float(n) * 40.f, 0.04f, 0.08f);
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
    return edge || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_X) ||
           p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_TURBO);
}

void Game::measure() {
    Proj p = project(lat_, z_);
    walkerX_ = p.x;
    walkerH_ = std::clamp(kWalkerH * p.ppm, 4.f, 160.f);
    walkerFeet_ = p.y;
    float bob = 0.f;
    if (phase_ == Phase::Stride) bob = std::sin(step_ * kPi) * std::min(6.f, walkerH_ * 0.1f);
    else bob = std::sin(t_ * 2.2f) * 1.2f;
    walkerFeet_ -= bob;
    walkerChest_ = walkerFeet_ - walkerH_ * 0.62f;
    walkerFog_ = fogFor(z_);
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
        sightX_ += dir * 320.f * kDt;
    }
    sightX_ = std::clamp(sightX_, 72.f, 248.f);
    if (bot_) {
        bool third = pace_ == 3 && (phase_ == Phase::Hold || phase_ == Phase::Stride);
        bool settled = phase_ == Phase::Stride || phaseT_ > 0.16f;
        bool aligned = std::fabs(walkerX_ - sightX_) < 12.f;
        return third && settled && aligned && !shot_;
    }
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
    sys_->apu.tone(0, 523.f, 0.08f);
    sys_->apu.noiseBurst(0.55f, 180.f, 0.18f);
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
    sys_->apu.tone(0, 146.f, 0.07f);
}

void Game::resolveShot() {
    shot_ = true;
    shotPace_ = pace_;
    flash_ = 0.16f;
    flashX_ = sightX_;
    flashY_ = walkerChest_;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.62f, 1600.f, 0.18f);
    if (pace_ != 3) {
        lose("TOO SOON");
        return;
    }
    if (std::fabs(sightX_ - walkerX_) <= kHit) win();
    else lose("MISSED");
}

void Game::updatePlay() {
    t_ += kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.8f);
    if (flash_ > 0.f) flash_ = std::max(0.f, flash_ - kDt);
    if (puff_ > 0.f) puff_ = std::max(0.f, puff_ - kDt);

    phaseT_ += kDt;
    if (phase_ == Phase::Stride) {
        float u = smooth(std::min(1.f, phaseT_ / strideDur()));
        z_ = lerpf(fromZ_, toZ_, u);
        lat_ = lerpf(fromLat_, toLat_, u);
        step_ = u;
    }
    measure();
    bool want = steer();
    if (want && !shot_) {
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
        return;
    }
    z_ = toZ_;
    lat_ = toLat_;
    if (pace_ >= 3) {
        z_ = kThroughZ;
        lat_ = 0.05f;
        lose("THROUGH THE GATE");
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
        static const float good[] = {523.f, 659.f, 784.f, 1046.f};
        static const float bad[] = {196.f, 146.f, 110.f};
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
    t_ += (mode_ == Mode::Play) ? 0.f : kDt;
    if (mode_ != Mode::Play) {
        if (flash_ > 0.f) flash_ = std::max(0.f, flash_ - kDt);
        if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.8f);
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
    int iw = std::clamp(int(std::lround(w)), 1, 2000);
    int ih = std::clamp(int(std::lround(h)), 1, 2000);
    s.w = int16_t(iw);
    s.h = int16_t(ih);
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
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
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
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
    v.setFogColor(mode_ == Mode::Over ? gs::rgb4(8, 3, 4) : gs::rgb4(6, 4, 6));
    const float span = float(gs::SCREEN_H) - kHorizon;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            int r = std::clamp(int(1.f + u * 11.f), 0, 15);
            int g = std::clamp(int(1.f + u * 5.f), 0, 15);
            int b = std::clamp(int(8.f - u * 5.f), 0, 15);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) + 0.5f - kHorizon) / span;
        t = std::max(t, 0.012f);
        float wz = kZNear / t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + shx;
        rd.hw = std::max(2.f, kRoadHalf * kPpm * t);
        rd.v = wz * 36.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 1;
        rd.band = (int(std::floor(wz * 0.28f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((wz - 8.f) / 22.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 13.f);
        v.lineBackdrop[y] = gs::rgb4(2, 3, 2);
    }
    v.roadTime = int(t_ * 60.f);
}

void Game::drawGate(float shx) {
    int lampPal = PAL_GOLD;
    if (mode_ == Mode::Victory || (mode_ == Mode::Play && pace_ >= 3)) lampPal = PAL_LIVE;
    if (mode_ == Mode::Over) lampPal = PAL_ALERT;
    spr(art_.pier, 30.f + shx, 224.f, 168.f, PAL_STONE, false, 0, true, false);
    spr(art_.pier, 290.f + shx, 224.f, 168.f, PAL_STONE, true, 0, true, false);
    for (int i = 0; i < 7; i++) {
        float x = 46.f + float(i) * 38.f;
        spr(art_.block, x + shx, 16.f, 20.f, PAL_STONE, false, 0, false, false);
    }
    spr(art_.keystone, 160.f + shx, 30.f, 34.f, PAL_STONE, false, 0, false, false);
    float bob = std::sin(t_ * 6.f) * 1.1f;
    spr(art_.lamp, 36.f + shx, 96.f + bob, 18.f, lampPal, false, 0, false, false);
    spr(art_.lamp, 284.f + shx, 96.f + bob, 18.f, lampPal, true, 0, false, false);
}

void Game::drawWorld(float shx) {
    struct Item {
        float z;
        int kind;
        int id;
    };
    Item items[16];
    int n = 0;
    items[n++] = {z_, 0, 0};
    float marks[3] = {kZ0, kEnd[1].z, kEnd[2].z};
    for (int i = 0; i < 3; i++) {
        items[n++] = {marks[i], 1, i};
        items[n++] = {marks[i], 1, i + 3};
    }
    items[n++] = {kLine, 2, 0};
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
            float h = fell_ ? walkerH_ * 0.42f : walkerH_;
            float y = fell_ ? walkerFeet_ - 4.f : walkerFeet_;
            spr(art_.shadow, walkerX_ + shx, walkerFeet_ + 2.f, h * 0.28f, PAL_FX, false, 0, false, true);
            spr(body, walkerX_ + shx, y, h, PAL_FIGURE, false, walkerFog_, !fell_, false);
            if (puff_ > 0.f && !fell_) {
                float u = 1.f - puff_ / 0.28f;
                spr(art_.dust, walkerX_ + shx, walkerFeet_ - u * 6.f, 10.f + u * 8.f, PAL_FX, false, walkerFog_, false, false);
            }
            continue;
        }
        if (it.kind == 2) {
            Proj p = project(0.f, kLine);
            if (!p.ok) continue;
            float w = std::min(280.f, kRoadHalf * 1.7f * p.ppm);
            sprBox(art_.stripe, p.x + shx, p.y, w, std::max(3.f, p.ppm * 0.14f), PAL_ALERT, fogFor(kLine));
            continue;
        }
        if (it.kind == 1) {
            int which = it.id % 3;
            float side = it.id < 3 ? -1.f : 1.f;
            float bz = marks[which];
            Proj p = project(side * (kRoadHalf + 0.28f), bz);
            if (!p.ok) continue;
            bool third = which == 2;
            int pal = PAL_STONE;
            if (third && (pace_ >= 3 || mode_ == Mode::Victory)) pal = PAL_LIVE;
            else if (third) pal = PAL_GOLD;
            else if (pace_ > which) pal = PAL_GOLD;
            float bh = (third ? 1.7f : 1.15f) * p.ppm;
            spr(art_.bollard, p.x + shx, p.y, std::clamp(bh, 4.f, 80.f), pal, side > 0, fogFor(bz), true, false);
            continue;
        }
        const Prop& pr = kProps[it.id];
        Proj p = project(pr.lat, pr.z);
        if (!p.ok) continue;
        float h = std::clamp(pr.h * p.ppm, 4.f, 120.f);
        spr(art_.tree, p.x + shx, p.y, h, PAL_TREE, pr.lat > 0, fogFor(pr.z), true, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 47.f) * 3.2f * std::min(shake_, 1.f);
    drawRoad(shx);

    if (mode_ == Mode::Title) {
        text("GATE PACE", 160.f, 42.f, 1.05f, PAL_GOLD);
    } else if (mode_ == Mode::Victory) {
        text("DONE", 160.f, 42.f, 1.15f, PAL_GOOD);
    } else if (mode_ == Mode::Over) {
        const char* word = "MISSED";
        if (reason_ && std::string(reason_) == "TOO SOON") word = "TOO SOON";
        else if (reason_ && std::string(reason_) == "THROUGH THE GATE") word = "THROUGH";
        text(word, 160.f, 42.f, 1.f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 42.f, 1.f, PAL_GOLD);
    } else if (pace_ == 0) {
        text("WAIT", 160.f, 42.f, 1.05f, PAL_GOLD);
    } else if (pace_ < 3) {
        text(pace_ == 1 ? "ONE" : "TWO", 160.f, 42.f, 1.1f, PAL_GOLD);
    } else {
        text("FIRE", 160.f, 42.f, 1.2f, PAL_LIVE);
    }

    for (int i = 0; i < 3; i++) {
        int pal = PAL_STONE;
        if (pace_ > i) pal = (i == 2) ? PAL_LIVE : PAL_GOLD;
        float s = (i == 2 && pace_ >= 3) ? 16.f : 11.f;
        spr(art_.pip, 136.f + float(i) * 24.f, 68.f, s, pal, false, 0, false, false);
    }

    if (mode_ == Mode::Play || flash_ > 0.f) {
        int beadPal = (pace_ >= 3 && mode_ == Mode::Play) || mode_ == Mode::Victory ? PAL_LIVE : PAL_HOLD;
        if (mode_ == Mode::Play) spr(art_.bead, sightX_ + shx, walkerChest_, 16.f, beadPal, false, 0, false, false);
        if (flash_ > 0.f) spr(art_.flash, flashX_ + shx, flashY_, 22.f + (0.16f - flash_) * 40.f, PAL_FX, false, 0, false, false);
    }

    float gunX = 160.f + (sightX_ - 160.f) * 0.42f;
    spr(art_.barrel, gunX + shx, 230.f, 58.f, PAL_METAL, false, 0, true, false);
    drawGate(shx);
    drawWorld(shx);
    spr(art_.moon, 262.f, 36.f, 16.f, PAL_NIGHT, false, 0, false, false);
    spr(art_.cloud, 78.f + std::sin(t_ * 0.2f) * 8.f, 52.f, 16.f, PAL_NIGHT, false, 0, false, false);
    spr(art_.cloud, 214.f + std::sin(t_ * 0.15f) * 6.f, 60.f, 13.f, PAL_NIGHT, true, 0, false, false);

    if (mode_ == Mode::Title) {
        hudC(21, "ONE GATE", PAL_TEXT);
        hudC(22, "WAIT UNTIL THE THIRD PACE", PAL_GOLD);
        hudC(23, "THEN FIRE", PAL_GOOD);
        hudC(25, "ARROWS AIM    Z FIRES", PAL_TEXT);
        hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(23, "FIRED ON THE THIRD PACE", PAL_GOOD);
        hudC(24, "THE GATE HOLDS", PAL_TEXT);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Over) {
        hudC(23, reason_, PAL_ALERT);
        hudC(24, "THE WATCH IS OVER", PAL_TEXT);
        hudC(26, "START RETRIES", PAL_TEXT);
    } else if (pace_ < 3) {
        hudC(0, pace_ == 0 ? "WAIT" : "HOLD", PAL_GOLD);
        hudC(24, "WAIT UNTIL THE THIRD PACE", PAL_TEXT);
        hudC(25, "DO NOT FIRE", PAL_ALERT);
    } else {
        hudC(0, "THIRD PACE", PAL_GOOD);
        hudC(24, "FIRE", PAL_GOOD);
        hudC(25, "BEFORE THEY CROSS", PAL_TEXT);
    }
}

}  // namespace pace
