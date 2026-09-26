#include "game/dawn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace yarddawn {
namespace {

constexpr float kX[Game::kFlares] = {44.f, 102.f, 160.f, 218.f, 276.f};
constexpr float kNight = 40.f;
constexpr float kSpeed = 188.f;
constexpr float kReach = 26.f;
constexpr float kFeedAt = 88.f;
constexpr float kDrain0 = 4.8f;
constexpr float kDrain1 = 7.6f;
constexpr float kCanvasWarn = 2.05f;
constexpr float kDrumWarn = 2.2f;
constexpr float kCanvasDmg = 22.f;
constexpr float kDrumDmg = 20.f;
constexpr float kFeet = 208.f;
constexpr float kPotFoot = 198.f;
constexpr float kManH = 46.f;
constexpr float kDrumFrom = 250.f;
constexpr float kSheetX = 42.f;
constexpr float kSheetY = 108.f;
constexpr int kSkyline = 132;
constexpr float kLampX[2] = {148.f, 274.f};
constexpr float kLampY[2] = {64.f, 102.f};

struct Ev {
    float t;
    int flare;
};
const Ev kCanvas[] = {{5.f, 0}, {11.5f, 4}, {18.f, 2}, {24.5f, 1}, {31.f, 3}, {36.f, 0}};
const Ev kDrums[] = {{8.f, 3}, {14.5f, 1}, {21.f, 4}, {28.f, 0}, {34.f, 2}};
const float kFan[] = {523.f, 659.f, 784.f, 1046.f};
const int kStars[][2] = {{18, 12}, {46, 28}, {78, 16}, {124, 34}, {186, 18}, {230, 30},
                         {268, 12}, {300, 26}, {150, 14}, {96, 40}, {210, 42}, {40, 44}};

static_assert(sizeof(kX) / sizeof(kX[0]) == Game::kFlares, "flare count");

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto L = [&](int x, int y) { return int(std::lround(x + (y - x) * t)); };
    return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
}

float smooth(float a, float b, float x) {
    float t = std::clamp((x - a) / (b - a), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    return 1;
}

int Game::lit() const {
    int n = 0;
    for (int i = 0; i < kFlares; i++)
        if (fuel_[i] > 0.5f) n++;
    return n;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.18f, 0.1f);
    buildArt(sys.vdp, art_);
    bootTitle();
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    fed_ = 0;
    hauled_ = 0;
    braced_ = 0;
    dead_ = -1;
    focus_ = -1;
    hold_ = 0;
    toneT_ = 0;
    fanI_ = 0;
    fanT_ = 0;
    canvasIx_ = 0;
    drumIx_ = 0;
    face_ = 1;
    t_ = 0;
    px_ = 160.f;
    move_ = 0;
    feedCd_ = 0;
    shake_ = 0;
    motes_.clear();
    for (int i = 0; i < kFlares; i++) {
        fuel_[i] = 100.f;
        pop_[i] = 0;
        hurt_[i] = 0;
        canvasOn_[i] = false;
        canvasEta_[i] = 0;
        drumOn_[i] = false;
        drumEta_[i] = 0;
        lowPing_[i] = false;
    }
}

void Game::beginWatch() {
    bootTitle();
    mode_ = Mode::Watch;
    blip(294.f, 0.05f, 10);
    sys_->apu.tone(1, 196.f, 0.03f);
}

void Game::beginWin() {
    mode_ = Mode::Won;
    won_ = true;
    hold_ = 0;
    fanI_ = 0;
    fanT_ = 0;
    move_ = 0;
    sys_->setLight(255, 176, 70);
}

void Game::beginLoss(int flare) {
    mode_ = Mode::Lost;
    won_ = false;
    dead_ = flare;
    hold_ = 0;
    move_ = 0;
    fuel_[flare] = 0;
    sys_->rumble(0.7f, 0.35f, 180);
    sys_->setLight(90, 8, 0);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    switch (mode_) {
        case Mode::Title: updateTitle(); break;
        case Mode::Watch: updateWatch(dt); break;
        case Mode::Pause: updatePause(); break;
        case Mode::Won: updateEnd(true); break;
        case Mode::Lost: updateEnd(false); break;
    }
    if (mode_ != Mode::Title) stepMotes(dt);
    if (toneT_ > 0 && --toneT_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt);
    draw();
}

void Game::updateTitle() {
    if (bot_) {
        if (sys_->frame >= 12) beginWatch();
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_MODE)) {
        if (sys_->hasHome()) sys_->eject();
        else sys_->quit();
        return;
    }
    bool go = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) ||
              p.pressed(gs::BTN_X) || p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_TURBO);
    if (go) beginWatch();
}

void Game::updatePause() {
    if (sys_->pad.pressed(gs::BTN_START)) mode_ = Mode::Watch;
    else if (sys_->pad.pressed(gs::BTN_MODE)) bootTitle();
}

void Game::updateEnd(bool dawn) {
    hold_++;
    if (dawn) {
        if (fanT_ > 0) fanT_--;
        if (fanT_ == 0 && fanI_ < 4) {
            float f = kFan[fanI_++];
            sys_->apu.tone(0, f, 0.07f);
            sys_->apu.tone(1, f * 0.5f, 0.03f);
            toneT_ = 14;
            fanT_ = 12;
            sys_->setLight(255, 186, 80);
        }
    } else if (hold_ == 1) {
        sys_->apu.tone(0, 98.f, 0.07f);
        sys_->apu.tone(1, 49.f, 0.04f);
        toneT_ = 36;
        sys_->apu.noiseBurst(0.22f, 640.f, 0.3f);
    }
    if (hold_ >= 90) over_ = true;
    if (!bot_ && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_C) ||
                  sys_->pad.pressed(gs::BTN_MODE)))
        bootTitle();
}

void Game::readPad(float& dir, bool& feed, bool& haul) const {
    const gs::Pad& p = sys_->pad;
    dir = 0;
    if (p.down(gs::BTN_LEFT)) dir -= 1.f;
    if (p.down(gs::BTN_RIGHT)) dir += 1.f;
    if (dir == 0.f && std::fabs(p.axisX) > 0.28f) dir = p.axisX > 0 ? 1.f : -1.f;
    feed = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO);
    haul = p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.down(gs::BTN_Y);
}

float Game::drain() const {
    float u = std::min(1.f, t_ / kNight);
    return kDrain0 + (kDrain1 - kDrain0) * u;
}

float Game::dawnEase() const {
    if (mode_ == Mode::Won) return 1.f;
    if (mode_ == Mode::Title || mode_ == Mode::Lost) return 0.f;
    return smooth(0.42f, 1.f, t_ / kNight);
}

float Game::lowestOther(int skip) const {
    float m = 999.f;
    for (int i = 0; i < kFlares; i++)
        if (i != skip) m = std::min(m, fuel_[i]);
    return m;
}

float Game::score(int i) const {
    float d = std::max(0.6f, drain());
    float tDie = fuel_[i] / d;
    float s = 820.f / (tDie + 0.2f);
    if (fuel_[i] < 46.f) s += 240.f;
    if (fuel_[i] < 28.f) s += 520.f;
    if (fuel_[i] < 16.f) s += 980.f;
    float travel = std::fabs(px_ - kX[i]) / kSpeed;
    s -= travel * 14.f;
    if (canvasOn_[i]) {
        float slack = canvasEta_[i] - travel;
        if (slack >= -0.04f) {
            s += 180.f;
            if (slack < 1.05f) s += 460.f;
            if (fuel_[i] - kCanvasDmg < 38.f) s += 720.f;
        }
    }
    if (drumOn_[i]) {
        float slack = drumEta_[i] - travel;
        if (slack >= -0.04f) {
            s += 200.f;
            if (slack < 1.2f) s += 500.f;
            if (fuel_[i] - kDrumDmg < 38.f) s += 740.f;
        }
    }
    return s;
}

bool Game::mustHold(int i) const {
    if (i < 0 || i >= kFlares) return false;
    float dist = std::fabs(px_ - kX[i]);
    if (dist > 24.f) return false;
    float other = lowestOther(i);
    if (drumOn_[i]) {
        if (drumEta_[i] < 1.05f) return true;
        if (drumEta_[i] < kDrumWarn && other > 34.f) return true;
    }
    if (canvasOn_[i]) {
        if (canvasEta_[i] < 0.85f) return true;
        if (canvasEta_[i] < 1.35f && other > 32.f) return true;
    }
    return false;
}

int Game::choose() {
    int best = 0;
    float bs = -1.f;
    for (int i = 0; i < kFlares; i++) {
        float s = score(i);
        if (s > bs) {
            bs = s;
            best = i;
        }
    }
    if (focus_ < 0 || focus_ >= kFlares) return best;
    if (mustHold(focus_)) return focus_;
    float dist = std::fabs(px_ - kX[focus_]);
    if (dist > 16.f && score(focus_) >= bs * 0.82f) return focus_;
    return best;
}

void Game::think(float& dir) {
    focus_ = choose();
    float dx = kX[focus_] - px_;
    if (mustHold(focus_) || std::fabs(dx) <= 12.f) dir = 0;
    else dir = dx > 0.f ? 1.f : -1.f;
}

void Game::spawn() {
    while (canvasIx_ < int(sizeof(kCanvas) / sizeof(kCanvas[0])) && t_ >= kCanvas[canvasIx_].t) {
        int i = kCanvas[canvasIx_].flare;
        if (!canvasOn_[i]) {
            canvasOn_[i] = true;
            canvasEta_[i] = kCanvasWarn;
        }
        canvasIx_++;
    }
    while (drumIx_ < int(sizeof(kDrums) / sizeof(kDrums[0])) && t_ >= kDrums[drumIx_].t) {
        int i = kDrums[drumIx_].flare;
        if (!drumOn_[i]) {
            drumOn_[i] = true;
            drumEta_[i] = kDrumWarn;
        }
        drumIx_++;
    }
}

int Game::nearest() const {
    int best = -1;
    float bd = kReach;
    for (int i = 0; i < kFlares; i++) {
        float d = std::fabs(px_ - kX[i]);
        if (d <= bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

float Game::drumX(int i) const {
    float p = std::clamp(1.f - drumEta_[i] / kDrumWarn, 0.f, 1.f);
    return kDrumFrom + (kX[i] - kDrumFrom) * p;
}

void Game::canvasAt(int i, float& x, float& y) const {
    float p = std::clamp(1.f - canvasEta_[i] / kCanvasWarn, 0.f, 1.f);
    x = kSheetX + (kX[i] - kSheetX) * p;
    y = kSheetY + (158.f - kSheetY) * p;
    y += std::sin(p * 16.f + i) * 3.f;
}

void Game::advanceThreats(float dt) {
    for (int i = 0; i < kFlares; i++) {
        if (canvasOn_[i]) {
            canvasEta_[i] -= dt;
            if (canvasEta_[i] <= 0.f) {
                canvasOn_[i] = false;
                fuel_[i] -= kCanvasDmg;
                hurt_[i] = 0.3f;
                shake_ = 0.12f;
                blip(128.f, 0.06f, 8);
                sys_->apu.noiseBurst(0.16f, 700.f, 0.18f);
                sys_->rumble(0.3f, 0.12f, 60);
                burst(kX[i], 160.f, 8, 24.f, PAL_CANVAS);
            }
        }
        if (drumOn_[i]) {
            drumEta_[i] -= dt;
            if ((sys_->frame % 5) == 0) burst(drumX(i), 190.f, 1, 10.f, PAL_SMOKE);
            if (drumEta_[i] <= 0.f) {
                drumOn_[i] = false;
                bool brace = std::fabs(px_ - kX[i]) <= kReach + 8.f;
                if (brace) {
                    braced_++;
                    blip(220.f, 0.05f, 6);
                    sys_->apu.noiseBurst(0.14f, 1400.f, 0.1f);
                    sys_->rumble(0.18f, 0.28f, 40);
                    burst(kX[i], 184.f, 7, 30.f, PAL_IRON);
                } else {
                    fuel_[i] -= kDrumDmg;
                    hurt_[i] = 0.32f;
                    shake_ = 0.16f;
                    blip(90.f, 0.07f, 10);
                    sys_->apu.noiseBurst(0.22f, 420.f, 0.22f);
                    sys_->rumble(0.45f, 0.2f, 90);
                    burst(kX[i], 186.f, 9, 26.f, PAL_DRUM);
                }
            }
        }
    }
}

void Game::updateWatch(float dt) {
    if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        move_ = 0;
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        return;
    }
    spawn();
    float dir = 0;
    bool feed = false, haul = false;
    if (bot_) think(dir);
    else readPad(dir, feed, haul);
    if (dir != 0.f) face_ = dir > 0.f ? 1 : -1;
    move_ = dir;
    px_ = std::clamp(px_ + dir * kSpeed * dt, 24.f, 300.f);
    if (feedCd_ > 0.f) feedCd_ -= dt;

    int here = nearest();
    if (bot_ && here >= 0 && fuel_[here] < kFeedAt) feed = true;
    if (bot_ && here >= 0 && canvasOn_[here] && canvasEta_[here] < 0.4f) haul = true;

    if (here >= 0 && feed && feedCd_ <= 0.f && fuel_[here] < kFeedAt) {
        fuel_[here] = 100.f;
        pop_[here] = 0.22f;
        feedCd_ = 0.16f;
        lowPing_[here] = false;
        fed_++;
        blip(460.f + here * 36.f, 0.05f, 6);
        burst(kX[here], 168.f, 8, 32.f, PAL_FIRE);
    }
    if (here >= 0 && haul && canvasOn_[here]) {
        canvasOn_[here] = false;
        hauled_++;
        blip(340.f, 0.05f, 6);
        sys_->apu.noiseBurst(0.1f, 1600.f, 0.08f);
        float cx, cy;
        canvasAt(here, cx, cy);
        burst(cx, cy, 7, 28.f, PAL_CANVAS);
    }

    advanceThreats(dt);
    float d = drain() * dt;
    for (int i = 0; i < kFlares; i++) {
        fuel_[i] -= d;
        if (pop_[i] > 0.f) pop_[i] -= dt;
        if (hurt_[i] > 0.f) hurt_[i] -= dt;
        if (fuel_[i] < 30.f && !lowPing_[i]) {
            lowPing_[i] = true;
            blip(188.f, 0.04f, 5);
        }
        if (fuel_[i] > 40.f) lowPing_[i] = false;
        if ((sys_->frame + i * 3) % 14 == 0 && fuel_[i] > 8.f)
            burst(kX[i] + std::sin(t_ * 3.f + i) * 2.f, 162.f, 1, 16.f, PAL_EMBER);
    }
    for (int i = 0; i < kFlares; i++) {
        if (fuel_[i] <= 0.f) {
            beginLoss(i);
            return;
        }
    }
    t_ += dt;
    if (t_ >= kNight) beginWin();
}

void Game::stepMotes(float dt) {
    for (auto& m : motes_) {
        m.x += m.vx * dt;
        m.y += m.vy * dt;
        m.vy += 28.f * dt;
        m.life -= dt;
    }
    motes_.erase(std::remove_if(motes_.begin(), motes_.end(), [](const Mote& m) { return m.life <= 0.f; }),
                 motes_.end());
    if (motes_.size() > 40) motes_.erase(motes_.begin(), motes_.begin() + int(motes_.size() - 40));
}

void Game::burst(float x, float y, int n, float speed, int pal) {
    for (int i = 0; i < n; i++) {
        float a = (i + 0.35f) / float(std::max(n, 1)) * 6.28318f;
        motes_.push_back({x, y, std::cos(a) * speed, std::sin(a) * speed - 12.f, 0.36f, pal});
    }
}

void Game::blip(float freq, float vol, int frames) {
    sys_->apu.tone(0, freq, vol);
    toneT_ = frames;
}

float Game::lineWidth(const std::string& s, float h) const {
    float w = 0;
    for (char ch : s) {
        if (ch == ' ') {
            w += h * 0.45f;
            continue;
        }
        char c = ch;
        if (c >= 'a' && c <= 'z') c = char(c - 32);
        if (c < 32 || c >= 127) continue;
        const gs::Mipped& g = art_.glyph[int(c) - 32];
        if (g.h < 1) continue;
        w += h * float(g.w) / float(g.h) + 1.f;
    }
    return w;
}

void Game::word(const std::string& s, float cx, float y, float h, int pal) {
    float pen = cx - lineWidth(s, h) * 0.5f;
    for (char ch : s) {
        if (ch == ' ') {
            pen += h * 0.45f;
            continue;
        }
        char c = ch;
        if (c >= 'a' && c <= 'z') c = char(c - 32);
        if (c < 32 || c >= 127) continue;
        const gs::Mipped& g = art_.glyph[int(c) - 32];
        if (g.h < 1) continue;
        float w = h * float(g.w) / float(g.h);
        spr(g, pen + w * 0.5f, y, h, pal);
        pen += w + 1.f;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow,
               int clip) {
    if (h < 1.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(std::max(1.f, w)));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    s.clipY = int16_t(clip);
    sys_->vdp.sprite(s);
}

void Game::sky() {
    float e = dawnEase();
    const uint16_t n0 = gs::rgb4(1, 1, 4);
    const uint16_t n1 = gs::rgb4(2, 2, 6);
    const uint16_t n2 = gs::rgb4(4, 3, 5);
    const uint16_t n3 = gs::rgb4(3, 2, 3);
    const uint16_t d0 = gs::rgb4(8, 8, 12);
    const uint16_t d1 = gs::rgb4(14, 9, 6);
    const uint16_t d2 = gs::rgb4(15, 11, 5);
    const uint16_t d3 = gs::rgb4(10, 6, 4);
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t night, dawn;
        if (y < 52) {
            float u = y / 52.f;
            night = mix(n0, n1, u);
            dawn = mix(d0, d1, u);
        } else if (y < 120) {
            float u = (y - 52) / 68.f;
            night = mix(n1, n2, u);
            dawn = mix(d1, d2, u);
        } else {
            float u = std::min(1.f, (y - 120) / 104.f);
            night = mix(n2, n3, u);
            dawn = mix(d2, d3, u);
        }
        uint16_t c = mix(night, dawn, e);
        if (mode_ == Mode::Lost) c = mix(c, gs::rgb4(6, 1, 1), hold_ < 20 ? 0.4f : 0.18f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
    }
}

void Game::lamp() {
    if (mode_ == Mode::Won) sys_->setLight(255, 176, 70);
    else if (mode_ == Mode::Lost) sys_->setLight(80, 6, 0);
    else if (mode_ == Mode::Title) sys_->setLight(28, 36, 84);
    else {
        float f = 0;
        for (int i = 0; i < kFlares; i++) f += fuel_[i];
        f = std::clamp(f / 500.f, 0.f, 1.f);
        float e = dawnEase();
        int r = int(40 + 90 * f + 110 * e);
        int g = int(22 + 30 * f + 70 * e);
        int b = int(18 + 10 * f + 20 * (1.f - e));
        sys_->setLight(std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255));
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

const char* Game::hint() const {
    int soon = -1;
    float eta = 99.f;
    bool drum = false;
    for (int i = 0; i < kFlares; i++) {
        if (canvasOn_[i] && canvasEta_[i] < eta) {
            eta = canvasEta_[i];
            soon = i;
            drum = false;
        }
        if (drumOn_[i] && drumEta_[i] < eta) {
            eta = drumEta_[i];
            soon = i;
            drum = true;
        }
    }
    bool dying = false;
    for (int i = 0; i < kFlares; i++)
        if (fuel_[i] < 26.f) dying = true;
    if (dying && (soon < 0 || eta > 0.8f)) return "A FLARE IS DYING";
    if (soon >= 0 && drum) return "DRUM - STAND AND BRACE";
    if (soon >= 0) return "CANVAS - HAUL IT";
    return "Z/C FEED   X HAUL   STAND BRACES";
}

void Game::drawWorld() {
    const uint64_t fr = sys_->frame;
    float ease = dawnEase();
    float jx = 0, jy = 0;
    if (shake_ > 0.f) {
        jx = std::sin(float(fr) * 1.7f) * shake_ * 10.f;
        jy = std::cos(float(fr) * 2.1f) * shake_ * 6.f;
    }

    if (mode_ == Mode::Title) {
        word("S3 YARD DAWN", 160.f, 12.f, 14.f, PAL_GOLD);
        word("KEEP THE FLARES LIT", 160.f, 32.f, 11.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        word("DAWN", 160.f, 14.f, 18.f, PAL_GOLD);
    } else if (mode_ == Mode::Lost) {
        word("DARK", 160.f, 14.f, 18.f, PAL_ALERT);
    }

    if (mode_ != Mode::Title) {
        for (int i = 0; i < kFlares; i++) {
            int n = 0;
            if (fuel_[i] > 0.5f) n = std::clamp(1 + int(fuel_[i] / 20.f), 1, 5);
            bool blink = fuel_[i] < 24.f && ((fr / 6) % 2 == 0);
            for (int k = 0; k < 5; k++) {
                int pal = PAL_PIP;
                if (k < n) pal = blink ? PAL_ALERT : (fuel_[i] < 32.f ? PAL_EMBER : PAL_GOLD);
                spr(art_.spark, kX[i] - 16.f + k * 8.f + jx, 78.f, k < n ? 5.f : 3.f, pal);
            }
            if (mode_ == Mode::Watch && (canvasOn_[i] || drumOn_[i] || fuel_[i] < 26.f)) {
                const gs::Mipped& g = art_.glyph[int('!') - 32];
                spr(g, kX[i] + jx, 64.f, 11.f, fuel_[i] < 26.f ? PAL_ALERT : PAL_GOLD);
            }
        }
    }

    for (auto& m : motes_) spr(art_.spark, m.x + jx, m.y + jy, 4.f, m.pal);

    for (int i = 0; i < kFlares; i++) {
        if (!drumOn_[i]) continue;
        float x = drumX(i);
        int frd = (int(std::fabs(x) / 8.f) & 1);
        spr(art_.drum[frd], x + jx, 188.f, 20.f, PAL_DRUM, x < kDrumFrom);
    }
    for (int i = 0; i < kFlares; i++) {
        if (!canvasOn_[i]) continue;
        float x, y;
        canvasAt(i, x, y);
        spr(art_.canvas, x + jx, y, 16.f, PAL_CANVAS, ((fr / 6) & 1));
    }

    int ff = int((fr / 7) % 2);
    for (int i = 0; i < kFlares; i++) {
        if (fuel_[i] <= 0.5f) continue;
        if (hurt_[i] > 0.16f && ((fr / 3) & 1)) continue;
        bool low = fuel_[i] < 18.f && ((fr / 4) % 2 == 0);
        if (low) continue;
        float fh = 16.f + fuel_[i] * 0.14f + std::max(0.f, pop_[i]) * 16.f;
        if (canvasOn_[i]) fh *= 0.82f;
        int pal = fuel_[i] < 32.f ? PAL_EMBER : PAL_FIRE;
        float fx = kX[i] + std::sin(float(fr) * 0.35f + i) * 1.4f;
        spr(art_.flame[ff], fx + jx, 176.f, fh, pal, false, 0, true);
    }

    float bob = move_ != 0.f ? std::sin(px_ * 0.38f) * 1.2f : std::sin(float(fr) * 0.08f) * 0.45f;
    int mf = (move_ != 0.f) ? (int(px_ / 7.f) & 1) : 0;
    spr(art_.man[mf], px_ + jx, kFeet - bob + jy, kManH, PAL_MAN, face_ < 0, 0, true);
    spr(art_.shade, px_ + jx, kFeet + 2.f, 6.f, PAL_MAN, false, 0, false, true);

    for (int i = 0; i < kFlares; i++) {
        if (fuel_[i] <= 0.5f || (mode_ == Mode::Lost && i == dead_)) {
            float sy = 150.f - float((fr / 5 + i) % 8);
            spr(art_.smoke[(fr / 8 + i) & 1], kX[i] + jx, sy, 16.f, PAL_SMOKE);
        }
    }

    for (int i = 0; i < kFlares; i++) {
        spr(art_.shade, kX[i] + jx, kPotFoot + 2.f, 5.f, PAL_MAN, false, 0, false, true);
        spr(art_.pot, kX[i] + jx, kPotFoot, 18.f, PAL_IRON, false, 0, true);
    }

    for (int i = 0; i < 2; i++) {
        if ((fr / 9 + i) % 11 == 0) continue;
        float lh = 14.f + ((fr / 6 + i) % 3 == 0 ? 2.f : 0.f);
        int fog = ease > 0.85f ? 6 : 0;
        spr(art_.lamp, kLampX[i], kLampY[i], lh, PAL_LAMP, false, fog);
    }

    if (ease < 0.8f) {
        for (int s = 0; s < int(sizeof(kStars) / sizeof(kStars[0])); s++) {
            if (ease > 0.4f && ((s + int(fr / 10)) % 3 == 0)) continue;
            bool titleBand = kStars[s][1] < 46 && kStars[s][0] > 28 && kStars[s][0] < 292;
            if (mode_ == Mode::Title && titleBand) continue;
            float tw = (s % 3 == 0) ? 5.f : 4.f;
            spr(art_.star, float(kStars[s][0]), float(kStars[s][1]), tw, PAL_MOON);
        }
    }
    if (ease < 0.72f) spr(art_.moon, 26.f, 22.f, 18.f * (1.f - ease * 0.25f), PAL_MOON);
    if (ease > 0.48f) {
        float sunY = 118.f - (ease - 0.48f) * 2.f * 70.f;
        spr(art_.sun, 246.f, sunY, 28.f, PAL_SUN, false, 0, false, false, kSkyline);
    }
}

void Game::drawHud() {
    if (mode_ == Mode::Title) {
        for (int row = 24; row <= 27; row++)
            for (int x = 0; x < 40; x++) sys_->vdp.HUD.set(x, row, gs::entry(art_.bar, PAL_HUD));
        hudC(25, "FEED THE POTS   HAUL THE CANVAS", PAL_HUD);
        hudC(26, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        if ((sys_->frame / 30) % 2 == 0) hudC(27, "ENTER TAKES THE YARD", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
        hudC(14, "ENTER", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Won) {
        hudC(11, "THE FLARES HELD", PAL_GOLD);
        hudC(13, "UNTIL DAWN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Lost) {
        hudC(11, lit() == 0 ? "THE FLARES WENT OUT" : "A FLARE WENT OUT", PAL_ALERT);
        hudC(13, "THE YARD GOES DARK", PAL_HUD);
        return;
    }
    hud(1, 0, "YARD", PAL_HUD);
    int left = std::max(0, int(std::ceil(kNight - t_ - 0.001f)));
    char buf[20];
    std::snprintf(buf, sizeof buf, "DAWN %d:%02d", left / 60, left % 60);
    hud(30, 0, buf, left <= 10 ? PAL_ALERT : PAL_GOLD);
    const char* h = hint();
    bool warn = h[0] == 'A' || h[0] == 'D' || h[0] == 'C';
    if (!warn || (sys_->frame / 10) % 2 == 0) hudC(27, h, warn ? PAL_ALERT : PAL_HUD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    lamp();
    drawWorld();
    drawHud();
}

}  // namespace yarddawn
