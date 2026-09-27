#include "game/well.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace well {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kHorizon = 96.f;

// 0 left, 1 right, 2 seaward. Three waves, fixed so a watch can be learned.
const int kWave0[] = {0, 1, 0, 1, 2};
const int kWave1[] = {2, 0, 1, 2, 1, 0};
const int kWave2[] = {0, 2, 1, 0, 2, 1, 0, 1};

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto c = [&](int u, int v) { return int(std::lround(u + (v - u) * t)); };
    return gs::rgb4(c(ar, br), c(ag, bg), c(ab, bb));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (phase_ == Phase::Lull) return 2;
    return 1;
}

const char* Game::braceName(int d) const {
    if (d == 0) return "BRACE LEFT";
    if (d == 1) return "BRACE RIGHT";
    return "BRACE THE SEA";
}

int Game::surgeCount() const {
    if (wave_ <= 0) return 5;
    if (wave_ == 1) return 6;
    return 8;
}

int Game::surgeDir() const {
    const int* row = wave_ <= 0 ? kWave0 : wave_ == 1 ? kWave1 : kWave2;
    int n = surgeCount();
    int i = std::clamp(surge_, 0, n - 1);
    return row[i];
}

float Game::warnLen() const { return wave_ == 0 ? 0.72f : wave_ == 1 ? 0.56f : 0.44f; }
float Game::strikeLen() const { return wave_ == 0 ? 0.42f : wave_ == 1 ? 0.36f : 0.30f; }

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    reason_ = "";
    score_ = 0;
    wave_ = 0;
    stones_ = 6;
    lean_ = 0;
    swell_ = 0;
    shake_ = 0;
    modeT_ = 0;
    phase_ = Phase::Lull;
    phaseT_ = 0.9f;
    surge_ = 0;
    braced_ = false;
    sys_->setLight(40, 90, 110);
}

void Game::startWave() {
    surge_ = 0;
    nextSurge();
    sys_->apu.noiseBurst(0.18f, 800.f, 0.25f);
}

void Game::nextSurge() {
    braced_ = false;
    phase_ = Phase::Warn;
    phaseT_ = warnLen();
    swell_ = 0;
}

void Game::resolveStrike() {
    if (braced_) {
        score_ += 100;
        lean_ = 0;
        sys_->apu.keyOn(0, 520.f, 0.18f);
    } else {
        stones_ -= 1;
        lean_ = surgeDir() == 0 ? -1 : surgeDir() == 1 ? 1 : 0;
        shake_ = 1.f;
        score_ = std::max(0, score_ - 40);
        sys_->apu.noiseBurst(0.45f, 180.f, 0.35f);
        sys_->setLight(140, 40, 30);
        if (stones_ <= 0) {
            stones_ = 0;
            mode_ = Mode::Over;
            over_ = true;
            won_ = false;
            reason_ = "THE WELL FELL";
            sys_->apu.keyOn(1, 90.f, 0.4f);
            return;
        }
    }
    surge_++;
    if (surge_ >= surgeCount()) {
        score_ += 500;
        wave_++;
        if (wave_ >= 3) {
            mode_ = Mode::Victory;
            over_ = true;
            won_ = true;
            reason_ = "THE WELL STOOD";
            wave_ = 2;
            sys_->apu.keyOn(2, 523.f, 0.35f);
            sys_->apu.keyOn(3, 659.f, 0.28f);
            sys_->setLight(80, 180, 90);
            return;
        }
        phase_ = Phase::Lull;
        phaseT_ = 1.15f;
        lean_ = 0;
        sys_->apu.keyOn(1, 330.f, 0.22f);
        return;
    }
    phase_ = Phase::Gap;
    phaseT_ = wave_ == 0 ? 0.28f : 0.20f;
}

void Game::readBrace(int& dir, bool& any) {
    dir = -1;
    any = false;
    if (bot_) {
        dir = surgeDir();
        any = phase_ == Phase::Warn || phase_ == Phase::Strike;
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT) || p.axisX < -0.35f) dir = 0;
    else if (p.down(gs::BTN_RIGHT) || p.axisX > 0.35f) dir = 1;
    else if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.axisY > 0.35f) dir = 2;
    any = dir >= 0;
}

void Game::update(float dt) {
    if (phase_ == Phase::Lull) {
        phaseT_ -= dt;
        if (phaseT_ <= 0) startWave();
        return;
    }
    int dir = -1;
    bool any = false;
    readBrace(dir, any);
    if ((phase_ == Phase::Warn || phase_ == Phase::Strike) && any && dir == surgeDir()) braced_ = true;

    if (phase_ == Phase::Warn) {
        phaseT_ -= dt;
        swell_ = std::clamp(1.f - phaseT_ / warnLen(), 0.f, 1.f);
        if (phaseT_ <= 0) {
            phase_ = Phase::Strike;
            phaseT_ = strikeLen();
            sys_->apu.keyOn(0, 180.f, 0.16f);
        }
        return;
    }
    if (phase_ == Phase::Strike) {
        swell_ = 1.f;
        phaseT_ -= dt;
        if (phaseT_ <= 0) resolveStrike();
        return;
    }
    if (phase_ == Phase::Gap) {
        swell_ = std::max(0.f, swell_ - dt * 3.f);
        phaseT_ -= dt;
        if (phaseT_ <= 0) nextSurge();
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 2.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.y > gs::SCREEN_H + 40 || s.x + s.w < -40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::shadow(float cx, float cy, float w) {
    if (w < 4.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 4L, 400L));
    s.h = int16_t(std::max(3L, std::lround(double(w) * 0.16)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy));
    s.img = art_.shadow.pick(float(s.h));
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.f * scale;
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, float(g.h) * scale, pal, false, 0);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || row < 0 || row > 27 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float storm = mode_ == Mode::Title ? 0.15f : std::clamp((wave_ + swell_) / 3.f, 0.f, 1.f);
    uint16_t skyTop = mix(gs::rgb4(2, 4, 8), gs::rgb4(1, 2, 4), storm);
    uint16_t skyHor = mix(gs::rgb4(6, 9, 10), gs::rgb4(3, 5, 6), storm);
    v.setFogColor(gs::rgb4(2, 4, 6));
    v.roadTime = int(modeT_ * 60.f);
    float shx = shake_ > 0 ? std::sin(modeT_ * 80.f) * 4.f * shake_ : 0.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = std::clamp((float(y) - 8.f) / 88.f, 0.f, 1.f);
        v.lineBackdrop[y] = mix(skyTop, skyHor, t);
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        r = {};
        if (y < int(kHorizon)) continue;
        r.on = true;
        r.style = 2;
        r.cx = 160.f + shx + std::sin(modeT_ * 1.3f + y * 0.02f) * (4.f + storm * 10.f);
        r.hw = 900.f;
        r.v = modeT_ * (30.f + storm * 40.f) + float(y) * 2.2f;
        r.pal = PAL_WATER;
        r.band = ((y / 5) & 1) ? 1 : 0;
        r.left = gs::GROUND_WATER;
        r.right = gs::GROUND_WATER;
    }
    v.A.scroll(int(shx), 0);

    if (mode_ == Mode::Title) text("S3 HARBOR WELL", 160, 26, 0.52f, PAL_AMBER);
    else if (mode_ == Mode::Victory) text("STOOD", 160, 22, 1.0f, PAL_GREEN);
    else if (mode_ == Mode::Over) text("FELL", 160, 22, 1.0f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSE", 160, 22, 0.9f, PAL_HUD);
    else if (phase_ == Phase::Lull) text("STILL STANDING", 160, 22, 0.48f, PAL_GREEN);
    else if (phase_ == Phase::Strike) text(braceName(surgeDir()), 160, 22, 0.48f, PAL_RED);
    else text(braceName(surgeDir()), 160, 22, 0.42f, PAL_AMBER);

    int d = (mode_ == Mode::Watch) ? surgeDir() : 2;
    float sprayX = d == 0 ? 70.f : d == 1 ? 250.f : 160.f;
    sprayX += shx;
    float sprayH = 18.f + swell_ * (28.f + wave_ * 8.f);
    if (mode_ != Mode::Title && swell_ > 0.05f)
        spr(art_.spray, sprayX, 150.f - swell_ * 18.f, sprayH, PAL_SPRAY, d == 0, 0);

    float wx = 168.f + shx + float(lean_) * 6.f;
    float wy = 156.f;
    shadow(wx, wy + 28.f, 70.f);
    spr(art_.well, wx, wy, 78.f - (6 - stones_) * 2.f, PAL_STONE, false, 0);
    spr(art_.roof, wx, wy - 46.f, 28.f, PAL_WOOD, false, 0);
    int cracks = 6 - stones_;
    for (int i = 0; i < cracks && i < 4; i++)
        spr(art_.crack, wx - 16.f + float(i) * 10.f, wy - 8.f + float(i % 2) * 12.f, 16.f, PAL_RED, i & 1, 0);

    if (braced_ && (phase_ == Phase::Warn || phase_ == Phase::Strike)) {
        float bx = d == 0 ? wx - 28.f : d == 1 ? wx + 28.f : wx;
        float by = d == 2 ? wy + 8.f : wy + 4.f;
        spr(art_.beam, bx, by, 12.f, PAL_WOOD, d == 0, 0);
    }

    int fr = int(modeT_ * 6.f) & 1;
    float kx = wx + (d == 0 ? -22.f : d == 1 ? 22.f : -8.f);
    shadow(kx, wy + 30.f, 22.f);
    spr(art_.keeper[fr], kx, wy + 8.f, 36.f, PAL_KEEPER, d == 0, 0);

    spr(art_.lamp, 36.f, 78.f, 40.f, PAL_NIGHT, false, 0);
    spr(art_.lamp, 286.f, 78.f, 40.f, PAL_NIGHT, false, 0);
    float gx = 50.f + std::sin(modeT_ * 0.7f) * 30.f;
    spr(art_.gull, gx, 42.f + std::sin(modeT_ * 2.1f) * 3.f, 8.f, PAL_NIGHT, false, 2);

    char buf[48];
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        std::snprintf(buf, sizeof buf, "WAVE %d/3", std::min(wave_ + 1, 3));
        hud(2, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(34, 1, buf, PAL_AMBER);
        std::snprintf(buf, sizeof buf, "STONES %d", stones_);
        hud(2, 26, buf, stones_ <= 2 ? PAL_RED : PAL_HUD);
        hud(24, 26, braced_ ? "BEAM SET" : "BEAM FREE", braced_ ? PAL_GREEN : PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hud(6, 23, "THREE WAVES  KEEP THE WELL UP", PAL_HUD);
        hud(4, 25, "LEFT  RIGHT  UP OR A BRACES IT", PAL_AMBER);
    } else if (mode_ == Mode::Victory) {
        hud(8, 25, "THE WELL STOOD", PAL_GREEN);
    } else if (mode_ == Mode::Over) {
        hud(10, 25, reason_, PAL_RED);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.16f, 0.3f, 0.18f);
    if (bot_) beginWatch();
    else {
        mode_ = Mode::Title;
        sys.setLight(30, 70, 100);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (bot_ && mode_ == Mode::Title) beginWatch();
    modeT_ += DT;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT * 1.6f);
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) beginWatch();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Watch) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(DT);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Watch;
        else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else if (mode_ == Mode::Victory || mode_ == Mode::Over) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) beginWatch();
    }
    draw();
}

}  // namespace well
