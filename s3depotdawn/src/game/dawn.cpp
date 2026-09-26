#include "game/dawn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace depotdawn {
namespace {

constexpr float kX[Game::kFlares] = {44.f, 100.f, 188.f, 236.f, 292.f};
constexpr int kNightTick = 32 * 60;
constexpr int kWarnTick = 120;
constexpr int kDripTick = 96;
constexpr float kSpeed = 250.f;
constexpr float kReach = 22.f;
constexpr float kFeedAt = 88.f;
constexpr float kDrain0 = 4.2f;
constexpr float kDrain1 = 6.6f;
constexpr float kDrip = 10.f;
constexpr float kBlast = 20.f;
constexpr float kManFoot = 218.f;
constexpr float kPotFoot = 214.f;
constexpr float kLocoFoot = 180.f;
constexpr float kFan[] = {392.f, 494.f, 587.f, 784.f, 988.f};

struct Ev {
    int tick;
    int pot;
};
const Ev kDrips[] = {{210, 1}, {630, 4}, {990, 0}, {1350, 3}, {1710, 2}};
const int kShunts[] = {480, 870, 1260, 1650};
const int kStars[][2] = {{12, 10}, {28, 26}, {58, 14}, {300, 12}, {278, 30}, {250, 8},
                         {186, 6}, {140, 12}, {96, 8}, {210, 18}, {320, 20}, {70, 32}};

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
        if (fuel_[i] > 0.f) n++;
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
    poured_ = 0;
    hooded_ = 0;
    dead_ = -1;
    focus_ = 0;
    hold_ = 0;
    toneT_ = 0;
    fanI_ = 0;
    fanT_ = 0;
    tick_ = 0;
    shuntIx_ = 0;
    shuntImpact_ = 0;
    dripIx_ = 0;
    dripPot_ = -1;
    dripEnd_ = 0;
    face_ = 1;
    shuntOn_ = false;
    dripOn_ = false;
    seat_ = 0;
    px_ = 150.f;
    move_ = 0;
    feedCd_ = 0;
    shake_ = 0;
    ox_ = oy_ = 0;
    yardV_ = 0;
    motes_.clear();
    for (int i = 0; i < kFlares; i++) {
        fuel_[i] = 100.f;
        pop_[i] = 0;
        hurt_[i] = 0;
    }
}

void Game::beginWatch() {
    bootTitle();
    mode_ = Mode::Watch;
    blip(330.f, 0.05f, 8);
}

void Game::beginWin() {
    mode_ = Mode::Won;
    won_ = true;
    hold_ = 0;
    fanI_ = 0;
    fanT_ = 0;
    move_ = 0;
    shuntOn_ = false;
    dripOn_ = false;
    for (int i = 0; i < kFlares; i++) burst(kX[i], kPotFoot - 20.f, 6, 30.f, PAL_GOLD);
    sys_->setLight(255, 170, 60);
}

void Game::beginLoss(int pot) {
    mode_ = Mode::Lost;
    won_ = false;
    dead_ = pot;
    hold_ = 0;
    move_ = 0;
    fuel_[pot] = 0;
    shuntOn_ = false;
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
        case Mode::Won:
        case Mode::Lost: updateEnd(); break;
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 1.8f);
    stepMotes(dt);
    if (toneT_ > 0 && --toneT_ == 0) {
        sys.apu.tone(0, 0, 0);
        if (mode_ != Mode::Watch || !shuntOn_) sys.apu.tone(1, 0, 0);
    }
    serviceAudio();
    draw();
}

void Game::updateTitle() {
    float w = std::sin(sys_->frame * 0.03f);
    px_ = 150.f + w * 70.f;
    face_ = std::cos(sys_->frame * 0.03f) >= 0.f ? 1 : -1;
    move_ = std::fabs(std::cos(sys_->frame * 0.03f)) > 0.25f ? float(face_) : 0.f;
    yardV_ += 0.35f;
    embers();
    if (bot_) {
        if (sys_->frame >= 20) beginWatch();
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

void Game::updateEnd() {
    hold_++;
    yardV_ += 0.2f;
    embers();
    if (mode_ == Mode::Won) {
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
    if (bot_) return;
    if (hold_ > 24 && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A))) beginWatch();
    else if (sys_->pad.pressed(gs::BTN_MODE)) bootTitle();
}

void Game::readPad(float& dir, bool& feed) const {
    const gs::Pad& p = sys_->pad;
    dir = 0;
    if (p.down(gs::BTN_LEFT)) dir -= 1.f;
    if (p.down(gs::BTN_RIGHT)) dir += 1.f;
    if (dir == 0.f && std::fabs(p.axisX) > 0.28f) dir = p.axisX > 0 ? 1.f : -1.f;
    feed = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X) || p.down(gs::BTN_Y) ||
           p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO);
}

float Game::drain() const {
    float u = std::min(1.f, tick_ / float(kNightTick));
    return kDrain0 + (kDrain1 - kDrain0) * u;
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

int Game::choose() {
    auto travelTo = [&](int i) { return std::fabs(kX[i] - px_) / kSpeed; };
    if (shuntOn_) {
        int eta = shuntImpact_ - tick_;
        if (eta < 0) eta = 0;
        int best = -1;
        float worst = 1e9f;
        for (int i = 0; i < kFlares; i++) {
            if (travelTo(i) * 60.f > float(eta) + 1.f) continue;
            float after = fuel_[i] - drain() * (eta / 60.f) - kBlast;
            if (dripOn_ && dripPot_ == i) after -= kDrip * std::min(1.6f, eta / 60.f);
            if (after < worst) {
                worst = after;
                best = i;
            }
        }
        if (worst < 40.f && best >= 0) return best;
        if (best < 0 && eta < 40) {
            int here = nearest();
            if (here >= 0) return here;
        }
    }
    int low = 0;
    for (int i = 1; i < kFlares; i++)
        if (fuel_[i] < fuel_[low]) low = i;
    if (fuel_[low] < 36.f) return low;
    if (dripOn_ && fuel_[dripPot_] < 84.f) {
        bool other = false;
        for (int i = 0; i < kFlares; i++)
            if (i != dripPot_ && fuel_[i] + 6.f < fuel_[dripPot_]) other = true;
        if (!other) return dripPot_;
    }
    int best = 0;
    float bs = 1e9f;
    for (int i = 0; i < kFlares; i++) {
        float rate = drain();
        if (dripOn_ && dripPot_ == i) rate += kDrip;
        float f = fuel_[i] - rate * travelTo(i);
        if (f < bs) {
            bs = f;
            best = i;
        }
    }
    if (focus_ >= 0 && focus_ < kFlares && std::fabs(kX[focus_] - px_) > 12.f) {
        float rate = drain();
        if (dripOn_ && dripPot_ == focus_) rate += kDrip;
        float f = fuel_[focus_] - rate * travelTo(focus_);
        if (f < bs + 8.f) return focus_;
    }
    return best;
}

void Game::think(float& dir, bool& feed) {
    focus_ = choose();
    float dx = kX[focus_] - px_;
    if (dx > 8.f) dir = 1.f;
    else if (dx < -8.f) dir = -1.f;
    else dir = 0.f;
    int here = nearest();
    feed = here >= 0 && fuel_[here] < kFeedAt && (here == focus_ || fuel_[here] < 62.f);
}

void Game::openDrip() {
    if (dripOn_) return;
    int n = int(sizeof(kDrips) / sizeof(kDrips[0]));
    if (dripIx_ >= n) return;
    if (tick_ < kDrips[dripIx_].tick) return;
    dripOn_ = true;
    dripPot_ = kDrips[dripIx_].pot;
    dripEnd_ = tick_ + kDripTick;
    seat_ = 0;
    dripIx_++;
    blip(880.f, 0.03f, 4);
}

void Game::openShunt() {
    if (shuntOn_) return;
    int n = int(sizeof(kShunts) / sizeof(kShunts[0]));
    if (shuntIx_ >= n) return;
    if (tick_ < kShunts[shuntIx_] - kWarnTick) return;
    shuntOn_ = true;
    shuntImpact_ = kShunts[shuntIx_];
    shuntIx_++;
    sys_->apu.noiseBurst(0.1f, 500.f, 0.25f);
    blip(180.f, 0.04f, 10);
}

void Game::dripStep(float dt) {
    if (!dripOn_) return;
    bool here = std::fabs(px_ - kX[dripPot_]) <= kReach;
    if (here) {
        seat_ += dt;
        if (seat_ >= 0.2f) {
            dripOn_ = false;
            hooded_++;
            blip(640.f, 0.045f, 6);
            burst(kX[dripPot_], kPotFoot - 24.f, 5, 22.f, PAL_RAIN);
            sys_->rumble(0.12f, 0.2f, 40);
            return;
        }
    } else {
        seat_ = 0;
        fuel_[dripPot_] -= kDrip * dt;
    }
    if (tick_ >= dripEnd_) dripOn_ = false;
}

void Game::resolveShunt() {
    if (!shuntOn_ || tick_ < shuntImpact_) return;
    shuntOn_ = false;
    int here = nearest();
    bool hit = false;
    for (int i = 0; i < kFlares; i++) {
        if (i == here) {
            hooded_++;
            burst(kX[i], kPotFoot - 22.f, 5, 24.f, PAL_GOLD);
        } else {
            fuel_[i] -= kBlast;
            hurt_[i] = 0.35f;
            hit = true;
            burst(kX[i], kPotFoot - 16.f, 6, 20.f, PAL_SMOKE);
        }
    }
    shake_ = hit ? 1.f : 0.35f;
    if (hit) {
        blip(120.f, 0.06f, 8);
        sys_->apu.noiseBurst(0.2f, 800.f, 0.2f);
        sys_->rumble(0.45f, 0.2f, 90);
    } else {
        blip(700.f, 0.05f, 6);
    }
}

void Game::tryFeed() {
    if (feedCd_ > 0.f) return;
    int i = nearest();
    if (i < 0 || fuel_[i] >= kFeedAt) return;
    fuel_[i] = 100.f;
    pop_[i] = 0.22f;
    feedCd_ = 0.14f;
    poured_++;
    blip(500.f + i * 40.f, 0.05f, 6);
    burst(kX[i], kPotFoot - 20.f, 7, 28.f, PAL_FIRE);
    sys_->rumble(0.08f, 0.04f, 24);
}

void Game::updateWatch(float dt) {
    if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        move_ = 0;
        return;
    }
    if (!bot_ && sys_->pad.pressed(gs::BTN_MODE)) {
        bootTitle();
        return;
    }
    openDrip();
    openShunt();
    float dir = 0;
    bool feed = false;
    if (bot_) think(dir, feed);
    else readPad(dir, feed);
    if (dir > 0.2f) face_ = 1;
    else if (dir < -0.2f) face_ = -1;
    move_ = dir;
    px_ = std::clamp(px_ + dir * kSpeed * dt, 28.f, 308.f);
    if (feedCd_ > 0.f) feedCd_ -= dt;
    if (feed) tryFeed();
    dripStep(dt);
    resolveShunt();
    float loss = drain() * dt;
    for (int i = 0; i < kFlares; i++) {
        float before = fuel_[i];
        fuel_[i] -= loss;
        if (pop_[i] > 0.f) pop_[i] -= dt;
        if (hurt_[i] > 0.f) hurt_[i] -= dt;
        if (before >= 32.f && fuel_[i] < 32.f) blip(196.f, 0.04f, 5);
        if (fuel_[i] <= 0.f) {
            beginLoss(i);
            return;
        }
    }
    yardV_ += 22.f * dt;
    embers();
    tick_++;
    if (tick_ >= kNightTick) beginWin();
}

void Game::embers() {
    if ((sys_->frame % 5) != 0) return;
    int i = int(sys_->frame / 5) % kFlares;
    if (fuel_[i] <= 0.5f) return;
    float j = std::sin(sys_->frame * 0.7f + i * 1.7f);
    motes_.push_back({kX[i] + j * 2.f, kPotFoot - 18.f, j * 6.f, -18.f - std::fabs(j) * 8.f, 0.42f, PAL_FIRE});
    if (motes_.size() > 48) motes_.erase(motes_.begin(), motes_.begin() + int(motes_.size() - 48));
}

void Game::stepMotes(float dt) {
    for (auto& m : motes_) {
        m.x += m.vx * dt;
        m.y += m.vy * dt;
        m.vy += 18.f * dt;
        m.life -= dt;
    }
    motes_.erase(std::remove_if(motes_.begin(), motes_.end(), [](const Mote& m) { return m.life <= 0.f; }),
                 motes_.end());
}

void Game::burst(float x, float y, int n, float speed, int pal) {
    for (int i = 0; i < n; i++) {
        float a = (i + 0.5f) / float(n) * 6.28318f;
        motes_.push_back({x, y, std::cos(a) * speed, std::sin(a) * speed - 8.f, 0.36f, pal});
    }
}

void Game::blip(float freq, float vol, int frames) {
    sys_->apu.tone(0, freq, vol);
    toneT_ = frames;
}

void Game::serviceAudio() {
    if (mode_ == Mode::Watch && shuntOn_) sys_->apu.tone(1, 96.f, 0.028f);
}

float Game::dawnEase() const {
    if (mode_ == Mode::Won) return 1.f;
    if (mode_ == Mode::Title || mode_ == Mode::Lost) return 0.f;
    return smooth(0.52f, 1.f, tick_ / float(kNightTick));
}

const char* Game::hint() const {
    if (shuntOn_) {
        int worst = 0;
        for (int i = 1; i < kFlares; i++)
            if (fuel_[i] < fuel_[worst]) worst = i;
        if (std::fabs(px_ - kX[worst]) <= kReach) return "STAND AND HOOD IT";
        if (kX[worst] < px_) return "SHUNT  GO LEFT";
        return "SHUNT  GO RIGHT";
    }
    if (dripOn_) {
        if (std::fabs(px_ - kX[dripPot_]) <= kReach) return "HOLD THE HOOD";
        if (kX[dripPot_] < px_) return "RAIN  GO LEFT";
        return "RAIN  GO RIGHT";
    }
    int low = 0;
    for (int i = 1; i < kFlares; i++)
        if (fuel_[i] < fuel_[low]) low = i;
    if (fuel_[low] < 72.f) {
        if (std::fabs(px_ - kX[low]) <= kReach) return "POUR THE POT";
        if (kX[low] < px_) return "LOW POT LEFT";
        return "LOW POT RIGHT";
    }
    return "KEEP THEM LIT";
}

void Game::lamp() {
    if (mode_ == Mode::Won) sys_->setLight(255, 170, 60);
    else if (mode_ == Mode::Lost) sys_->setLight(80, 0, 0);
    else if (mode_ == Mode::Title) sys_->setLight(30, 36, 90);
    else {
        float f = 0;
        for (int i = 0; i < kFlares; i++) f += fuel_[i];
        f = std::clamp(f / 500.f, 0.f, 1.f);
        int r = int(60 + 160 * f);
        int g = int(24 + 36 * f);
        if (shuntOn_) r = std::min(255, r + 40);
        sys_->setLight(r, g, 28);
    }
}

void Game::sky() {
    float e = dawnEase();
    const uint16_t n0 = gs::rgb4(1, 1, 4);
    const uint16_t n1 = gs::rgb4(2, 2, 7);
    const uint16_t n2 = gs::rgb4(4, 2, 5);
    const uint16_t n3 = gs::rgb4(5, 3, 3);
    const uint16_t d0 = gs::rgb4(5, 6, 12);
    const uint16_t d1 = gs::rgb4(11, 7, 8);
    const uint16_t d2 = gs::rgb4(15, 9, 4);
    const uint16_t d3 = gs::rgb4(10, 5, 3);
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t night, dawn;
        if (y < 40) {
            float t = y / 40.f;
            night = mix(n0, n1, t);
            dawn = mix(d0, d1, t);
        } else if (y < 110) {
            float t = (y - 40) / 70.f;
            night = mix(n1, n2, t);
            dawn = mix(d1, d2, t);
        } else {
            float t = std::min(1.f, (y - 110) / 114.f);
            night = mix(n2, n3, t);
            dawn = mix(d2, d3, t);
        }
        uint16_t c = mix(night, dawn, e);
        if (mode_ == Mode::Lost) c = mix(c, gs::rgb4(5, 1, 1), hold_ < 24 ? 0.5f : 0.22f);
        v.lineBackdrop[y] = c;
        int fog = 0;
        if (y >= 80 && y <= 136) {
            float depth = 1.f - float(y - 80) / 56.f;
            fog = int(std::lround(depth * 4.f * (1.f - e)));
        }
        v.lineFog[y] = uint8_t(std::clamp(fog, 0, 16));
    }
    v.setFogColor(mix(gs::rgb4(1, 1, 3), gs::rgb4(8, 5, 3), e * 0.45f));
}

void Game::yard() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) v.road[y].on = false;
    for (int y = 86; y <= 175; y++) {
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f;
        if (y <= 136) r.hw = 16.f;
        else r.hw = 16.f + (y - 136) * (104.f / 39.f);
        r.v = yardV_ + (y - 86) * 18.f;
        r.pal = PAL_YARD;
        r.band = (int(r.v / 36.f) & 1) ? 1 : 0;
        r.style = 0;
        r.left = r.right = (y <= 136) ? gs::GROUND_DROP : gs::GROUND_LAND;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow,
               int clip) {
    if (h < 1.f || m.h < 1 || m.w < 1) return;
    cx += ox_;
    cy += oy_;
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

void Game::word(const std::string& s, float cx, float y, float h, int pal) {
    float width = 0;
    for (char ch : s) {
        if (ch == ' ') {
            width += h * 0.42f;
            continue;
        }
        char c = ch;
        if (c >= 'a' && c <= 'z') c = char(c - 32);
        if (c < 32 || c >= 127) continue;
        const gs::Mipped& g = art_.glyph[int(c) - 32];
        if (g.h < 1) continue;
        width += h * float(g.w) / float(g.h) + 1.f;
    }
    float pen = cx - width * 0.5f;
    for (char ch : s) {
        if (ch == ' ') {
            pen += h * 0.42f;
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

void Game::drawWorld() {
    const uint64_t fr = sys_->frame;
    float ease = dawnEase();
    if (mode_ == Mode::Title) {
        word("S3 DEPOT DAWN", 160.f, 12.f, 15.f, PAL_GOLD);
        word("KEEP THE FLARES LIT", 160.f, 30.f, 11.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        word("DAWN", 160.f, 14.f, 18.f, PAL_GOLD);
    } else if (mode_ == Mode::Lost) {
        word("DARK", 160.f, 14.f, 18.f, PAL_ALERT);
    }

    int ff = int((fr / 7) % 2);
    for (int i = 0; i < kFlares; i++) {
        int n = 0;
        if (fuel_[i] > 0.5f) n = std::clamp(1 + int(fuel_[i] / 20.f), 1, 5);
        bool blink = fuel_[i] < 28.f && ((fr / 6) % 2 == 0);
        for (int k = 0; k < 5; k++) {
            int pal = PAL_PIP;
            if (k < n) pal = blink ? PAL_ALERT : (fuel_[i] < 40.f ? PAL_EMBER : PAL_GOLD);
            spr(art_.spark, kX[i] - 10.f + k * 5.f, 150.f, 4.f, pal);
        }
        bool warn = fuel_[i] < 32.f || (dripOn_ && dripPot_ == i) || (shuntOn_ && fuel_[i] < 55.f);
        if (mode_ == Mode::Watch && warn) {
            const gs::Mipped& g = art_.glyph[int('!') - 32];
            spr(g, kX[i], 138.f, 10.f, fuel_[i] < 32.f ? PAL_ALERT : PAL_GOLD);
        }
    }

    for (const auto& m : motes_) {
        int pal = m.pal;
        if (pal == PAL_FIRE) spr(art_.spark, m.x, m.y, 4.f, PAL_FIRE);
        else if (pal == PAL_GOLD) spr(art_.spark, m.x, m.y, 4.f, PAL_GOLD);
        else spr(art_.smoke[(fr / 6) & 1], m.x, m.y, 8.f, pal == PAL_RAIN ? PAL_RAIN : PAL_SMOKE);
    }

    float bob = move_ != 0.f ? std::sin(px_ * 0.4f) * 1.1f : std::sin(float(fr) * 0.08f) * 0.35f;
    int mf = move_ != 0.f ? (int(std::fabs(px_) / 5.f) & 1) : 0;
    spr(art_.man[mf], px_, kManFoot - bob, 48.f, PAL_MAN, face_ < 0, 0, true);

    for (int i = 0; i < kFlares; i++) {
        if (fuel_[i] <= 0.5f) continue;
        if (hurt_[i] > 0.18f && (fr / 3) % 2 == 0) continue;
        bool hooding = std::fabs(px_ - kX[i]) <= kReach && (shuntOn_ || (dripOn_ && dripPot_ == i));
        if (hooding) {
            spr(art_.hood, kX[i], kPotFoot - 16.f, 10.f, PAL_IRON);
            continue;
        }
        float fh = 14.f + fuel_[i] * 0.12f + std::max(0.f, pop_[i]) * 16.f;
        int pal = fuel_[i] < 36.f ? PAL_EMBER : PAL_FIRE;
        float fx = kX[i];
        if (shuntOn_) fx += std::sin(float(fr) * 0.5f + i) * 1.5f;
        spr(art_.flame[ff], fx, kPotFoot - 14.f, fh, pal, false, 0, true);
    }

    if (dripOn_) {
        for (int s = 0; s < 5; s++) {
            float rx = kX[dripPot_] - 16.f + s * 8.f;
            float ry = 96.f + float((fr * 3 + s * 17) % 70);
            spr(art_.rain, rx, ry, 12.f, PAL_RAIN);
        }
    }

    spr(art_.shade, px_, kManFoot + 1.f, 7.f, PAL_MAN, false, 0, false, true);
    for (int i = 0; i < kFlares; i++) {
        spr(art_.shade, kX[i], kPotFoot + 1.f, 6.f, PAL_IRON, false, 0, false, true);
        spr(art_.pot, kX[i], kPotFoot, 16.f, PAL_IRON, false, 0, true);
        if (fuel_[i] <= 0.5f || (mode_ == Mode::Lost && i == dead_)) {
            float sy = kPotFoot - 28.f - float((fr / 5) % 6);
            spr(art_.smoke[(fr / 8) & 1], kX[i], sy, 14.f, PAL_SMOKE);
        }
    }

    float locoX = 400.f;
    if (shuntOn_) {
        float u = 1.f - float(shuntImpact_ - tick_) / float(kWarnTick);
        u = std::clamp(u, 0.f, 1.f);
        locoX = -80.f + u * 460.f;
    }
    if (shuntOn_ && locoX > -90.f && locoX < 360.f) {
        spr(art_.loco[(fr / 5) & 1], locoX, kLocoFoot, 40.f, PAL_LOCO, false, 0, true);
        spr(art_.smoke[(fr / 6) & 1], locoX - 28.f, kLocoFoot - 36.f, 12.f, PAL_SMOKE);
    }

    spr(art_.lamp, 104.f, 78.f, 16.f, PAL_SHED);
    spr(art_.lamp, 220.f, 78.f, 16.f, PAL_SHED);
    spr(art_.buffer, 158.f, 176.f, 24.f, PAL_IRON, false, 0, true);
    spr(art_.wagon, 8.f, kLocoFoot, 30.f, PAL_LOCO, false, 0, true);

    if (ease < 0.8f) {
        for (int s = 0; s < int(sizeof(kStars) / sizeof(kStars[0])); s++) {
            int sx = kStars[s][0], sy = kStars[s][1];
            if (sy > 36) continue;
            bool titleBand = mode_ == Mode::Title && sx > 70 && sx < 250 && sy < 40;
            if (titleBand) continue;
            if (ease > 0.4f && ((s + int(fr / 8)) % 3 == 0)) continue;
            float tw = (s % 3 == 0) ? 5.f : 4.f;
            spr(art_.star, float(sx), float(sy), tw, PAL_MOON);
        }
    }
    if (ease < 0.72f) spr(art_.moon, 292.f, 18.f + ease * 16.f, 18.f, PAL_MOON);
    if (ease > 0.45f) {
        float sunY = 70.f - (ease - 0.45f) * 90.f;
        spr(art_.sun, 248.f, sunY, 28.f, PAL_SUN, false, 0, false, false, 48);
    }
}

void Game::drawHud() {
    if (mode_ == Mode::Title) {
        hudC(25, "MISS THAT AND THE WATCH IS OVER", PAL_ALERT);
        if ((sys_->frame / 30) % 2 == 0) hudC(27, "ENTER TAKES THE WATCH", PAL_HUD);
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
        hudC(13, "THE WATCH IS OVER", PAL_HUD);
        return;
    }
    hud(1, 0, "DEPOT", PAL_HUD);
    int left = std::max(0, (kNightTick - tick_ + 59) / 60);
    char buf[20];
    std::snprintf(buf, sizeof buf, "DAWN %d:%02d", left / 60, left % 60);
    hud(30, 0, buf, left <= 8 ? PAL_ALERT : PAL_GOLD);
    hudC(26, hint(), PAL_ALERT);
    hudC(27, "Z POURS    STAND HOODS", PAL_HUD);
}

void Game::draw() {
    if (shake_ > 0.f) {
        ox_ = std::sin(sys_->frame * 1.3f) * shake_ * 2.4f;
        oy_ = std::cos(sys_->frame * 1.7f) * shake_ * 1.4f;
    } else {
        ox_ = oy_ = 0;
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    lamp();
    yard();
    drawWorld();
    drawHud();
}

}  // namespace depotdawn
