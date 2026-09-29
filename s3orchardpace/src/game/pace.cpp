#include "game/pace.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace orchard {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 86.f;
constexpr float kZNear = 2.2f;
constexpr float kPpm = 92.f;
constexpr float kLane = 1.55f;
constexpr float kBodyH = 1.68f;
constexpr float kZ0 = 16.8f;
constexpr float kShedZ = 3.55f;
constexpr float kBarnZ = 2.9f;
constexpr float kIntro = 0.55f;
constexpr float kHit = 28.f;
constexpr float kPi = 3.14159265f;

struct Mark {
    float z, lat;
};
constexpr Mark kStop[4] = {
    {0.f, 0.f},
    {12.6f, -0.42f},
    {8.15f, 0.55f},
    {4.85f, -0.12f},
};

struct Tree {
    float z, lat;
};
constexpr Tree kTrees[] = {
    {15.4f, -2.35f}, {15.1f, 2.5f}, {12.2f, -2.55f}, {11.6f, 2.4f},
    {9.0f, -2.3f},   {8.5f, 2.65f},  {6.1f, -2.45f},  {5.6f, 2.35f},
    {3.8f, -2.2f},   {3.4f, 2.5f},
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

float Game::holdDur() const { return pace_ >= 3 ? 0.86f : 0.46f; }
float Game::strideDur() const { return pace_ >= 3 ? 0.95f : 0.62f; }

int Game::fogFor(float z) const {
    float t = std::clamp((z - 5.f) / 16.f, 0.f, 1.f);
    return int(t * 9.f);
}

Game::Proj Game::project(float lat, float z) const {
    Proj p;
    if (!(z > 0.35f)) return p;
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
    down_ = false;
    won_ = false;
    over_ = false;
    flash_ = 0.f;
    shake_ = 0.f;
    puff_ = 0.f;
    drop_ = 0.f;
    sightX_ = 160.f;
    reason_ = "";
    fanStep_ = -1;
    fanT_ = 0.f;
    blip_ = 0.f;
    trigWas_ = false;
}

void Game::beginRow() {
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
    toZ_ = kStop[n].z;
    toLat_ = kStop[n].lat;
    puff_ = 0.22f;
    sys_->apu.noiseBurst(0.16f, n == 3 ? 620.f : 280.f, 0.06f);
    blip(160.f + float(n) * 70.f, n == 3 ? 0.07f : 0.03f, n == 3 ? 0.16f : 0.06f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    t_ = 0.f;
    resetPose();
    mode_ = Mode::Title;
    if (bot_) beginRow();
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
    Proj p = project(lat_, z_);
    pickerX_ = p.x;
    pickerH_ = std::clamp(kBodyH * p.ppm, 4.f, 160.f);
    pickerFeet_ = p.y;
    float bob = 0.f;
    if (phase_ == Phase::Stride) bob = std::sin(step_ * kPi) * std::min(6.f, pickerH_ * 0.1f);
    else bob = std::sin(t_ * 2.2f) * 0.7f;
    pickerFeet_ -= bob;
    pickerChest_ = pickerFeet_ - pickerH_ * 0.55f;
    pickerFog_ = fogFor(z_);
}

bool Game::steer() {
    if (bot_) {
        float dx = pickerX_ - sightX_;
        float maxStep = 520.f * kDt;
        if (std::fabs(dx) <= maxStep) sightX_ = pickerX_;
        else sightX_ += std::copysign(maxStep, dx);
    } else {
        float dir = 0.f;
        if (sys_->pad.down(gs::BTN_LEFT)) dir -= 1.f;
        if (sys_->pad.down(gs::BTN_RIGHT)) dir += 1.f;
        if (std::fabs(sys_->pad.axisX) > 0.25f) dir = sys_->pad.axisX;
        sightX_ += dir * 300.f * kDt;
    }
    sightX_ = std::clamp(sightX_, 70.f, 250.f);
    if (bot_) {
        bool third = pace_ == 3 && phase_ == Phase::Hold;
        bool settled = phaseT_ > 0.20f;
        bool aligned = std::fabs(pickerX_ - sightX_) < 12.f;
        return third && settled && aligned && !shot_;
    }
    return firePressed();
}

void Game::win() {
    won_ = true;
    over_ = true;
    down_ = true;
    drop_ = 1.f;
    mode_ = Mode::Victory;
    reason_ = "FIRED ON THE THIRD PACE";
    fanGood_ = true;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->apu.tone(0, 523.f, 0.08f);
    sys_->apu.noiseBurst(0.4f, 180.f, 0.14f);
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
    flashY_ = pickerChest_;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.5f, 900.f, 0.12f);
    if (pace_ != 3) {
        lose("TOO SOON");
        return;
    }
    if (std::fabs(sightX_ - pickerX_) <= kHit) win();
    else lose("WIDE");
}

void Game::updatePlay() {
    t_ += kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.7f);
    if (flash_ > 0.f) flash_ = std::max(0.f, flash_ - kDt);
    if (puff_ > 0.f) puff_ = std::max(0.f, puff_ - kDt);
    if (drop_ > 0.f) drop_ = std::max(0.f, drop_ - kDt * 0.7f);

    phaseT_ += kDt;
    if (phase_ == Phase::Stride) {
        float u = smooth(std::min(1.f, phaseT_ / strideDur()));
        z_ = lerpf(fromZ_, toZ_, u);
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
        fromZ_ = z_;
        fromLat_ = lat_;
        return;
    }
    z_ = toZ_;
    lat_ = toLat_;
    if (pace_ >= 3) {
        z_ = kBarnZ;
        lat_ = 0.f;
        lose("PAST THE ROW");
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
        static const float bad[] = {196.f, 146.f, 98.f};
        const float* notes = fanGood_ ? good : bad;
        int n = fanGood_ ? 4 : 3;
        if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.055f);
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
        if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.7f);
        if (drop_ > 0.f) drop_ = std::max(0.f, drop_ - kDt * 0.7f);
    }

    if (mode_ == Mode::Title) {
        if (startPressed()) beginRow();
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
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) beginRow();
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

void Game::drawRow(float shx) {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(mode_ == Mode::Over ? gs::rgb4(8, 4, 3) : gs::rgb4(9, 12, 14));
    const float span = float(gs::SCREEN_H) - kHorizon;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            int r = std::clamp(int(6.f + (1.f - u) * 6.f), 0, 15);
            int g = std::clamp(int(8.f + (1.f - u) * 5.f), 0, 15);
            int b = std::clamp(int(10.f + (1.f - u) * 5.f), 0, 15);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) + 0.5f - kHorizon) / span;
        t = std::max(t, 0.012f);
        float wz = kZNear / t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + shx * 0.25f;
        rd.hw = std::max(10.f, kLane * kPpm * t);
        rd.v = wz * 28.f;
        rd.pal = uint8_t(PAL_ROW);
        rd.style = 0;
        rd.band = (int(std::floor(wz * 0.7f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((wz - 9.f) / 18.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 8.f);
        v.lineBackdrop[y] = gs::rgb4(3, 6, 2);
    }
    v.roadTime = int(t_ * 18.f);
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
    items[n++] = {kShedZ, 2, 0};
    for (int i = 0; i < int(sizeof kTrees / sizeof kTrees[0]); i++) items[n++] = {kTrees[i].z, 1, i};
    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.z < b.z; });

    for (int i = 0; i < n; i++) {
        const Item& it = items[i];
        if (it.kind == 0) {
            int fr = (phase_ == Phase::Stride && step_ >= 0.5f) ? 1 : 0;
            const gs::Mipped& body = down_ ? art_.down : art_.step[fr];
            float h = down_ ? pickerH_ * 0.42f : pickerH_;
            float y = down_ ? pickerFeet_ - 2.f : pickerFeet_;
            spr(art_.shadow, pickerX_ + shx, pickerFeet_ + 2.f, h * 0.28f, PAL_FX, false, 0, false, true);
            spr(body, pickerX_ + shx, y, h, PAL_PICK, lat_ > 0.f, pickerFog_, !down_, false);
            if (!down_) {
                spr(art_.basket, pickerX_ + shx - h * 0.22f, pickerFeet_ - h * 0.28f, h * 0.28f, PAL_BARK, false, pickerFog_, false, false);
            }
            if (puff_ > 0.f && !down_) {
                float u = 1.f - puff_ / 0.22f;
                spr(art_.dust, pickerX_ + shx, pickerFeet_ - u * 3.f, 7.f + u * 5.f, PAL_FX, false, pickerFog_, false, false);
            }
            continue;
        }
        if (it.kind == 2) {
            Proj p = project(0.15f, kShedZ);
            if (!p.ok) continue;
            int pal = (mode_ == Mode::Victory) ? PAL_GOOD : (mode_ == Mode::Over ? PAL_ALERT : PAL_BARK);
            float h = std::clamp(2.6f * p.ppm, 10.f, 150.f);
            spr(art_.shed, p.x + shx, p.y, h, pal, false, fogFor(kShedZ), true, false);
            continue;
        }
        const Tree& tr = kTrees[it.id];
        Proj p = project(tr.lat, tr.z);
        if (!p.ok) continue;
        float h = std::clamp(4.4f * p.ppm, 8.f, 200.f);
        int fog = fogFor(tr.z);
        spr(art_.tree, p.x + shx, p.y, h, PAL_LEAF, tr.lat > 0, fog, true, false);
        float ay = p.y - h * 0.62f;
        if (mode_ == Mode::Victory && it.id == 6) ay += (1.f - drop_) * h * 0.35f;
        spr(art_.apple, p.x + shx - h * 0.12f, ay, std::max(4.f, h * 0.1f), PAL_APPLE, false, fog, false, false);
        spr(art_.apple, p.x + shx + h * 0.16f, ay + h * 0.08f, std::max(3.f, h * 0.08f), PAL_APPLE, false, fog, false, false);
    }

    Proj stake = project(-0.85f, kStop[3].z);
    if (stake.ok) {
        int pal = pace_ >= 3 ? PAL_GOOD : PAL_POST;
        spr(art_.stake, stake.x + shx, stake.y, std::clamp(1.5f * stake.ppm, 6.f, 80.f), pal, false, fogFor(kStop[3].z), true, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 37.f) * 2.2f * std::min(shake_, 1.f);
    drawRow(shx);

    const char* banner = "ROW";
    int bannerPal = PAL_AMBER;
    if (mode_ == Mode::Title) banner = "ORCHARD";
    else if (mode_ == Mode::Victory) {
        banner = "DONE";
        bannerPal = PAL_GOOD;
    } else if (mode_ == Mode::Over) {
        banner = reason_ && reason_[0] ? reason_ : "LOST";
        bannerPal = PAL_ALERT;
    } else if (mode_ == Mode::Pause) banner = "PAUSED";
    else if (pace_ == 0) banner = "WAIT";
    else if (pace_ == 1) banner = "ONE";
    else if (pace_ == 2) banner = "TWO";
    else {
        banner = "FIRE";
        bannerPal = PAL_GOOD;
    }
    text(banner, 160.f, 22.f, mode_ == Mode::Title ? 0.9f : 0.85f, bannerPal);

    for (int i = 0; i < 3; i++) {
        int pal = PAL_BARK;
        if (pace_ > i) pal = (i == 2) ? PAL_GOOD : PAL_AMBER;
        float s = (i == 2 && pace_ >= 3) ? 13.f : 8.f;
        spr(art_.pip, 136.f + float(i) * 24.f, 46.f, s, pal, false, 0, false, false);
    }

    if (mode_ == Mode::Play || flash_ > 0.f) {
        int beadPal = (pace_ >= 3 && mode_ == Mode::Play) || mode_ == Mode::Victory ? PAL_GOOD : PAL_SIGHT;
        if (mode_ == Mode::Play) spr(art_.bead, sightX_ + shx, pickerChest_, 16.f, beadPal, false, 0, false, false);
        if (flash_ > 0.f)
            spr(art_.flare, flashX_ + shx, flashY_, 16.f + (0.16f - flash_) * 40.f, PAL_FX, false, 0, false, false);
    }

    float slingX = 168.f + (sightX_ - 160.f) * 0.28f;
    spr(art_.sling, slingX + shx, 228.f, 52.f, PAL_BARK, false, 0, true, false);
    drawWorld(shx);

    if (mode_ == Mode::Title) {
        hudC(20, "ONE ORCHARD", PAL_TEXT);
        hudC(21, "WAIT UNTIL THE THIRD PACE", PAL_AMBER);
        hudC(22, "THEN FIRE", PAL_GOOD);
        hudC(24, "ARROWS AIM    C FIRES", PAL_TEXT);
        hudC(26, "START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(22, "FIRED ON THE THIRD PACE", PAL_GOOD);
        hudC(23, "THE ORCHARD IS DONE", PAL_TEXT);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Over) {
        hudC(22, reason_, PAL_ALERT);
        hudC(23, "THE ORCHARD IS NOT DONE", PAL_TEXT);
        hudC(26, "START RETRIES", PAL_TEXT);
    } else if (pace_ < 3) {
        hudC(0, "HOLD", PAL_AMBER);
        hudC(24, "WAIT UNTIL THE THIRD PACE", PAL_TEXT);
        hudC(25, "DO NOT FIRE", PAL_ALERT);
    } else {
        hudC(0, "THIRD PACE", PAL_GOOD);
        hudC(24, "FIRE", PAL_GOOD);
        hudC(25, "BEFORE THE SHED", PAL_TEXT);
    }
}

}  // namespace orchard
