#include "game/granary.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace gpurs {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FOCAL = 200.f;
constexpr float HORIZON = 68.f;
constexpr float GROUND = 2.05f;
constexpr float LANE_X = 0.92f;
constexpr float Z_SPAWN = 24.f;
constexpr float Z_RAM_HI = 12.4f;
constexpr float Z_HIT = 6.2f;
constexpr float MOVE = 4.2f;
constexpr float RAM_CD = 0.24f;

constexpr int AUGER = 0;
constexpr int TIPPER = 1;
constexpr int THRESH = 2;

struct Row {
    float t;
    int kind;
    int lane;
};

const Row kRows[] = {
    {0.00f, AUGER, -1}, {5.40f, TIPPER, 1}, {11.00f, THRESH, 0},
    {17.20f, AUGER, 1},  {22.80f, TIPPER, -1}, {28.60f, THRESH, 0},
};

struct Prop {
    float x, z, h;
    int kind;
};

const Prop kProps[] = {
    {-2.20f, 8.4f, 3.2f, 0}, {2.20f, 9.2f, 3.4f, 0},
    {-2.30f, 14.0f, 3.0f, 0}, {2.25f, 15.2f, 3.3f, 0},
    {-2.05f, 11.0f, 0.7f, 1}, {2.10f, 16.6f, 0.7f, 1},
    {-2.15f, 18.8f, 1.5f, 2}, {2.12f, 12.4f, 1.5f, 2},
};

float speedOf(int kind) {
    if (kind == TIPPER) return 2.50f;
    if (kind == THRESH) return 1.70f;
    return 2.20f;
}

int hpOf(int kind) { return kind == THRESH ? 2 : 1; }

int ptsOf(int kind) {
    if (kind == TIPPER) return 220;
    if (kind == THRESH) return 380;
    return 160;
}

float bodyH(int kind) {
    if (kind == TIPPER) return 1.70f;
    if (kind == THRESH) return 2.10f;
    return 1.60f;
}

int palOf(int kind) {
    if (kind == TIPPER) return PAL_TIP;
    if (kind == THRESH) return PAL_THRESH;
    return PAL_AUGER;
}

const char* whoOf(int kind) {
    if (kind == TIPPER) return "GRAIN TIPPER";
    if (kind == THRESH) return "THRESHER";
    return "AUGER";
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

int fogFor(float z) { return std::clamp(int((z - 9.f) * 0.6f), 0, 13); }

gs::FMPatch millPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.35f;
    p.op[0] = {0.5f, 0.75f, 0.08f, 0.40f, 0.80f, 0.30f};
    p.op[1] = {1.f, 0.30f, 0.05f, 0.45f, 0.55f, 0.22f, 2.0f};
    p.op[2] = {2.f, 0.12f, 0.04f, 0.28f, 0.32f, 0.18f};
    p.op[3] = {0.25f, 0.28f, 0.10f, 0.48f, 0.65f, 0.32f};
    p.vol = 0.13f;
    p.drive = 0.40f;
    p.tone = 480.f;
    p.vibRate = 5.f;
    p.vibDepth = 0.010f;
    return p;
}

gs::FMPatch metalPatch() {
    gs::FMPatch p;
    p.alg = 2;
    p.fb = 0.38f;
    p.op[0] = {2.f, 1.f, 0.004f, 0.09f, 0.14f, 0.12f};
    p.op[1] = {3.2f, 0.40f, 0.004f, 0.08f, 0.10f, 0.10f};
    p.op[2] = {5.f, 0.22f, 0.005f, 0.07f, 0.08f, 0.10f};
    p.op[3] = {1.f, 0.30f, 0.004f, 0.12f, 0.18f, 0.14f};
    p.vol = 0.18f;
    p.drive = 0.22f;
    p.tone = 1500.f;
    return p;
}

gs::FMPatch brassPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.16f;
    p.op[0] = {1.f, 1.f, 0.015f, 0.16f, 0.70f, 0.16f};
    p.op[1] = {2.f, 0.36f, 0.015f, 0.16f, 0.48f, 0.14f};
    p.op[2] = {3.f, 0.18f, 0.020f, 0.18f, 0.34f, 0.14f};
    p.op[3] = {1.f, 0.24f, 0.015f, 0.16f, 0.52f, 0.16f};
    p.vol = 0.17f;
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
    if (std::fabs(px_ - float(lane)) > 0.38f) return 99;
    return lane;
}

const gs::Mipped& Game::bodyOf(int kind) const {
    if (kind == TIPPER) return art_.tipper;
    if (kind == THRESH) return art_.thresher;
    return art_.auger;
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
    blip(m.hp > 0 ? 210.f : 460.f, 0.05f);
    if (m.hp > 0) {
        sys_->apu.noiseBurst(0.14f, 760.f, 0.05f);
        sys_->rumble(0.2f, 0.28f, 40);
        return;
    }
    m.alive = false;
    stalled_ += 1;
    score_ += m.points;
    Puff puff;
    puff.x = float(m.lane) * LANE_X;
    puff.z = m.z;
    puff.t = 0.42f;
    puff.kind = 0;
    puffs_.push_back(puff);
    Puff spark = puff;
    spark.t = 0.22f;
    spark.kind = 1;
    puffs_.push_back(spark);
    if (puffs_.size() > 16) puffs_.erase(puffs_.begin());
    static const float notes[] = {196.f, 247.f, 294.f, 349.f, 392.f, 494.f};
    int n = std::clamp(stalled_ - 1, 0, 5);
    sys_->apu.keyOn(1, notes[n], 0.18f);
    sys_->apu.noiseBurst(0.22f, 1100.f, 0.07f);
    sys_->rumble(0.3f, 0.5f, 60);
    shake_ = std::max(shake_, 0.16f);
}

void Game::doRam() {
    if (ramCd_ > 0.f || mode_ != Mode::Watch || won_) return;
    lunge_ = 0.15f;
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
        grit_ -= 1;
        shake_ = std::max(shake_, 0.25f);
        blip(88.f, 0.04f);
        sys_->apu.noiseBurst(0.10f, 220.f, 0.05f);
        if (grit_ <= 0) loseWatch("YOU STALLED YOURSELF");
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
    shake_ = 0.10f;
    sys_->apu.noiseBurst(0.10f, 1400.f, 0.05f);
    sys_->rumble(0.22f, 0.5f, 160);
    sys_->setLight(90, 140, 40);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over || mode_ == Mode::Victory) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    engineOn_ = false;
    shake_ = 0.65f;
    sys_->apu.keyOff(0);
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.48f, 110.f, 0.30f);
    blip(60.f, 0.08f);
    sys_->rumble(0.8f, 0.35f, 220);
    sys_->setLight(150, 40, 16);
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
    grit_ = 3;
    px_ = 0;
    watch_ = 0;
    endT_ = 0;
    lunge_ = ramCd_ = shake_ = beep_ = fanT_ = wallT_ = doorOpen_ = 0;
    machs_.clear();
    puffs_.clear();
    script_.clear();
    for (const Row& r : kRows) script_.push_back({r.t, r.kind, r.lane});
    fleet_ = int(script_.size());
    if (!sys_) return;
    sys_->apu.keyOff(1);
    sys_->apu.keyOff(2);
    sys_->apu.keyOn(0, 46.f, 0.055f);
    engineOn_ = true;
    sys_->setLight(140, 100, 30);
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
    float steer = 0.f;
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
        steer = std::clamp(axis, -1.f, 1.f);
        px_ += steer * MOVE * dt;
        wantRam = pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) ||
                  pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_X) || pad.pressed(gs::BTN_Z);
    }
    float before = px_;
    px_ = std::clamp(px_, -1.05f, 1.05f);
    bool scraped = std::fabs(before) > 1.05f && std::fabs(steer) > 0.4f;
    if (scraped) wallT_ += dt;
    else wallT_ = std::max(0.f, wallT_ - dt);
    if (wallT_ > 0.45f && mode_ == Mode::Watch) {
        grit_ -= 1;
        wallT_ = 0;
        shake_ = 0.4f;
        blip(70.f, 0.05f);
        if (grit_ <= 0) {
            loseWatch("YOU STALLED YOURSELF");
            return;
        }
    }

    if (bot_ && best >= 0) {
        const Mach& m = machs_[size_t(best)];
        if (std::fabs(px_ - float(m.lane)) <= 0.28f && m.z <= Z_RAM_HI && m.z > Z_HIT + 0.4f) wantRam = true;
    }
    if (wantRam && mode_ == Mode::Watch) doRam();
    if (mode_ != Mode::Watch) return;

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
        puff.t = 0.45f;
        puff.kind = 1;
        puffs_.push_back(puff);
        loseWatch("A MACHINE GOT THROUGH");
        break;
    }

    machs_.erase(std::remove_if(machs_.begin(), machs_.end(), [](const Mach& m) { return !m.alive; }), machs_.end());

    if (hot && !wasHot_) blip(680.f, 0.03f);
    wasHot_ = hot && mode_ == Mode::Watch;

    if (mode_ == Mode::Watch && spawnAt_ == int(script_.size())) {
        bool any = false;
        for (const Mach& m : machs_)
            if (m.alive) any = true;
        if (!any && stalled_ >= fleet_ && fleet_ > 0 && grit_ > 0) winWatch();
    }
}

void Game::tickFx(float dt) {
    if (lunge_ > 0.f) lunge_ = std::max(0.f, lunge_ - dt);
    if (ramCd_ > 0.f) ramCd_ = std::max(0.f, ramCd_ - dt);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 1.5f);
    if (mode_ == Mode::Victory) doorOpen_ = std::min(1.f, doorOpen_ + dt * 0.7f);
    for (Puff& p : puffs_) p.t -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0.f; }), puffs_.end());
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

void Game::layYard(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = mode_ == Mode::Over ? gs::rgb4(4, 2, 1) : gs::rgb4(6, 8, 12);
    uint16_t skyHor = mode_ == Mode::Over ? gs::rgb4(8, 3, 1)
                      : mode_ == Mode::Victory ? gs::rgb4(12, 10, 4)
                                               : gs::rgb4(12, 10, 6);
    v.setFogColor(mode_ == Mode::Over ? gs::rgb4(6, 2, 1) : gs::rgb4(8, 7, 3));
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
        rd.hw = row * 0.95f;
        rd.v = 780.f / row + t_ * 8.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = gs::ROAD_RUTS;
        rd.band = (int(std::floor(rd.v / 36.f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fog = std::clamp(1.f - row / 110.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fog * (mode_ == Mode::Over ? 6.f : 8.f));
        v.lineBackdrop[y] = gs::rgb4(5, 6, 2);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;

    float shx = shake_ > 0.f ? std::sin(t_ * 88.f) * 5.f * shake_ : 0.f;
    float shy = shake_ > 0.f ? std::cos(t_ * 66.f) * 3.f * shake_ : 0.f;
    layYard(shx * 0.3f);

    if (mode_ == Mode::Title) text("GRANARY PURSE", 160.f + shx, 54.f, 0.95f, PAL_YOU);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f, 78.f, 1.0f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("LAST", 160.f + shx, 52.f, 1.2f, PAL_MILL);
    else if (mode_ == Mode::Over) text("OVER", 160.f + shx, 52.f, 1.15f, PAL_THRESH);

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
        add(15.f, 3, AUGER);
        add(12.6f, 3, TIPPER);
        add(18.f, 3, THRESH);
    }

    float doorZ = mode_ == Mode::Victory ? 22.f - doorOpen_ * 7.f : 22.f;
    add(doorZ, 4, 0);
    add(8.2f, 5, 0);
    add(4.2f, 6, 0);

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z > b.z; });

    for (const Item& it : items) {
        if (it.kind == 0) {
            const Mach& m = machs_[size_t(it.i)];
            float sx, sy, s;
            project(float(m.lane) * LANE_X, m.z, sx, sy, s);
            sx += shx;
            sy += shy;
            float h = bodyH(m.kind) * s;
            int pal = m.flash > 0.f ? PAL_SHOCK : palOf(m.kind);
            shadow(sx, sy, h * 0.8f);
            spr(bodyOf(m.kind), sx, sy, h, pal, m.lane < 0, fogFor(m.z), true);
        } else if (it.kind == 1) {
            const Prop& pr = kProps[it.i];
            float sx, sy, s;
            project(pr.x, pr.z, sx, sy, s);
            sx += shx;
            sy += shy;
            float h = std::clamp(pr.h * s, 6.f, 180.f);
            int fog = fogFor(pr.z);
            const gs::Mipped* img = &art_.silo;
            int pal = PAL_SILO;
            if (pr.kind == 1) {
                img = &art_.sack;
                pal = PAL_SACK;
            } else if (pr.kind == 2) {
                img = &art_.lamp;
                pal = PAL_SACK;
            }
            if (pr.kind != 2) shadow(sx, sy, h * 0.5f);
            spr(*img, sx, sy, h, pal, pr.x > 0.f, fog, true);
        } else if (it.kind == 2) {
            const Puff& p = puffs_[size_t(it.i)];
            float sx, sy, s;
            project(p.x, p.z, sx, sy, s);
            float rise = (0.4f - p.t) * 16.f;
            float h = (p.kind ? 0.45f : 0.7f) * s;
            spr(p.kind ? art_.spark : art_.puff, sx + shx, sy - rise + shy, std::max(8.f, h),
                p.kind ? PAL_SHOCK : PAL_FX, false, fogFor(p.z));
        } else if (it.kind == 3) {
            int kind = it.i;
            int lane = kind == TIPPER ? 1 : kind == THRESH ? 0 : -1;
            float z = 12.f + float(kind) * 1.5f;
            float sx, sy, s;
            project(float(lane) * LANE_X, z, sx, sy, s);
            spr(bodyOf(kind), sx + shx, sy + shy, bodyH(kind) * s, palOf(kind), lane < 0, fogFor(z), true);
        } else if (it.kind == 4) {
            float sx, sy, s;
            project(0.f, doorZ, sx, sy, s);
            float h = std::clamp(2.6f * s, 18.f, 100.f);
            spr(art_.door, sx + shx, sy + shy, h, mode_ == Mode::Victory ? PAL_MILL : PAL_SILO, false, fogFor(doorZ),
                true);
        } else if (it.kind == 5) {
            float sx, sy, s;
            project(px_ * LANE_X, 8.2f, sx, sy, s);
            int pal = PAL_YOU;
            const Mach* near = nullptr;
            for (const Mach& m : machs_) {
                if (!m.alive) continue;
                if (!near || m.z < near->z) near = &m;
            }
            if (near && near->z <= Z_RAM_HI) pal = nearestLane() == near->lane ? PAL_MILL : PAL_THRESH;
            spr(art_.chev, sx + shx, sy + shy, std::max(8.f, 0.36f * s), pal, false, 0, true);
        } else if (it.kind == 6) {
            float bob = std::sin(t_ * 8.f) * 1.2f;
            float lift = lunge_ > 0.f ? std::sin((0.15f - lunge_) / 0.15f * 3.14159f) * 14.f : 0.f;
            float x = 160.f + px_ * LANE_X * (FOCAL / 4.6f) + shx;
            float feet = 206.f + shy + bob - lift;
            shadow(x, feet - 4.f, 70.f);
            spr(art_.miller, x, feet, 72.f, mode_ == Mode::Over ? PAL_SILO : PAL_YOU, false, 0, true);
            if (engineOn_ && (int(t_ * 8.f) & 1) == 0) spr(art_.puff, x - 18.f, feet - 28.f, 12.f, PAL_FX, false);
            if ((int(t_ * 10.f) & 3) == 0) spr(art_.grain, x + 16.f, feet - 36.f, 6.f, PAL_SACK, false);
        }
    }

    spr(art_.chute, 22.f, 148.f, 56.f, PAL_SILO, false, 0, true);
    spr(art_.chute, 298.f, 144.f, 60.f, PAL_SILO, true, 0, true);

    char buf[80];
    if (mode_ == Mode::Title) {
        hudC(16, "BE THE LAST MACHINE STILL RUNNING", PAL_MILL);
        hudC(18, "STALL EVERY OTHER MACHINE", PAL_HUD);
        hudC(21, "ARROWS STEER THE MILL", PAL_HUD);
        hudC(22, "UP OR A RAMS", PAL_SACK);
        if ((int(t_ * 2.f) & 1) == 0) hudC(25, "PRESS START", PAL_SACK);
        std::string ver = S3_VERSION_STRING;
        hud(39 - int(ver.size()), 1, ver, PAL_HUD);
    } else if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        std::snprintf(buf, sizeof buf, "OTHERS %d", others());
        hud(1, 1, buf, others() <= 1 ? PAL_MILL : PAL_HUD);
        std::snprintf(buf, sizeof buf, "GRIT %d", grit_);
        hud(39 - int(std::strlen(buf)), 1, buf, grit_ > 1 ? PAL_YOU : PAL_THRESH);
        hud(1, 2, engineOn_ ? "YOU RUNNING" : "YOU SEIZED", engineOn_ ? PAL_MILL : PAL_THRESH);

        const Mach* near = nullptr;
        for (const Mach& m : machs_) {
            if (!m.alive) continue;
            if (!near || m.z < near->z) near = &m;
        }
        if (near) {
            std::string line = std::string(sideOf(near->lane)) + "  " + whoOf(near->kind);
            hudC(21, line, palOf(near->kind));
            bool aligned = nearestLane() == near->lane;
            if (near->z <= Z_RAM_HI) hudC(23, aligned ? "RAM NOW" : "GET IN LANE", aligned ? PAL_MILL : PAL_THRESH);
            else if (stalled_ == 0) hudC(23, "MATCH ITS LANE AND RAM", PAL_HUD);
        } else if (spawnAt_ < fleet_) {
            hudC(21, "THE NEXT MACHINE IS OUT", PAL_HUD);
        } else {
            hudC(21, "THE YARD IS QUIET", PAL_MILL);
        }
        std::snprintf(buf, sizeof buf, "STALLED %d", stalled_);
        hud(1, 26, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(39 - int(std::strlen(buf)), 26, buf, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(25, "START RESUMES", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(18, "THE LAST MACHINE STILL RUNNING", PAL_MILL);
        hudC(20, "THE GRANARY DOOR OPENS", PAL_YOU);
        std::snprintf(buf, sizeof buf, "STALLED %d   SCORE %d", stalled_, score_);
        hudC(22, buf, PAL_HUD);
    } else if (mode_ == Mode::Over) {
        hudC(18, "THE WATCH IS OVER", PAL_THRESH);
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
        float pitch = 44.f + std::min(watch_, 20.f) * 0.28f + (lunge_ > 0.f ? 12.f : 0.f);
        sys_->apu.setFreq(0, pitch);
        sys_->apu.setVol(0, mode_ == Mode::Watch ? 0.06f : 0.04f);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {262.f, 330.f, 392.f, 523.f};
        fanT_ += DT;
        if (fanT_ > 0.16f) {
            if (fanStep_ < 4) sys_->apu.keyOn(2, notes[fanStep_], 0.16f);
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
    sys.apu.setEcho(0.14f, 0.26f, 0.16f);
    sys.apu.setPatch(0, millPatch());
    sys.apu.setPatch(1, metalPatch());
    sys.apu.setPatch(2, brassPatch());
    if (bot_) beginWatch();
    else {
        mode_ = Mode::Title;
        sys.apu.keyOn(0, 34.f, 0.035f);
        engineOn_ = true;
        sys.setLight(120, 90, 30);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        px_ = std::sin(t_ * 0.6f) * 0.5f;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            blip(600.f, 0.05f);
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
        }
    }

    tickFx(DT);
    draw();
    serviceAudio();
}

}  // namespace gpurs
