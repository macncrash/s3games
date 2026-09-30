#include "game/foundry.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace foundrypace {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 90.f;
constexpr float kZNear = 2.35f;
constexpr float kPpm = 80.f;
constexpr float kPathHalf = 1.9f;
constexpr float kWalkerH = 1.72f;
constexpr float kZ0 = 15.2f;
constexpr float kLineZ = 4.05f;
constexpr float kThroughZ = 3.2f;
constexpr float kIntro = 0.72f;
constexpr float kPi = 3.14159265f;

constexpr float kMarkZ[4] = {0.f, 11.6f, 7.55f, 5.05f};
constexpr float kMarkLat[4] = {0.f, 0.16f, -0.24f, 0.04f};

struct Prop {
    float z, lat, h;
    int kind;
};

constexpr Prop kProps[] = {
    {14.8f, -3.6f, 5.2f, 0}, {13.4f, 4.2f, 3.4f, 1}, {12.2f, -4.4f, 1.6f, 2}, {11.0f, 4.0f, 2.8f, 1},
    {9.6f, -4.1f, 1.4f, 2},  {8.4f, 3.7f, 2.2f, 1},  {7.2f, -3.8f, 1.2f, 2},  {6.4f, 3.5f, 1.8f, 1},
    {13.8f, -2.6f, 0.55f, 3}, {10.2f, 2.7f, 0.5f, 3}, {7.8f, -2.8f, 0.45f, 3},
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

float Game::holdDur() const { return pace_ >= 3 ? 0.95f : 0.38f; }
float Game::strideDur() const { return pace_ >= 3 ? 0.92f : 0.52f; }

float Game::bendAt(float row) const {
    float crest = std::sin(row * 0.012f) * 10.f;
    float heat = std::sin(t_ * 1.4f) * 1.2f * std::clamp(row / 80.f, 0.f, 1.f);
    return crest + heat;
}

int Game::fogFor(float z) const {
    float t = std::clamp((z - 6.5f) / 13.f, 0.f, 1.f);
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
    if (sys_) sys_->setLight(180, 70, 20);
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
    puff_ = 0.22f;
    sys_->apu.noiseBurst(0.2f, n == 3 ? 520.f : 280.f, 0.07f);
    if (n == 3) {
        blip(440.f, 0.07f, 0.16f);
        sys_->setLight(40, 180, 60);
    } else {
        blip(140.f + float(n) * 50.f, 0.04f, 0.07f);
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
    if (!fell_ && phase_ == Phase::Stride) bob = std::sin(step_ * kPi) * std::min(5.f, walkerH_ * 0.1f);
    else if (!fell_) bob = std::sin(t_ * 2.4f) * 0.8f;
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
        bool stepping = phase_ == Phase::Stride && phaseT_ > 0.08f;
        if (phase_ == Phase::Stride && phaseT_ > 0.4f) sightX_ = walkerX_;
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
    sys_->apu.noiseBurst(0.45f, 160.f, 0.14f);
    sys_->rumble(0.3f, 0.7f, 80);
    sys_->setLight(30, 190, 60);
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
    sys_->setLight(160, 20, 10);
}

void Game::resolveShot() {
    shot_ = true;
    shotPace_ = pace_;
    flash_ = 0.16f;
    flashX_ = sightX_;
    flashY_ = walkerChest_;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.65f, 1200.f, 0.16f);
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
        static const float bad[] = {180.f, 130.f, 98.f};
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
    if (puff_ > 0.f) puff_ = std::max(0.f, puff_ - kDt);
    sys.vdp.roadTime = int(t_ * 22.f);

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

void Game::drawFloor(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(1, 1, 2);
    uint16_t skyHor = gs::rgb4(12, 4, 1);
    if (mode_ == Mode::Over) skyHor = gs::rgb4(8, 1, 1);
    else if (mode_ == Mode::Victory) skyHor = gs::rgb4(6, 10, 3);
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
        t = std::max(t, 0.02f);
        float wz = kZNear / t;
        float row = float(y) - kHorizon;
        rd.on = true;
        rd.cx = 160.f + bendAt(row) + shx;
        rd.hw = std::max(6.f, kPathHalf * kPpm * t);
        rd.v = wz * 28.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = gs::ROAD_ROCKY;
        rd.band = (int(std::floor(wz * 0.35f + t_ * 3.f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_DROP;
        rd.right = gs::GROUND_DROP;
        float fogT = std::clamp((wz - 5.5f) / 16.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 12.f);
        float dropT = std::clamp(row / span, 0.f, 1.f);
        v.lineBackdrop[y] = mixC(gs::rgb4(6, 2, 1), gs::rgb4(1, 1, 1), dropT);
    }
}

void Game::drawKit(float shx) {
    int cloth = PAL_EMBER;
    if (mode_ == Mode::Victory || (mode_ == Mode::Play && pace_ >= 3)) cloth = PAL_LIVE;
    if (mode_ == Mode::Over) cloth = PAL_ALERT;
    spr(art_.post, 28.f + shx, 208.f, 92.f, PAL_IRON, false, 0, true, false);
    int fr = std::sin(t_ * 6.f) > 0.f ? 0 : 1;
    spr(art_.bellows[fr], 48.f + shx, 118.f, 16.f, cloth, false, 0, false, false);
    spr(art_.lip, 54.f + shx, 226.f, 52.f, PAL_BRICK, false, 0, true, false);
    spr(art_.lip, 284.f + shx, 230.f, 60.f, PAL_BRICK, true, 0, true, false);
    float gunX = 160.f + (sightX_ - 160.f) * 0.35f;
    spr(art_.iron, gunX + shx, 230.f, 70.f, PAL_IRON, false, 0, true, false);
}

void Game::drawWorld(float shx) {
    struct Item {
        float z;
        int kind;
        int id;
    };
    Item items[32];
    int n = 0;
    items[n++] = {z_, 0, 0};
    items[n++] = {kLineZ, 1, 0};
    items[n++] = {kLineZ - 0.1f, 2, 0};
    items[n++] = {kLineZ - 0.1f, 2, 1};
    for (int i = 0; i < 3; i++) items[n++] = {kMarkZ[i + 1], 4, i};
    items[n++] = {kMarkZ[3] - 0.06f, 3, 0};
    for (int i = 0; i < int(sizeof kProps / sizeof kProps[0]); i++) items[n++] = {kProps[i].z, 5, i};

    std::sort(items, items + n, [](const Item& a, const Item& b) {
        if (a.z != b.z) return a.z < b.z;
        return a.kind < b.kind;
    });

    int signal = PAL_EMBER;
    if (mode_ == Mode::Victory || (mode_ == Mode::Play && pace_ >= 3)) signal = PAL_LIVE;
    else if (mode_ == Mode::Over) signal = PAL_ALERT;

    for (int i = 0; i < n; i++) {
        const Item& it = items[i];
        if (it.kind == 0) {
            int fr = 0;
            if (phase_ == Phase::Stride) fr = step_ < 0.5f ? 0 : 1;
            const gs::Mipped& body = fell_ ? art_.fallen : art_.smith[fr];
            float h = fell_ ? walkerH_ * 0.42f : walkerH_;
            spr(art_.shadow, walkerX_ + shx, walkerFeet_ + 2.f, h * (fell_ ? 0.65f : 0.28f), PAL_FX, false, 0, false,
                true);
            spr(body, walkerX_ + shx, fell_ ? walkerFeet_ - 1.f : walkerFeet_, h, PAL_FIGURE, false, walkerFog_, true,
                false);
            if (puff_ > 0.f && !fell_) {
                float u = 1.f - puff_ / 0.22f;
                spr(art_.dust, walkerX_ + shx, walkerFeet_ - u * 6.f, 7.f + u * 8.f, PAL_FX, false, walkerFog_, false,
                    false);
            }
            continue;
        }
        if (it.kind == 1) {
            Proj p = project(0.f, kLineZ);
            if (!p.ok) continue;
            float w = std::min(280.f, kPathHalf * 1.85f * p.ppm);
            sprBox(art_.stripe, p.x + shx, p.y, w, std::max(3.f, p.ppm * 0.09f),
                   mode_ == Mode::Over ? PAL_ALERT : PAL_EMBER, fogFor(kLineZ));
            continue;
        }
        if (it.kind == 2) {
            float side = it.id == 0 ? -1.f : 1.f;
            Proj p = project(side * (kPathHalf + 0.1f), kLineZ - 0.1f);
            if (!p.ok) continue;
            float h = std::clamp(1.25f * p.ppm, 6.f, 80.f);
            spr(art_.stake, p.x + shx, p.y, h, signal, side > 0, fogFor(kLineZ), true, false);
            continue;
        }
        if (it.kind == 3) {
            Proj p = project(kPathHalf + 0.2f, kMarkZ[3]);
            if (!p.ok) continue;
            float ch = std::clamp(1.4f * p.ppm, 6.f, 64.f);
            spr(art_.flame[int(t_ * 8.f) & 1], p.x + shx, p.y - ch * 0.7f, std::max(6.f, ch * 0.35f), PAL_GLOW, false,
                fogFor(kMarkZ[3]), false, false);
            continue;
        }
        if (it.kind == 4) {
            int which = it.id;
            float side = which == 1 ? 1.f : -1.f;
            float cz = kMarkZ[which + 1];
            Proj p = project(side * (kPathHalf + 0.32f), cz);
            if (!p.ok) continue;
            bool third = which == 2;
            float h = std::clamp((third ? 1.1f : 0.75f) * p.ppm, 5.f, 60.f);
            spr(third ? art_.crucible : art_.ingot, p.x + shx, p.y, h, PAL_BRICK, side > 0, fogFor(cz), true, false);
            continue;
        }
        const Prop& pr = kProps[it.id];
        Proj p = project(pr.lat, pr.z);
        if (!p.ok) continue;
        float h = std::clamp(pr.h * p.ppm, 3.f, 150.f);
        int fog = fogFor(pr.z);
        if (pr.kind == 0) {
            spr(art_.furnace, p.x + shx, p.y, h, PAL_STACK, false, fog, true, false);
            spr(art_.flame[int(t_ * 7.f + pr.z) & 1], p.x + shx, p.y - h * 0.42f, h * 0.22f, PAL_GLOW, false, fog,
                false, false);
        } else if (pr.kind == 1)
            spr(art_.chimney, p.x + shx, p.y, h, PAL_SOOT, pr.lat > 0, fog, true, false);
        else if (pr.kind == 2)
            spr(art_.crucible, p.x + shx, p.y, h, PAL_BRICK, pr.lat < 0, fog, true, false);
        else
            spr(art_.ingot, p.x + shx, p.y, h, PAL_IRON, false, fog, true, false);
    }
}

void Game::drawGlow(float shx) {
    float sx = 70.f + std::sin(t_ * 0.7f) * 18.f;
    spr(art_.spark, sx + shx, 36.f + std::sin(t_ * 1.6f) * 8.f, 8.f, PAL_GLOW, false, 1, false, false);
    spr(art_.spark, 210.f + std::sin(t_ * 0.9f) * 14.f + shx, 28.f, 6.f, PAL_EMBER, false, 2, false, false);
    spr(art_.hood, 78.f + shx, kHorizon + 4.f, 32.f, PAL_STACK, false, 6, true, false);
    spr(art_.hood, 246.f + shx, kHorizon + 6.f, 26.f, PAL_STACK, true, 7, true, false);
    spr(art_.flame[int(t_ * 5.f) & 1], 52.f + shx, 52.f, 18.f, PAL_GLOW, false, 1, false, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 50.f) * 3.f * std::min(shake_, 1.f);
    drawFloor(shx);

    const char* word = "FOUNDRY";
    int wordPal = PAL_EMBER;
    float wordSc = 1.0f;
    if (mode_ == Mode::Victory) {
        word = "HELD";
        wordPal = PAL_GOOD;
        wordSc = 1.26f;
    } else if (mode_ == Mode::Over) {
        word = reason_ && reason_[0] ? reason_ : "OVER";
        wordPal = PAL_ALERT;
        wordSc = 1.02f;
    } else if (mode_ == Mode::Pause) {
        word = "PAUSED";
        wordPal = PAL_EMBER;
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
    text(word, 160.f + shx, 30.f, wordSc, wordPal);

    for (int i = 0; i < 3; i++) {
        int pal = PAL_IRON;
        float s = 10.f;
        if (mode_ == Mode::Over) {
            if (i < pace_) pal = PAL_ALERT;
        } else if (i < pace_ || mode_ == Mode::Victory) {
            pal = (i == 2 && pace_ >= 3) ? PAL_LIVE : PAL_EMBER;
            if (i == 2 && pace_ >= 3) s = 14.f + std::sin(t_ * 9.f) * 1.1f;
        }
        spr(art_.pip, 136.f + float(i) * 24.f + shx, 54.f, s, pal, false, 0, false, false);
    }

    int beadPal = PAL_HOLD;
    if (mode_ == Mode::Victory || (mode_ == Mode::Play && pace_ >= 3)) beadPal = PAL_LIVE;
    if (mode_ == Mode::Over) beadPal = PAL_ALERT;
    if (mode_ != Mode::Pause) spr(art_.bead, sightX_ + shx, walkerChest_, 15.f, beadPal, false, 0, false, false);
    if (flash_ > 0.f) {
        spr(art_.flash, flashX_ + shx, flashY_, 16.f + (0.16f - flash_) * 55.f, PAL_FX, false, 0, false, false);
        float gunX = 160.f + (sightX_ - 160.f) * 0.35f;
        spr(art_.dust, gunX + shx, 174.f, 12.f, PAL_FX, false, 0, false, false);
    }

    drawKit(shx);
    drawWorld(shx);
    drawGlow(shx);

    if (mode_ == Mode::Title) {
        hudC(21, "YOU HAVE THE FOUNDRY", PAL_EMBER);
        hudC(22, "WAIT UNTIL THE THIRD PACE", PAL_TEXT);
        hudC(23, "THEN FIRE", PAL_GOOD);
        hudC(25, "ARROWS AIM    Z FIRES", PAL_TEXT);
        hudC(26, "START", PAL_EMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(23, "FIRED ON THE THIRD PACE", PAL_GOOD);
        hudC(24, "THE FOUNDRY HOLDS", PAL_TEXT);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Over) {
        hudC(23, reason_, PAL_ALERT);
        hudC(24, "ANYTHING ELSE IS A LOSS", PAL_TEXT);
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

}  // namespace foundrypace
