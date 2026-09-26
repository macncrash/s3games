#include "game/dawn.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace rdawn {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kU[Game::kFlares] = {-0.78f, -0.26f, 0.26f, 0.78f};
constexpr float kZ[Game::kFlares] = {4.45f, 4.95f, 5.55f, 6.25f};
constexpr float kPlayerZ = 2.55f;
constexpr float kHorizon = 78.f;
constexpr float kZScale = 248.f;
constexpr float kSpeed = 1.75f;
constexpr float kWind = 0.92f;
constexpr float kReach = 0.17f;
constexpr float kEdge = 0.98f;
constexpr float kWarn = 2.6f;
constexpr float kGustDmg = 36.f;
constexpr float kDrain0 = 8.2f;
constexpr float kDrain1 = 13.8f;
constexpr float kFeedAt = 82.f;
constexpr float kNight = 36.f;
constexpr int kHold = 90;

struct GustEv {
    float t;
    int flare;
    int dir;
};
const GustEv kGusts[] = {{3.4f, 1, -1}, {8.6f, 3, 1},  {13.8f, 0, 1},
                         {19.0f, 2, -1}, {24.2f, 0, -1}, {29.6f, 3, -1}};
const float kFan[] = {330.f, 392.f, 494.f, 659.f, 784.f};
const int kStars[][2] = {{18, 14},  {46, 22},  {70, 12},  {248, 16}, {276, 28}, {300, 12},
                         {196, 18}, {220, 30}, {150, 14}, {112, 20}, {96, 40},  {260, 44}};

struct Prop {
    float z, u, h;
    int kind;
};
const Prop kProps[] = {{8.4f, -0.58f, 36.f, 0}, {9.6f, 0.70f, 32.f, 1}, {12.2f, 0.36f, 28.f, 0},
                       {14.0f, -0.74f, 26.f, 1}, {17.5f, 0.18f, 22.f, 0}, {21.0f, -0.40f, 18.f, 1}};

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

float smooth(float a, float b, float x) {
    float t = std::clamp((x - a) / (b - a), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    if (mode_ == Mode::Title) return 0;
    return 1;
}

int Game::lit() const {
    int n = 0;
    for (int i = 0; i < kFlares; ++i)
        if (fuel_[i] > 0.f) ++n;
    return n;
}

float Game::drain() const {
    float u = std::clamp(t_ / kNight, 0.f, 1.f);
    return kDrain0 + (kDrain1 - kDrain0) * u;
}

float Game::gustField() const {
    if (!gustOn_) return 0.f;
    float ramp = std::clamp((kWarn - gustEta_) / 0.35f, 0.f, 1.f);
    return float(gustDir_) * kWind * ramp;
}

float Game::windVel() const {
    if (!gustOn_) return 0.f;
    if (gustFlare_ >= 0 && std::fabs(u_ - kU[gustFlare_]) <= kReach) return 0.f;
    return gustField();
}

float Game::bendAt(float row) const { return std::sin(0.35f + row * 0.016f) * (6.f + row * 0.03f); }

float Game::halfAt(float row) const { return 24.f + row * 0.88f; }

int Game::horizon() const { return std::clamp(int(std::lround(kHorizon + shy_)), 60, 100); }

float Game::dawnEase() const {
    if (mode_ == Mode::Won) return 1.f;
    if (mode_ == Mode::Title) return 0.f;
    float u = std::clamp(t_ / kNight, 0.f, 1.f);
    if (mode_ == Mode::Lost) return u * 0.35f;
    return smooth(0.55f, 1.f, u);
}

Game::Spot Game::spot(float u, float z, float base) const {
    Spot s;
    if (!(z > 1.05f)) return s;
    float row = kZScale / z;
    float playerRow = kZScale / kPlayerZ;
    float t = std::clamp(row / playerRow, 0.04f, 1.45f);
    s.h = std::max(3.f, base * std::pow(t, 0.74f));
    s.x = 160.f + bendAt(row) + u * halfAt(row) + shx_;
    s.y = float(hor_) + row + shy_;
    float along = std::clamp((z - kPlayerZ) / 18.f, 0.f, 1.f);
    s.fog = int(std::clamp(along * 10.f, 0.f, 9.f));
    s.ok = true;
    return s;
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    reason_ = "UNFINISHED";
    fed_ = 0;
    shielded_ = 0;
    missed_ = 0;
    dead_ = -1;
    focus_ = 1;
    gustIx_ = 0;
    gustFlare_ = -1;
    gustDir_ = 1;
    hold_ = 0;
    toneT_ = 0;
    fanI_ = 0;
    fanT_ = 0;
    face_ = 1;
    t_ = 0;
    anim_ = 0;
    u_ = 0;
    feedCd_ = 0;
    shake_ = 0;
    gustEta_ = 0;
    gustOn_ = false;
    motes_.clear();
    for (int i = 0; i < kFlares; ++i) {
        fuel_[i] = 100.f;
        pop_[i] = 0;
        hurt_[i] = 0;
    }
}

void Game::beginWatch() {
    bootTitle();
    mode_ = Mode::Watch;
    blip(392.f, 0.05f, 8);
    sys_->setLight(90, 40, 16);
}

void Game::beginWin() {
    mode_ = Mode::Won;
    won_ = true;
    reason_ = "THE FLARES HELD UNTIL DAWN";
    hold_ = 0;
    fanI_ = 0;
    fanT_ = 0;
    gustOn_ = false;
    sys_->setLight(255, 170, 60);
}

void Game::beginLoss(int flare) {
    mode_ = Mode::Lost;
    won_ = false;
    dead_ = flare;
    reason_ = "A FLARE WENT OUT";
    hold_ = 0;
    gustOn_ = false;
    if (flare >= 0 && flare < kFlares) fuel_[flare] = 0;
    sys_->rumble(0.7f, 0.35f, 180);
    sys_->setLight(90, 8, 0);
    sys_->apu.noiseBurst(0.28f, 500.f, 0.3f);
}

void Game::blip(float freq, float vol, int frames) {
    sys_->apu.tone(0, freq, vol);
    toneT_ = frames;
}

void Game::readPad(float& dir, bool& feed) const {
    const gs::Pad& p = sys_->pad;
    dir = 0;
    if (p.down(gs::BTN_LEFT)) dir -= 1.f;
    if (p.down(gs::BTN_RIGHT)) dir += 1.f;
    if (dir == 0.f && std::fabs(p.axisX) > 0.22f) dir = std::clamp(p.axisX, -1.f, 1.f);
    feed = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO);
}

int Game::choose() {
    float d = std::max(0.4f, drain());
    int critical = -1;
    float critT = 2.4f;
    for (int i = 0; i < kFlares; ++i) {
        float td = fuel_[i] / d;
        if (td < critT) {
            critT = td;
            critical = i;
        }
    }
    if (critical >= 0) {
        bool gustHot = gustOn_ && gustEta_ < critT && fuel_[gustFlare_] - kGustDmg < fuel_[critical];
        if (!gustHot) return critical;
    }
    if (gustOn_ && gustFlare_ >= 0) return gustFlare_;
    int best = 0;
    float bestS = 1e9f;
    for (int i = 0; i < kFlares; ++i) {
        float td = fuel_[i] / d;
        float travel = std::fabs(kU[i] - u_) / kSpeed;
        float s = td + travel * 0.2f;
        if (s < bestS) {
            bestS = s;
            best = i;
        }
    }
    if (focus_ >= 0 && focus_ < kFlares && std::fabs(kU[focus_] - u_) > 0.08f) {
        float cur = fuel_[focus_] / d + std::fabs(kU[focus_] - u_) / kSpeed * 0.2f;
        if (cur < bestS + 0.4f) return focus_;
    }
    return best;
}

void Game::think(float& dir) {
    focus_ = choose();
    float du = kU[focus_] - u_;
    float wind = windVel();
    if (u_ > 0.90f && wind > 0.2f) dir = -1.f;
    else if (u_ < -0.90f && wind < -0.2f) dir = 1.f;
    else if (std::fabs(du) < 0.05f) {
        dir = 0.f;
        if (wind > 0.15f) dir = -1.f;
        else if (wind < -0.15f) dir = 1.f;
    } else {
        dir = du > 0.f ? 1.f : -1.f;
    }
}

int Game::nearest() const {
    int best = -1;
    float bd = kReach;
    for (int i = 0; i < kFlares; ++i) {
        float d = std::fabs(u_ - kU[i]);
        if (d <= bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::openGusts() {
    if (gustOn_) return;
    int n = int(sizeof(kGusts) / sizeof(kGusts[0]));
    if (gustIx_ >= n) return;
    if (t_ < kGusts[gustIx_].t) return;
    gustOn_ = true;
    gustFlare_ = kGusts[gustIx_].flare;
    gustDir_ = kGusts[gustIx_].dir;
    gustEta_ = kWarn;
    ++gustIx_;
    shake_ = std::max(shake_, 0.25f);
    sys_->apu.noiseBurst(0.12f, 1400.f, 0.18f);
    blip(220.f, 0.03f, 5);
}

void Game::resolveGust(float dt) {
    if (!gustOn_) return;
    gustEta_ -= dt;
    if (gustEta_ > 0.f) return;
    gustOn_ = false;
    int i = gustFlare_;
    bool shield = i >= 0 && std::fabs(u_ - kU[i]) <= kReach;
    if (shield) {
        ++shielded_;
        blip(660.f, 0.05f, 6);
        Spot s = spot(kU[i], kZ[i], 40.f);
        if (s.ok) burst(s.x, s.y - s.h * 0.8f, 7, 26.f, PAL_GOLD);
    } else if (i >= 0) {
        fuel_[i] -= kGustDmg;
        hurt_[i] = 0.32f;
        ++missed_;
        shake_ = std::max(shake_, 0.55f);
        blip(120.f, 0.06f, 8);
        sys_->apu.noiseBurst(0.2f, 700.f, 0.16f);
        sys_->rumble(0.4f, 0.15f, 80);
        Spot s = spot(kU[i], kZ[i], 40.f);
        if (s.ok) burst(s.x, s.y - s.h * 0.7f, 8, 20.f, PAL_SMOKE);
    }
}

void Game::tryFeed() {
    if (feedCd_ > 0.f) return;
    int i = nearest();
    if (i < 0 || fuel_[i] >= kFeedAt) return;
    fuel_[i] = 100.f;
    pop_[i] = 0.26f;
    feedCd_ = 0.14f;
    ++fed_;
    blip(500.f + i * 55.f, 0.05f, 6);
    Spot s = spot(kU[i], kZ[i], 36.f);
    if (s.ok) burst(s.x, s.y - s.h, 6, 28.f, PAL_FIRE);
}

void Game::embers() {
    if ((sys_->frame % 6) != 0) return;
    int i = int(sys_->frame / 6) % kFlares;
    if (fuel_[i] <= 0.5f) return;
    Spot s = spot(kU[i], kZ[i], 52.f);
    if (!s.ok) return;
    float j = std::sin(float(sys_->frame) * 1.7f + i * 2.1f);
    float top = s.y - s.h * 0.95f;
    motes_.push_back({s.x + j * 3.f, top, j * 8.f, -16.f - std::fabs(j) * 10.f, 0.42f, PAL_FIRE});
    if (motes_.size() > 40) motes_.erase(motes_.begin(), motes_.begin() + int(motes_.size() - 40));
}

void Game::stepMotes(float dt) {
    for (auto& m : motes_) {
        m.x += m.vx * dt;
        m.y += m.vy * dt;
        m.vy -= 10.f * dt;
        m.life -= dt;
    }
    motes_.erase(std::remove_if(motes_.begin(), motes_.end(), [](const Mote& m) { return m.life <= 0.f; }),
                 motes_.end());
}

void Game::burst(float x, float y, int n, float speed, int pal) {
    for (int i = 0; i < n; ++i) {
        float a = (i + 0.5f) / float(n) * 6.28318f;
        motes_.push_back({x, y, std::cos(a) * speed, std::sin(a) * speed - 8.f, 0.36f, pal});
    }
}

void Game::updateTitle(float dt) {
    anim_ += dt;
    u_ = std::sin(anim_ * 0.55f) * 0.2f;
    embers();
    if (bot_) {
        if (sys_->frame >= 16) beginWatch();
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

void Game::updateEnd(float dt) {
    ++hold_;
    anim_ += dt;
    if (mode_ == Mode::Won) {
        if (fanT_ > 0) --fanT_;
        if (fanT_ == 0 && fanI_ < 5) {
            float f = kFan[fanI_++];
            sys_->apu.tone(0, f, 0.07f);
            sys_->apu.tone(1, f * 0.5f, 0.03f);
            toneT_ = 16;
            fanT_ = 12;
        }
    } else if (hold_ == 1) {
        sys_->apu.tone(0, 98.f, 0.07f);
        sys_->apu.tone(1, 49.f, 0.04f);
        toneT_ = 40;
    }
    if (hold_ >= kHold) over_ = true;
    if (!bot_ && hold_ > 30 && sys_->pad.pressed(gs::BTN_START)) beginWatch();
    else if (!bot_ && sys_->pad.pressed(gs::BTN_MODE)) bootTitle();
    embers();
}

void Game::updateWatch(float dt) {
    if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        return;
    }
    if (!bot_ && sys_->pad.pressed(gs::BTN_MODE)) {
        bootTitle();
        return;
    }
    openGusts();
    float dir = 0;
    bool feed = false;
    if (bot_) {
        think(dir);
        feed = true;
    } else {
        readPad(dir, feed);
    }
    if (dir > 0.2f) face_ = 1;
    else if (dir < -0.2f) face_ = -1;
    float wind = windVel();
    u_ = std::clamp(u_ + (dir * kSpeed + wind) * dt, -kEdge, kEdge);
    if ((u_ <= -kEdge + 0.001f && wind < -0.2f) || (u_ >= kEdge - 0.001f && wind > 0.2f)) {
        shake_ = std::max(shake_, 0.35f);
        sys_->rumble(0.25f, 0.1f, 40);
    }
    if (feedCd_ > 0.f) feedCd_ -= dt;
    if (feed) tryFeed();
    resolveGust(dt);
    float loss = drain() * dt;
    for (int i = 0; i < kFlares; ++i) {
        fuel_[i] -= loss;
        if (pop_[i] > 0.f) pop_[i] -= dt;
        if (hurt_[i] > 0.f) hurt_[i] -= dt;
        if (fuel_[i] <= 0.f) {
            beginLoss(i);
            return;
        }
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 0.8f);
    t_ += dt;
    embers();
    if (t_ >= kNight) beginWin();
}

void Game::serviceAudio() {
    if (mode_ == Mode::Watch && gustOn_) sys_->apu.tone(2, 46.f, 0.022f);
    else if (toneT_ == 0) sys_->apu.tone(2, 0.f, 0.f);
}

const char* Game::hint() const {
    float wind = windVel();
    if (u_ > 0.90f && wind > 0.15f) return "THE LIP  STEP LEFT";
    if (u_ < -0.90f && wind < -0.15f) return "THE LIP  STEP RIGHT";
    if (gustOn_ && gustFlare_ >= 0) {
        if (std::fabs(u_ - kU[gustFlare_]) <= kReach) return "SHIELD THE FLARE";
        if (kU[gustFlare_] < u_) return "GUST  STEP LEFT";
        return "GUST  STEP RIGHT";
    }
    int low = 0;
    for (int i = 1; i < kFlares; ++i)
        if (fuel_[i] < fuel_[low]) low = i;
    if (fuel_[low] < 48.f) {
        if (std::fabs(u_ - kU[low]) <= kReach) return "FEED THE FLARE";
        if (kU[low] < u_) return "LOW FLARE LEFT";
        return "LOW FLARE RIGHT";
    }
    return "KEEP THEM LIT";
}

void Game::lamp() {
    if (mode_ == Mode::Won) sys_->setLight(255, 170, 60);
    else if (mode_ == Mode::Lost) sys_->setLight(80, 6, 0);
    else if (mode_ == Mode::Title) sys_->setLight(28, 36, 90);
    else {
        float f = 0;
        for (int i = 0; i < kFlares; ++i) f += fuel_[i];
        f = std::clamp(f / 400.f, 0.f, 1.f);
        sys_->setLight(int(50 + 170 * f), int(18 + 40 * f), 16);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow,
               int clip) {
    if (!(h > 1.5f) || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -2000, 2000));
    s.y = int16_t(std::clamp(int(std::lround(feet ? cy - s.h : cy - s.h * 0.5f)), -2000, 2000));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    s.clipY = int16_t(clip);
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) width += 10.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false);
        x += gw;
    }
}

void Game::layRoad() {
    gs::VDP& v = sys_->vdp;
    float e = dawnEase();
    uint16_t skyTop = mix(gs::rgb4(1, 1, 4), gs::rgb4(5, 6, 12), e);
    uint16_t skyMid = mix(gs::rgb4(2, 2, 8), gs::rgb4(12, 7, 6), e);
    uint16_t skyHor = mix(gs::rgb4(6, 4, 8), gs::rgb4(15, 10, 4), e);
    uint16_t abyss = mix(gs::rgb4(1, 1, 2), gs::rgb4(6, 3, 2), e);
    if (mode_ == Mode::Lost) {
        skyTop = mix(skyTop, gs::rgb4(5, 1, 2), 0.45f);
        skyMid = mix(skyMid, gs::rgb4(8, 2, 2), 0.4f);
        skyHor = mix(skyHor, gs::rgb4(10, 3, 2), 0.35f);
        abyss = mix(abyss, gs::rgb4(4, 1, 1), 0.4f);
    }
    v.setFogColor(skyHor);
    int warm = int(std::lround(e * 4.f));
    v.setColor(PAL_ROAD * 16 + 6, gs::rgb4(std::min(15, 4 + warm), std::min(15, 4 + warm / 2), 5));
    v.setColor(PAL_ROAD * 16 + 7, gs::rgb4(std::min(15, 3 + warm), 3 + warm / 3, 4));
    v.setColor(PAL_ROAD * 16 + 9, gs::rgb4(std::min(15, 5 + warm), std::min(15, 5 + warm / 2), 6));
    int flick = 10 + int(std::sin(float(sys_->frame) * 0.45f) * 3.f);
    v.setColor(PAL_FIRE * 16 + 3, gs::rgb4(15, std::clamp(flick, 6, 15), 2));
    v.setColor(PAL_FIRE * 16 + 4, gs::rgb4(15, std::clamp(flick + 3, 8, 15), 4));
    hor_ = horizon();
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& r = v.road[y];
        if (y <= hor_) {
            float t = float(y) / float(std::max(hor_, 1));
            uint16_t c = t < 0.55f ? mix(skyTop, skyMid, t / 0.55f) : mix(skyMid, skyHor, (t - 0.55f) / 0.45f);
            v.lineBackdrop[y] = c;
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y - hor_);
        r.on = true;
        r.cx = 160.f + bendAt(row) + shx_;
        r.hw = halfAt(row);
        r.v = row * 8.f + 40.f;
        r.pal = uint8_t(PAL_ROAD);
        r.style = gs::ROAD_ROCKY;
        r.band = (int(row) / 10) & 1;
        r.left = r.right = gs::GROUND_DROP;
        v.lineFog[y] = uint8_t(std::clamp(int(11.f - row * 0.14f), 0, 10));
        float dropT = std::clamp(row / 130.f, 0.f, 1.f);
        v.lineBackdrop[y] = mix(skyHor, abyss, dropT);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    shx_ = shy_ = 0.f;
    if (shake_ > 0.f) {
        float clk = mode_ == Mode::Title ? anim_ : t_;
        shx_ = std::sin(clk * 71.f) * 3.2f * shake_;
        shy_ = std::cos(clk * 53.f) * 1.6f * shake_;
    }
    layRoad();
    float e = dawnEase();
    float lean = gustField() * (windVel() == 0.f && gustOn_ ? 0.12f : 1.f) * 16.f;
    int ff = int(sys_->frame / 7) & 1;

    if (mode_ == Mode::Title) {
        text("ONE RIDGE", 160.f + shx_, 16.f, 0.95f, PAL_GOLD);
        text("KEEP THE FLARES LIT", 160.f + shx_, 36.f, 0.62f, PAL_HUD);
        text("UNTIL DAWN", 160.f + shx_, 54.f, 0.7f, PAL_GOLD);
    } else if (mode_ == Mode::Won) {
        text("DAWN", 160.f + shx_, 22.f, 1.15f, PAL_GOLD);
    } else if (mode_ == Mode::Lost) {
        text("DARK", 160.f + shx_, 22.f, 1.15f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f + shx_, 24.f, 0.9f, PAL_GOLD);
    }

    for (const auto& m : motes_) spr(art_.spark, m.x, m.y, 4.5f, m.pal, false, 0, false);

    float clk = mode_ == Mode::Title ? anim_ : t_;
    float bob = std::sin(clk * (std::fabs(u_) > 0.01f ? 14.f : 3.f)) * 1.4f;
    Spot you = spot(u_, kPlayerZ, 78.f);
    if (you.ok) {
        int fr = int(std::floor(std::fabs(u_) * 10.f + clk * 2.f)) & 1;
        spr(art_.warden[fr], you.x + lean * 0.35f, you.y + bob, you.h, PAL_YOU, face_ < 0, 0, true);
        spr(art_.shadow, you.x, you.y + 2.f, you.h * 0.22f, PAL_YOU, false, 0, false, true);
    }

    if (gustOn_) {
        float p = std::clamp(1.f - gustEta_ / kWarn, 0.f, 1.f);
        float gx = gustDir_ > 0 ? (8.f + p * 300.f) : (312.f - p * 300.f);
        spr(art_.gust, gx, 108.f + std::sin(clk * 8.f) * 3.f, 16.f, PAL_WIND, gustDir_ < 0, 0, false);
        spr(art_.gust, gx - gustDir_ * 48.f, 126.f, 11.f, PAL_WIND, gustDir_ < 0, 2, false);
    }

    int order[kFlares];
    for (int i = 0; i < kFlares; ++i) order[i] = i;
    std::sort(order, order + kFlares, [](int a, int b) { return kZ[a] < kZ[b]; });
    for (int n = 0; n < kFlares; ++n) {
        int i = order[n];
        Spot s = spot(kU[i], kZ[i], 74.f);
        if (!s.ok) continue;
        bool dead = fuel_[i] <= 0.5f || (mode_ == Mode::Lost && i == dead_);
        if (!dead) {
            bool low = fuel_[i] < 28.f;
            bool blink = low && ((sys_->frame / 4) % 2 == 0);
            if (!blink) {
                float fh = s.h * (0.34f + fuel_[i] * 0.0042f + std::max(0.f, pop_[i]) * 0.45f);
                int pal = low || hurt_[i] > 0.f ? PAL_EMBER : PAL_FIRE;
                float fx = s.x + lean * (0.4f + 0.15f * i);
                spr(art_.flame[ff], fx, s.y - s.h * 0.86f, fh, pal, lean < 0, s.fog, true);
                spr(art_.glow, fx, s.y - s.h * 0.8f, fh * 1.35f, pal, false, std::min(12, s.fog + 2), false);
            }
        } else {
            float sy = s.y - s.h * 0.9f - float((sys_->frame / 5) % 8);
            spr(art_.smoke[(sys_->frame / 8) & 1], s.x, sy, s.h * 0.32f, PAL_SMOKE, false, s.fog, false);
        }
        spr(art_.post, s.x, s.y, s.h, PAL_IRON, false, s.fog, true);
        spr(art_.shadow, s.x, s.y + 1.f, s.h * 0.16f, PAL_IRON, false, s.fog, false, true);
    }

    for (int i = 0; i < int(sizeof(kProps) / sizeof(kProps[0])); ++i) {
        const Prop& pr = kProps[i];
        Spot s = spot(pr.u, pr.z, pr.h);
        if (!s.ok) continue;
        if (pr.kind == 0) spr(art_.cairn, s.x, s.y, s.h, PAL_STONE, pr.u > 0, s.fog, true);
        else spr(art_.stake, s.x, s.y, s.h, PAL_IRON, pr.u < 0, s.fog, true);
    }

    if (e < 0.72f) spr(art_.moon, 34.f + shx_ * 0.2f, 30.f, 22.f * (1.f - e * 0.4f), PAL_NIGHT, false, int(e * 8), false);
    if (e > 0.2f) {
        float sunY = float(hor_) + 16.f - (e - 0.2f) * 78.f;
        spr(art_.sun, 188.f, sunY, 28.f, PAL_SUN, false, 0, false, false, hor_ + 3);
    }
    if (e < 0.6f) {
        for (int i = 0; i < int(sizeof(kStars) / sizeof(kStars[0])); ++i) {
            if (e > 0.35f && ((i + sys_->frame / 10) % 3 == 0)) continue;
            float tw = (i % 3 == 0) ? 6.f : 4.5f;
            if ((sys_->frame / 14 + i) % 8 == 0) tw = 3.f;
            spr(art_.star, float(kStars[i][0]), float(kStars[i][1]), tw, PAL_NIGHT, false, int(e * 6), false);
        }
    }
    spr(art_.peak[0], 54.f + shx_ * 0.15f, float(hor_) + 2.f, 52.f, PAL_MOUNT, false, 4, true, false, hor_ + 8);
    spr(art_.peak[1], 250.f + shx_ * 0.15f, float(hor_) + 4.f, 44.f, PAL_MOUNT, false, 5, true, false, hor_ + 8);
    spr(art_.peak[2], 168.f, float(hor_) - 2.f, 30.f, PAL_MOUNT, false, 7, true, false, hor_ + 4);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(23, "ONE RIDGE", PAL_GOLD);
        hudC(24, "KEEP THE FLARES LIT UNTIL DAWN", PAL_HUD);
        hudC(25, "THEN IT IS DONE", PAL_GOOD);
        hudC(26, "ARROWS WALK THE CREST", PAL_HUD);
        if ((sys_->frame / 30) % 2 == 0) hudC(27, "PRESS START", PAL_GOLD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_HUD);
        hudC(26, "ESC TITLE", PAL_HUD);
    } else if (mode_ == Mode::Won) {
        hudC(23, "THE FLARES HELD", PAL_GOLD);
        hudC(24, "UNTIL DAWN", PAL_HUD);
        hudC(25, "THEN IT IS DONE", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "FED %d  SHIELDED %d", fed_, shielded_);
        hudC(26, buf, PAL_HUD);
        hudC(27, "START", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudC(23, "A FLARE WENT OUT", PAL_ALERT);
        hudC(24, "THE RIDGE GOES DARK", PAL_HUD);
        hudC(27, "START RETRIES", PAL_HUD);
    } else {
        hud(1, 0, "RIDGE", PAL_GOLD);
        int left = std::max(0, int(std::ceil(kNight - t_ - 0.001f)));
        std::snprintf(buf, sizeof buf, "DAWN %d:%02d", left / 60, left % 60);
        hud(40 - int(std::strlen(buf)) - 1, 0, buf, left <= 8 ? PAL_ALERT : PAL_GOLD);
        int bars = 0;
        std::string row;
        int caretAt = -1;
        int closest = 0;
        float bd = 1e9f;
        for (int i = 0; i < kFlares; ++i) {
            float du = std::fabs(u_ - kU[i]);
            if (du < bd) {
                bd = du;
                closest = i;
            }
        }
        for (int i = 0; i < kFlares; ++i) {
            if (i) row += "  ";
            int n = fuel_[i] <= 0.f ? 0 : std::clamp(1 + int((fuel_[i] - 0.01f) / 25.f), 1, 4);
            bool here = std::fabs(u_ - kU[i]) <= kReach;
            bool low = fuel_[i] < 30.f;
            int pal = here ? PAL_GOLD : (low ? PAL_ALERT : PAL_GOOD);
            if (low && (sys_->frame / 8) % 2 == 0) pal = PAL_ALERT;
            std::string cell;
            for (int k = 0; k < 4; ++k) cell.push_back(k < n ? '#' : '-');
            int col = 9 + int(row.size());
            hud(col, 1, cell, pal);
            if (i == closest) caretAt = col + 1;
            row += cell;
            ++bars;
        }
        (void)bars;
        if (caretAt >= 0) hud(caretAt, 2, "^", PAL_GOLD);
        bool gustWarn = gustOn_ && (sys_->frame / 8) % 2 == 0;
        if (gustWarn) hudC(2, hint(), PAL_ALERT);
        else hudC(3, hint(), gustOn_ ? PAL_ALERT : PAL_HUD);
        hudC(27, "LEFT RIGHT WALK    Z OR C FEEDS", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.16f, 0.22f, 0.12f);
    if (bot_) beginWatch();
    else {
        bootTitle();
        sys.setLight(28, 36, 90);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) updateTitle(kDt);
    else if (mode_ == Mode::Pause) updatePause();
    else if (mode_ == Mode::Watch) updateWatch(kDt);
    else updateEnd(kDt);
    stepMotes(kDt);
    if (toneT_ > 0 && --toneT_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
    serviceAudio();
    lamp();
    draw();
}

}  // namespace rdawn
