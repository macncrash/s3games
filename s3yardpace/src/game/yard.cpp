#include "game/yard.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace yardpace {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 84.f;
constexpr float kZNear = 2.2f;
constexpr float kPpm = 82.f;
constexpr float kRoadHalf = 4.6f;
constexpr float kWalkerH = 1.78f;
constexpr float kZ0 = 14.6f;
constexpr float kLine = 4.05f;
constexpr float kThrough = 2.75f;
constexpr float kIntro = 0.64f;
constexpr float kStride = 0.56f;
constexpr float kHold = 0.44f;
constexpr float kHold3 = 0.92f;
constexpr float kCross = 0.70f;
constexpr float kPi = 3.14159265f;

struct Mark {
    float z, lat;
};
constexpr Mark kMark[4] = {
    {0.f, 0.f},
    {11.4f, -1.05f},
    {7.6f, 1.25f},
    {4.9f, -0.18f},
};

struct Prop {
    float z, lat, h;
    int kind;
};

constexpr Prop kProps[] = {
    {13.4f, -4.6f, 1.55f, 0}, {13.0f, 4.8f, 1.40f, 0}, {9.2f, -3.6f, 1.50f, 0},
    {9.5f, 3.5f, 1.35f, 0},   {6.4f, 2.8f, 1.20f, 0},  {6.8f, -2.7f, 0.62f, 1},
    {5.2f, 2.5f, 0.55f, 1},   {8.4f, -2.4f, 0.38f, 2}, {4.4f, -2.35f, 0.62f, 3},
    {11.8f, 3.2f, 2.00f, 4},  {7.4f, -3.1f, 1.80f, 4}, {10.6f, 2.5f, 0.80f, 5},
};

float lerpf(float a, float b, float u) { return a + (b - a) * u; }

float smooth(float u) {
    u = std::clamp(u, 0.f, 1.f);
    return u * u * (3.f - 2.f * u);
}

uint16_t mix4(uint16_t a, uint16_t b, float u) {
    auto ch = [](int x, int y, float t) { return std::clamp(int(std::lround(x + (y - x) * t)), 0, 15); };
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(ch(ar, br, u), ch(ag, bg, u), ch(ab, bb, u));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Over) return 4;
    if (pace_ >= 3) return 2;
    return 1;
}

float Game::phaseDur() const {
    if (phase_ == Phase::Intro) return kIntro;
    if (phase_ == Phase::Hold) return pace_ >= 3 ? kHold3 : kHold;
    if (phase_ == Phase::Cross) return kCross;
    return kStride;
}

int Game::fogFor(float z) const {
    float t = std::clamp((z - 4.f) / 14.f, 0.f, 1.f);
    return int(t * 11.f);
}

int Game::lampPal() const {
    if (mode_ == Mode::Over) return PAL_ALERT;
    if (mode_ == Mode::Victory || (mode_ == Mode::Play && pace_ >= 3)) return PAL_LIVE;
    return PAL_GOLD;
}

Game::Proj Game::project(float lat, float z) const {
    Proj p;
    if (!(z > 0.4f)) return p;
    float t = kZNear / z;
    p.ppm = kPpm * t;
    p.y = kHorizon + t * (float(gs::SCREEN_H) - kHorizon);
    p.x = 160.f + lat * p.ppm;
    p.ok = true;
    return p;
}

float Game::reach() const { return std::clamp(20.f + walkerH_ * 0.42f, 28.f, 52.f); }

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
    facingRight_ = true;
    won_ = false;
    over_ = false;
    flash_ = 0.f;
    shake_ = 0.f;
    puff_ = 0.f;
    sightX_ = 160.f;
    walkerX_ = 160.f;
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
    if (sys_) sys_->setLight(210, 130, 40);
}

void Game::beginStride(int n) {
    pace_ = n;
    phase_ = Phase::Stride;
    phaseT_ = 0.f;
    step_ = 0.f;
    fromZ_ = z_;
    fromLat_ = lat_;
    toZ_ = kMark[n].z;
    toLat_ = kMark[n].lat;
    facingRight_ = toLat_ >= fromLat_;
    puff_ = 0.26f;
    sys_->apu.noiseBurst(0.22f, n == 3 ? 640.f : 380.f, 0.08f);
    if (n == 3) {
        blip(392.f, 0.07f, 0.16f);
        sys_->setLight(40, 180, 70);
    } else {
        blip(180.f + float(n) * 48.f, 0.045f, 0.08f);
        sys_->setLight(200, 120, 36);
    }
}

void Game::beginHold() {
    phase_ = Phase::Hold;
    phaseT_ = 0.f;
    step_ = 0.f;
    z_ = toZ_;
    lat_ = toLat_;
    if (pace_ >= 3) {
        blip(523.f, 0.08f, 0.18f);
        sys_->setLight(50, 200, 80);
    }
}

void Game::beginCross() {
    phase_ = Phase::Cross;
    phaseT_ = 0.f;
    step_ = 0.f;
    fromZ_ = z_;
    fromLat_ = lat_;
    toZ_ = kThrough;
    toLat_ = 0.04f;
    facingRight_ = true;
    puff_ = 0.2f;
    blip(146.f, 0.05f, 0.1f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    t_ = 0.f;
    resetPose();
    mode_ = Mode::Title;
    sys.setLight(210, 130, 40);
    if (bot_) beginWatch();
}

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A);
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
    walkerH_ = std::clamp(kWalkerH * p.ppm, 4.f, 170.f);
    walkerFeet_ = p.y;
    float bob = 0.f;
    if (!fell_ && phase_ == Phase::Stride) bob = std::sin(step_ * kPi) * std::min(6.f, walkerH_ * 0.1f);
    else if (!fell_) bob = std::sin(t_ * 2.1f) * 1.f;
    walkerFeet_ -= bob;
    walkerChest_ = walkerFeet_ - walkerH_ * 0.58f;
    walkerFog_ = fogFor(z_);
}

bool Game::aim() {
    if (bot_) {
        float dx = walkerX_ - sightX_;
        float maxStep = 780.f * kDt;
        if (std::fabs(dx) <= maxStep) sightX_ = walkerX_;
        else sightX_ += std::copysign(maxStep, dx);
        sightX_ = std::clamp(sightX_, 36.f, 284.f);
        if (shot_ || pace_ != 3 || phase_ != Phase::Hold || phaseT_ < 0.14f) return false;
        if (std::fabs(walkerX_ - sightX_) > 8.f) sightX_ = std::clamp(walkerX_, 36.f, 284.f);
        return std::fabs(walkerX_ - sightX_) <= 14.f;
    }
    float dir = 0.f;
    if (sys_->pad.down(gs::BTN_LEFT)) dir -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) dir += 1.f;
    if (std::fabs(sys_->pad.axisX) > 0.22f) dir = sys_->pad.axisX;
    sightX_ += dir * 340.f * kDt;
    sightX_ = std::clamp(sightX_, 36.f, 284.f);
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
    sys_->apu.noiseBurst(0.5f, 160.f, 0.16f);
    sys_->rumble(0.3f, 0.7f, 90);
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
    sys_->rumble(0.5f, 0.15f, 120);
    sys_->setLight(200, 30, 24);
}

void Game::resolveShot() {
    shot_ = true;
    shotPace_ = pace_;
    flash_ = 0.16f;
    flashX_ = sightX_;
    flashY_ = walkerChest_;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.6f, 1500.f, 0.16f);
    sys_->rumble(0.4f, 0.8f, 70);
    if (pace_ != 3) {
        lose("TOO SOON");
        return;
    }
    if (std::fabs(sightX_ - walkerX_) <= reach()) win();
    else lose("MISSED");
}

void Game::updatePlay() {
    phaseT_ += kDt;
    if (phase_ == Phase::Stride || phase_ == Phase::Cross) {
        float u = smooth(std::min(1.f, phaseT_ / phaseDur()));
        z_ = lerpf(fromZ_, toZ_, u);
        lat_ = lerpf(fromLat_, toLat_, u);
        step_ = u;
    }
    measure();
    if (aim() && !shot_) {
        resolveShot();
        if (mode_ != Mode::Play) return;
    }
    if (phaseT_ + 0.0001f < phaseDur()) return;
    if (phase_ == Phase::Intro) {
        beginStride(1);
        return;
    }
    if (phase_ == Phase::Stride) {
        z_ = toZ_;
        lat_ = toLat_;
        beginHold();
        return;
    }
    if (phase_ == Phase::Hold) {
        if (pace_ >= 3) beginCross();
        else beginStride(pace_ + 1);
        return;
    }
    z_ = toZ_;
    lat_ = toLat_;
    lose("THROUGH THE YARD");
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
        static const float good[] = {392.f, 523.f, 659.f, 784.f};
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
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.8f);
    if (puff_ > 0.f) puff_ = std::max(0.f, puff_ - kDt);

    if (mode_ == Mode::Title) {
        if (startPressed()) beginWatch();
        else if (sys.pad.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
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

void Game::drawYard(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(4, 5, 10);
    uint16_t skyHor = gs::rgb4(14, 9, 4);
    if (mode_ == Mode::Over) {
        skyTop = gs::rgb4(6, 2, 3);
        skyHor = gs::rgb4(12, 4, 3);
    } else if (mode_ == Mode::Victory) {
        skyTop = gs::rgb4(3, 6, 8);
        skyHor = gs::rgb4(10, 12, 6);
    }
    v.setFogColor(mode_ == Mode::Over ? gs::rgb4(8, 3, 3) : gs::rgb4(10, 7, 4));
    const float span = float(gs::SCREEN_H) - kHorizon;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            rd.on = false;
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = mix4(skyTop, skyHor, u * u);
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) + 0.5f - kHorizon) / span;
        t = std::max(t, 0.014f);
        float wz = kZNear / t;
        rd.on = true;
        rd.cx = 160.f + shx;
        rd.hw = std::max(4.f, kRoadHalf * kPpm * t);
        rd.v = wz * 32.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 0;
        rd.band = (int(std::floor(wz * 0.24f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((wz - 7.f) / 18.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 12.f);
        v.lineBackdrop[y] = gs::rgb4(5, 4, 2);
    }
    v.roadTime = int(t_ * 24.f);
}

void Game::drawFrame(float shx) {
    spr(art_.crib, 30.f + shx, 224.f, 148.f, PAL_WOOD, false, 0, true, false);
    spr(art_.crib, 290.f + shx, 224.f, 148.f, PAL_WOOD, true, 0, true, false);
    const float beams[] = {52.f, 116.f, 180.f, 244.f};
    for (float x : beams) spr(art_.beam, x + shx, 60.f, 16.f, PAL_WOOD, false, 0, false, false);
    int wave = int(t_ * 5.f) & 1;
    spr(art_.pennant[wave], 92.f + shx, 46.f, 18.f, PAL_ALERT, false, 0, false, false);
    spr(art_.pennant[wave ^ 1], 214.f + shx, 44.f, 16.f, PAL_ALERT, true, 0, false, false);
    float bob = std::sin(t_ * 2.4f) * 1.4f;
    spr(art_.hoist, 118.f + shx, 88.f + bob, 36.f, PAL_IRON, false, 0, false, false);
}

void Game::drawWorld(float shx) {
    struct Item {
        float z;
        int kind;
        int id;
    };
    Item items[28];
    int n = 0;
    items[n++] = {z_ - 0.04f, 0, 0};
    items[n++] = {kLine, 1, 0};
    for (int i = 0; i < 3; i++) {
        items[n++] = {kMark[i + 1].z, 2, i};
        items[n++] = {kMark[i + 1].z, 2, i + 3};
    }
    for (int i = 0; i < int(sizeof kProps / sizeof kProps[0]); i++) items[n++] = {kProps[i].z, 3, i};

    std::sort(items, items + n, [](const Item& a, const Item& b) {
        if (a.z < b.z) return true;
        if (b.z < a.z) return false;
        return a.kind < b.kind;
    });

    for (int i = 0; i < n; i++) {
        const Item& it = items[i];
        if (it.kind == 0) {
            int fr = 0;
            if (!fell_ && phase_ == Phase::Stride) fr = step_ < 0.5f ? 1 : 0;
            const gs::Mipped& body = fell_ ? art_.fallen : art_.hand[fr];
            float h = fell_ ? walkerH_ * 0.46f : walkerH_;
            spr(art_.shadow, walkerX_ + shx, walkerFeet_ + 2.f, h * (fell_ ? 0.7f : 0.28f), PAL_FX, false, 0, false, true);
            spr(body, walkerX_ + shx, fell_ ? walkerFeet_ - 1.f : walkerFeet_, h, PAL_FIGURE, !facingRight_, walkerFog_,
                true, false);
            if (puff_ > 0.f && !fell_) {
                float u = 1.f - puff_ / 0.26f;
                spr(art_.dust, walkerX_ + shx, walkerFeet_ - u * 6.f, 8.f + u * 10.f, PAL_FX, false, walkerFog_, false,
                    false);
            }
            continue;
        }
        if (it.kind == 1) {
            Proj p = project(0.f, kLine);
            if (!p.ok) continue;
            float w = std::min(240.f, kRoadHalf * 1.5f * p.ppm);
            sprBox(art_.stripe, p.x + shx, p.y, w, std::max(3.f, p.ppm * 0.1f), PAL_TEXT, fogFor(kLine));
            continue;
        }
        if (it.kind == 2) {
            int which = it.id % 3;
            float side = it.id < 3 ? -1.f : 1.f;
            float bz = kMark[which + 1].z;
            Proj p = project(side * 2.05f, bz);
            if (!p.ok) continue;
            bool third = which == 2;
            int pal = PAL_WOOD;
            if (third && (pace_ >= 3 || mode_ == Mode::Victory)) pal = PAL_LIVE;
            else if (pace_ > which) pal = PAL_GOLD;
            float bh = std::clamp((third ? 1.55f : 1.05f) * p.ppm, 5.f, 90.f);
            spr(art_.stake, p.x + shx, p.y, bh, pal, side > 0, fogFor(bz), true, false);
            continue;
        }
        const Prop& pr = kProps[it.id];
        Proj p = project(pr.lat, pr.z);
        if (!p.ok) continue;
        float h = std::clamp(pr.h * p.ppm, 4.f, 120.f);
        int fog = fogFor(pr.z);
        if (pr.kind == 0) {
            spr(art_.balk, p.x + shx, p.y, h, PAL_WOOD, pr.lat > 0, fog, true, false);
        } else if (pr.kind == 1) {
            spr(art_.keg, p.x + shx, p.y, h, PAL_WOOD, false, fog, true, false);
        } else if (pr.kind == 2) {
            spr(art_.coil, p.x + shx, p.y, h, PAL_WOOD, false, fog, true, false);
        } else if (pr.kind == 3) {
            spr(art_.horse, p.x + shx, p.y, h, PAL_WOOD, pr.lat < 0, fog, true, false);
        } else if (pr.kind == 4) {
            spr(art_.post, p.x + shx, p.y, h, PAL_WOOD, false, fog, true, false);
            float bob = std::sin(t_ * 3.f + pr.z) * 1.2f;
            spr(art_.lantern, p.x + shx, p.y - h + bob, h * 0.28f, lampPal(), false, fog, false, false);
        } else {
            spr(art_.barrow, p.x + shx, p.y, h, PAL_WOOD, pr.lat < 0, fog, true, false);
        }
    }
}

void Game::drawSky(float shx) {
    float drift = std::sin(t_ * 0.25f) * 10.f;
    spr(art_.cloud, 46.f + drift + shx, 30.f, 14.f, PAL_SKY, false, 0, false, false);
    spr(art_.cloud, 150.f + drift * 0.6f + shx, 22.f, 11.f, PAL_SKY, true, 0, false, false);
    spr(art_.sun, 286.f + shx, 28.f, 18.f, PAL_SKY, false, 0, false, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 47.f) * 3.f * std::min(shake_, 1.f);
    drawYard(shx);

    const char* word = "YARD PACE";
    int wordPal = PAL_GOLD;
    float wordSc = 0.95f;
    if (mode_ == Mode::Victory) {
        word = "DONE";
        wordPal = PAL_GOOD;
        wordSc = 1.25f;
    } else if (mode_ == Mode::Over) {
        word = "MISSED";
        if (reason_ && std::strcmp(reason_, "TOO SOON") == 0) word = "TOO SOON";
        else if (reason_ && std::strcmp(reason_, "THROUGH THE YARD") == 0) word = "THROUGH";
        wordPal = PAL_ALERT;
        wordSc = 1.f;
    } else if (mode_ == Mode::Pause) {
        word = "PAUSED";
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
    text(word, 160.f, 20.f, wordSc, wordPal);

    for (int i = 0; i < 3; i++) {
        int pal = PAL_WOOD;
        float s = 11.f;
        if (mode_ == Mode::Over) {
            if (i < pace_) pal = PAL_ALERT;
        } else if (i < pace_ || mode_ == Mode::Victory) {
            pal = (i == 2 && (pace_ >= 3 || mode_ == Mode::Victory)) ? PAL_LIVE : PAL_GOLD;
            if (i == 2 && pace_ >= 3 && mode_ != Mode::Over) s = 15.f + std::sin(t_ * 8.f) * 1.3f;
        }
        spr(art_.pip, 136.f + float(i) * 24.f, 44.f, s, pal, false, 0, false, false);
    }

    int beadPal = PAL_HOLD;
    if (mode_ == Mode::Victory || (mode_ == Mode::Play && pace_ >= 3)) beadPal = PAL_LIVE;
    if (mode_ == Mode::Over) beadPal = PAL_ALERT;
    if (mode_ != Mode::Pause) spr(art_.bead, sightX_ + shx, walkerChest_, 16.f, beadPal, false, 0, false, false);
    if (flash_ > 0.f)
        spr(art_.flash, flashX_ + shx, flashY_, 18.f + (0.16f - flash_) * 46.f, PAL_FX, false, 0, false, false);

    float gunX = 160.f + (sightX_ - 160.f) * 0.38f;
    spr(art_.rifle, gunX + shx, 228.f, 58.f, PAL_IRON, false, 0, true, false);
    drawFrame(shx);
    drawWorld(shx);
    drawSky(shx);

    if (mode_ == Mode::Title) {
        hudC(21, "ONE YARD", PAL_TEXT);
        hudC(22, "WAIT UNTIL THE THIRD PACE", PAL_GOLD);
        hudC(23, "THEN FIRE", PAL_GOOD);
        hudC(25, "ARROWS AIM    Z FIRES", PAL_TEXT);
        hudC(26, "ENTER", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "ENTER RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(22, "FIRED ON THE THIRD PACE", PAL_GOOD);
        hudC(23, "THE YARD HOLDS", PAL_TEXT);
        hudC(26, "ENTER", PAL_TEXT);
    } else if (mode_ == Mode::Over) {
        hudC(22, reason_ ? reason_ : "LOST", PAL_ALERT);
        hudC(23, "THE WATCH IS OVER", PAL_TEXT);
        hudC(26, "ENTER RETRIES", PAL_TEXT);
    } else if (pace_ < 3) {
        hud(1, 1, "HOLD FIRE", PAL_ALERT);
        hud(36, 1, pace_ <= 0 ? "0/3" : pace_ == 1 ? "1/3" : "2/3", PAL_GOLD);
        hudC(23, "WAIT UNTIL THE THIRD PACE", PAL_TEXT);
        hudC(24, "THEN FIRE", PAL_GOLD);
    } else {
        hud(1, 1, "THIRD PACE", PAL_GOOD);
        hud(36, 1, "3/3", PAL_GOOD);
        hudC(23, "FIRE", PAL_GOOD);
        hudC(24, "BEFORE THEY CROSS", PAL_TEXT);
    }
}

}  // namespace yardpace
