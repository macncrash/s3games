#include "game/rearguard.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "version.h"

namespace rearguard {
namespace {

constexpr float kDt = 1.0f / 60.0f;
constexpr float kHorizon = 54.0f;
constexpr float kSpan = 170.0f;
constexpr float kZNear = 1.12f;
constexpr float kPpm = 158.0f;
constexpr float kRoadHalf = 1.18f;
constexpr float kPlayerZ = 1.42f;
constexpr float kLeakZ = 1.18f;
constexpr float kPerson = 96.0f;
constexpr float kSpawnZ = 11.4f;
constexpr float kGoal = 36.0f;
constexpr float kScroll = 1.15f;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.25f;
    p.op[0] = {1, 1, 0.06f, 0.45f, 0.8f, 0.35f};
    p.op[1] = {2, 0.35f, 0.08f, 0.5f, 0.55f, 0.3f};
    p.op[2] = {0.5f, 0.28f, 0.1f, 0.6f, 0.7f, 0.4f};
    p.op[3] = {1, 0.18f, 0.06f, 0.4f, 0.5f, 0.35f};
    p.vol = 0.1f;
    p.tone = 480;
    p.drive = 0.12f;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.38f;
    p.op[0] = {1, 1, 0.01f, 0.18f, 0.7f, 0.14f};
    p.op[1] = {2, 0.5f, 0.01f, 0.22f, 0.5f, 0.14f};
    p.op[2] = {3, 0.28f, 0.02f, 0.28f, 0.4f, 0.18f};
    p.op[3] = {1, 0.32f, 0.01f, 0.2f, 0.55f, 0.16f};
    p.vol = 0.2f;
    p.drive = 0.08f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Fight && leg_ >= 2) return 2;
    if (mode_ == Mode::Fight || mode_ == Mode::Pause) return 1;
    return 0;
}

int Game::legNow() const {
    if (march_ < kGoal * 0.34f) return 0;
    if (march_ < kGoal * 0.67f) return 1;
    return 2;
}

const char* Game::legName() const {
    static const char* n[3] = {"THE LANE", "THE WOODS", "THE LAST RISE"};
    return n[std::clamp(leg_, 0, 2)];
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.0f / 16777216.0f);
}

float Game::bend(float z) const {
    float s = clock_ * 2.2f + z * 0.4f;
    return std::sin(s * 0.55f) * 0.26f + std::sin(s * 0.17f + 0.7f) * 0.14f;
}

Game::Proj Game::project(float x, float z) const {
    float zc = std::max(z, kZNear);
    float t = std::min(kZNear / zc, 1.05f);
    Proj p;
    p.t = t;
    p.y = kHorizon + t * kSpan;
    p.x = 160.0f + shx_ + (bend(zc) - bend(kZNear) + x) * (kPpm * t);
    p.fog = std::clamp(int((zc - 5.2f) * 1.2f), 0, 13);
    return p;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 18.0f * scale;
    float w = float(s.size()) * adv;
    x -= w * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::blip(int ch, float freq, float vol, float dur) {
    if (ch < 0 || ch > 2) return;
    sys_->apu.tone(ch, freq, vol);
    psgT_[ch] = dur;
}

void Game::drum() {
    bool left = (stepPar_ & 1) != 0;
    blip(0, left ? 128.0f : 96.0f, mode_ == Mode::Title ? 0.035f : 0.055f, 0.04f);
    stepPar_++;
}

void Game::fanfare() {
    fanStep_ = 0;
    fanT_ = 0;
}

void Game::beginMarch() {
    foes_.clear();
    shots_.clear();
    pops_.clear();
    cues_.clear();
    march_ = 0;
    column_ = 8;
    wounds_ = 3;
    score_ = 0;
    leg_ = 0;
    cue_ = 0;
    px_ = 0;
    pz_ = kPlayerZ;
    vx_ = 0;
    fireCd_ = 0;
    fireFlash_ = 0;
    invuln_ = 0.4f;
    shake_ = 0;
    banner_ = 2.2f;
    bannerText_ = "HOLD THE REAR";
    leakFlash_ = 0;
    won_ = false;
    over_ = false;
    victoryT_ = 0;
    failT_ = 0;
    mode_ = Mode::Fight;
    const float lane[8] = {-0.72f, 0.58f, -0.18f, 0.78f, -0.55f, 0.22f, 0.48f, -0.82f};
    for (int i = 0; i < 8; i++) cues_.push_back({1.1f + i * 1.42f, 0, lane[i]});
    const float wood[8] = {0.7f, -0.68f, 0.22f, -0.36f, 0.8f, -0.12f, 0.42f, -0.78f};
    for (int i = 0; i < 8; i++) cues_.push_back({13.0f + i * 1.25f, i % 2 ? 1 : 0, wood[i]});
    cues_.push_back({24.2f, 2, -0.6f});
    cues_.push_back({25.6f, 1, 0.66f});
    cues_.push_back({27.0f, 2, 0.18f});
    cues_.push_back({28.3f, 0, -0.74f});
    cues_.push_back({28.3f, 1, 0.7f});
    cues_.push_back({30.0f, 2, -0.22f});
    cues_.push_back({31.4f, 1, 0.46f});
    cues_.push_back({31.4f, 0, -0.52f});
    cues_.push_back({33.2f, 3, 0.02f});
    cues_.push_back({34.5f, 1, -0.7f});
    cues_.push_back({34.5f, 1, 0.7f});
}

void Game::spawnCue(const Cue& c) {
    Foe e;
    e.kind = c.kind;
    e.lane = c.lane;
    e.x = c.lane;
    e.z = kSpawnZ + ((c.kind == 1) ? 0.45f : 0.0f);
    e.age = rnd() * 2.0f;
    if (c.kind == 1) {
        e.hp = 1;
        e.speed = 2.62f;
        e.score = 150;
        e.hit = 0.22f;
        e.tall = 0.96f;
    } else if (c.kind == 2) {
        e.hp = 2;
        e.speed = 2.28f;
        e.score = 280;
        e.hit = 0.32f;
        e.tall = 1.32f;
    } else if (c.kind == 3) {
        e.hp = 3;
        e.speed = 1.92f;
        e.score = 500;
        e.hit = 0.24f;
        e.tall = 1.1f;
    } else {
        e.hp = 1;
        e.speed = 2.12f;
        e.score = 100;
        e.hit = 0.24f;
        e.tall = 1.0f;
    }
    foes_.push_back(e);
}

void Game::note(int pts, float wx, float wz) {
    score_ += pts;
    Proj p = project(wx, wz);
    pops_.push_back({p.x, p.y, 0.7f, pts});
    if (pops_.size() > 5) pops_.erase(pops_.begin());
}

void Game::hurt() {
    if (mode_ != Mode::Fight || invuln_ > 0) return;
    wounds_--;
    invuln_ = 1.15f;
    shake_ = 0.4f;
    sys_->rumble(0.7f, 0.9f, 180);
    sys_->apu.noiseBurst(0.45f, 900.0f, 0.16f);
    sys_->setLight(180, 40, 30);
    if (wounds_ <= 0) breakColumn();
}

void Game::leak() {
    if (mode_ != Mode::Fight) return;
    column_--;
    leakFlash_ = 0.45f;
    shake_ = 0.28f;
    score_ = std::max(0, score_ - 50);
    sys_->rumble(0.4f, 0.5f, 120);
    sys_->apu.noiseBurst(0.35f, 280.0f, 0.2f);
    blip(2, 220.0f, 0.07f, 0.12f);
    if (column_ <= 0) breakColumn();
}

void Game::breakColumn() {
    if (mode_ != Mode::Fight) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = false;
    failT_ = 0;
    sys_->apu.noiseBurst(0.6f, 160.0f, 0.45f);
    sys_->setLight(160, 16, 16);
}

void Game::reachGate() {
    if (mode_ != Mode::Fight) return;
    mode_ = Mode::Victory;
    won_ = true;
    score_ += column_ * 250 + wounds_ * 100;
    victoryT_ = 0;
    foes_.clear();
    shots_.clear();
    fanfare();
    sys_->setLight(40, 170, 70);
}

void Game::stick(float& sx, float& sy, bool& fire) {
    sx = sy = 0;
    fire = false;
    if (bot_) {
        const Foe* best = nullptr;
        for (const Foe& e : foes_) {
            if (!e.alive) continue;
            if (!best) {
                best = &e;
                continue;
            }
            float dz = e.z - best->z;
            if (dz < -0.45f || (std::fabs(dz) <= 0.45f && std::fabs(e.x - px_) < std::fabs(best->x - px_))) best = &e;
        }
        if (best) {
            sx = std::clamp((best->x - px_) * 7.0f, -1.0f, 1.0f);
            fire = std::fabs(best->x - px_) < 0.16f && std::fabs(vx_) < 1.45f;
        } else {
            sx = std::clamp(-px_ * 4.0f, -1.0f, 1.0f);
        }
        sy = std::clamp((kPlayerZ - pz_) * 5.0f, -1.0f, 1.0f);
        return;
    }
    const gs::Pad& pad = sys_->pad;
    sx = std::fabs(pad.axisX) > 0.08f ? pad.axisX : float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
    sy = float(pad.down(gs::BTN_UP)) - float(pad.down(gs::BTN_DOWN));
    fire = pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.accel > 0.45f;
}

void Game::updateTitle(float dt) {
    px_ = std::sin(clock_ * 0.55f) * 0.32f;
    pz_ = kPlayerZ;
    stepAcc_ += dt;
    if (stepAcc_ >= 0.52f) {
        stepAcc_ -= 0.52f;
        drum();
    }
}

void Game::updateFight(float dt) {
    float sx, sy;
    bool fire = false;
    stick(sx, sy, fire);
    vx_ += sx * 20.0f * dt;
    vx_ -= vx_ * 11.0f * dt;
    vx_ = std::clamp(vx_, -3.5f, 3.5f);
    px_ = std::clamp(px_ + vx_ * dt, -0.9f, 0.9f);
    pz_ = std::clamp(pz_ + sy * 0.85f * dt, 1.30f, 2.35f);
    if (invuln_ > 0) invuln_ -= dt;
    if (fireFlash_ > 0) fireFlash_ -= dt;
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - dt);
    if (leakFlash_ > 0) leakFlash_ -= dt;
    if (banner_ > 0) banner_ -= dt;

    float rate = 1.0f;
    for (const Foe& e : foes_)
        if (e.alive && e.z < pz_ + 2.15f) rate = 0.35f;
    march_ += rate * dt;
    stepAcc_ += rate * dt;
    if (stepAcc_ >= 0.46f) {
        stepAcc_ -= 0.46f;
        drum();
    }

    int leg = legNow();
    if (leg != leg_) {
        leg_ = leg;
        banner_ = 1.7f;
        bannerText_ = legName();
        blip(2, leg == 2 ? 440.0f : 330.0f, 0.05f, 0.1f);
    }

    while (cue_ < int(cues_.size()) && march_ >= cues_[cue_].t) {
        spawnCue(cues_[cue_]);
        cue_++;
    }

    for (Foe& e : foes_) {
        if (!e.alive) continue;
        e.age += dt;
        e.z -= e.speed * dt;
        e.x = e.lane + std::sin(e.age * 1.6f) * 0.03f;
        if (e.flash > 0) e.flash -= dt;
    }

    fireCd_ = std::max(0.0f, fireCd_ - dt);
    if (fire && fireCd_ <= 0) {
        fireCd_ = 0.15f;
        fireFlash_ = 0.07f;
        shots_.push_back({px_, pz_ + 0.1f, true});
        sys_->apu.noiseBurst(0.26f, 4800.0f, 0.04f);
        blip(1, 160.0f, 0.04f, 0.03f);
    }

    for (Shot& s : shots_) {
        if (!s.alive) continue;
        s.z += 11.0f * dt;
        if (s.z > 14.0f) {
            s.alive = false;
            continue;
        }
        for (Foe& e : foes_) {
            if (!e.alive) continue;
            if (std::fabs(s.x - e.x) > e.hit) continue;
            if (s.z < e.z - 0.2f || s.z > e.z + 0.7f) continue;
            e.hp -= 1;
            e.flash = 0.14f;
            s.alive = false;
            sys_->apu.noiseBurst(0.2f, 1400.0f, 0.06f);
            if (e.hp <= 0) {
                e.alive = false;
                note(e.score, e.x, e.z);
            }
            break;
        }
    }

    if (mode_ == Mode::Fight) {
        for (Foe& e : foes_) {
            if (!e.alive || mode_ != Mode::Fight) continue;
            float reach = (e.kind == 2) ? 0.36f : 0.24f;
            if (e.z <= pz_ + 0.1f && std::fabs(e.x - px_) < reach) {
                hurt();
                e.alive = false;
            } else if (e.z < kLeakZ) {
                leak();
                e.alive = false;
            }
        }
    }

    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& e) { return !e.alive; }), foes_.end());
    shots_.erase(std::remove_if(shots_.begin(), shots_.end(), [](const Shot& s) { return !s.alive; }), shots_.end());

    if (mode_ == Mode::Fight && march_ >= kGoal) reachGate();
}

void Game::driftProps(float dt) {
    for (Prop& p : props_) {
        p.z -= kScroll * dt;
        if (p.z < 1.05f) {
            p.z += 11.6f;
            float sign = p.x < 0 ? -1.0f : 1.0f;
            p.x = sign * (kRoadHalf + 0.36f + rnd() * 0.45f);
            p.kind = rnd() < 0.28f ? 1 : 0;
        }
    }
}

void Game::audio(float dt) {
    for (int i = 0; i < 3; i++) {
        if (psgT_[i] > 0) {
            psgT_[i] -= dt;
            if (psgT_[i] <= 0) sys_->apu.tone(i, 0, 0);
        }
    }
    if (droneOn_) {
        float burble = 1.0f + 0.03f * std::sin(clock_ * 6.0f);
        float vol = (mode_ == Mode::Fight) ? 0.09f : (mode_ == Mode::Victory ? 0.05f : 0.06f);
        sys_->apu.setFreq(0, 74.0f * burble);
        sys_->apu.setVol(0, vol);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {392.0f, 523.0f, 659.0f, 784.0f};
        fanT_ += dt;
        if (fanT_ > 0.14f) {
            if (fanStep_ < 4) sys_->apu.keyOn(1, notes[fanStep_], 0.2f);
            else sys_->apu.keyOff(1);
            fanStep_++;
            fanT_ = 0;
            if (fanStep_ > 7) fanStep_ = -1;
        }
    }
    for (Pop& p : pops_) {
        p.t -= dt;
        p.y -= 16.0f * dt;
    }
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0; }), pops_.end());
}

void Game::drawFoe(int kind, float x, float z, float tall, int frame, float flash) {
    Proj p = project(x, z);
    float h = kPerson * tall * p.t;
    spr(art_.shadow, p.x, p.y, h * 0.22f, PAL_FX, false, p.fog, false, true);
    if (flash > 0) spr(art_.puff, p.x, p.y - h * 0.45f, h * 0.55f, PAL_FX, false, p.fog);
    if (kind == 2) spr(art_.horse, p.x, p.y, h, PAL_HORSE, x < 0, p.fog, true);
    else if (kind == 3) spr(art_.officer, p.x, p.y, h, PAL_ENEMY, false, p.fog, true);
    else if (kind == 1) spr(art_.runner, p.x, p.y, h, PAL_ENEMY, false, p.fog, true);
    else spr(art_.rifle[frame & 1], p.x, p.y, h, PAL_ENEMY, false, p.fog, true);
}

void Game::drawColumn() {
    if (column_ <= 0) return;
    int n = std::min(column_, 8);
    for (int i = 0; i < n; i++) {
        bool wagon = i < 2;
        float ax = 16.0f + float(i / 2) * 24.0f;
        float x = (i % 2 == 0) ? ax : 320.0f - ax;
        float bob = std::sin(clock_ * 8.0f + i * 0.8f) * 1.5f;
        float h = wagon ? 36.0f : 32.0f;
        spr(wagon ? art_.wagon : art_.file[(stepPar_ + i) & 1], x + shx_ * 0.25f, 224.0f + bob, h, PAL_FRIEND, i % 2 == 1, 0,
            true);
    }
}

void Game::drawWorld() {
    gs::VDP& v = sys_->vdp;
    int leg = (mode_ == Mode::Title) ? 0 : leg_;
    uint16_t skyTop, skyHor, fogC;
    if (mode_ == Mode::Fail) {
        skyTop = gs::rgb4(6, 1, 2);
        skyHor = gs::rgb4(12, 4, 3);
        fogC = gs::rgb4(10, 3, 2);
    } else if (leg <= 0) {
        skyTop = gs::rgb4(2, 3, 8);
        skyHor = gs::rgb4(13, 8, 5);
        fogC = gs::rgb4(8, 6, 5);
    } else if (leg == 1) {
        skyTop = gs::rgb4(2, 4, 6);
        skyHor = gs::rgb4(9, 8, 5);
        fogC = gs::rgb4(6, 6, 5);
    } else {
        skyTop = gs::rgb4(6, 2, 4);
        skyHor = gs::rgb4(14, 7, 3);
        fogC = gs::rgb4(10, 6, 4);
    }
    v.setFogColor(fogC);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(kHorizon)) {
            v.road[y].on = false;
            v.lineBackdrop[y] = lerpC(skyTop, skyHor, y / kHorizon);
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) - kHorizon) / kSpan;
        t = std::clamp(t, 0.02f, 1.0f);
        float z = kZNear / t;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.hw = std::max(2.0f, kRoadHalf * kPpm * t);
        r.cx = 160.0f + shx_ + (bend(z) - bend(kZNear)) * (kPpm * t);
        r.v = clock_ * 78.0f + 360.0f / t;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(r.v / 42.0f)) & 1) ? 1 : 0;
        r.style = (leg >= 2) ? gs::ROAD_RUTS : gs::ROAD_MUD;
        r.left = r.right = gs::GROUND_LAND;
        int fog = int(std::clamp((0.2f - t) / 0.2f, 0.0f, 1.0f) * 11.0f);
        if (leg == 1) fog = std::min(14, fog + 2);
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = skyHor;
    }

    for (int i = 0; i < 4; i++) {
        float x = std::fmod(20.0f + i * 86.0f + clock_ * 4.0f, 380.0f) - 30.0f;
        spr(art_.cloud, x, 18.0f + (i % 2) * 10.0f, 16.0f + (i % 3) * 4.0f, PAL_FX, i & 1, 2);
    }

    std::vector<int> order(props_.size());
    for (int i = 0; i < int(props_.size()); i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return props_[a].z < props_[b].z; });
    for (int i = int(order.size()) - 1; i >= 0; i--) {
        const Prop& p = props_[order[i]];
        Proj q = project(p.x, p.z);
        float h = (p.kind ? 70.0f : 150.0f) * q.t;
        spr(p.kind ? art_.bush : art_.tree, q.x, q.y, h, PAL_PROP, p.x < 0, q.fog, true);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    shx_ = 0;
    if (shake_ > 0) shx_ = std::sin(clock_ * 48.0f) * 7.0f * std::min(shake_ * 3.0f, 1.0f);
    drawWorld();

    if (mode_ == Mode::Victory) {
        float u = std::clamp(victoryT_ / 1.5f, 0.0f, 1.0f);
        Proj gate = project(0.0f, 6.2f);
        float gh = 78.0f + u * 10.0f;
        int n = std::max(column_, 1);
        for (int i = n - 1; i >= 0; i--) {
            float z = 1.7f + u * (3.6f + (i % 4) * 0.18f);
            float x = ((i % 2) ? 0.22f : -0.22f) + (i - 3) * 0.04f;
            bool wagon = i < 2;
            Proj p = project(x, z);
            float h = (wagon ? 130.0f : 100.0f) * p.t;
            spr(wagon ? art_.wagon : art_.file[(stepPar_ + i) & 1], p.x, p.y, h, PAL_FRIEND, i & 1, p.fog, true);
        }
        spr(art_.gate, gate.x, gate.y, gh, PAL_PROP, false, 3, true);
        hudC(2, "THE COLUMN IS HOME", PAL_AMBER);
        char buf[32];
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(4, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "FILES %d", column_);
        hudC(6, buf, PAL_GREEN);
        return;
    }

    // Near sprites first so they cover the road.
    drawColumn();

    bool showPlayer = mode_ == Mode::Title || mode_ == Mode::Fight || mode_ == Mode::Pause || mode_ == Mode::Fail;
    if (showPlayer && !(invuln_ > 0 && (int(clock_ * 18.0f) & 1))) {
        Proj p = project(px_, pz_);
        float h = kPerson * p.t;
        float bob = std::sin(clock_ * 8.0f) * 1.2f;
        spr(art_.shadow, p.x, p.y, h * 0.2f, PAL_FX, false, 0, false, true);
        spr(art_.player[stepPar_ & 1], p.x, p.y + bob, h, PAL_PLAYER, vx_ < -0.25f, 0, true);
        if (fireFlash_ > 0) spr(art_.flash, p.x, p.y - h * 0.72f, 16.0f, PAL_FX, false);
    }

    for (const Shot& s : shots_) {
        Proj p = project(s.x, s.z);
        spr(art_.shot, p.x, p.y, std::max(8.0f, 36.0f * p.t), PAL_FX, false, p.fog);
    }

    std::vector<int> order(foes_.size());
    for (int i = 0; i < int(foes_.size()); i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z < foes_[b].z; });
    for (int i = int(order.size()) - 1; i >= 0; i--) {
        const Foe& e = foes_[order[i]];
        drawFoe(e.kind, e.x, e.z, e.tall, stepPar_, e.flash);
    }

    if (mode_ == Mode::Title) {
        float z = 6.4f + std::sin(clock_ * 0.65f) * 1.4f;
        float x = std::sin(clock_ * 0.45f) * 0.4f;
        drawFoe(0, x, z, 1.0f, stepPar_, 0);
        text("REARGUARD", 160, 20, 1.05f, PAL_AMBER);
        hudC(5, "THE FIGHT IS BEHIND YOU", PAL_HUD);
        hudC(21, "ARROWS MOVE    C OR Z FIRES", PAL_HUD);
        hudC(23, "WALK THEM TO THE GATE", PAL_AMBER);
        if ((int(clock_ * 2.0f) & 1) == 0) hudC(25, "PRESS START", PAL_HUD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_HUD);
        return;
    }

    char buf[48];
    std::snprintf(buf, sizeof buf, "%d", score_);
    hud(1, 1, legName(), PAL_AMBER);
    hud(39 - int(std::strlen(buf)), 1, buf, PAL_HUD);
    if (banner_ > 0 && (mode_ == Mode::Fight || mode_ == Mode::Pause)) hudC(3, bannerText_, PAL_HUD);

    hud(1, 26, "COLUMN", leakFlash_ > 0 ? PAL_RED : PAL_HUD);
    for (int i = 0; i < 8; i++) hud(8 + i, 26, i < column_ ? "#" : "-", i < column_ ? PAL_GREEN : PAL_HUD);
    hud(17, 26, "YOU", wounds_ <= 1 ? PAL_RED : PAL_HUD);
    for (int i = 0; i < 3; i++) hud(21 + i, 26, i < wounds_ ? "*" : "-", i < wounds_ ? PAL_AMBER : PAL_HUD);
    hud(25, 26, "HOME", PAL_HUD);
    int filled = std::clamp(int(march_ / kGoal * 8.0f + 0.001f), 0, 8);
    for (int i = 0; i < 8; i++) hud(30 + i, 26, i < filled ? "=" : "-", i < filled ? PAL_GREEN : PAL_HUD);

    if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_AMBER);
        hudC(15, "ARROWS ACROSS   UP CLOSER", PAL_HUD);
        hudC(17, "C FIRES DOWN THE ROAD", PAL_HUD);
        hudC(19, "START RESUMES    ESC TITLE", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(12, "THE COLUMN BREAKS", PAL_RED);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(15, buf, PAL_HUD);
        if (!bot_) hudC(18, "PRESS START", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(8, 6, 5));
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.16f, 0.22f, 0.14f);
    sys.apu.setPatch(0, dronePatch());
    sys.apu.setPatch(1, hornPatch());
    sys.apu.keyOn(0, 74.0f, 0.06f);
    droneOn_ = true;
    props_.clear();
    for (int i = 0; i < 10; i++) {
        Prop p;
        p.z = 1.5f + i * 1.12f;
        float sign = (i % 2) ? 1.0f : -1.0f;
        p.x = sign * (kRoadHalf + 0.4f + (i % 3) * 0.16f);
        p.kind = (i % 5 == 0) ? 1 : 0;
        props_.push_back(p);
    }
    if (bot_) beginMarch();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = kDt;
    const gs::Pad& pad = sys.pad;
    bool frozen = mode_ == Mode::Pause || mode_ == Mode::Fail;
    if (!frozen) clock_ += dt;

    if (mode_ == Mode::Title) {
        updateTitle(dt);
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            beginMarch();
            blip(2, 523.0f, 0.06f, 0.08f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Fight) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else updateFight(dt);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Fight;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            foes_.clear();
            shots_.clear();
            march_ = 0;
            banner_ = 0;
        }
    } else if (mode_ == Mode::Victory) {
        victoryT_ += dt;
        stepAcc_ += dt;
        if (stepAcc_ >= 0.42f) {
            stepAcc_ -= 0.42f;
            stepPar_++;
        }
        if (victoryT_ > 1.55f) over_ = true;
        if (!bot_ && over_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            foes_.clear();
            shots_.clear();
            march_ = 0;
        }
    } else if (mode_ == Mode::Fail) {
        failT_ += dt;
        if (failT_ > 1.2f) over_ = true;
        if (!bot_ && over_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            foes_.clear();
            shots_.clear();
            march_ = 0;
        }
    }

    if (!frozen && mode_ != Mode::Fail) driftProps(dt);
    audio(dt);
    draw();
}

}  // namespace rearguard
