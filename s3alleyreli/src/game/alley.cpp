#include "game/alley.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace alley {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float FOCAL = 220.0f;
constexpr float HORIZON = 78.0f;
constexpr float GROUND = 2.15f;
constexpr float LANE_X = 0.92f;
constexpr float Z_SPAWN = 46.0f;
constexpr float Z_HIT = 4.6f;
constexpr float Z_MELEE = 10.5f;
constexpr float Z_TOSS = 5.4f;
constexpr float TOSS_V = 32.0f;
constexpr float MOVE = 2.4f;
constexpr float BELL_AT = 48.0f;
constexpr float BELL_END = 58.5f;
constexpr float ROPE_NEED = 1.15f;
constexpr int ALLEY_MAX = 8;

struct Arr {
    float arr;
    int kind;
    int lane;
};
const Arr kArr[] = {
    {10.4f, 0, -1}, {12.8f, 0, 1},  {15.6f, 0, 0}, {18.2f, 0, -1}, {21.0f, 1, 1},
    {24.4f, 0, 0},  {27.0f, 0, -1}, {29.8f, 0, 1}, {33.2f, 2, 0},  {37.4f, 0, 1},
    {40.0f, 1, -1}, {43.2f, 0, 0},  {45.6f, 0, 1},
};

float speedOf(int kind) { return kind == 0 ? 3.9f : kind == 1 ? 3.0f : 2.15f; }
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
    p.fb = 0.28f;
    p.op[0] = {1.0f, 0.6f, 0.35f, 1.0f, 0.7f, 0.4f};
    p.op[1] = {2.0f, 0.22f, 0.25f, 0.7f, 0.4f, 0.35f};
    p.op[2] = {0.5f, 0.4f, 0.4f, 1.1f, 0.55f, 0.45f};
    p.op[3] = {3.0f, 0.12f, 0.18f, 0.5f, 0.25f, 0.25f};
    p.vol = 0.12f;
    p.tone = 480.0f;
    p.drive = 0.05f;
    return p;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.1f;
    p.op[0] = {1.0f, 1.0f, 0.004f, 0.55f, 0.1f, 0.9f};
    p.op[1] = {2.7f, 0.38f, 0.004f, 0.4f, 0.07f, 0.7f};
    p.op[2] = {5.2f, 0.18f, 0.006f, 0.3f, 0.04f, 0.55f};
    p.op[3] = {1.5f, 0.24f, 0.005f, 0.45f, 0.08f, 0.75f};
    p.vol = 0.24f;
    p.echo = 0.38f;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.18f;
    p.op[0] = {1, 1, 0.02f, 0.2f, 0.55f, 0.14f};
    p.op[1] = {2, 0.35f, 0.02f, 0.22f, 0.35f, 0.14f};
    p.op[2] = {3, 0.18f, 0.03f, 0.26f, 0.24f, 0.16f};
    p.op[3] = {1, 0.28f, 0.02f, 0.2f, 0.42f, 0.14f};
    p.vol = 0.18f;
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
        tone(420.0f, 0.05f);
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
    sys_->apu.noiseBurst(0.18f, 1200.0f, 0.06f);
    tone(680.0f, 0.05f);
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    endT_ = 0;
    reason_ = "THE WATCH HELD UNTIL THE RELIEF BELL";
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.14f, 2000.0f, 0.07f);
    fanStep_ = 0;
    fanT_ = 0;
    sys_->setLight(40, 180, 90);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    endT_ = 0;
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.48f, 240.0f, 0.34f);
    sys_->apu.keyOn(2, 130.0f, 0.16f);
    sys_->setLight(180, 20, 40);
    shake_ = 0.45f;
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "WATCH OVER";
    alley_ = ALLEY_MAX;
    score_ = 0;
    nextId_ = 1;
    focus_ = -1;
    spawnAt_ = 0;
    t_ = 0;
    watch_ = 0;
    px_ = 0;
    rope_ = 0;
    meleeCd_ = tossCd_ = swing_ = shake_ = endT_ = bellTick_ = 0;
    fanStep_ = -1;
    foes_.clear();
    tosses_.clear();
    pops_.clear();
    script_.clear();
    for (const Arr& a : kArr) {
        Spawn s;
        s.t = a.arr - (Z_SPAWN - Z_HIT) / speedOf(a.kind);
        s.kind = a.kind;
        s.lane = a.lane;
        script_.push_back(s);
    }
    std::sort(script_.begin(), script_.end(), [](const Spawn& a, const Spawn& b) { return a.t < b.t; });
    if (sys_) sys_->setLight(40, 20, 80);
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
        bellTick_ = 0;
        sys_->apu.keyOn(1, 660.0f, 0.26f);
        sys_->rumble(0.22f, 0.4f, 130);
    } else if (bell_ && !won_) {
        bellTick_ += dt;
        if (bellTick_ >= 0.95f) {
            bellTick_ = 0;
            sys_->apu.keyOn(1, 660.0f, 0.2f);
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
        bool closing = bell_ && (BELL_END - watch_) < ROPE_NEED + 0.5f;
        bool want = bell_ && watch_ < BELL_END && (mind > 1.9f || (closing && mind > 0.55f));
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
    tossCd_ = std::max(0.0f, tossCd_ - dt);
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
                meleeCd_ = 0.32f;
                swing_ = 0.14f;
                sys_->apu.noiseBurst(0.26f, 800.0f, 0.05f);
                tone(180.0f, 0.06f);
            } else if (far && tossCd_ <= 0 && int(tosses_.size()) < 2) {
                Toss b;
                b.lane = lane;
                b.z = b.prev = Z_TOSS;
                tosses_.push_back(b);
                tossCd_ = 0.42f;
                tone(540.0f, 0.04f);
            }
        }
    }

    std::vector<Toss> keep;
    keep.reserve(tosses_.size());
    for (Toss b : tosses_) {
        b.prev = b.z;
        b.z += TOSS_V * dt;
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
    tosses_.swap(keep);

    if (!won_) {
        for (Foe& f : foes_) {
            if (!f.alive) continue;
            f.age += dt;
            if (f.flash > 0) f.flash -= dt;
            f.z -= f.speed * dt;
            if (f.z > Z_HIT) continue;
            f.alive = false;
            alley_ -= f.kind == 2 ? 2 : 1;
            shake_ = 0.4f;
            sys_->rumble(0.7f, 1.0f, 180);
            sys_->apu.noiseBurst(0.45f, 360.0f, 0.16f);
            tone(80.0f, 0.08f);
            if (alley_ <= 0) {
                alley_ = 0;
                loseWatch("THE ALLEY BROKE");
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

    float dawn = 0.08f;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) dawn = std::clamp(watch_ / BELL_AT, 0.0f, 1.0f) * 0.35f;
    if (bell_) dawn = std::max(dawn, 0.55f);
    if (mode_ == Mode::Victory) dawn = 0.85f;
    uint16_t skyTop = lerpC(gs::rgb4(1, 0, 3), gs::rgb4(2, 2, 6), dawn);
    uint16_t skyHor = lerpC(gs::rgb4(3, 1, 5), gs::rgb4(8, 4, 6), dawn);
    v.setFogColor(lerpC(gs::rgb4(1, 1, 3), gs::rgb4(6, 3, 5), dawn * 0.5f));

    float shx = shake_ > 0 ? std::sin(t_ * 90.0f) * 5.0f * shake_ : 0;
    float shy = shake_ > 0 ? std::cos(t_ * 70.0f) * 2.5f * shake_ : 0;
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
        r.hw = 18.0f + row * 0.72f;
        r.v = 2800.0f / row;
        r.pal = PAL_FIELD;
        r.band = (int(std::floor(r.v / 36.0f)) & 1) ? 1 : 0;
        r.style = 1;
        r.left = r.right = 0;
        float fog = std::clamp(1.0f - row / 70.0f, 0.0f, 1.0f);
        v.lineFog[y] = uint8_t(fog * (bell_ ? 4.0f : 9.0f));
        v.lineBackdrop[y] = skyHor;
    }

    auto fogAt = [](float z) { return std::clamp(int((z - 14.0f) / 2.8f), 0, 12); };

    struct Item {
        float z;
        int kind;
        int i;
    };
    std::vector<Item> items;
    if (mode_ != Mode::Title) {
        for (int i = 0; i < int(foes_.size()); i++)
            if (foes_[i].alive) items.push_back({foes_[i].z, 0, i});
        for (int i = 0; i < int(tosses_.size()); i++) items.push_back({tosses_[i].z, 1, i});
    }
    static const float kLamp[][2] = {{-1.55f, 9.0f}, {1.55f, 11.2f}, {-1.7f, 18.5f}, {1.7f, 22.0f}, {-1.85f, 32.0f}};
    for (int i = 0; i < 5; i++) items.push_back({kLamp[i][1], 2, i});
    items.push_back({7.4f, 4, 0});
    items.push_back({8.2f, 4, 1});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z > b.z; });

    if (mode_ == Mode::Title) text("ALLEY RELIEF", 160, 40, 0.85f, PAL_NEON);
    else if (mode_ == Mode::Pause) text("PAUSED", 160, 40, 1.0f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("WATCH HELD", 160, 36, 0.9f, PAL_BELL);
    else if (mode_ == Mode::Over) text(reason_, 160, 36, 0.62f, PAL_HOOD);

    float playerX = 160.0f + px_ * LANE_X * FOCAL / Z_HIT + shx;
    float feet = 208.0f + shy;
    spr(art_.wall, 22 + shx, 118, 230, PAL_BRICK, false);
    spr(art_.wall, 298 + shx, 118, 230, PAL_BRICK, true);
    spr(art_.escape, 48 + shx, 100, 150, PAL_NIGHT, false);
    spr(art_.escape, 272 + shx, 100, 150, PAL_NIGHT, true);
    spr(art_.sign, 160 + shx, 28, 22, PAL_NEON, false);

    float swing = 0;
    if (bell_ || mode_ == Mode::Victory)
        swing = std::sin(t_ * (rope_ > 0.05f ? 14.0f : 7.0f)) * (rope_ > 0.05f ? 10.0f : 4.0f);
    spr(art_.bell, 160 + swing + shx, 46, 22, PAL_BELL, false);
    spr(art_.cord, 160 + swing * 0.2f + shx, 78, 56, PAL_WOOD, false);

    spr(art_.shadow, playerX, feet - 2, 10, PAL_BRICK, false);
    spr(art_.watch[swing_ > 0 ? 1 : 0], playerX, feet, 86, PAL_WATCH, false, 0, true);
    if (swing_ > 0) spr(art_.stick, playerX + 22, feet - 48, 10, PAL_WOOD, false);

    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        if (it->kind == 0) {
            const Foe& f = foes_[it->i];
            float sx, sy, sc;
            project(float(f.lane) * LANE_X, f.z, sx, sy, sc);
            sx += shx;
            sy += shy;
            int fog = fogAt(f.z);
            float body = f.kind == 2 ? 1.7f : f.kind == 1 ? 1.85f : 1.55f;
            float h = body * sc;
            if (f.flash > 0) spr(art_.steam, sx, sy - h * 0.5f, h * 0.4f, PAL_NEON, false, fog);
            if (f.kind == 2) spr(art_.barrel, sx, sy, h, PAL_WOOD, false, fog, true);
            else if (f.kind == 1) spr(art_.bruiser, sx, sy, h, PAL_HOOD, f.lane > 0, fog, true);
            else spr(art_.hood[int(f.age * 6.0f) & 1], sx, sy, h, PAL_HOOD, f.lane < 0, fog, true);
        } else if (it->kind == 1) {
            const Toss& b = tosses_[it->i];
            float sx, sy, sc;
            project(float(b.lane) * LANE_X, b.z, sx, sy, sc);
            spr(art_.lantern, sx + shx, sy - 1.0f * sc + shy, std::max(8.0f, 0.4f * sc), PAL_NEON, false, fogAt(b.z));
        } else if (it->kind == 2) {
            float sx, sy, sc;
            project(kLamp[it->i][0], kLamp[it->i][1], sx, sy, sc);
            float h = 1.5f * sc;
            int frame = int(t_ * 8.0f + it->i) & 1;
            spr(art_.pole, sx + shx, sy + shy, h, PAL_NIGHT, false, fogAt(kLamp[it->i][1]), true);
            spr(art_.lamp[frame], sx + shx, sy - h + shy, h * 0.35f, PAL_NEON, false, fogAt(kLamp[it->i][1]));
        } else {
            float z = it->z;
            float x = it->i == 0 ? -1.15f : 1.15f;
            float sx, sy, sc;
            project(x, z, sx, sy, sc);
            spr(art_.dumpster, sx + shx, sy + shy, 0.85f * sc, PAL_NIGHT, it->i == 1, fogAt(z), true);
            if ((int(t_ * 3.0f) + it->i) % 4 == 0)
                spr(art_.steam, sx + shx, sy - 0.7f * sc + shy, 0.35f * sc, PAL_NIGHT, false, fogAt(z));
        }
    }

    for (const Pop& p : pops_) {
        float sx, sy, sc;
        project(p.x, p.z, sx, sy, sc);
        char buf[16];
        std::snprintf(buf, sizeof buf, "+%d", p.pts);
        text(buf, sx, sy - sc * 1.1f - (0.65f - p.t) * 14.0f, 0.4f, PAL_NEON);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        if (int(t_ * 2.0f) % 2 == 0) hudC(20, "PRESS START", PAL_HUD);
        hudC(22, "HOLD THE ALLEY", PAL_NEON);
        hudC(24, "ARROWS SHIFT LANES", PAL_HUD);
        hudC(25, "C SWINGS OR THROWS", PAL_HUD);
        hudC(26, "UP ANSWERS THE BELL", PAL_BELL);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 1, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        hud(1, 1, "ALLEY", alley_ <= 2 ? PAL_HOOD : PAL_HUD);
        for (int i = 0; i < ALLEY_MAX; i++) {
            int pal = i < alley_ ? (alley_ <= 2 ? PAL_HOOD : PAL_BELL) : PAL_HUD;
            hud(7 + i, 1, i < alley_ ? "=" : "-", pal);
        }
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(39 - int(std::strlen(buf)), 1, buf, PAL_HUD);
        if (bell_) {
            hudC(22, std::fabs(px_) > 0.4f ? "GET UNDER THE BELL" : "HOLD UP ON THE CORD", PAL_BELL);
            int n = std::clamp(int(std::lround(rope_ / ROPE_NEED * 10.0f)), 0, 10);
            std::string meter = "BELL ";
            for (int i = 0; i < 10; i++) meter += i < n ? "=" : "-";
            hudC(24, meter, rope_ > 0 ? PAL_BELL : PAL_NEON);
        } else {
            hud(1, 26, "WATCH", PAL_HUD);
            int n = std::clamp(int(watch_ / BELL_AT * 10.0f), 0, 10);
            for (int i = 0; i < 10; i++) hud(8 + i, 26, i < n ? "=" : "-", i < n ? PAL_NEON : PAL_HUD);
        }
        if (mode_ == Mode::Pause) hudC(24, "START RESUMES", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(22, "RELIEF HAS THE ALLEY", PAL_BELL);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(24, buf, PAL_HUD);
        hudC(26, "START", PAL_HUD);
    } else if (mode_ == Mode::Over) {
        hudC(22, "THE WATCH IS OVER", PAL_HOOD);
        std::snprintf(buf, sizeof buf, "ALLEY %d   SCORE %d", alley_, score_);
        hudC(24, buf, PAL_HUD);
        hudC(26, "START TRIES AGAIN", PAL_HUD);
    }

    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (droneOn_) {
        sys_->apu.setFreq(0, 52.0f + dawn * 12.0f);
        sys_->apu.setVol(0, mode_ == Mode::Watch ? 0.08f : 0.05f);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {392.0f, 494.0f, 587.0f, 784.0f};
        fanT_ += DT;
        if (fanT_ > 0.16f) {
            if (fanStep_ < 4) sys_->apu.keyOn(2, notes[fanStep_], 0.18f);
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
    sys.apu.setEcho(0.22f, 0.28f, 0.16f);
    sys.apu.setPatch(0, dronePatch());
    sys.apu.setPatch(1, bellPatch());
    sys.apu.setPatch(2, hornPatch());
    sys.apu.keyOn(0, 52.0f, 0.06f);
    droneOn_ = true;
    if (bot_) beginWatch();
    else {
        mode_ = Mode::Title;
        sys.setLight(50, 20, 90);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        px_ = std::sin(t_ * 0.4f) * 0.04f;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            tone(620.0f, 0.06f);
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
            tosses_.clear();
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

}  // namespace alley
