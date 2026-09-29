#include "game/dawn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace orcharddawn {
namespace {

constexpr float kX[Game::kFlares] = {48.f, 104.f, 160.f, 216.f, 272.f};
constexpr float kTreeX[6] = {18.f, 76.f, 132.f, 188.f, 244.f, 302.f};
constexpr float kNight = 36.f;
constexpr float kSpeed = 220.f;
constexpr float kReach = 28.f;
constexpr float kFeedAt = 74.f;
constexpr float kDrain0 = 3.4f;
constexpr float kDrain1 = 5.2f;
constexpr float kAppleWarn = 2.35f;
constexpr float kGustWarn = 2.15f;
constexpr float kAppleDmg = 16.f;
constexpr float kGustDmg = 14.f;
constexpr float kFeet = 206.f;
constexpr float kPotFoot = 198.f;
constexpr float kManH = 42.f;
constexpr float kAppleFromY = 78.f;
constexpr float kGustFrom = -18.f;
constexpr int kHorizon = 148;
const float kFan[] = {392.f, 494.f, 587.f, 784.f};
const int kStars[][2] = {{22, 18}, {54, 32}, {90, 14}, {130, 36}, {174, 16}, {214, 28},
                         {250, 12}, {292, 30}, {40, 48}, {110, 46}, {200, 44}, {270, 40}};

struct Ev {
    float t;
    int flare;
};
const Ev kApples[] = {{7.f, 2}, {13.f, 0}, {19.5f, 4}, {26.f, 1}, {32.f, 3}};
const Ev kGusts[] = {{4.5f, 4}, {10.5f, 1}, {16.5f, 3}, {23.f, 0}, {29.f, 2}};

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
    sys.apu.setEcho(0.1f, 0.16f, 0.08f);
    buildArt(sys.vdp, art_);
    bootTitle();
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    fed_ = 0;
    picked_ = 0;
    braced_ = 0;
    dead_ = -1;
    focus_ = -1;
    hold_ = 0;
    toneT_ = 0;
    fanI_ = 0;
    fanT_ = 0;
    appleIx_ = 0;
    gustIx_ = 0;
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
        appleOn_[i] = false;
        appleEta_[i] = 0;
        gustOn_[i] = false;
        gustEta_[i] = 0;
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
    sys_->setLight(255, 170, 64);
}

void Game::beginLoss(int flare) {
    mode_ = Mode::Lost;
    won_ = false;
    dead_ = flare;
    hold_ = 0;
    move_ = 0;
    fuel_[flare] = 0;
    sys_->rumble(0.7f, 0.3f, 180);
    sys_->setLight(80, 10, 0);
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
        if (sys_->frame >= 10) beginWatch();
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
        sys_->apu.tone(0, 92.f, 0.07f);
        sys_->apu.tone(1, 46.f, 0.04f);
        toneT_ = 36;
        sys_->apu.noiseBurst(0.2f, 520.f, 0.28f);
    }
    if (hold_ >= 90) over_ = true;
    if (!bot_ && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_C) ||
                  sys_->pad.pressed(gs::BTN_MODE)))
        bootTitle();
}

void Game::readPad(float& dir, bool& feed, bool& pick) const {
    const gs::Pad& p = sys_->pad;
    dir = 0;
    if (p.down(gs::BTN_LEFT)) dir -= 1.f;
    if (p.down(gs::BTN_RIGHT)) dir += 1.f;
    if (dir == 0.f && std::fabs(p.axisX) > 0.28f) dir = p.axisX > 0 ? 1.f : -1.f;
    feed = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO);
    pick = p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.down(gs::BTN_Y);
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
    float s = 700.f / (tDie + 0.25f);
    if (fuel_[i] < 50.f) s += 180.f;
    if (fuel_[i] < 32.f) s += 420.f;
    if (fuel_[i] < 18.f) s += 900.f;
    float travel = std::fabs(px_ - kX[i]) / kSpeed;
    s -= travel * 12.f;
    if (appleOn_[i]) {
        float slack = appleEta_[i] - travel;
        if (slack >= -0.05f) {
            s += 260.f;
            if (slack < 1.1f) s += 620.f;
            if (fuel_[i] - kAppleDmg < 36.f) s += 800.f;
        }
    }
    if (gustOn_[i]) {
        float slack = gustEta_[i] - travel;
        if (slack >= -0.05f) {
            s += 240.f;
            if (slack < 1.0f) s += 580.f;
            if (fuel_[i] - kGustDmg < 36.f) s += 760.f;
        }
    }
    return s;
}

bool Game::mustHold(int i) const {
    if (i < 0 || i >= kFlares) return false;
    float dist = std::fabs(px_ - kX[i]);
    if (dist > 22.f) return false;
    float other = lowestOther(i);
    if (gustOn_[i]) {
        if (gustEta_[i] < 0.95f) return true;
        if (gustEta_[i] < kGustWarn && other > 30.f) return true;
    }
    if (appleOn_[i] && appleEta_[i] < 0.35f && other > 28.f) return true;
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
    while (appleIx_ < int(sizeof(kApples) / sizeof(kApples[0])) && t_ >= kApples[appleIx_].t) {
        int i = kApples[appleIx_].flare;
        if (!appleOn_[i]) {
            appleOn_[i] = true;
            appleEta_[i] = kAppleWarn;
        }
        appleIx_++;
    }
    while (gustIx_ < int(sizeof(kGusts) / sizeof(kGusts[0])) && t_ >= kGusts[gustIx_].t) {
        int i = kGusts[gustIx_].flare;
        if (!gustOn_[i]) {
            gustOn_[i] = true;
            gustEta_[i] = kGustWarn;
        }
        gustIx_++;
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

void Game::appleAt(int i, float& x, float& y) const {
    float p = std::clamp(1.f - appleEta_[i] / kAppleWarn, 0.f, 1.f);
    x = kTreeX[i] + (kX[i] - kTreeX[i]) * p;
    y = kAppleFromY + (168.f - kAppleFromY) * (p * p);
}

float Game::gustX(int i) const {
    float p = std::clamp(1.f - gustEta_[i] / kGustWarn, 0.f, 1.f);
    return kGustFrom + (kX[i] - kGustFrom) * p;
}

void Game::advanceThreats(float dt) {
    for (int i = 0; i < kFlares; i++) {
        if (appleOn_[i]) {
            appleEta_[i] -= dt;
            if (appleEta_[i] <= 0.f) {
                appleOn_[i] = false;
                fuel_[i] -= kAppleDmg;
                hurt_[i] = 0.28f;
                shake_ = 0.1f;
                blip(140.f, 0.05f, 8);
                sys_->apu.noiseBurst(0.12f, 900.f, 0.12f);
                burst(kX[i], 170.f, 6, 22.f, PAL_APPLE);
            }
        }
        if (gustOn_[i]) {
            gustEta_[i] -= dt;
            if ((sys_->frame % 4) == 0) burst(gustX(i), 150.f + (i % 3) * 6.f, 1, 14.f, PAL_WIND);
            if (gustEta_[i] <= 0.f) {
                gustOn_[i] = false;
                bool brace = std::fabs(px_ - kX[i]) <= kReach + 6.f;
                if (brace) {
                    braced_++;
                    blip(330.f, 0.05f, 6);
                    burst(kX[i], 160.f, 6, 28.f, PAL_LEAF);
                } else {
                    fuel_[i] -= kGustDmg;
                    hurt_[i] = 0.26f;
                    shake_ = 0.12f;
                    blip(110.f, 0.06f, 8);
                    sys_->apu.noiseBurst(0.16f, 480.f, 0.16f);
                    burst(kX[i], 164.f, 8, 24.f, PAL_WIND);
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
    bool feed = false, pick = false;
    if (bot_) think(dir);
    else readPad(dir, feed, pick);
    if (dir != 0.f) face_ = dir > 0.f ? 1 : -1;
    move_ = dir;
    px_ = std::clamp(px_ + dir * kSpeed * dt, 22.f, 304.f);
    if (feedCd_ > 0.f) feedCd_ -= dt;

    int here = nearest();
    if (bot_ && here >= 0 && fuel_[here] < kFeedAt && !(gustOn_[here] && gustEta_[here] < 0.7f)) feed = true;
    if (bot_ && here >= 0 && appleOn_[here]) pick = true;

    if (here >= 0 && feed && feedCd_ <= 0.f && fuel_[here] < kFeedAt) {
        fuel_[here] = 100.f;
        pop_[here] = 0.2f;
        feedCd_ = 0.14f;
        lowPing_[here] = false;
        fed_++;
        blip(420.f + here * 28.f, 0.05f, 6);
        burst(kX[here], 160.f, 7, 30.f, PAL_FIRE);
    }
    if (here >= 0 && pick && appleOn_[here]) {
        appleOn_[here] = false;
        picked_++;
        blip(520.f, 0.05f, 5);
        float ax, ay;
        appleAt(here, ax, ay);
        burst(ax, ay, 5, 20.f, PAL_APPLE);
    }

    advanceThreats(dt);
    float d = drain() * dt;
    for (int i = 0; i < kFlares; i++) {
        fuel_[i] -= d;
        if (pop_[i] > 0.f) pop_[i] -= dt;
        if (hurt_[i] > 0.f) hurt_[i] -= dt;
        if (fuel_[i] < 28.f && !lowPing_[i]) {
            lowPing_[i] = true;
            blip(176.f, 0.04f, 5);
        }
        if (fuel_[i] > 40.f) lowPing_[i] = false;
        if ((sys_->frame + i * 3) % 16 == 0 && fuel_[i] > 8.f)
            burst(kX[i] + std::sin(t_ * 2.4f + i) * 2.f, 156.f, 1, 14.f, PAL_EMBER);
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
        m.vy += 22.f * dt;
        m.life -= dt;
    }
    motes_.erase(std::remove_if(motes_.begin(), motes_.end(), [](const Mote& m) { return m.life <= 0.f; }),
                 motes_.end());
    if (motes_.size() > 48) motes_.erase(motes_.begin(), motes_.begin() + int(motes_.size() - 48));
}

void Game::burst(float x, float y, int n, float speed, int pal) {
    for (int i = 0; i < n; i++) {
        float a = (i + 0.3f) / float(std::max(n, 1)) * 6.28318f;
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
    const uint16_t n0 = gs::rgb4(1, 1, 5);
    const uint16_t n1 = gs::rgb4(1, 2, 6);
    const uint16_t n2 = gs::rgb4(2, 3, 5);
    const uint16_t g0 = gs::rgb4(1, 3, 1);
    const uint16_t d0 = gs::rgb4(7, 7, 12);
    const uint16_t d1 = gs::rgb4(14, 8, 5);
    const uint16_t d2 = gs::rgb4(15, 11, 6);
    const uint16_t dg = gs::rgb4(4, 8, 2);
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t night, dawn;
        if (y < kHorizon) {
            float u = y / float(kHorizon);
            night = mix(n0, y < 70 ? n1 : n2, u);
            dawn = mix(d0, y < 80 ? d1 : d2, u);
        } else {
            float u = (y - kHorizon) / float(gs::SCREEN_H - kHorizon);
            night = mix(g0, gs::rgb4(2, 4, 1), u);
            dawn = mix(dg, gs::rgb4(6, 7, 2), u);
        }
        uint16_t c = mix(night, dawn, e);
        if (mode_ == Mode::Lost) c = mix(c, gs::rgb4(5, 1, 1), hold_ < 18 ? 0.35f : 0.16f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::lamp() {
    if (mode_ == Mode::Won) sys_->setLight(255, 168, 60);
    else if (mode_ == Mode::Lost) sys_->setLight(70, 8, 0);
    else if (mode_ == Mode::Title) sys_->setLight(24, 40, 70);
    else {
        float f = 0;
        for (int i = 0; i < kFlares; i++) f += fuel_[i];
        f = std::clamp(f / 500.f, 0.f, 1.f);
        float e = dawnEase();
        int r = int(36 + 80 * f + 120 * e);
        int g = int(28 + 24 * f + 80 * e);
        int b = int(16 + 8 * f + 18 * (1.f - e));
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
    bool gust = false;
    for (int i = 0; i < kFlares; i++) {
        if (appleOn_[i] && appleEta_[i] < eta) {
            eta = appleEta_[i];
            soon = i;
            gust = false;
        }
        if (gustOn_[i] && gustEta_[i] < eta) {
            eta = gustEta_[i];
            soon = i;
            gust = true;
        }
    }
    bool dying = false;
    for (int i = 0; i < kFlares; i++)
        if (fuel_[i] < 26.f) dying = true;
    if (dying && (soon < 0 || eta > 0.9f)) return "A FLARE IS DYING";
    if (soon >= 0 && gust) return "WIND - STAND AND BRACE";
    if (soon >= 0) return "APPLE - PICK IT";
    return "Z FEEDS   X PICKS   STAND BRACES";
}

void Game::drawWorld() {
    const uint64_t fr = sys_->frame;
    float ease = dawnEase();
    float jx = 0, jy = 0;
    if (shake_ > 0.f) {
        jx = std::sin(float(fr) * 1.6f) * shake_ * 8.f;
        jy = std::cos(float(fr) * 2.f) * shake_ * 5.f;
    }

    if (mode_ == Mode::Title) {
        word("S3 ORCHARD DAWN", 160.f, 14.f, 13.f, PAL_GOLD);
        word("KEEP THE FLARES LIT", 160.f, 34.f, 10.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        word("DAWN", 160.f, 16.f, 18.f, PAL_GOLD);
    } else if (mode_ == Mode::Lost) {
        word("DARK", 160.f, 16.f, 18.f, PAL_ALERT);
    }

    if (ease < 0.75f) {
        for (int s = 0; s < 12; s++) {
            if (ease > 0.35f && ((s + int(fr / 12)) % 3 == 0)) continue;
            spr(art_.star, float(kStars[s][0]), float(kStars[s][1]), (s % 2) ? 4.f : 5.f, PAL_MOON);
        }
        spr(art_.moon, 36.f, 28.f, 16.f * (1.f - ease * 0.2f), PAL_MOON);
    }
    if (ease > 0.42f) {
        float sunY = 120.f - (ease - 0.42f) * 90.f;
        spr(art_.sun, 250.f, sunY, 26.f, PAL_SUN, false, 0, false, false, kHorizon);
    }

    for (int i = 0; i < 6; i++) {
        float h = 64.f + (i % 2) * 8.f;
        spr(art_.tree, kTreeX[i] + jx, 168.f, h, PAL_LEAF, false, ease > 0.7f ? 4 : 0, true);
    }

    for (auto& m : motes_) {
        const gs::Mipped& img = (m.pal == PAL_WIND || m.pal == PAL_LEAF) ? art_.leaf : art_.spark;
        spr(img, m.x + jx, m.y + jy, m.pal == PAL_WIND ? 7.f : 4.f, m.pal);
    }

    for (int i = 0; i < kFlares; i++) {
        if (!gustOn_[i]) continue;
        float x = gustX(i);
        spr(art_.leaf, x + jx, 148.f + std::sin(float(fr) * 0.4f + i) * 4.f, 10.f, PAL_WIND, (fr / 5 + i) & 1);
        spr(art_.leaf, x + 10.f + jx, 158.f, 8.f, PAL_LEAF, (fr / 4) & 1);
    }
    for (int i = 0; i < kFlares; i++) {
        if (!appleOn_[i]) continue;
        float x, y;
        appleAt(i, x, y);
        spr(art_.apple, x + jx, y, 12.f, PAL_APPLE);
    }

    int ff = int((fr / 6) % 2);
    for (int i = 0; i < kFlares; i++) {
        if (fuel_[i] <= 0.5f) continue;
        if (hurt_[i] > 0.14f && ((fr / 3) & 1)) continue;
        float fh = 14.f + fuel_[i] * 0.12f + std::max(0.f, pop_[i]) * 12.f;
        int pal = fuel_[i] < 30.f ? PAL_EMBER : PAL_FIRE;
        float fx = kX[i] + std::sin(float(fr) * 0.3f + i) * 1.2f;
        spr(art_.flame[ff], fx + jx, kPotFoot - 8.f, fh, pal, false, 0, true);
    }

    float bob = move_ != 0.f ? std::sin(px_ * 0.35f) * 1.1f : std::sin(float(fr) * 0.07f) * 0.4f;
    int mf = (move_ != 0.f) ? (int(px_ / 8.f) & 1) : 0;
    spr(art_.shade, px_ + jx, kFeet + 1.f, 5.f, PAL_MAN, false, 0, false, true);
    spr(art_.man[mf], px_ + jx, kFeet - bob + jy, kManH, PAL_MAN, face_ < 0, 0, true);

    for (int i = 0; i < kFlares; i++) {
        if (fuel_[i] <= 0.5f || (mode_ == Mode::Lost && i == dead_))
            spr(art_.leaf, kX[i] + jx, 150.f - float((fr / 6 + i) % 6), 8.f, PAL_SMOKE);
    }

    for (int i = 0; i < kFlares; i++) {
        spr(art_.shade, kX[i] + jx, kPotFoot + 1.f, 4.f, PAL_IRON, false, 0, false, true);
        spr(art_.pot, kX[i] + jx, kPotFoot, 16.f, PAL_IRON, false, 0, true);
        if (mode_ == Mode::Title) continue;
        int n = fuel_[i] > 0.5f ? std::clamp(1 + int(fuel_[i] / 22.f), 1, 5) : 0;
        bool blink = fuel_[i] < 24.f && ((fr / 6) % 2 == 0);
        for (int k = 0; k < 5; k++) {
            int pal = PAL_PIP;
            if (k < n) pal = blink ? PAL_ALERT : (fuel_[i] < 32.f ? PAL_EMBER : PAL_GOLD);
            spr(art_.spark, kX[i] - 14.f + k * 7.f + jx, 86.f, k < n ? 4.f : 3.f, pal);
        }
    }
}

void Game::drawHud() {
    if (mode_ == Mode::Title) {
        for (int row = 24; row <= 27; row++)
            for (int x = 0; x < 40; x++) sys_->vdp.HUD.set(x, row, gs::entry(art_.bar, PAL_HUD));
        hudC(25, "FEED THE FLARES   PICK THE FRUIT", PAL_HUD);
        hudC(26, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        if ((sys_->frame / 30) % 2 == 0) hudC(27, "ENTER TAKES THE ORCHARD", PAL_GOLD);
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
        hudC(11, "A FLARE WENT OUT", PAL_ALERT);
        hudC(13, "THE ORCHARD GOES DARK", PAL_HUD);
        return;
    }
    hud(1, 0, "ORCHARD", PAL_HUD);
    int left = std::max(0, int(std::ceil(kNight - t_ - 0.001f)));
    char buf[24];
    std::snprintf(buf, sizeof buf, "DAWN %d:%02d", left / 60, left % 60);
    hud(29, 0, buf, left <= 8 ? PAL_ALERT : PAL_GOLD);
    const char* h = hint();
    bool warn = h[0] == 'A' || h[0] == 'W';
    if (!warn || (sys_->frame / 10) % 2 == 0) hudC(27, h, warn ? PAL_ALERT : PAL_HUD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    sky();
    lamp();
    drawWorld();
    drawHud();
}

}  // namespace orcharddawn
