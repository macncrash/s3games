#include "game/dawn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace gatedawn {
namespace {

constexpr float kX[Game::kFlares] = {64.f, 112.f, 160.f, 208.f, 256.f};
constexpr float kNight = 36.f;
constexpr float kSpeed = 172.f;
constexpr float kReach = 22.f;
constexpr float kFeedAt = 92.f;
constexpr float kDrain0 = 8.2f;
constexpr float kDrain1 = 13.6f;
constexpr float kWarn = 1.75f;
constexpr float kClimbT = 2.7f;
constexpr float kGustDmg = 24.f;
constexpr float kClimbDmg = 30.f;
constexpr float kGround = 198.f;
constexpr float kPostFoot = 190.f;
constexpr float kPostH = 48.f;
constexpr float kManH = 46.f;
constexpr int kSkyline = 84;

struct Ev {
    float t;
    int flare;
};
const Ev kGusts[] = {{3.5f, 4}, {9.5f, 0}, {15.5f, 2}, {21.5f, 4}, {27.5f, 1}, {32.2f, 3}};
const Ev kClimbs[] = {{6.4f, 0}, {12.4f, 4}, {18.4f, 1}, {24.4f, 3}, {30.0f, 2}};
const float kFan[] = {392.f, 494.f, 587.f, 784.f, 988.f};
const int kStars[][2] = {{10, 8},   {300, 10}, {312, 22}, {86, 58}, {118, 68}, {146, 54},
                         {188, 64}, {214, 56}, {236, 72}, {96, 74},  {170, 76}, {200, 48}};

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
    buildArt(sys.vdp, art_);
    bootTitle();
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    tended_ = 0;
    struck_ = 0;
    shielded_ = 0;
    dead_ = -1;
    focus_ = -1;
    hold_ = 0;
    toneT_ = 0;
    fanI_ = 0;
    fanT_ = 0;
    gustIx_ = 0;
    climbIx_ = 0;
    t_ = 0;
    px_ = 160.f;
    face_ = 1.f;
    move_ = 0;
    feedCd_ = 0;
    motes_.clear();
    for (int i = 0; i < kFlares; i++) {
        fuel_[i] = 100.f;
        idle_[i] = 0;
        pop_[i] = 0;
        hurt_[i] = 0;
        gustOn_[i] = false;
        gustEta_[i] = 0;
        climbOn_[i] = false;
        climbEta_[i] = 0;
    }
}

void Game::beginWatch() {
    bootTitle();
    mode_ = Mode::Watch;
    blip(330.f, 0.06f, 10);
    sys_->apu.tone(1, 495.f, 0.03f);
}

void Game::beginWin() {
    mode_ = Mode::Won;
    won_ = true;
    hold_ = 0;
    fanI_ = 0;
    fanT_ = 0;
    move_ = 0;
    sys_->setLight(255, 170, 60);
}

void Game::beginLoss(int flare) {
    mode_ = Mode::Lost;
    won_ = false;
    dead_ = flare;
    hold_ = 0;
    move_ = 0;
    fuel_[flare] = 0;
    sys_->rumble(0.7f, 0.4f, 180);
    sys_->setLight(90, 0, 0);
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
    draw();
}

void Game::updateTitle() {
    if (bot_) {
        if (sys_->frame >= 18) beginWatch();
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_MODE) && sys_->hasHome()) {
        sys_->eject();
        return;
    }
    bool go = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) ||
              p.pressed(gs::BTN_X) || p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_TURBO);
    if (go) beginWatch();
}

void Game::updatePause() {
    if (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_MODE)) mode_ = Mode::Watch;
}

void Game::updateEnd(bool dawn) {
    hold_++;
    if (dawn) {
        if (fanT_ > 0) fanT_--;
        if (fanT_ == 0 && fanI_ < 5) {
            float f = kFan[fanI_++];
            sys_->apu.tone(0, f, 0.07f);
            sys_->apu.tone(1, f * 0.5f, 0.035f);
            toneT_ = 14;
            fanT_ = 11;
            sys_->setLight(255, 180, 70);
        }
    } else if (hold_ == 1) {
        sys_->apu.tone(0, 110.f, 0.07f);
        sys_->apu.tone(1, 55.f, 0.04f);
        toneT_ = 36;
        sys_->apu.noiseBurst(0.22f, 700.f, 0.3f);
    }
    if (hold_ >= 80) over_ = true;
    if (!bot_ && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_C)))
        bootTitle();
}

void Game::readPad(float& dir, bool& feed, bool& strike) const {
    const gs::Pad& p = sys_->pad;
    dir = 0;
    if (p.down(gs::BTN_LEFT)) dir -= 1.f;
    if (p.down(gs::BTN_RIGHT)) dir += 1.f;
    if (dir == 0.f && std::fabs(p.axisX) > 0.28f) dir = p.axisX > 0 ? 1.f : -1.f;
    feed = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO);
    strike = p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.down(gs::BTN_Y);
}

float Game::drain() const {
    float u = std::min(1.f, t_ / kNight);
    return kDrain0 + (kDrain1 - kDrain0) * u;
}

float Game::flareScore(int i) const {
    float tDie = fuel_[i] / std::max(0.8f, drain());
    float s = 200.f / (tDie + 0.12f);
    s += std::min(idle_[i], 2.5f) * 22.f;
    if (fuel_[i] < 40.f) s += 180.f;
    if (fuel_[i] < 24.f) s += 260.f;
    float travel = std::fabs(px_ - kX[i]) / kSpeed;
    if (gustOn_[i]) {
        float slack = gustEta_[i] - travel;
        if (fuel_[i] - kGustDmg < 36.f) s += 520.f;
        else if (slack < 0.85f) s += 160.f;
        else s += 36.f;
    }
    if (climbOn_[i]) {
        float slack = climbEta_[i] - travel;
        if (fuel_[i] - kClimbDmg < 36.f) s += 560.f;
        else if (slack < 0.95f) s += 420.f;
        else s += 48.f;
    }
    return s;
}

int Game::choose() {
    int best = 0;
    float bs = -1.f;
    for (int i = 0; i < kFlares; i++) {
        float s = flareScore(i);
        if (s > bs) {
            bs = s;
            best = i;
        }
    }
    if (focus_ < 0 || focus_ >= kFlares) return best;
    float dist = std::fabs(px_ - kX[focus_]);
    // Stay planted once a hit is about to land under our feet.
    bool gustLock = gustOn_[focus_] && gustEta_[focus_] < 1.05f && dist < 36.f;
    bool climbLock = climbOn_[focus_] && climbEta_[focus_] < 1.2f && dist < 36.f;
    if (gustLock || climbLock) return focus_;
    if (dist > 16.f && flareScore(focus_) >= bs * 0.72f) return focus_;
    return best;
}

void Game::think(float& dir, bool& feed, bool& strike) {
    focus_ = choose();
    float dx = kX[focus_] - px_;
    dir = 0;
    feed = false;
    strike = false;
    if (dx > 15.f) dir = 1.f;
    else if (dx < -15.f) dir = -1.f;
    else {
        if (fuel_[focus_] < kFeedAt) feed = true;
        if (climbOn_[focus_]) strike = true;
    }
}

void Game::spawn() {
    while (gustIx_ < int(sizeof(kGusts) / sizeof(kGusts[0])) && t_ >= kGusts[gustIx_].t) {
        int i = kGusts[gustIx_].flare;
        gustOn_[i] = true;
        gustEta_[i] = kWarn;
        gustIx_++;
    }
    while (climbIx_ < int(sizeof(kClimbs) / sizeof(kClimbs[0])) && t_ >= kClimbs[climbIx_].t) {
        int i = kClimbs[climbIx_].flare;
        if (!climbOn_[i]) {
            climbOn_[i] = true;
            climbEta_[i] = kClimbT;
        }
        climbIx_++;
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

void Game::advanceThreats(float dt) {
    for (int i = 0; i < kFlares; i++) {
        if (gustOn_[i]) {
            gustEta_[i] -= dt;
            if (gustEta_[i] <= 0.f) {
                gustOn_[i] = false;
                bool shield = std::fabs(px_ - kX[i]) <= kReach + 6.f;
                if (shield) {
                    shielded_++;
                    blip(660.f, 0.05f, 6);
                    burst(kX[i], 130.f, 6, 28.f);
                } else {
                    fuel_[i] -= kGustDmg;
                    hurt_[i] = 0.28f;
                    blip(140.f, 0.06f, 8);
                    sys_->apu.noiseBurst(0.18f, 900.f, 0.2f);
                    sys_->rumble(0.35f, 0.15f, 70);
                    burst(kX[i], 136.f, 8, 22.f);
                }
            }
        }
        if (climbOn_[i]) {
            climbEta_[i] -= dt;
            if (climbEta_[i] <= 0.f) {
                climbOn_[i] = false;
                fuel_[i] -= kClimbDmg;
                hurt_[i] = 0.28f;
                blip(120.f, 0.06f, 8);
                sys_->apu.noiseBurst(0.2f, 600.f, 0.22f);
                burst(kX[i], 150.f, 7, 18.f);
            }
        }
    }
}

void Game::updateWatch(float dt) {
    if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        move_ = 0;
        return;
    }
    spawn();
    float dir = 0;
    bool feed = false, strike = false;
    if (bot_) think(dir, feed, strike);
    else readPad(dir, feed, strike);
    if (dir != 0.f) face_ = dir;
    move_ = dir;
    px_ = std::clamp(px_ + dir * kSpeed * dt, 40.f, 280.f);
    if (feedCd_ > 0.f) feedCd_ -= dt;

    int here = nearest();
    if (here >= 0 && feed && feedCd_ <= 0.f && fuel_[here] < kFeedAt) {
        fuel_[here] = 100.f;
        pop_[here] = 0.22f;
        feedCd_ = 0.18f;
        tended_++;
        blip(520.f + here * 70.f, 0.055f, 7);
        burst(kX[here], 128.f, 8, 34.f);
    }
    if (here >= 0 && strike && climbOn_[here]) {
        climbOn_[here] = false;
        struck_++;
        sys_->apu.noiseBurst(0.16f, 1800.f, 0.12f);
        blip(220.f, 0.04f, 4);
        burst(kX[here] + 6.f, 160.f, 6, 26.f);
        sys_->rumble(0.2f, 0.4f, 40);
    }

    advanceThreats(dt);
    float d = drain() * dt;
    for (int i = 0; i < kFlares; i++) {
        fuel_[i] -= d;
        if (pop_[i] > 0.f) pop_[i] -= dt;
        if (hurt_[i] > 0.f) hurt_[i] -= dt;
        if (fuel_[i] < 25.f && fuel_[i] + d >= 25.f) blip(196.f, 0.04f, 5);
        idle_[i] += dt;
    }
    if (dir == 0.f && here >= 0) idle_[here] = 0.f;

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
    if (motes_.size() > 48) motes_.erase(motes_.begin(), motes_.begin() + int(motes_.size() - 48));
}

void Game::burst(float x, float y, int n, float speed) {
    for (int i = 0; i < n; i++) {
        float a = (i + 0.5f) / float(n) * 6.28318f;
        motes_.push_back({x, y, std::cos(a) * speed, std::sin(a) * speed - 10.f, 0.38f});
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
    float u = (mode_ == Mode::Won) ? 1.f : (mode_ == Mode::Title || mode_ == Mode::Lost) ? 0.f : t_ / kNight;
    float e = (mode_ == Mode::Won) ? 1.f : smooth(0.48f, 1.f, u);
    const uint16_t n0 = gs::rgb4(1, 1, 5);
    const uint16_t n1 = gs::rgb4(2, 2, 8);
    const uint16_t n2 = gs::rgb4(5, 3, 7);
    const uint16_t n3 = gs::rgb4(3, 2, 4);
    const uint16_t d0 = gs::rgb4(6, 5, 11);
    const uint16_t d1 = gs::rgb4(12, 7, 8);
    const uint16_t d2 = gs::rgb4(15, 9, 4);
    const uint16_t d3 = gs::rgb4(8, 4, 3);
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t night, dawn;
        if (y < 48) {
            float t = y / 48.f;
            night = mix(n0, n1, t);
            dawn = mix(d0, d1, t);
        } else if (y < 100) {
            float t = (y - 48) / 52.f;
            night = mix(n1, n2, t);
            dawn = mix(d1, d2, t);
        } else {
            float t = (y - 100) / 124.f;
            night = mix(n2, n3, std::min(1.f, t));
            dawn = mix(d2, d3, std::min(1.f, t));
        }
        uint16_t c = mix(night, dawn, e);
        if (mode_ == Mode::Lost) c = mix(c, gs::rgb4(5, 1, 2), hold_ < 24 ? 0.45f : 0.22f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
    }
}

void Game::lamp() {
    if (mode_ == Mode::Won) sys_->setLight(255, 170, 60);
    else if (mode_ == Mode::Lost) sys_->setLight(80, 0, 0);
    else if (mode_ == Mode::Title) sys_->setLight(30, 40, 90);
    else {
        float f = 0;
        for (int i = 0; i < kFlares; i++) f += fuel_[i];
        f = std::clamp(f / 500.f, 0.f, 1.f);
        sys_->setLight(int(70 + 150 * f), int(28 + 40 * f), 24);
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

void Game::drawWorld() {
    const uint64_t fr = sys_->frame;
    float ease = 0.f;
    if (mode_ == Mode::Won) ease = 1.f;
    else if (mode_ == Mode::Watch || mode_ == Mode::Pause) ease = smooth(0.48f, 1.f, t_ / kNight);

    if (mode_ == Mode::Title) {
        word("S3 GATE DAWN", 160.f, 14.f, 15.f, PAL_GOLD);
        word("KEEP THE FLARES LIT", 160.f, 34.f, 12.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        word("DAWN", 160.f, 16.f, 18.f, PAL_GOLD);
    } else if (mode_ == Mode::Lost) {
        word("DARK", 160.f, 16.f, 18.f, PAL_ALERT);
    }

    if (mode_ != Mode::Title) {
        for (int i = 0; i < kFlares; i++) {
            int n = 0;
            if (fuel_[i] > 0.5f) n = std::clamp(1 + int((fuel_[i] - 0.01f) / 20.f), 1, 5);
            bool blink = fuel_[i] < 22.f && ((fr / 6) % 2 == 0);
            for (int k = 0; k < 5; k++) {
                int pal = PAL_PIP;
                if (k < n) pal = blink ? PAL_ALERT : (fuel_[i] < 30.f ? PAL_EMBER : PAL_GOLD);
                spr(art_.pip, kX[i] - 16.f + k * 8.f, 74.f, 5.f, pal);
            }
            if (mode_ == Mode::Watch && (gustOn_[i] || climbOn_[i] || fuel_[i] < 28.f)) {
                const gs::Mipped& g = art_.glyph[int('!') - 32];
                spr(g, kX[i], 62.f, 11.f, fuel_[i] < 28.f ? PAL_ALERT : PAL_GOLD);
            }
        }
    }

    int ff = int((fr / 7) % 2);
    for (auto& m : motes_) spr(art_.spark, m.x, m.y, 4.f, PAL_FIRE);

    float bob = move_ != 0.f ? std::sin(px_ * 0.35f) * 1.1f : std::sin(float(fr) * 0.08f) * 0.4f;
    float scale = kManH / 46.f;
    float tx = px_ + face_ * 13.f * scale;
    float ty = kGround - bob - 36.f * scale;
    spr(art_.flame[ff], tx, ty, 11.f, PAL_FIRE);

    for (int i = 0; i < kFlares; i++) {
        if (fuel_[i] <= 0.5f) continue;
        if (hurt_[i] > 0.18f) continue;
        bool low = fuel_[i] < 22.f && ((fr / 4) % 2 == 0);
        if (low && fuel_[i] < 14.f) continue;
        float fh = 14.f + fuel_[i] * 0.16f + std::max(0.f, pop_[i]) * 18.f;
        if (gustOn_[i]) fh += std::sin(float(fr) * 0.6f + i) * (1.f - gustEta_[i] / kWarn) * 3.f;
        int pal = fuel_[i] < 30.f ? PAL_EMBER : PAL_FIRE;
        float fx = kX[i] + (gustOn_[i] ? std::sin(float(fr) * 0.5f + i) * 2.f : 0.f);
        spr(art_.flame[ff], fx, 138.f, fh, pal, false, 0, true);
    }

    for (int i = 0; i < kFlares; i++) {
        if (!climbOn_[i]) continue;
        float p = 1.f - climbEta_[i] / kClimbT;
        float y = 208.f + (152.f - 208.f) * std::clamp(p, 0.f, 1.f);
        int frs = int(fr / 8) % 2;
        spr(art_.sneak[frs], kX[i] + 7.f, y, 30.f, PAL_SNEAK, false, 0, true);
    }

    int mf = (move_ != 0.f) ? (int(px_ / 6.f) & 1) : 0;
    spr(art_.man[mf], px_, kGround - bob, kManH, PAL_MAN, face_ < 0.f, 0, true);
    spr(art_.shade, px_, kGround + 1.f, 7.f, PAL_MAN, false, 0, false, true);

    for (int i = 0; i < kFlares; i++) {
        spr(art_.shade, kX[i], kPostFoot + 1.f, 6.f, PAL_MAN, false, 0, false, true);
        spr(art_.post, kX[i], kPostFoot, kPostH, PAL_IRON, false, 0, true);
        spr(art_.basket, kX[i], kPostFoot - kPostH, 14.f, PAL_IRON);
        if (fuel_[i] <= 0.5f || (mode_ == Mode::Lost && i == dead_)) {
            float sy = 128.f - float((fr / 5) % 6);
            spr(art_.smoke[(fr / 8) & 1], kX[i], sy, 14.f, PAL_SMOKE);
        }
    }

    for (int i = 0; i < kFlares; i++) {
        if (!gustOn_[i]) continue;
        float p = std::clamp(1.f - gustEta_[i] / kWarn, 0.f, 1.f);
        float from = kX[i] < 160.f ? kX[i] - 86.f : kX[i] + 86.f;
        float gx = from + (kX[i] - from) * p;
        spr(art_.gust, gx, 124.f, 16.f, PAL_WIND, from > kX[i]);
    }

    if (ease < 0.82f) {
        for (int s = 0; s < int(sizeof(kStars) / sizeof(kStars[0])); s++) {
            if (ease > 0.45f && ((s + int(fr / 8)) % 3 == 0)) continue;
            bool titleBand = kStars[s][1] < 46 && kStars[s][0] > 36 && kStars[s][0] < 284;
            if (mode_ == Mode::Title && titleBand) continue;
            float tw = (s % 3 == 0) ? 5.f : 4.f;
            if ((fr / 12 + s) % 7 == 0) tw = 3.f;
            spr(art_.star, float(kStars[s][0]), float(kStars[s][1]), tw, PAL_MOON);
        }
    }
    if (ease < 0.7f) spr(art_.moon, 28.f, 20.f, 20.f * (1.f - ease * 0.3f), PAL_MOON);

    if (ease > 0.5f) {
        float sunY = 100.f - (ease - 0.5f) * 2.f * 62.f;
        spr(art_.sun, 160.f, sunY, 30.f, PAL_SUN, false, 0, false, false, kSkyline);
    }
}

void Game::drawHud() {
    if (mode_ == Mode::Title) {
        hudC(26, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        bool blink = (sys_->frame / 30) % 2 == 0;
        if (blink) hudC(27, "ENTER TAKES THE GATE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
        hudC(14, "ENTER", PAL_HUD);
    }
    if (mode_ == Mode::Won) {
        hudC(11, "THE FLARES HELD", PAL_GOLD);
        hudC(13, "UNTIL DAWN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Lost) {
        hudC(11, lit() == 0 ? "THE FLARES WENT OUT" : "A FLARE WENT OUT", PAL_ALERT);
        hudC(13, "THE GATE IS LOST", PAL_HUD);
        return;
    }
    hud(1, 0, "GATE", PAL_HUD);
    int left = std::max(0, int(std::ceil(kNight - t_ - 0.001f)));
    char buf[16];
    std::snprintf(buf, sizeof buf, "DAWN %d:%02d", left / 60, left % 60);
    hud(31, 0, buf, left <= 10 ? PAL_ALERT : PAL_GOLD);
    bool dying = false;
    for (int i = 0; i < kFlares; i++)
        if (fuel_[i] < 25.f) dying = true;
    if (dying && (sys_->frame / 10) % 2 == 0) hudC(26, "A FLARE IS DYING", PAL_ALERT);
    hudC(27, "Z/C FEED    X STRIKE", PAL_HUD);
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

}  // namespace gatedawn
