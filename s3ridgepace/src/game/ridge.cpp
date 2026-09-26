#include "game/ridge.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace rpace {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 86.f;
constexpr float kZNear = 2.15f;
constexpr float kPpm = 86.f;
constexpr float kRoadHalf = 2.15f;
constexpr float kWalkerH = 1.78f;
constexpr float kZ0 = 16.8f;
constexpr float kLineZ = 3.9f;
constexpr float kThroughZ = 3.12f;
constexpr float kIntro = 0.85f;
constexpr float kPi = 3.14159265f;

constexpr float kMarkZ[4] = {0.f, 12.6f, 8.15f, 5.2f};
constexpr float kMarkLat[4] = {0.f, -0.28f, 0.32f, 0.04f};

struct Prop {
    float z, lat, h;
    int kind;
};

constexpr Prop kProps[] = {
    {15.4f, -4.6f, 4.8f, 0}, {14.8f, 4.8f, 5.2f, 0}, {12.9f, -5.0f, 4.0f, 0}, {12.2f, 4.4f, 4.6f, 0},
    {10.4f, -4.7f, 3.5f, 0}, {9.6f, 5.1f, 3.8f, 0},  {11.5f, -3.6f, 2.2f, 1}, {8.8f, 3.8f, 1.8f, 1},
    {13.6f, 3.5f, 2.4f, 1},  {7.2f, -4.2f, 1.6f, 1}, {14.0f, -3.3f, 0.9f, 2}, {10.8f, 3.2f, 0.8f, 2},
    {8.4f, -3.4f, 0.7f, 2},  {6.4f, 3.1f, 0.75f, 2}, {15.0f, 3.0f, 0.55f, 3}, {11.0f, -2.9f, 0.5f, 3},
    {9.0f, 2.85f, 0.48f, 3}, {6.8f, -3.0f, 0.5f, 3},
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

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Over) return 4;
    if (pace_ >= 3) return 2;
    return 1;
}

float Game::holdDur() const { return pace_ >= 3 ? 1.05f : 0.42f; }
float Game::strideDur() const { return pace_ >= 3 ? 1.00f : 0.56f; }

float Game::bendAt(float row) const {
    float crest = std::sin(row * 0.020f) * 24.f + std::sin(row * 0.0075f + 0.4f) * 8.f;
    float wind = std::sin(t_ * 0.7f) * 2.4f * std::clamp(row / 90.f, 0.f, 1.f);
    return crest + wind;
}

int Game::fogFor(float z) const {
    float t = std::clamp((z - 7.f) / 14.f, 0.f, 1.f);
    return int(t * 11.f);
}

float Game::reach() const { return std::max(30.f, walkerH_ * 0.85f); }

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
    if (sys_) sys_->setLight(180, 120, 40);
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
    puff_ = 0.24f;
    sys_->apu.noiseBurst(0.22f, n == 3 ? 700.f : 380.f, 0.08f);
    if (n == 3) {
        blip(392.f, 0.07f, 0.18f);
        sys_->setLight(80, 180, 70);
    } else {
        blip(180.f + float(n) * 48.f, 0.045f, 0.08f);
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
    walkerH_ = std::clamp(kWalkerH * p.ppm, 4.f, 180.f);
    walkerFeet_ = p.y;
    float bob = 0.f;
    if (!fell_ && phase_ == Phase::Stride) bob = std::sin(step_ * kPi) * std::min(6.f, walkerH_ * 0.12f);
    else if (!fell_) bob = std::sin(t_ * 2.3f) * 1.1f;
    walkerFeet_ -= bob;
    walkerChest_ = walkerFeet_ - walkerH_ * 0.58f;
    walkerFog_ = fogFor(z_);
}

bool Game::aim() {
    if (bot_) {
        float dx = walkerX_ - sightX_;
        float maxStep = 960.f * kDt;
        if (std::fabs(dx) <= maxStep) sightX_ = walkerX_;
        else sightX_ += std::copysign(maxStep, dx);
        sightX_ = std::clamp(sightX_, 4.f, 316.f);
        if (shot_ || pace_ < 3) return false;
        bool stepping = phase_ == Phase::Stride && phaseT_ > 0.10f;
        if (phase_ == Phase::Stride && phaseT_ > 0.45f) sightX_ = walkerX_;
        if (!stepping) return false;
        return std::fabs(walkerX_ - sightX_) <= reach();
    }
    float dir = 0.f;
    if (sys_->pad.down(gs::BTN_LEFT)) dir -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) dir += 1.f;
    if (std::fabs(sys_->pad.axisX) > 0.22f) dir = sys_->pad.axisX;
    sightX_ += dir * 360.f * kDt;
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
    sys_->apu.noiseBurst(0.5f, 200.f, 0.16f);
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
    fanT_ = 1.f;
    blip_ = 0.f;
    sys_->setLight(180, 30, 20);
}

void Game::resolveShot() {
    shot_ = true;
    shotPace_ = pace_;
    flash_ = 0.18f;
    flashX_ = sightX_;
    flashY_ = walkerChest_;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.7f, 1500.f, 0.18f);
    sys_->rumble(0.55f, 0.9f, 70);
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
    t_ += kDt;
    if (flash_ > 0.f) flash_ = std::max(0.f, flash_ - kDt);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 2.2f);
    if (puff_ > 0.f) puff_ = std::max(0.f, puff_ - kDt);

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
    if (!(h > 1.5f) || m.h < 1) return;
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

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog) {
    if (!(h > 1.f) || !(w > 1.f) || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -500, 500));
    s.y = int16_t(std::clamp(int(std::lround(cy - s.h * 0.5f)), -500, 500));
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
    uint16_t skyTop = gs::rgb4(2, 4, 9);
    uint16_t skyHor = gs::rgb4(14, 8, 5);
    if (mode_ == Mode::Over) skyHor = gs::rgb4(11, 4, 3);
    else if (mode_ == Mode::Victory) skyHor = gs::rgb4(12, 9, 6);
    v.setFogColor(skyHor);
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
        t = std::max(t, 0.018f);
        float wz = kZNear / t;
        float row = float(y) - kHorizon;
        rd.on = true;
        rd.cx = 160.f + bendAt(row) + shx;
        rd.hw = std::max(5.f, kRoadHalf * kPpm * t);
        rd.v = wz * 42.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = gs::ROAD_ROCKY;
        rd.band = (int(std::floor(wz * 0.33f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_DROP;
        rd.right = gs::GROUND_DROP;
        float fogT = std::clamp((wz - 6.f) / 18.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 12.f);
        float dropT = std::clamp(row / span, 0.f, 1.f);
        v.lineBackdrop[y] = mixC(gs::rgb4(4, 3, 3), gs::rgb4(1, 1, 2), dropT);
    }
}

void Game::drawKit(float shx) {
    int cloth = PAL_GOLD;
    if (mode_ == Mode::Victory || (mode_ == Mode::Play && pace_ >= 3)) cloth = PAL_LIVE;
    if (mode_ == Mode::Over) cloth = PAL_ALERT;
    spr(art_.post, 26.f + shx, 206.f, 96.f, PAL_WOOD, false, 0, true, false);
    int fr = std::sin(t_ * 7.f) > 0.f ? 0 : 1;
    spr(art_.pennant[fr], 40.f + shx, 112.f, 20.f, cloth, false, 0, false, false);
    spr(art_.lip, 52.f + shx, 228.f, 62.f, PAL_STONE, false, 0, true, false);
    spr(art_.lip, 286.f + shx, 232.f, 70.f, PAL_STONE, true, 0, true, false);
    float gunX = 160.f + (sightX_ - 160.f) * 0.38f;
    spr(art_.rifle, gunX + shx, 232.f, 64.f, PAL_WOOD, false, 0, true, false);
}

void Game::drawWorld(float shx) {
    struct Item {
        float z;
        int kind;
        int id;
    };
    Item items[48];
    int n = 0;
    items[n++] = {z_, 0, 0};
    items[n++] = {kLineZ, 1, 0};
    items[n++] = {kLineZ - 0.12f, 2, 0};
    items[n++] = {kLineZ - 0.12f, 2, 1};
    for (int i = 0; i < 3; i++) {
        items[n++] = {kMarkZ[i + 1], 4, i};
        items[n++] = {kMarkZ[i + 1], 4, i + 3};
    }
    items[n++] = {kMarkZ[3] - 0.08f, 3, 0};
    for (int i = 0; i < int(sizeof kProps / sizeof kProps[0]); i++) items[n++] = {kProps[i].z, 5, i};

    std::sort(items, items + n, [](const Item& a, const Item& b) {
        if (a.z != b.z) return a.z < b.z;
        return a.kind < b.kind;
    });

    int signal = PAL_STONE;
    if (mode_ == Mode::Victory || (mode_ == Mode::Play && pace_ >= 3)) signal = PAL_LIVE;
    else if (mode_ == Mode::Over) signal = PAL_ALERT;
    else signal = PAL_GOLD;

    for (int i = 0; i < n; i++) {
        const Item& it = items[i];
        if (it.kind == 0) {
            int fr = 0;
            if (phase_ == Phase::Stride) fr = step_ < 0.5f ? 0 : 1;
            const gs::Mipped& body = fell_ ? art_.fallen : art_.scout[fr];
            float h = fell_ ? walkerH_ * 0.46f : walkerH_;
            spr(art_.shadow, walkerX_ + shx, walkerFeet_ + 2.f, h * (fell_ ? 0.7f : 0.32f), PAL_FX, false, 0, false,
                true);
            spr(body, walkerX_ + shx, fell_ ? walkerFeet_ - 1.f : walkerFeet_, h, PAL_FIGURE, false, walkerFog_, true,
                false);
            if (puff_ > 0.f && !fell_) {
                float u = 1.f - puff_ / 0.24f;
                spr(art_.dust, walkerX_ + shx, walkerFeet_ - u * 8.f, 8.f + u * 10.f, PAL_FX, false, walkerFog_, false,
                    false);
            }
            continue;
        }
        if (it.kind == 1) {
            Proj p = project(0.f, kLineZ);
            if (!p.ok) continue;
            float w = std::min(300.f, kRoadHalf * 1.9f * p.ppm);
            sprBox(art_.stripe, p.x + shx, p.y, w, std::max(3.f, p.ppm * 0.1f),
                   mode_ == Mode::Over ? PAL_ALERT : PAL_GOLD, fogFor(kLineZ));
            continue;
        }
        if (it.kind == 2) {
            float side = it.id == 0 ? -1.f : 1.f;
            Proj p = project(side * (kRoadHalf + 0.08f), kLineZ - 0.12f);
            if (!p.ok) continue;
            float h = std::clamp(1.35f * p.ppm, 6.f, 90.f);
            spr(art_.stake, p.x + shx, p.y, h, signal, side > 0, fogFor(kLineZ), true, false);
            continue;
        }
        if (it.kind == 3) {
            Proj p = project(kRoadHalf + 0.2f, kMarkZ[3]);
            if (!p.ok) continue;
            float ch = std::clamp(1.7f * p.ppm, 6.f, 80.f);
            int ragPal = signal == PAL_STONE ? PAL_GOLD : signal;
            spr(art_.rag, p.x + shx, p.y - ch * 0.92f, std::max(6.f, ch * 0.28f), ragPal, false, fogFor(kMarkZ[3]),
                false, false);
            continue;
        }
        if (it.kind == 4) {
            int which = it.id % 3;
            float side = it.id < 3 ? -1.f : 1.f;
            float cz = kMarkZ[which + 1];
            Proj p = project(side * (kRoadHalf + 0.22f), cz);
            if (!p.ok) continue;
            bool third = which == 2;
            float h = std::clamp((third ? 1.7f : 1.15f) * p.ppm, 5.f, 84.f);
            spr(art_.cairn[third ? 1 : 0], p.x + shx, p.y, h, PAL_STONE, side > 0, fogFor(cz), true, false);
            continue;
        }
        const Prop& pr = kProps[it.id];
        float lat = pr.lat;
        if (pr.kind == 3) lat += std::sin(t_ * 2.4f + pr.z) * 0.06f;
        Proj p = project(lat, pr.z);
        if (!p.ok) continue;
        float h = std::clamp(pr.h * p.ppm, 3.f, 130.f);
        int fog = fogFor(pr.z);
        if (pr.kind == 0) spr(art_.pine, p.x + shx, p.y, h, PAL_PINE, pr.lat > 0, fog, true, false);
        else if (pr.kind == 1) spr(art_.crag, p.x + shx, p.y, h, PAL_STONE, pr.lat > 0, fog, true, false);
        else if (pr.kind == 2) spr(art_.bush, p.x + shx, p.y, h, PAL_PINE, pr.lat < 0, fog, true, false);
        else spr(art_.grass, p.x + shx, p.y, h, PAL_PINE, false, fog, true, false);
    }
}

void Game::drawSky(float shx) {
    spr(art_.hawk[int(t_ * 4.f) & 1], 250.f + std::sin(t_ * 0.45f) * 28.f + shx, 36.f + std::sin(t_ * 0.9f) * 5.f,
        12.f, PAL_FX, std::sin(t_ * 0.45f) > 0.f, 2, false, false);
    spr(art_.peak, 78.f + shx, kHorizon + 2.f, 58.f, PAL_SKY, false, 6, true, false);
    spr(art_.peak, 168.f + shx, kHorizon + 4.f, 34.f, PAL_SKY, true, 9, true, false);
    spr(art_.peak, 252.f + shx, kHorizon + 1.f, 50.f, PAL_SKY, true, 7, true, false);
    spr(art_.sun, 42.f + shx, 68.f, 28.f, PAL_SKY, false, 1, false, false);
    spr(art_.cloud, 120.f + std::sin(t_ * 0.16f) * 18.f + shx, 28.f, 16.f, PAL_SKY, false, 3, false, false);
    spr(art_.cloud, 210.f + std::sin(t_ * 0.12f + 1.f) * 14.f + shx, 48.f, 12.f, PAL_SKY, true, 4, false, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 48.f) * 3.4f * std::min(shake_, 1.f);
    drawRoad(shx);

    const char* word = "RIDGE PACE";
    int wordPal = PAL_GOLD;
    float wordSc = 1.02f;
    if (mode_ == Mode::Victory) {
        word = "HELD";
        wordPal = PAL_GOOD;
        wordSc = 1.28f;
    } else if (mode_ == Mode::Over) {
        word = reason_ && reason_[0] ? reason_ : "OVER";
        wordPal = PAL_ALERT;
        wordSc = 1.05f;
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
            wordSc = 1.3f;
        }
    }
    text(word, 160.f + shx, 32.f, wordSc, wordPal);

    for (int i = 0; i < 3; i++) {
        int pal = PAL_STONE;
        float s = 10.f;
        if (mode_ == Mode::Over) {
            if (i < pace_) pal = PAL_ALERT;
        } else if (i < pace_ || mode_ == Mode::Victory) {
            pal = (i == 2 && pace_ >= 3) ? PAL_LIVE : PAL_GOLD;
            if (i == 2 && pace_ >= 3) s = 15.f + std::sin(t_ * 8.f) * 1.2f;
        }
        spr(art_.pip, 136.f + float(i) * 24.f + shx, 56.f, s, pal, false, 0, false, false);
    }

    int beadPal = PAL_HOLD;
    if (mode_ == Mode::Victory || (mode_ == Mode::Play && pace_ >= 3)) beadPal = PAL_LIVE;
    if (mode_ == Mode::Over) beadPal = PAL_ALERT;
    if (mode_ != Mode::Pause) spr(art_.bead, sightX_ + shx, walkerChest_, 16.f, beadPal, false, 0, false, false);
    if (flash_ > 0.f) {
        spr(art_.flash, flashX_ + shx, flashY_, 18.f + (0.18f - flash_) * 50.f, PAL_FX, false, 0, false, false);
        float gunX = 160.f + (sightX_ - 160.f) * 0.38f;
        spr(art_.dust, gunX + shx, 176.f, 14.f, PAL_FX, false, 0, false, false);
    }

    drawKit(shx);
    drawWorld(shx);
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

}  // namespace rpace
