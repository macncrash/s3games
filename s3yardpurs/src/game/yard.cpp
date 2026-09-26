#include "game/yard.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace ypurs {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FOCAL = 210.f;
constexpr float HORIZON = 76.f;
constexpr float GROUND = 2.2f;
constexpr float LANE_X = 1.12f;
constexpr float Z_SPAWN = 26.f;
constexpr float Z_RAM_HI = 13.2f;
constexpr float Z_HIT = 6.5f;
constexpr float MOVE = 3.6f;
constexpr float RAM_CD = 0.26f;

constexpr int MULE = 0;
constexpr int WELD = 1;
constexpr int CRUSH = 2;
constexpr int BALE = 3;

struct Row {
    float t;
    int kind;
    int lane;
};

// Window opens after (Z_SPAWN - Z_RAM_HI) / speed. Gaps leave a lane change.
const Row kRows[] = {
    {0.00f, MULE, -1}, {3.34f, WELD, 1}, {3.71f, CRUSH, 0},
    {7.38f, BALE, -1}, {11.20f, MULE, 1}, {14.54f, WELD, 0},
};

struct Prop {
    float x, z, h;
    int kind;
};

const Prop kProps[] = {
    {-2.55f, 9.4f, 1.8f, 0}, {2.65f, 10.6f, 2.0f, 0}, {-2.70f, 15.6f, 1.15f, 1},
    {2.55f, 17.0f, 1.2f, 1}, {-2.85f, 12.4f, 2.5f, 2}, {2.90f, 18.6f, 2.6f, 2},
    {-2.45f, 20.8f, 2.7f, 3}, {2.40f, 13.2f, 1.25f, 5}, {-2.35f, 22.4f, 1.25f, 5},
    {0.f, 30.f, 5.0f, 4},
};

struct Star {
    float x, y;
};

const Star kStars[] = {{28, 18}, {74, 34}, {118, 14}, {188, 26}, {246, 16}, {300, 38}, {156, 22}, {48, 46}};

float speedOf(int kind) {
    if (kind == WELD) return 3.70f;
    if (kind == CRUSH) return 2.25f;
    if (kind == BALE) return 2.55f;
    return 3.05f;
}

int hpOf(int kind) { return (kind == CRUSH || kind == BALE) ? 2 : 1; }

int ptsOf(int kind) {
    if (kind == WELD) return 180;
    if (kind == CRUSH) return 340;
    if (kind == BALE) return 300;
    return 140;
}

float bodyH(int kind) {
    if (kind == WELD) return 1.70f;
    if (kind == CRUSH) return 2.15f;
    if (kind == BALE) return 2.00f;
    return 1.90f;
}

int palOf(int kind) {
    if (kind == WELD) return PAL_WELD;
    if (kind == CRUSH) return PAL_CRUSH;
    if (kind == BALE) return PAL_BALE;
    return PAL_MULE;
}

const char* whoOf(int kind) {
    if (kind == WELD) return "WELDER";
    if (kind == CRUSH) return "CRUSHER";
    if (kind == BALE) return "BALER";
    return "MULE";
}

const char* sideOf(int lane) {
    if (lane < 0) return "LEFT";
    if (lane > 0) return "RIGHT";
    return "CENTER";
}

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

int fogFor(float z) { return std::clamp(int((z - 10.f) * 0.55f), 0, 13); }

gs::FMPatch dieselPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.55f;
    p.op[0] = {0.5f, 0.9f, 0.08f, 0.40f, 0.85f, 0.35f};
    p.op[1] = {1.f, 0.45f, 0.05f, 0.50f, 0.70f, 0.30f, 2.5f};
    p.op[2] = {2.f, 0.18f, 0.04f, 0.35f, 0.40f, 0.25f};
    p.op[3] = {0.25f, 0.35f, 0.10f, 0.60f, 0.80f, 0.40f};
    p.vol = 0.15f;
    p.drive = 0.62f;
    p.tone = 640.f;
    p.vibRate = 5.5f;
    p.vibDepth = 0.018f;
    return p;
}

gs::FMPatch metalPatch() {
    gs::FMPatch p;
    p.alg = 2;
    p.fb = 0.45f;
    p.op[0] = {2.f, 1.f, 0.004f, 0.09f, 0.15f, 0.14f};
    p.op[1] = {3.5f, 0.45f, 0.004f, 0.08f, 0.10f, 0.12f};
    p.op[2] = {5.2f, 0.25f, 0.005f, 0.07f, 0.08f, 0.10f};
    p.op[3] = {1.f, 0.35f, 0.004f, 0.12f, 0.20f, 0.16f};
    p.vol = 0.20f;
    p.drive = 0.30f;
    p.tone = 1800.f;
    return p;
}

gs::FMPatch brassPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.20f;
    p.op[0] = {1.f, 1.f, 0.015f, 0.16f, 0.75f, 0.18f};
    p.op[1] = {2.f, 0.40f, 0.015f, 0.18f, 0.55f, 0.16f};
    p.op[2] = {3.f, 0.22f, 0.020f, 0.20f, 0.40f, 0.16f};
    p.op[3] = {1.f, 0.28f, 0.015f, 0.18f, 0.60f, 0.18f};
    p.vol = 0.20f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (over_ || mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (mode_ != Mode::Watch && mode_ != Mode::Pause) return 0;
    if (fleet_ > 0 && others() <= 1 && stalled_ > 0) return 2;
    return 1;
}

int Game::nearestLane() const {
    int lane = int(std::lround(px_));
    if (lane < -1 || lane > 1) return 99;
    if (std::fabs(px_ - float(lane)) > 0.42f) return 99;
    return lane;
}

const gs::Mipped& Game::bodyOf(int kind) const {
    if (kind == WELD) return art_.welder;
    if (kind == CRUSH) return art_.crusher;
    if (kind == BALE) return art_.baler;
    return art_.mule;
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = 0.07f;
}

void Game::project(float worldX, float z, float& sx, float& sy, float& s) const {
    float zz = std::max(0.85f, z);
    s = FOCAL / zz;
    sx = 160.f + worldX * s;
    sy = HORIZON + GROUND * s;
}

void Game::hurt(Mach& m) {
    m.hp -= 1;
    m.flash = 0.12f;
    blip(m.hp > 0 ? 240.f : 520.f, 0.05f);
    if (m.hp > 0) {
        sys_->apu.noiseBurst(0.16f, 900.f, 0.05f);
        sys_->rumble(0.2f, 0.3f, 40);
        return;
    }
    m.alive = false;
    stalled_ += 1;
    score_ += m.points;
    Puff puff;
    puff.x = float(m.lane) * LANE_X;
    puff.z = m.z;
    puff.t = 0.40f;
    puff.kind = 0;
    puffs_.push_back(puff);
    Puff spark = puff;
    spark.t = 0.22f;
    spark.kind = 1;
    puffs_.push_back(spark);
    Pop pop;
    pop.x = puff.x;
    pop.z = m.z;
    pop.t = 0.65f;
    pop.pts = m.points;
    pops_.push_back(pop);
    if (puffs_.size() > 16) puffs_.erase(puffs_.begin());
    if (pops_.size() > 6) pops_.erase(pops_.begin());
    static const float notes[] = {196.f, 233.f, 277.f, 330.f, 392.f, 466.f};
    int n = std::clamp(stalled_ - 1, 0, 5);
    sys_->apu.keyOn(1, notes[n], 0.20f);
    sys_->apu.noiseBurst(0.24f, 1400.f, 0.08f);
    sys_->rumble(0.35f, 0.55f, 70);
    shake_ = std::max(shake_, 0.18f);
}

void Game::doRam() {
    if (ramCd_ > 0.f || mode_ != Mode::Watch || won_) return;
    lunge_ = 0.16f;
    ramCd_ = RAM_CD;
    int lane = nearestLane();
    int hit = -1;
    float best = 1.0e9f;
    if (lane >= -1 && lane <= 1) {
        for (int i = 0; i < int(machs_.size()); ++i) {
            const Mach& m = machs_[size_t(i)];
            if (!m.alive || m.lane != lane || m.z <= Z_HIT || m.z > Z_RAM_HI) continue;
            if (m.z < best) {
                best = m.z;
                hit = i;
            }
        }
    }
    if (hit < 0) {
        blip(120.f, 0.03f);
        sys_->apu.noiseBurst(0.06f, 320.f, 0.03f);
        return;
    }
    hurt(machs_[size_t(hit)]);
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE LAST MACHINE STILL RUNNING";
    score_ += 500;
    fanStep_ = 0;
    fanT_ = 0;
    shake_ = 0.12f;
    sys_->apu.noiseBurst(0.10f, 1600.f, 0.05f);
    sys_->rumble(0.25f, 0.55f, 180);
    sys_->setLight(40, 170, 70);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over || mode_ == Mode::Victory) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    engineOn_ = false;
    shake_ = 0.7f;
    sys_->apu.keyOff(0);
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.50f, 140.f, 0.32f);
    blip(70.f, 0.08f);
    sys_->rumble(0.85f, 0.4f, 240);
    sys_->setLight(180, 30, 20);
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    wasHot_ = false;
    reason_ = "THE WATCH IS OVER";
    score_ = 0;
    stalled_ = 0;
    spawnAt_ = 0;
    fanStep_ = -1;
    px_ = 0;
    watch_ = 0;
    endT_ = 0;
    lunge_ = ramCd_ = shake_ = beep_ = fanT_ = 0;
    machs_.clear();
    puffs_.clear();
    pops_.clear();
    script_.clear();
    for (const Row& r : kRows) script_.push_back({r.t, r.kind, r.lane});
    fleet_ = int(script_.size());
    if (!sys_) return;
    sys_->apu.keyOff(1);
    sys_->apu.keyOff(2);
    sys_->apu.keyOn(0, 42.f, 0.06f);
    engineOn_ = true;
    sys_->setLight(140, 90, 30);
}

void Game::update(float dt) {
    watch_ += dt;
    while (spawnAt_ < int(script_.size()) && script_[size_t(spawnAt_)].t <= watch_) {
        const Spawn& s = script_[size_t(spawnAt_)];
        ++spawnAt_;
        Mach m;
        m.kind = s.kind;
        m.lane = s.lane;
        m.hp = hpOf(s.kind);
        m.points = ptsOf(s.kind);
        m.z = Z_SPAWN;
        m.alive = true;
        machs_.push_back(m);
    }

    int best = -1;
    float bestZ = 1.0e9f;
    for (int i = 0; i < int(machs_.size()); ++i) {
        const Mach& m = machs_[size_t(i)];
        if (!m.alive) continue;
        if (m.z < bestZ) {
            bestZ = m.z;
            best = i;
        }
    }

    bool wantRam = false;
    if (bot_) {
        if (best >= 0) {
            float dest = float(machs_[size_t(best)].lane);
            float step = MOVE * dt;
            if (std::fabs(px_ - dest) <= step) px_ = dest;
            else px_ += (dest > px_ ? step : -step);
        }
    } else {
        const gs::Pad& pad = sys_->pad;
        float digital = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        float axis = std::fabs(pad.axisX) > 0.18f ? pad.axisX : digital;
        px_ += std::clamp(axis, -1.f, 1.f) * MOVE * dt;
        wantRam = pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) ||
                  pad.down(gs::BTN_X) || pad.down(gs::BTN_Z) || pad.down(gs::BTN_TURBO) || pad.accel > 0.45f;
    }
    px_ = std::clamp(px_, -1.f, 1.f);

    if (bot_ && best >= 0) {
        const Mach& m = machs_[size_t(best)];
        if (std::fabs(px_ - float(m.lane)) <= 0.40f && m.z <= Z_RAM_HI && m.z > Z_HIT) wantRam = true;
    }
    if (wantRam) doRam();

    bool hot = false;
    for (Mach& m : machs_) {
        if (!m.alive) continue;
        m.age += dt;
        if (m.flash > 0.f) m.flash -= dt;
        m.z -= speedOf(m.kind) * dt;
        if (m.z <= Z_RAM_HI && m.z > Z_HIT) hot = true;
        if (m.z > Z_HIT) continue;
        m.alive = false;
        Puff puff;
        puff.x = float(m.lane) * LANE_X;
        puff.z = Z_HIT;
        puff.t = 0.50f;
        puff.kind = 1;
        puffs_.push_back(puff);
        loseWatch("A MACHINE GOT THROUGH");
        break;
    }

    machs_.erase(std::remove_if(machs_.begin(), machs_.end(), [](const Mach& m) { return !m.alive; }), machs_.end());

    if (hot && !wasHot_) blip(740.f, 0.035f);
    wasHot_ = hot && mode_ == Mode::Watch;

    if (mode_ == Mode::Watch && spawnAt_ == int(script_.size())) {
        bool any = false;
        for (const Mach& m : machs_)
            if (m.alive) any = true;
        if (!any && stalled_ >= fleet_ && fleet_ > 0) winWatch();
    }
}

void Game::tickFx(float dt) {
    if (lunge_ > 0.f) lunge_ = std::max(0.f, lunge_ - dt);
    if (ramCd_ > 0.f) ramCd_ = std::max(0.f, ramCd_ - dt);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 1.5f);
    for (Puff& p : puffs_) p.t -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0.f; }), puffs_.end());
    for (Pop& p : pops_) p.t -= dt;
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0.f; }), pops_.end());
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
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
    sys_->vdp.sprite(s);
}

void Game::shadow(float cx, float cy, float w) {
    if (w < 4.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 4, 400));
    s.h = int16_t(std::max(3, int(std::lround(double(w) * 0.22))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = art_.shadow.pick(float(s.h));
    s.pal = 0;
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) width += 12.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    if (width > 304.f) scale *= 304.f / width;
    width = 0.f;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) width += 12.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) {
            x += 12.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false);
        x += gw;
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::layRoad(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = mode_ == Mode::Over ? gs::rgb4(4, 1, 1) : gs::rgb4(1, 1, 3);
    uint16_t skyHor = mode_ == Mode::Over ? gs::rgb4(9, 3, 2)
                      : mode_ == Mode::Victory ? gs::rgb4(11, 7, 3)
                                               : gs::rgb4(7, 4, 3);
    uint16_t fogC = mode_ == Mode::Over ? gs::rgb4(6, 2, 1) : gs::rgb4(4, 3, 3);
    v.setFogColor(fogC);
    const int horizon = int(HORIZON);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& rd = v.road[y];
        if (y < horizon) {
            v.lineBackdrop[y] = mix(skyTop, skyHor, float(y) / float(horizon));
            v.lineFog[y] = 0;
            rd.on = false;
            continue;
        }
        float row = float(y - horizon) + 1.f;
        rd.on = true;
        rd.cx = 160.f + shx;
        rd.hw = row * 1.28f;
        rd.v = 900.f / row + t_ * 14.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = gs::ROAD_RUTS;
        rd.band = (int(std::floor(rd.v / 48.f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fog = std::clamp(1.f - row / 92.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fog * (mode_ == Mode::Over ? 8.f : 11.f));
        v.lineBackdrop[y] = gs::rgb4(2, 2, 1);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;

    float shx = shake_ > 0.f ? std::sin(t_ * 90.f) * 5.f * shake_ : 0.f;
    float shy = shake_ > 0.f ? std::cos(t_ * 70.f) * 3.f * shake_ : 0.f;
    layRoad(shx * 0.35f);

    if (mode_ == Mode::Title) text("YARD PURSE", 160.f + shx, 64.f, 1.05f, PAL_YOU);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f, 86.f, 1.0f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("LAST", 160.f + shx, 62.f, 1.2f, PAL_PURSE);
    else if (mode_ == Mode::Over) text("OVER", 160.f + shx, 62.f, 1.15f, PAL_CRUSH);

    struct Item {
        float z;
        int kind;
        int i;
    };
    std::vector<Item> items;
    auto add = [&](float z, int kind, int i) { items.push_back({z, kind, i}); };

    for (int i = 0; i < int(machs_.size()); ++i)
        if (machs_[size_t(i)].alive) add(machs_[size_t(i)].z, 0, i);
    for (int i = 0; i < int(sizeof kProps / sizeof kProps[0]); ++i) add(kProps[i].z, 1, i);
    for (int i = 0; i < int(puffs_.size()); ++i) add(puffs_[size_t(i)].z, 2, i);

    if (mode_ == Mode::Title) {
        add(16.f, 3, MULE);
        add(13.f, 3, WELD);
        add(18.5f, 3, CRUSH);
        add(21.f, 3, BALE);
    }

    float purseZ = 22.f;
    float purseX = 0.f;
    if (mode_ == Mode::Title) purseZ = 11.f + std::sin(t_ * 1.3f) * 0.25f;
    if (mode_ == Mode::Victory) {
        float u = std::clamp(endT_ / 0.85f, 0.f, 1.f);
        purseZ = 22.f + (4.8f - 22.f) * u;
        purseX = px_ * LANE_X * u;
    }
    add(purseZ, 4, 0);

    add(8.6f, 5, 0);
    add(4.2f, 6, 0);

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    for (const Item& it : items) {
        if (it.kind == 0) {
            const Mach& m = machs_[size_t(it.i)];
            float sx, sy, s;
            project(float(m.lane) * LANE_X, m.z, sx, sy, s);
            sx += shx;
            sy += shy + std::sin(m.age * 7.f) * 1.2f;
            float h = bodyH(m.kind) * s;
            int pal = m.flash > 0.f ? PAL_SHOCK : palOf(m.kind);
            shadow(sx, sy, h * 0.85f);
            spr(bodyOf(m.kind), sx, sy, h, pal, m.lane > 0, fogFor(m.z), true);
            if (m.kind == WELD && (int(t_ * 14.f) & 1))
                spr(art_.spark, sx + h * 0.28f, sy - h * 0.72f, h * 0.28f, PAL_SHOCK, false, fogFor(m.z));
        } else if (it.kind == 1) {
            const Prop& pr = kProps[it.i];
            float sx, sy, s;
            project(pr.x, pr.z, sx, sy, s);
            sx += shx;
            sy += shy;
            float h = std::clamp(pr.h * s, 6.f, 150.f);
            int fog = fogFor(pr.z);
            if (pr.kind == 2) {
                spr(art_.post, sx, sy, h, PAL_PROP, false, fog, true);
                spr(art_.bulb, sx, sy - h * 0.92f, h * 0.28f, PAL_LAMP, false, 0, false);
            } else if (pr.kind == 4) {
                spr(art_.crane, sx, sy, h, PAL_PROP, false, fog, true);
            } else {
                const gs::Mipped* img = &art_.pile;
                if (pr.kind == 1) img = &art_.drum;
                else if (pr.kind == 3) img = &art_.shack;
                else if (pr.kind == 5) img = &art_.fence;
                shadow(sx, sy, h * 0.7f);
                spr(*img, sx, sy, h, PAL_PROP, pr.x > 0.f, fog, true);
            }
        } else if (it.kind == 2) {
            const Puff& p = puffs_[size_t(it.i)];
            float sx, sy, s;
            project(p.x, p.z, sx, sy, s);
            float rise = (0.45f - p.t) * 18.f;
            float h = (p.kind ? 0.55f : 0.8f) * s;
            spr(p.kind ? art_.spark : art_.puff, sx + shx, sy - rise + shy, std::max(8.f, h),
                p.kind ? PAL_SHOCK : PAL_FX, false, fogFor(p.z));
        } else if (it.kind == 3) {
            int kind = it.i;
            int lane = kind == WELD ? 1 : kind == BALE ? -1 : 0;
            float z = it.z;
            float sx, sy, s;
            project(float(lane) * LANE_X, z, sx, sy, s);
            float bob = std::sin(t_ * 2.f + z) * 2.f;
            spr(bodyOf(kind), sx + shx, sy + shy + bob, bodyH(kind) * s, palOf(kind), lane > 0, fogFor(z), true);
        } else if (it.kind == 4) {
            float sx, sy, s;
            project(purseX, purseZ, sx, sy, s);
            float h = std::clamp(1.15f * s, 12.f, 48.f);
            shadow(sx + shx, sy + shy, h * 0.7f);
            spr(art_.purse, sx + shx, sy + shy, h, PAL_PURSE, false, fogFor(purseZ), true);
        } else if (it.kind == 5) {
            float sx, sy, s;
            project(px_ * LANE_X, 8.6f, sx, sy, s);
            int pal = PAL_YOU;
            const Mach* near = nullptr;
            for (const Mach& m : machs_) {
                if (!m.alive) continue;
                if (!near || m.z < near->z) near = &m;
            }
            if (near && near->z <= Z_RAM_HI) pal = nearestLane() == near->lane ? PAL_BALE : PAL_CRUSH;
            spr(art_.chev, sx + shx, sy + shy, std::max(8.f, 0.42f * s), pal, false, 0, true);
        } else if (it.kind == 6) {
            float bob = std::sin(t_ * 8.f) * 1.5f;
            float lift = lunge_ > 0.f ? std::sin((0.16f - lunge_) / 0.16f * 3.14159f) * 18.f : 0.f;
            float x = 160.f + px_ * LANE_X * (FOCAL / 5.0f) + shx;
            float feet = 206.f + shy + bob - lift;
            float h = 86.f;
            shadow(x, feet - 4.f, 70.f);
            spr(art_.loader, x, feet, h, mode_ == Mode::Over ? PAL_PROP : PAL_YOU, false, 0, true);
            if (engineOn_) {
                float puffY = feet - 62.f + std::sin(t_ * 10.f) * 3.f;
                spr(art_.puff, x - 18.f, puffY, 16.f, PAL_FX, false);
                if ((int(t_ * 8.f) & 1) == 0) spr(art_.spark, x - 18.f, puffY - 8.f, 8.f, PAL_LAMP, false);
            }
            if (lunge_ > 0.f) spr(art_.spark, x, feet - 96.f, 18.f + lunge_ * 80.f, PAL_SHOCK, false);
        }
    }

    for (int i = 0; i < int(pops_.size()); ++i) {
        const Pop& p = pops_[size_t(i)];
        float sx, sy, s;
        project(p.x, p.z, sx, sy, s);
        char buf[16];
        std::snprintf(buf, sizeof buf, "+%d", p.pts);
        text(buf, sx + shx, sy - (0.65f - p.t) * 20.f, 0.45f, PAL_PURSE);
    }

    for (int i = 0; i < int(sizeof kStars / sizeof kStars[0]); ++i) {
        if (((int(t_ * 3.f) + i) % 5) == 0) continue;
        spr(art_.star, kStars[i].x, kStars[i].y, 6.f + float(i & 1), PAL_SHOCK, false);
    }
    spr(art_.moon, 262.f, 36.f, 20.f, PAL_FX, false);

    char buf[80];
    if (mode_ == Mode::Title) {
        hudC(16, "BE THE LAST MACHINE STILL RUNNING", PAL_PURSE);
        hudC(18, "STALL EVERY OTHER MACHINE", PAL_HUD);
        hudC(21, "ARROWS STEER THE LOADER", PAL_HUD);
        hudC(22, "UP OR C RAMS", PAL_LAMP);
        if ((int(t_ * 2.f) & 1) == 0) hudC(25, "PRESS START", PAL_LAMP);
        std::string ver = S3_VERSION_STRING;
        hud(39 - int(ver.size()), 1, ver, PAL_HUD);
    } else if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        std::snprintf(buf, sizeof buf, "OTHERS %d", others());
        hud(1, 1, buf, others() <= 1 ? PAL_PURSE : PAL_HUD);
        int rpm = engineOn_ ? 620 + int(std::sin(t_ * 9.f) * 18.f) + int(lunge_ * 900.f) : 0;
        std::snprintf(buf, sizeof buf, "RPM %d", rpm);
        hud(39 - int(std::strlen(buf)), 1, buf, engineOn_ ? PAL_YOU : PAL_CRUSH);
        hud(1, 2, engineOn_ ? "YOU RUNNING" : "YOU SEIZED", engineOn_ ? PAL_BALE : PAL_CRUSH);

        const Mach* near = nullptr;
        for (const Mach& m : machs_) {
            if (!m.alive) continue;
            if (!near || m.z < near->z) near = &m;
        }
        if (near) {
            std::string line = std::string(sideOf(near->lane)) + "  " + whoOf(near->kind);
            if (near->hp > 1) {
                line += "  ";
                for (int i = 0; i < near->hp; ++i) line += "O";
            }
            hudC(21, line, palOf(near->kind));
            bool aligned = nearestLane() == near->lane;
            if (near->z <= Z_RAM_HI) hudC(23, aligned ? "RAM NOW" : "GET IN LANE", aligned ? PAL_BALE : PAL_CRUSH);
            else if (stalled_ == 0) hudC(23, "MATCH ITS LANE AND RAM", PAL_HUD);
        } else if (spawnAt_ < fleet_) {
            hudC(21, "THE NEXT MACHINE IS OUT", PAL_HUD);
        } else {
            hudC(21, "THE YARD IS QUIET", PAL_BALE);
        }
        std::snprintf(buf, sizeof buf, "STALLED %d", stalled_);
        hud(1, 26, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(39 - int(std::strlen(buf)), 26, buf, PAL_HUD);
        if (mode_ == Mode::Pause) {
            hudC(24, "START RESUMES", PAL_HUD);
            hudC(25, "ESC LEAVES THE WATCH", PAL_HUD);
        }
    } else if (mode_ == Mode::Victory) {
        hudC(18, "THE LAST MACHINE STILL RUNNING", PAL_PURSE);
        hudC(20, "THE PURSE IS YOURS", PAL_YOU);
        std::snprintf(buf, sizeof buf, "STALLED %d   SCORE %d", stalled_, score_);
        hudC(22, buf, PAL_HUD);
        hudC(25, "START", PAL_HUD);
    } else if (mode_ == Mode::Over) {
        hudC(18, "THE WATCH IS OVER", PAL_CRUSH);
        hudC(20, reason_, PAL_HUD);
        std::snprintf(buf, sizeof buf, "STALLED %d   SCORE %d", stalled_, score_);
        hudC(22, buf, PAL_HUD);
        hudC(25, "START TRIES AGAIN", PAL_HUD);
    }
}

void Game::serviceAudio() {
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (engineOn_) {
        float pitch = 40.f + std::min(watch_, 24.f) * 0.35f + (lunge_ > 0.f ? 18.f : 0.f);
        if (mode_ == Mode::Victory) pitch += 8.f;
        sys_->apu.setFreq(0, pitch);
        float vol = mode_ == Mode::Watch ? 0.07f : 0.045f;
        if (lunge_ > 0.f) vol += 0.03f;
        sys_->apu.setVol(0, vol);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {262.f, 330.f, 392.f, 523.f};
        fanT_ += DT;
        if (fanT_ > 0.16f) {
            if (fanStep_ < 4) sys_->apu.keyOn(2, notes[fanStep_], 0.18f);
            else sys_->apu.keyOff(2);
            ++fanStep_;
            fanT_ = 0;
            if (fanStep_ > 8) fanStep_ = -1;
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.16f, 0.28f, 0.18f);
    sys.apu.setPatch(0, dieselPatch());
    sys.apu.setPatch(1, metalPatch());
    sys.apu.setPatch(2, brassPatch());
    if (bot_) beginWatch();
    else {
        mode_ = Mode::Title;
        sys.apu.keyOn(0, 38.f, 0.04f);
        engineOn_ = true;
        sys.setLight(120, 80, 30);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        px_ = std::sin(t_ * 0.7f) * 0.72f;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            blip(660.f, 0.06f);
            beginWatch();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Watch) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            machs_.clear();
            stalled_ = 0;
        } else update(DT);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Watch;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            machs_.clear();
        }
    } else {
        endT_ += DT;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            if (mode_ == Mode::Over) beginWatch();
            else {
                mode_ = Mode::Title;
                over_ = false;
                won_ = false;
                machs_.clear();
            }
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            machs_.clear();
        }
    }

    tickFx(DT);
    draw();
    serviceAudio();
}

}  // namespace ypurs
