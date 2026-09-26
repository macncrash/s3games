#include "game/gate.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "version.h"

namespace gate {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float FOCAL = 230.0f;
constexpr float HORIZON = 96.0f;
constexpr float GROUND = 2.45f;
constexpr float LANE_X = 1.58f;
constexpr float Z_SPAWN = 50.0f;
constexpr float Z_HIT = 5.2f;
constexpr float Z_MELEE = 12.0f;
constexpr float Z_BOLT = 6.2f;
constexpr float BOLT_V = 34.0f;
constexpr float MOVE = 2.7f;
constexpr float BELL_AT = 49.2f;
constexpr float BELL_END = 59.7f;
constexpr float ROPE_NEED = 1.2f;
constexpr int GATE_MAX = 8;

// Arrival times along the road. Spawn is arrival minus the walk.
struct Arr {
    float arr;
    int kind;
    int lane;
};
const Arr kArr[] = {
    {11.2f, 0, -1}, {13.8f, 0, 1}, {16.4f, 0, 0}, {19.0f, 0, -1}, {21.8f, 1, 1},
    {25.2f, 0, 0}, {27.8f, 0, -1}, {30.4f, 0, 1}, {34.2f, 2, 0},  {39.7f, 0, 1},
    {42.4f, 1, -1}, {46.2f, 0, 0}, {49.0f, 0, 1}, {51.8f, 0, -1}, {55.4f, 0, 1},
    {58.7f, 0, -1},
};

float speedOf(int kind) { return kind == 0 ? 4.15f : kind == 1 ? 3.15f : 2.25f; }
int hpOf(int kind) { return kind == 0 ? 1 : kind == 1 ? 2 : 4; }
int ptsOf(int kind) { return kind == 0 ? 100 : kind == 1 ? 250 : 500; }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.32f;
    p.op[0] = {1.0f, 0.75f, 0.4f, 1.0f, 0.85f, 0.45f};
    p.op[1] = {2.0f, 0.32f, 0.3f, 0.8f, 0.55f, 0.4f};
    p.op[2] = {0.5f, 0.5f, 0.45f, 1.2f, 0.7f, 0.5f};
    p.op[3] = {3.0f, 0.16f, 0.2f, 0.6f, 0.35f, 0.3f};
    p.vol = 0.14f;
    p.tone = 620.0f;
    p.drive = 0.06f;
    return p;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.12f;
    p.op[0] = {1.0f, 1.0f, 0.004f, 0.6f, 0.12f, 0.95f};
    p.op[1] = {2.76f, 0.42f, 0.004f, 0.42f, 0.08f, 0.75f};
    p.op[2] = {5.4f, 0.2f, 0.006f, 0.32f, 0.04f, 0.6f};
    p.op[3] = {1.5f, 0.28f, 0.005f, 0.5f, 0.1f, 0.8f};
    p.vol = 0.26f;
    p.echo = 0.4f;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.22f;
    p.op[0] = {1, 1, 0.02f, 0.2f, 0.62f, 0.16f};
    p.op[1] = {2, 0.4f, 0.02f, 0.24f, 0.4f, 0.16f};
    p.op[2] = {3, 0.22f, 0.03f, 0.28f, 0.28f, 0.18f};
    p.op[3] = {1, 0.32f, 0.02f, 0.22f, 0.5f, 0.16f};
    p.vol = 0.2f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (over_ || mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (mode_ == Mode::Watch && bell_) return 2;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) return 1;
    return 0;
}

float Game::eta(const Foe& f) const { return (f.z - Z_HIT) / f.speed; }

int Game::indexOf(int id) const {
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].id == id && foes_[i].alive) return i;
    return -1;
}

int Game::nearestLane() const {
    int lane = int(std::lround(px_));
    lane = std::clamp(lane, -1, 1);
    if (std::fabs(px_ - float(lane)) > 0.42f) return 99;
    return lane;
}

void Game::tone(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = 0.07f;
}

void Game::hurt(Foe& f) {
    f.hp -= 1;
    f.flash = 0.1f;
    if (f.hp > 0) {
        tone(480.0f, 0.05f);
        return;
    }
    f.alive = false;
    score_ += f.points;
    Pop p;
    p.x = float(f.lane) * LANE_X;
    p.z = f.z;
    p.t = 0.65f;
    p.pts = f.points;
    pops_.push_back(p);
    if (pops_.size() > 5) pops_.erase(pops_.begin());
    sys_->apu.noiseBurst(0.2f, 1400.0f, 0.07f);
    tone(740.0f, 0.05f);
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    endT_ = 0;
    reason_ = "THE WATCH HELD UNTIL THE RELIEF BELL";
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.16f, 2200.0f, 0.08f);
    fanStep_ = 0;
    fanT_ = 0;
    sys_->setLight(40, 170, 70);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    endT_ = 0;
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.5f, 280.0f, 0.36f);
    sys_->apu.keyOn(2, 146.0f, 0.18f);
    sys_->setLight(190, 24, 18);
    shake_ = 0.45f;
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    bellAnnounced_ = false;
    reason_ = "WATCH OVER";
    gate_ = GATE_MAX;
    score_ = 0;
    nextId_ = 1;
    focus_ = -1;
    spawnAt_ = 0;
    t_ = 0;
    watch_ = 0;
    px_ = 0;
    rope_ = 0;
    meleeCd_ = boltCd_ = swing_ = shake_ = endT_ = bellTick_ = 0;
    fanStep_ = -1;
    foes_.clear();
    bolts_.clear();
    pops_.clear();
    script_.clear();
    script_.reserve(sizeof kArr / sizeof kArr[0]);
    for (const Arr& a : kArr) {
        Spawn s;
        s.t = a.arr - (Z_SPAWN - Z_HIT) / speedOf(a.kind);
        s.kind = a.kind;
        s.lane = a.lane;
        script_.push_back(s);
    }
    std::sort(script_.begin(), script_.end(), [](const Spawn& a, const Spawn& b) { return a.t < b.t; });
    if (sys_) sys_->setLight(30, 40, 80);
}

void Game::update(float dt) {
    while (spawnAt_ < int(script_.size()) && script_[spawnAt_].t <= watch_) {
        const Spawn& s = script_[spawnAt_++];
        Foe f;
        f.id = nextId_++;
        f.kind = s.kind;
        f.lane = s.lane;
        f.z = Z_SPAWN;
        f.speed = speedOf(s.kind);
        f.hp = hpOf(s.kind);
        f.points = ptsOf(s.kind);
        f.alive = true;
        foes_.push_back(f);
    }

    bool wasBell = bell_;
    bell_ = watch_ >= BELL_AT;
    if (bell_ && !wasBell) {
        bellAnnounced_ = true;
        bellTick_ = 0;
        sys_->apu.keyOn(1, 698.0f, 0.26f);
        sys_->rumble(0.25f, 0.45f, 140);
    } else if (bell_ && !won_) {
        bellTick_ += dt;
        if (bellTick_ >= 0.95f) {
            bellTick_ = 0;
            sys_->apu.keyOn(1, 698.0f, 0.22f);
        }
    }

    int best = -1;
    float bestEta = 1.0e9f;
    float mind = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[i].alive) continue;
        float e = eta(foes_[i]);
        if (e < mind) mind = e;
        if (e < bestEta) {
            bestEta = e;
            best = i;
        }
    }
    int fi = indexOf(focus_);
    if (fi >= 0) {
        if (best >= 0 && eta(foes_[best]) + 0.35f < eta(foes_[fi])) focus_ = foes_[best].id;
    } else if (best >= 0) {
        focus_ = foes_[best].id;
    }
    fi = indexOf(focus_);

    float dir = 0;
    bool strike = false;
    bool ropeHeld = false;
    if (bot_) {
        bool closing = bell_ && (BELL_END - watch_) < ROPE_NEED + 0.45f;
        bool want = bell_ && watch_ < BELL_END && (mind > 2.05f || (closing && mind > 0.6f));
        if (want) {
            if (px_ > 0.03f) dir = -1;
            else if (px_ < -0.03f) dir = 1;
            else ropeHeld = true;
        } else if (fi >= 0) {
            float dest = float(foes_[fi].lane);
            if (px_ < dest - 0.02f) dir = 1.0f;
            else if (px_ > dest + 0.02f) dir = -1.0f;
            if (std::fabs(px_ - dest) < 0.38f) strike = true;
        }
    } else {
        const gs::Pad& pad = sys_->pad;
        float digital = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        float axis = std::fabs(pad.axisX) > 0.18f ? pad.axisX : digital;
        bool up = pad.down(gs::BTN_UP) || pad.down(gs::BTN_B);
        if (bell_ && std::fabs(px_) < 0.40f && up) ropeHeld = true;
        else dir = std::clamp(axis, -1.0f, 1.0f);
        strike = pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.accel > 0.45f;
    }

    if (ropeHeld) {
        rope_ += dt;
        if (rope_ >= ROPE_NEED) winWatch();
    } else {
        if (rope_ > 0) rope_ = std::max(0.0f, rope_ - 1.2f * dt);
        px_ = std::clamp(px_ + dir * MOVE * dt, -1.0f, 1.0f);
    }

    meleeCd_ = std::max(0.0f, meleeCd_ - dt);
    boltCd_ = std::max(0.0f, boltCd_ - dt);
    if (swing_ > 0) swing_ -= dt;

    if (strike && !ropeHeld && !won_) {
        int lane = nearestLane();
        if (lane >= -1 && lane <= 1) {
            bool close = false, far = false;
            for (const Foe& f : foes_) {
                if (!f.alive || f.lane != lane) continue;
                if (f.z <= Z_MELEE && f.z > Z_HIT) close = true;
                if (f.z > Z_MELEE) far = true;
            }
            if (close && meleeCd_ <= 0) {
                for (Foe& f : foes_) {
                    if (!f.alive || f.lane != lane || f.z > Z_MELEE || f.z <= Z_HIT) continue;
                    hurt(f);
                }
                meleeCd_ = 0.34f;
                swing_ = 0.14f;
                sys_->apu.noiseBurst(0.28f, 900.0f, 0.06f);
                tone(210.0f, 0.06f);
            } else if (far && boltCd_ <= 0 && int(bolts_.size()) < 2) {
                Bolt b;
                b.lane = lane;
                b.z = b.prev = Z_BOLT;
                bolts_.push_back(b);
                boltCd_ = 0.40f;
                tone(620.0f, 0.04f);
            }
        }
    }

    std::vector<Bolt> keep;
    keep.reserve(bolts_.size());
    for (Bolt b : bolts_) {
        b.prev = b.z;
        b.z += BOLT_V * dt;
        int hit = -1;
        for (int i = 0; i < int(foes_.size()); i++) {
            Foe& f = foes_[i];
            if (!f.alive || f.lane != b.lane) continue;
            if (b.prev - 0.2f <= f.z && b.z + 0.35f >= f.z) {
                if (hit < 0 || f.z < foes_[hit].z) hit = i;
            }
        }
        if (hit >= 0) hurt(foes_[hit]);
        else if (b.z < Z_SPAWN + 3.0f) keep.push_back(b);
    }
    bolts_.swap(keep);

    if (!won_) {
        for (Foe& f : foes_) {
            if (!f.alive) continue;
            f.age += dt;
            if (f.flash > 0) f.flash -= dt;
            f.z -= f.speed * dt;
            if (f.z > Z_HIT) continue;
            f.alive = false;
            gate_ -= f.kind == 2 ? 2 : 1;
            shake_ = 0.42f;
            sys_->rumble(0.75f, 1.0f, 200);
            sys_->apu.noiseBurst(0.5f, 420.0f, 0.18f);
            tone(90.0f, 0.08f);
            if (gate_ <= 0) {
                gate_ = 0;
                loseWatch("THE GATE FELL");
                break;
            }
        }
    }

    for (Pop& p : pops_) p.t -= dt;
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0; }), pops_.end());
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - dt * 1.4f);

    watch_ += dt;
    if (!won_ && mode_ == Mode::Watch && watch_ >= BELL_END) loseWatch("MISSED THE BELL");
}

void Game::project(float worldX, float z, float& sx, float& sy, float& s) const {
    float zz = std::max(0.8f, z);
    s = FOCAL / zz;
    sx = 160.0f + worldX * s;
    sy = HORIZON + GROUND * s;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
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

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float dawn = 0.1f;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) dawn = std::clamp(watch_ / BELL_AT, 0.0f, 1.0f);
    if (bell_) dawn = std::max(dawn, 0.78f);
    if (mode_ == Mode::Victory) dawn = 1.0f;
    uint16_t skyTop = lerpC(gs::rgb4(1, 1, 5), gs::rgb4(4, 6, 11), dawn);
    uint16_t skyHor = lerpC(gs::rgb4(2, 3, 7), gs::rgb4(13, 8, 4), dawn * dawn);
    uint16_t fogC = lerpC(gs::rgb4(1, 2, 4), gs::rgb4(8, 6, 4), dawn * 0.65f);
    v.setFogColor(fogC);

    float shx = shake_ > 0 ? std::sin(t_ * 92.0f) * 6.0f * shake_ : 0;
    float shy = shake_ > 0 ? std::cos(t_ * 74.0f) * 3.0f * shake_ : 0;
    const int horizon = int(HORIZON);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < horizon) {
            v.lineBackdrop[y] = lerpC(skyTop, skyHor, y / float(horizon));
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - horizon) + 1.0f;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.0f + shx;
        r.hw = 10.0f + row * 1.22f;
        r.v = 3400.0f / row;
        r.pal = PAL_FIELD;
        r.band = (int(std::floor(r.v / 48.0f)) & 1) ? 1 : 0;
        r.style = 1;
        r.left = r.right = 0;
        float fog = std::clamp(1.0f - row / 80.0f, 0.0f, 1.0f);
        v.lineFog[y] = uint8_t(fog * (bell_ ? 5.0f : 8.0f));
        v.lineBackdrop[y] = skyHor;
    }

    auto fogAt = [](float z) { return std::clamp(int((z - 16.0f) / 3.1f), 0, 12); };

    struct Item {
        float z;
        int kind;
        int i;
    };
    std::vector<Item> items;
    if (mode_ != Mode::Title) {
        for (int i = 0; i < int(foes_.size()); i++)
            if (foes_[i].alive) items.push_back({foes_[i].z, 0, i});
        for (int i = 0; i < int(bolts_.size()); i++) items.push_back({bolts_[i].z, 1, i});
    }
    static const float kTorch[][2] = {
        {-2.35f, 8.6f}, {2.35f, 9.4f}, {-2.55f, 14.2f}, {2.50f, 15.4f},
        {-2.70f, 22.0f}, {2.65f, 23.6f}, {-2.85f, 33.0f}, {2.80f, 35.0f},
    };
    for (int i = 0; i < 8; i++) items.push_back({kTorch[i][1], 2, i});
    if (mode_ == Mode::Victory) {
        items.push_back({std::max(8.0f, 10.2f - endT_ * 0.7f), 3, 0});
        items.push_back({std::max(9.2f, 12.0f - endT_ * 0.7f), 3, 1});
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    // Earlier sprites sit on top. Lettering first, then the arch, then the road.
    if (mode_ == Mode::Title) text("GATE RELIEF", 160, 46, 1.0f, PAL_FX);
    else if (mode_ == Mode::Pause) text("PAUSED", 160, 44, 1.0f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("WATCH HELD", 160, 44, 0.95f, PAL_BELL);
    else if (mode_ == Mode::Over) text(reason_, 160, 44, 0.7f, PAL_RAIDER);

    float playerX = 160.0f + px_ * LANE_X * FOCAL / Z_HIT + shx;
    float feet = 210.0f + shy;
    spr(art_.tower, 26 + shx, 112, 224, PAL_STONE, false);
    spr(art_.tower, 294 + shx, 112, 224, PAL_STONE, true);
    spr(art_.banner, 34 + shx, 78, 36, PAL_RAIDER, false);
    spr(art_.banner, 286 + shx, 78, 36, PAL_RAIDER, true);
    if (mode_ == Mode::Over && !won_) {
        float drop = std::min(1.0f, endT_ * 1.6f);
        spr(art_.grate, 160 + shx, 40 + drop * 130.0f, 168, PAL_STONE, false);
    }
    float swing = 0;
    if (bell_ || mode_ == Mode::Victory) swing = std::sin(t_ * (rope_ > 0.05f ? 14.0f : 8.0f)) * (rope_ > 0.05f ? 8.0f : 5.0f);
    spr(art_.bell, 160 + swing + shx, 20, 26, PAL_FX, false);
    spr(art_.rope, 160 + swing * 0.25f + shx, 58, 70, PAL_RAM, false);

    spr(art_.shadow, playerX, feet - 2, 12, PAL_STONE, false);
    int pose = swing_ > 0 ? 1 : 0;
    spr(art_.sentry[pose], playerX, feet, 92, PAL_SENTRY, false, 0, true);
    if (swing_ > 0) spr(art_.spear, playerX, feet - 78, 50, PAL_SENTRY, false);

    for (const Item& it : items) {
        if (it.kind == 0) {
            const Foe& f = foes_[it.i];
            float sx, sy, s;
            project(float(f.lane) * LANE_X, f.z, sx, sy, s);
            sx += shx;
            sy += shy + std::sin(f.age * 7.0f + f.lane) * 1.5f;
            int fog = fogAt(f.z);
            float body = f.kind == 2 ? 2.2f : f.kind == 1 ? 1.95f : 1.72f;
            float h = body * s;
            if (f.flash > 0) spr(art_.puff, sx, sy - h * 0.45f, h * 0.55f, PAL_FX, false, fog);
            if (f.kind == 2) spr(art_.ram, sx, sy, h, PAL_RAM, false, fog, true);
            else if (f.kind == 1) spr(art_.shield, sx, sy, h, PAL_RAIDER, f.lane > 0, fog, true);
            else spr(art_.runner[int(f.age * 6.0f) & 1], sx, sy, h, PAL_RAIDER, f.lane < 0, fog, true);
        } else if (it.kind == 1) {
            const Bolt& b = bolts_[it.i];
            float sx, sy, s;
            project(float(b.lane) * LANE_X, b.z, sx, sy, s);
            spr(art_.bolt, sx + shx, sy - 1.15f * s + shy, std::max(6.0f, 0.55f * s), PAL_FX, false, fogAt(b.z));
        } else if (it.kind == 2) {
            float sx, sy, s;
            project(kTorch[it.i][0], kTorch[it.i][1], sx, sy, s);
            float h = 1.35f * s;
            int frame = int(t_ * 9.0f + it.i * 3) & 1;
            spr(art_.post, sx + shx, sy + shy, h, PAL_NIGHT, false, fogAt(kTorch[it.i][1]), true);
            spr(art_.flame[frame], sx + shx, sy - h * 0.92f + shy, h * 0.42f, PAL_FX, false, fogAt(kTorch[it.i][1]));
        } else {
            float z = it.z;
            float x = it.i == 0 ? -0.35f : 0.42f;
            float sx, sy, s;
            project(x, z, sx, sy, s);
            spr(art_.relief[it.i], sx + shx, sy + shy, 1.85f * s, PAL_BELL, false, fogAt(z), true);
        }
    }

    static const float kStars[][2] = {{76, 16}, {98, 38}, {124, 14}, {186, 18}, {214, 12}, {232, 34}, {150, 26}};
    for (int i = 0; i < 7; i++) {
        if ((int(t_ * 2.0f) + i * 3) % 11 == 0) continue;
        spr(art_.star, kStars[i][0], kStars[i][1], 5, PAL_NIGHT, false);
    }
    spr(art_.moon, 214, 30, 30, PAL_NIGHT, false);

    for (const Pop& p : pops_) {
        float sx, sy, s;
        project(p.x, p.z, sx, sy, s);
        char buf[16];
        std::snprintf(buf, sizeof buf, "+%d", p.pts);
        text(buf, sx, sy - s * 1.2f - (0.65f - p.t) * 16.0f, 0.45f, PAL_FX);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        if (int(t_ * 2.0f) % 2 == 0) hudC(22, "PRESS START", PAL_HUD);
        hudC(24, "ARROWS SHIFT THE BAY", PAL_HUD);
        hudC(25, "C STRIKES OR LOOSES", PAL_HUD);
        hudC(26, "UP ANSWERS THE BELL", PAL_FX);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 1, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        hud(1, 1, "GATE", gate_ <= 2 ? PAL_RAIDER : PAL_HUD);
        for (int i = 0; i < GATE_MAX; i++) {
            int pal = i < gate_ ? (gate_ <= 2 || shake_ > 0.15f ? PAL_RAIDER : PAL_BELL) : PAL_HUD;
            hud(6 + i, 1, i < gate_ ? "=" : "-", pal);
        }
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(39 - int(std::strlen(buf)), 1, buf, PAL_HUD);

        const Foe* near = nullptr;
        for (const Foe& f : foes_) {
            if (!f.alive) continue;
            if (!near || f.z < near->z) near = &f;
        }
        if (bell_) {
            hudC(22, std::fabs(px_) > 0.4f ? "GET TO THE CENTER" : "HOLD UP ON THE ROPE", PAL_FX);
            int n = std::clamp(int(std::lround(rope_ / ROPE_NEED * 10.0f)), 0, 10);
            std::string meter = "BELL ";
            for (int i = 0; i < 10; i++) meter += i < n ? "=" : "-";
            hudC(24, meter, rope_ > 0 ? PAL_BELL : PAL_FX);
        } else if (near && near->z <= Z_MELEE) {
            hudC(22, "AT THE GATE", PAL_RAIDER);
        } else if (near) {
            const char* bay = near->lane < 0 ? "LEFT BAY" : near->lane > 0 ? "RIGHT BAY" : "CENTER BAY";
            hudC(22, bay, PAL_HUD);
        }
        if (!bell_) {
            hud(1, 26, "WATCH", PAL_HUD);
            int n = std::clamp(int(watch_ / BELL_AT * 10.0f), 0, 10);
            for (int i = 0; i < 10; i++) hud(8 + i, 26, i < n ? "=" : "-", i < n ? PAL_FX : PAL_HUD);
        }
        if (mode_ == Mode::Pause) {
            hudC(24, "START RESUMES", PAL_HUD);
            hudC(26, "ESC LEAVES THE WATCH", PAL_HUD);
        }
    } else if (mode_ == Mode::Victory) {
        hudC(22, "RELIEF HAS THE GATE", PAL_BELL);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(24, buf, PAL_HUD);
        hudC(26, "START", PAL_HUD);
    } else if (mode_ == Mode::Over) {
        hudC(22, "THE WATCH IS OVER", PAL_RAIDER);
        std::snprintf(buf, sizeof buf, "GATE %d   SCORE %d", gate_, score_);
        hudC(24, buf, PAL_HUD);
        hudC(26, "START TRIES AGAIN", PAL_HUD);
    }

    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (droneOn_) {
        sys_->apu.setFreq(0, 58.0f + dawn * 16.0f);
        sys_->apu.setVol(0, mode_ == Mode::Watch ? 0.09f : 0.055f);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {392.0f, 494.0f, 587.0f, 784.0f};
        fanT_ += DT;
        if (fanT_ > 0.16f) {
            if (fanStep_ < 4) sys_->apu.keyOn(2, notes[fanStep_], 0.2f);
            else sys_->apu.keyOff(2);
            fanStep_++;
            fanT_ = 0;
            if (fanStep_ > 7) fanStep_ = -1;
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.2f, 0.3f, 0.18f);
    sys.apu.setPatch(0, dronePatch());
    sys.apu.setPatch(1, bellPatch());
    sys.apu.setPatch(2, hornPatch());
    sys.apu.keyOn(0, 58.0f, 0.07f);
    droneOn_ = true;
    if (bot_) beginWatch();
    else {
        mode_ = Mode::Title;
        sys.setLight(30, 40, 90);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        px_ = std::sin(t_ * 0.45f) * 0.05f;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            tone(660.0f, 0.06f);
            beginWatch();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Watch) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(DT);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Watch;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            foes_.clear();
            bolts_.clear();
            bell_ = false;
            over_ = false;
        }
    } else if (mode_ == Mode::Victory || mode_ == Mode::Over) {
        endT_ += DT;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            if (mode_ == Mode::Over) beginWatch();
            else {
                mode_ = Mode::Title;
                over_ = false;
                won_ = false;
                foes_.clear();
                bell_ = false;
            }
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            foes_.clear();
            bell_ = false;
        }
    }

    if (mode_ != Mode::Watch && mode_ != Mode::Pause && shake_ > 0) shake_ = std::max(0.0f, shake_ - DT);
    draw();
}

}  // namespace gate
