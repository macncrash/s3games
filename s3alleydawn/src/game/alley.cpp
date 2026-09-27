#include "game/alley.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace alleydawn {
namespace {

constexpr float kX[Game::kFlares] = {72.f, 116.f, 160.f, 204.f, 248.f};
constexpr float kNight = 36.f;
constexpr float kSpeed = 200.f;
constexpr float kReach = 22.f;
constexpr float kFeedAt = 86.f;
constexpr float kDrain0 = 4.4f;
constexpr float kDrain1 = 7.2f;
constexpr float kTarpWarn = 2.0f;
constexpr float kCanWarn = 2.15f;
constexpr float kTarpDmg = 20.f;
constexpr float kCanDmg = 18.f;
constexpr float kFeet = 200.f;
constexpr float kCanFrom = 292.f;
constexpr float kSheetX = 36.f;
constexpr float kSheetY = 96.f;
const float kFan[] = {392.f, 494.f, 587.f, 784.f};

struct Ev {
    float t;
    int flare;
};
const Ev kTarps[] = {{4.2f, 0}, {9.6f, 4}, {15.2f, 2}, {20.8f, 1}, {26.4f, 3}, {31.5f, 0}};
const Ev kCans[] = {{6.8f, 3}, {12.4f, 1}, {18.0f, 4}, {23.6f, 0}, {29.2f, 2}};

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
    sys.apu.setEcho(0.14f, 0.22f, 0.12f);
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
    tarpIx_ = 0;
    canIx_ = 0;
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
        tarpOn_[i] = false;
        tarpEta_[i] = 0;
        canOn_[i] = false;
        canEta_[i] = 0;
        lowPing_[i] = false;
    }
}

void Game::beginWatch() {
    bootTitle();
    mode_ = Mode::Watch;
    blip(262.f, 0.05f, 10);
    sys_->apu.tone(1, 174.f, 0.03f);
}

void Game::beginWin() {
    mode_ = Mode::Won;
    won_ = true;
    hold_ = 0;
    fanI_ = 0;
    fanT_ = 0;
    move_ = 0;
    sys_->setLight(255, 168, 64);
}

void Game::beginLoss(int flare) {
    mode_ = Mode::Lost;
    won_ = false;
    dead_ = flare;
    hold_ = 0;
    move_ = 0;
    fuel_[flare] = 0;
    sys_->rumble(0.7f, 0.35f, 180);
    sys_->setLight(80, 8, 0);
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
        sys_->apu.tone(0, 90.f, 0.07f);
        sys_->apu.tone(1, 45.f, 0.04f);
        toneT_ = 36;
        sys_->apu.noiseBurst(0.22f, 520.f, 0.3f);
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
    return smooth(0.5f, 1.f, t_ / kNight);
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
    if (fuel_[i] < 48.f) s += 260.f;
    if (fuel_[i] < 30.f) s += 540.f;
    if (fuel_[i] < 16.f) s += 1000.f;
    float travel = std::fabs(px_ - kX[i]) / kSpeed;
    s -= travel * 12.f;
    if (tarpOn_[i]) {
        float slack = tarpEta_[i] - travel;
        if (slack >= -0.04f) {
            s += 200.f;
            if (slack < 1.0f) s += 480.f;
            if (fuel_[i] - kTarpDmg < 40.f) s += 700.f;
        }
    }
    if (canOn_[i]) {
        float slack = canEta_[i] - travel;
        if (slack >= -0.04f) {
            s += 220.f;
            if (slack < 1.15f) s += 520.f;
            if (fuel_[i] - kCanDmg < 40.f) s += 720.f;
        }
    }
    return s;
}

bool Game::mustHold(int i) const {
    if (i < 0 || i >= kFlares) return false;
    float dist = std::fabs(px_ - kX[i]);
    if (dist > 20.f) return false;
    float other = lowestOther(i);
    if (canOn_[i]) {
        if (canEta_[i] < 1.1f) return true;
        if (canEta_[i] < kCanWarn && other > 36.f) return true;
    }
    if (tarpOn_[i]) {
        if (tarpEta_[i] < 0.7f) return true;
        if (tarpEta_[i] < 1.2f && other > 34.f) return true;
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
    if (dist > 14.f && score(focus_) >= bs * 0.84f) return focus_;
    return best;
}

void Game::think(float& dir) {
    focus_ = choose();
    float dx = kX[focus_] - px_;
    if (mustHold(focus_) || std::fabs(dx) <= 10.f) dir = 0;
    else dir = dx > 0.f ? 1.f : -1.f;
}

void Game::spawn() {
    while (tarpIx_ < int(sizeof(kTarps) / sizeof(kTarps[0])) && t_ >= kTarps[tarpIx_].t) {
        int i = kTarps[tarpIx_].flare;
        if (!tarpOn_[i]) {
            tarpOn_[i] = true;
            tarpEta_[i] = kTarpWarn;
        }
        tarpIx_++;
    }
    while (canIx_ < int(sizeof(kCans) / sizeof(kCans[0])) && t_ >= kCans[canIx_].t) {
        int i = kCans[canIx_].flare;
        if (!canOn_[i]) {
            canOn_[i] = true;
            canEta_[i] = kCanWarn;
        }
        canIx_++;
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

float Game::canX(int i) const {
    float p = std::clamp(1.f - canEta_[i] / kCanWarn, 0.f, 1.f);
    return kCanFrom + (kX[i] - kCanFrom) * p;
}

void Game::tarpAt(int i, float& x, float& y) const {
    float p = std::clamp(1.f - tarpEta_[i] / kTarpWarn, 0.f, 1.f);
    x = kSheetX + (kX[i] - kSheetX) * p;
    y = kSheetY + (150.f - kSheetY) * p;
    y += std::sin(p * 14.f + i) * 3.f;
}

void Game::advanceThreats(float dt) {
    for (int i = 0; i < kFlares; i++) {
        if (tarpOn_[i]) {
            tarpEta_[i] -= dt;
            if (tarpEta_[i] <= 0.f) {
                tarpOn_[i] = false;
                fuel_[i] -= kTarpDmg;
                hurt_[i] = 0.3f;
                shake_ = 0.12f;
                blip(110.f, 0.06f, 8);
                sys_->apu.noiseBurst(0.14f, 800.f, 0.16f);
                sys_->rumble(0.28f, 0.1f, 50);
                burst(kX[i], 156.f, 8, 22.f, PAL_TARP);
            }
        }
        if (canOn_[i]) {
            canEta_[i] -= dt;
            if ((sys_->frame % 6) == 0) burst(canX(i), 186.f, 1, 8.f, PAL_SMOKE);
            if (canEta_[i] <= 0.f) {
                canOn_[i] = false;
                bool brace = std::fabs(px_ - kX[i]) <= kReach + 6.f;
                if (brace) {
                    braced_++;
                    blip(196.f, 0.05f, 6);
                    sys_->apu.noiseBurst(0.12f, 1200.f, 0.1f);
                    sys_->rumble(0.16f, 0.26f, 40);
                    burst(kX[i], 180.f, 6, 28.f, PAL_IRON);
                } else {
                    fuel_[i] -= kCanDmg;
                    hurt_[i] = 0.32f;
                    shake_ = 0.14f;
                    blip(82.f, 0.07f, 10);
                    sys_->apu.noiseBurst(0.2f, 380.f, 0.2f);
                    sys_->rumble(0.4f, 0.18f, 80);
                    burst(kX[i], 182.f, 8, 24.f, PAL_CAN);
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
    px_ = std::clamp(px_ + dir * kSpeed * dt, 52.f, 268.f);
    if (feedCd_ > 0.f) feedCd_ -= dt;

    int here = nearest();
    if (bot_ && here >= 0 && fuel_[here] < kFeedAt) feed = true;
    if (bot_ && here >= 0 && tarpOn_[here] && tarpEta_[here] < 0.45f) haul = true;

    if (here >= 0 && feed && feedCd_ <= 0.f && fuel_[here] < kFeedAt) {
        fuel_[here] = 100.f;
        pop_[here] = 0.22f;
        feedCd_ = 0.14f;
        lowPing_[here] = false;
        fed_++;
        blip(420.f + here * 30.f, 0.05f, 6);
        burst(kX[here], 164.f, 7, 30.f, PAL_FIRE);
    }
    if (here >= 0 && haul && tarpOn_[here]) {
        tarpOn_[here] = false;
        hauled_++;
        blip(320.f, 0.05f, 6);
        sys_->apu.noiseBurst(0.1f, 1500.f, 0.08f);
        float cx, cy;
        tarpAt(here, cx, cy);
        burst(cx, cy, 6, 26.f, PAL_TARP);
    }

    advanceThreats(dt);
    float d = drain() * dt;
    for (int i = 0; i < kFlares; i++) {
        fuel_[i] -= d;
        if (pop_[i] > 0.f) pop_[i] -= dt;
        if (hurt_[i] > 0.f) hurt_[i] -= dt;
        if (fuel_[i] < 28.f && !lowPing_[i]) {
            lowPing_[i] = true;
            blip(170.f, 0.04f, 5);
        }
        if (fuel_[i] > 42.f) lowPing_[i] = false;
        if ((sys_->frame + i * 3) % 16 == 0 && fuel_[i] > 8.f)
            burst(kX[i] + std::sin(t_ * 3.f + i) * 2.f, 158.f, 1, 14.f, PAL_EMBER);
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
        m.vy += 30.f * dt;
        m.life -= dt;
    }
    motes_.erase(std::remove_if(motes_.begin(), motes_.end(), [](const Mote& m) { return m.life <= 0.f; }),
                 motes_.end());
    if (motes_.size() > 36) motes_.erase(motes_.begin(), motes_.begin() + int(motes_.size() - 36));
}

void Game::burst(float x, float y, int n, float speed, int pal) {
    for (int i = 0; i < n; i++) {
        float a = (i + 0.35f) / float(std::max(n, 1)) * 6.28318f;
        motes_.push_back({x, y, std::cos(a) * speed, std::sin(a) * speed - 10.f, 0.34f, pal});
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
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
    s.shadow = false;
    s.clipY = gs::SCREEN_H;
    sys_->vdp.sprite(s);
}

void Game::sky() {
    float e = dawnEase();
    const uint16_t n0 = gs::rgb4(1, 1, 3);
    const uint16_t n1 = gs::rgb4(2, 2, 5);
    const uint16_t n2 = gs::rgb4(3, 2, 4);
    const uint16_t d0 = gs::rgb4(6, 6, 10);
    const uint16_t d1 = gs::rgb4(14, 8, 5);
    const uint16_t d2 = gs::rgb4(15, 11, 6);
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t night, dawn;
        if (y < 90) {
            float u = y / 90.f;
            night = mix(n0, n1, u);
            dawn = mix(d0, d1, u);
        } else {
            float u = std::min(1.f, (y - 90) / 134.f);
            night = mix(n1, n2, u);
            dawn = mix(d1, d2, u);
        }
        uint16_t c = mix(night, dawn, e);
        if (mode_ == Mode::Lost) c = mix(c, gs::rgb4(5, 1, 1), hold_ < 18 ? 0.45f : 0.16f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::lamp() {
    if (mode_ == Mode::Won) sys_->setLight(255, 168, 64);
    else if (mode_ == Mode::Lost) sys_->setLight(70, 6, 0);
    else if (mode_ == Mode::Title) sys_->setLight(24, 28, 70);
    else {
        float f = 0;
        for (int i = 0; i < kFlares; i++) f += fuel_[i];
        f = std::clamp(f / 500.f, 0.f, 1.f);
        float e = dawnEase();
        int r = int(36 + 80 * f + 120 * e);
        int g = int(18 + 28 * f + 60 * e);
        int b = int(16 + 8 * f + 16 * (1.f - e));
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
    bool can = false;
    for (int i = 0; i < kFlares; i++) {
        if (tarpOn_[i] && tarpEta_[i] < eta) {
            eta = tarpEta_[i];
            soon = i;
            can = false;
        }
        if (canOn_[i] && canEta_[i] < eta) {
            eta = canEta_[i];
            soon = i;
            can = true;
        }
    }
    bool dying = false;
    for (int i = 0; i < kFlares; i++)
        if (fuel_[i] < 26.f) dying = true;
    if (dying && (soon < 0 || eta > 0.8f)) return "A FLARE IS DYING";
    if (soon >= 0 && can) return "CAN - STAND AND BRACE";
    if (soon >= 0) return "TARP - HAUL IT";
    return "A FEED   B HAUL   STAND BRACES";
}

void Game::drawWorld() {
    gs::VDP& v = sys_->vdp;
    v.A.clear();
    v.B.clear();
    for (int y = 4; y < 26; y++) {
        for (int x = 0; x < 6; x++) v.A.set(x, y, gs::entry(art_.brick, PAL_BRICK));
        for (int x = 34; x < 40; x++) v.A.set(x, y, gs::entry(art_.brick, PAL_BRICK));
    }
    for (int x = 0; x < 40; x++) {
        v.A.set(x, 24, gs::entry(art_.stripe, PAL_WET));
        v.A.set(x, 25, gs::entry(art_.wet, PAL_WET));
        v.A.set(x, 26, gs::entry(art_.wet, PAL_WET));
        v.A.set(x, 27, gs::entry(art_.dark, PAL_WET));
    }
    // fire-escape rungs and a drain
    for (int y = 8; y < 20; y += 3) {
        v.A.set(6, y, gs::entry(art_.mortar, PAL_IRON));
        v.A.set(7, y, gs::entry(art_.mortar, PAL_IRON));
        v.A.set(32, y, gs::entry(art_.mortar, PAL_IRON));
        v.A.set(33, y, gs::entry(art_.mortar, PAL_IRON));
    }
    for (int y = 6; y < 22; y++) {
        v.A.set(8, y, gs::entry(art_.dark, PAL_IRON));
        v.A.set(31, y, gs::entry(art_.dark, PAL_IRON));
    }

    const uint64_t fr = sys_->frame;
    float jx = 0;
    if (shake_ > 0.f) jx = std::sin(float(fr) * 1.7f) * shake_ * 8.f;

    if (mode_ != Mode::Won) spr(art_.moon, 200.f, 28.f, 16.f, PAL_MOON);
    else word("DAWN", 160.f, 18.f, 16.f, PAL_GOLD);

    if (mode_ == Mode::Title) {
        word("S3 ALLEY DAWN", 160.f, 36.f, 13.f, PAL_GOLD);
        word("KEEP THE FLARES LIT", 160.f, 54.f, 10.f, PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        word("DARK", 160.f, 22.f, 16.f, PAL_ALERT);
    }

    for (int i = 0; i < kFlares; i++) {
        spr(art_.pot, kX[i] + jx, kFeet - 2.f, 18.f, PAL_IRON, false, 0, true);
        bool out = fuel_[i] <= 0.5f || (dead_ == i && mode_ == Mode::Lost);
        if (!out) {
            int flick = int(fr / 4 + i) & 1;
            float h = 16.f + pop_[i] * 18.f + std::sin(float(fr) * 0.4f + i) * 1.4f;
            if (fuel_[i] < 24.f) h *= 0.7f;
            int pal = hurt_[i] > 0.f ? PAL_ALERT : (fuel_[i] < 30.f ? PAL_EMBER : PAL_FIRE);
            spr(art_.flame[flick], kX[i] + jx, kFeet - 16.f, h, pal, false, 0, true);
        }
        if (tarpOn_[i]) {
            float tx, ty;
            tarpAt(i, tx, ty);
            spr(art_.tarp, tx + jx, ty, 14.f, PAL_TARP);
        }
        if (canOn_[i]) {
            int roll = int(fr / 3) & 1;
            spr(art_.can[roll], canX(i) + jx, kFeet - 4.f, 20.f, PAL_CAN, false, 0, true);
        }
    }

    int step = (std::fabs(move_) > 0.1f && (fr / 7) & 1) ? 1 : 0;
    spr(art_.man[step], px_ + jx, kFeet, 46.f, PAL_MAN, face_ < 0, 0, true);

    for (const Mote& m : motes_) {
        float h = 3.f + m.life * 4.f;
        spr(art_.spark, m.x, m.y, h, m.pal);
    }
}

void Game::drawHud() {
    if (mode_ == Mode::Title) {
        hudC(24, "START  KEEP THEM LIT", PAL_HUD);
        hudC(26, "A FEED  B HAUL  STAND BRACES A CAN", PAL_PIP);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(24, "PAUSED", PAL_GOLD);
        hudC(26, "START RESUME   MODE TITLE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Won) {
        hudC(24, "THE FLARES HELD UNTIL DAWN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Lost) {
        hudC(24, "THE ALLEY GOES DARK", PAL_ALERT);
        return;
    }
    int left = int(std::ceil(std::max(0.f, kNight - t_)));
    char line[48];
    std::snprintf(line, sizeof(line), "LIT %d   DAWN %02d", lit(), left);
    hud(1, 1, line, PAL_HUD);
    hudC(26, hint(), PAL_GOLD);
}

void Game::draw() {
    sky();
    lamp();
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    sys_->vdp.hudEnabled = true;
    sys_->vdp.A.enabled = true;
    sys_->vdp.B.enabled = false;
    drawWorld();
    drawHud();
}

}  // namespace alleydawn
