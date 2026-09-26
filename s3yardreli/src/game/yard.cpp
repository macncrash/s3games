#include "game/yard.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace yard {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float FOCAL = 200.0f;
constexpr float HORIZON = 86.0f;
constexpr float GROUND = 2.4f;
constexpr float LANE_X = 1.15f;
constexpr float Z_SPAWN = 42.0f;
constexpr float Z_HIT = 5.2f;
constexpr float Z_REACH = 12.0f;
constexpr float MOVE = 3.4f;
constexpr float DROP_CD = 0.30f;
constexpr float BELL_AT = 33.0f;
constexpr float BELL_END = 41.2f;
constexpr float CHARGE_NEED = 1.4f;
constexpr int YARD_MAX = 5;

constexpr int DOG = 0;
constexpr int SCRAP = 1;
constexpr int WRECK = 2;

struct Row {
    float arr;
    int kind;
    int lane;
};

// One threat at a time. The magnet is already waiting when each one enters reach.
const Row kRows[] = {
    {8.52f, DOG, -1}, {11.76f, SCRAP, 1}, {14.52f, DOG, 0}, {18.16f, SCRAP, -1},
    {23.30f, WRECK, 1}, {26.32f, DOG, -1}, {29.96f, SCRAP, 0},
};

struct Dress {
    float x, z;
    int kind;
};

const Dress kPiles[] = {
    {-2.45f, 7.6f, 0}, {2.45f, 8.4f, 1}, {-2.55f, 12.8f, 1}, {2.55f, 13.6f, 0},
    {-2.65f, 18.8f, 0}, {2.65f, 20.0f, 1}, {-2.75f, 26.5f, 1}, {2.75f, 28.4f, 0},
};
const Dress kDrums[] = {
    {-1.95f, 6.6f, 0}, {1.95f, 7.0f, 0}, {-2.15f, 15.2f, 0}, {2.15f, 16.4f, 0},
};
const Dress kLamps[] = {
    {-2.25f, 9.6f, 0}, {2.25f, 11.2f, 0}, {-2.35f, 17.4f, 0}, {2.35f, 22.0f, 0},
};

float speedOf(int kind) { return kind == DOG ? 4.4f : kind == SCRAP ? 3.3f : 2.15f; }
int hpOf(int kind) { return kind == WRECK ? 2 : 1; }
int ptsOf(int kind) { return kind == DOG ? 100 : kind == SCRAP ? 220 : 480; }
int dmgOf(int kind) { return kind == WRECK ? 2 : 1; }

const char* whoOf(int kind) { return kind == DOG ? "DOG" : kind == WRECK ? "WRECK" : "SCRAPPER"; }
const char* sideOf(int lane) { return lane < 0 ? "LEFT" : lane > 0 ? "RIGHT" : "CENTER"; }

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 1;
    p.fb = 0.4f;
    p.op[0] = {1.0f, 0.8f, 0.3f, 0.8f, 0.9f, 0.4f};
    p.op[1] = {2.0f, 0.3f, 0.2f, 0.6f, 0.7f, 0.3f};
    p.op[2] = {0.5f, 0.5f, 0.4f, 1.0f, 0.8f, 0.5f};
    p.op[3] = {3.0f, 0.15f, 0.2f, 0.5f, 0.4f, 0.3f};
    p.vol = 0.12f;
    p.tone = 480.0f;
    p.drive = 0.12f;
    return p;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.15f;
    p.op[0] = {1.0f, 1.0f, 0.004f, 0.55f, 0.1f, 0.9f};
    p.op[1] = {2.7f, 0.45f, 0.004f, 0.4f, 0.06f, 0.7f};
    p.op[2] = {5.2f, 0.22f, 0.006f, 0.3f, 0.03f, 0.55f};
    p.op[3] = {1.5f, 0.3f, 0.005f, 0.45f, 0.08f, 0.75f};
    p.vol = 0.24f;
    p.echo = 0.45f;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 6;
    p.fb = 0.2f;
    p.op[0] = {1.0f, 1.0f, 0.02f, 0.18f, 0.7f, 0.2f};
    p.op[1] = {2.0f, 0.35f, 0.02f, 0.22f, 0.5f, 0.2f};
    p.op[2] = {3.0f, 0.2f, 0.03f, 0.25f, 0.35f, 0.2f};
    p.op[3] = {1.0f, 0.25f, 0.02f, 0.2f, 0.55f, 0.2f};
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

int Game::nearestLane() const {
    int lane = int(std::lround(px_));
    if (lane < -1 || lane > 1) return 99;
    if (std::fabs(px_ - float(lane)) > 0.40f) return 99;
    return lane;
}

const gs::Mipped& Game::foeImg(const Foe& f) const {
    int frame = int(f.age * 8.0f) & 1;
    if (f.kind == DOG) return art_.dog[frame];
    if (f.kind == WRECK) return art_.wreck;
    return art_.scrap[frame];
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = 0.07f;
}

void Game::project(float worldX, float z, float& sx, float& sy, float& s) const {
    float zz = std::max(0.9f, z);
    s = FOCAL / zz;
    sx = 160.0f + worldX * s;
    sy = HORIZON + GROUND * s;
}

void Game::hurt(Foe& f) {
    f.hp -= 1;
    f.flash = 0.12f;
    if (f.hp > 0) {
        blip(220.0f, 0.05f);
        sys_->apu.noiseBurst(0.16f, 700.0f, 0.05f);
        return;
    }
    f.alive = false;
    score_ += f.points;
    stopped_++;
    Pop p;
    p.lane = float(f.lane) * LANE_X;
    p.z = f.z;
    p.t = 0.7f;
    p.pts = f.points;
    pops_.push_back(p);
    if (pops_.size() > 5) pops_.erase(pops_.begin());
    puffs_.push_back({float(f.lane) * LANE_X, f.z, 0.35f});
    if (puffs_.size() > 8) puffs_.erase(puffs_.begin());
    chainT_ = 0.16f;
    chainLane_ = f.lane;
    chainZ_ = f.z;
    sys_->apu.noiseBurst(0.22f, 1200.0f, 0.07f);
    blip(640.0f, 0.05f);
}

void Game::doDrop() {
    if (dropCd_ > 0.0f || won_ || mode_ != Mode::Watch) return;
    int lane = nearestLane();
    dropCd_ = DROP_CD;
    slam_ = 0.18f;
    if (lane < -1 || lane > 1) {
        blip(140.0f, 0.03f);
        return;
    }
    int hit = -1;
    float best = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        Foe& f = foes_[size_t(i)];
        if (!f.alive || f.lane != lane || f.z <= Z_HIT || f.z > Z_REACH) continue;
        if (f.z < best) {
            best = f.z;
            hit = i;
        }
    }
    if (hit < 0) {
        blip(160.0f, 0.03f);
        sys_->apu.noiseBurst(0.08f, 400.0f, 0.04f);
        return;
    }
    hurt(foes_[size_t(hit)]);
    sys_->rumble(0.2f, 0.35f, 40);
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE YARD HELD UNTIL THE RELIEF BELL";
    score_ += 700 + yard_ * 120;
    fanStep_ = 0;
    fanT_ = 0;
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.12f, 1800.0f, 0.06f);
    sys_->rumble(0.25f, 0.5f, 160);
    sys_->setLight(40, 160, 70);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    engineOn_ = false;
    shake_ = 0.55f;
    sys_->apu.keyOff(0);
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.48f, 180.0f, 0.32f);
    blip(80.0f, 0.08f);
    sys_->rumble(0.8f, 0.4f, 220);
    sys_->setLight(180, 30, 20);
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "THE WATCH IS OVER";
    score_ = 0;
    stopped_ = 0;
    yard_ = YARD_MAX;
    spawnAt_ = 0;
    fanStep_ = -1;
    px_ = 0;
    watch_ = 0;
    endT_ = 0;
    slam_ = dropCd_ = charge_ = chainT_ = shake_ = beep_ = bellTick_ = fanT_ = 0;
    chainLane_ = 0;
    chainZ_ = 0;
    foes_.clear();
    puffs_.clear();
    pops_.clear();
    script_.clear();
    script_.reserve(sizeof kRows / sizeof kRows[0]);
    for (const Row& r : kRows) {
        Spawn s;
        s.t = r.arr - (Z_SPAWN - Z_HIT) / speedOf(r.kind);
        s.kind = r.kind;
        s.lane = r.lane;
        if (s.t < 0.0f) s.t = 0.0f;
        script_.push_back(s);
    }
    std::sort(script_.begin(), script_.end(), [](const Spawn& a, const Spawn& b) { return a.t < b.t; });
    if (!sys_) return;
    sys_->apu.keyOff(1);
    sys_->apu.keyOn(0, 46.0f, 0.05f);
    engineOn_ = true;
    sys_->setLight(90, 60, 30);
}

void Game::update(float dt) {
    watch_ += dt;
    if (!bell_ && watch_ >= BELL_AT && yard_ > 0) {
        bell_ = true;
        bellTick_ = 0;
        sys_->apu.keyOn(1, 523.0f, 0.26f);
        sys_->rumble(0.3f, 0.55f, 140);
        sys_->setLight(180, 140, 40);
    } else if (bell_ && !won_ && mode_ == Mode::Watch) {
        bellTick_ += dt;
        if (bellTick_ >= 0.92f) {
            bellTick_ = 0;
            sys_->apu.keyOn(1, 523.0f, 0.22f);
        }
    }

    if (!bell_) {
        while (spawnAt_ < int(script_.size()) && script_[size_t(spawnAt_)].t <= watch_) {
            const Spawn& s = script_[size_t(spawnAt_)];
            spawnAt_++;
            Foe f;
            f.kind = s.kind;
            f.lane = s.lane;
            f.hp = hpOf(s.kind);
            f.points = ptsOf(s.kind);
            f.z = Z_SPAWN;
            f.alive = true;
            foes_.push_back(f);
        }
    }

    int best = -1;
    float bestRank = 1.0e9f;
    float mind = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        const Foe& f = foes_[size_t(i)];
        if (!f.alive) continue;
        float eta = (f.z - Z_HIT) / speedOf(f.kind);
        if (eta < mind) mind = eta;
        float rank = eta;
        if (f.z <= Z_REACH && f.z > Z_HIT) rank -= 20.0f;
        if (rank < bestRank) {
            bestRank = rank;
            best = i;
        }
    }

    float dir = 0;
    bool holdUp = false;
    bool wantDrop = false;
    if (bot_) {
        bool closing = bell_ && (BELL_END - watch_) < CHARGE_NEED + 0.6f;
        bool answer = bell_ && (mind > 1.8f || (closing && mind > 0.55f));
        if (answer) {
            if (px_ > 0.05f) dir = -1.0f;
            else if (px_ < -0.05f) dir = 1.0f;
            else holdUp = true;
        } else if (best >= 0) {
            float dest = float(foes_[size_t(best)].lane);
            if (px_ < dest - 0.03f) dir = 1.0f;
            else if (px_ > dest + 0.03f) dir = -1.0f;
        }
    } else {
        const gs::Pad& pad = sys_->pad;
        float digital = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        float axis = std::fabs(pad.axisX) > 0.2f ? pad.axisX : digital;
        dir = std::clamp(axis, -1.0f, 1.0f);
        holdUp = pad.down(gs::BTN_UP) || pad.down(gs::BTN_Y);
        wantDrop = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) ||
                   pad.down(gs::BTN_TURBO) || pad.accel > 0.5f;
    }

    bool charging = bell_ && holdUp && std::fabs(px_) < 0.42f && !won_;
    if (charging) {
        charge_ += dt;
        if (charge_ >= CHARGE_NEED) winWatch();
    } else {
        if (charge_ > 0.0f) charge_ = std::max(0.0f, charge_ - 0.9f * dt);
        px_ = std::clamp(px_ + dir * MOVE * dt, -1.0f, 1.0f);
        if (bot_ && best >= 0 && !won_) {
            const Foe& f = foes_[size_t(best)];
            if (std::fabs(px_ - float(f.lane)) < 0.40f && f.z <= Z_REACH && f.z > Z_HIT) wantDrop = true;
        }
        if (wantDrop && !won_) doDrop();
    }

    if (won_ || mode_ != Mode::Watch) {
        foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return !f.alive; }), foes_.end());
        return;
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age += dt;
        if (f.flash > 0.0f) f.flash -= dt;
        f.z -= speedOf(f.kind) * dt;
        if (f.z > Z_HIT) continue;
        f.alive = false;
        yard_ -= dmgOf(f.kind);
        if (yard_ < 0) yard_ = 0;
        shake_ = 0.5f;
        puffs_.push_back({float(f.lane) * LANE_X, Z_HIT, 0.4f});
        sys_->apu.noiseBurst(0.42f, 240.0f, 0.18f);
        blip(90.0f, 0.07f);
        sys_->rumble(0.75f, 0.45f, 180);
        sys_->setLight(160, 40, 20);
        if (yard_ <= 0) {
            loseWatch("THE YARD FELL");
            break;
        }
    }

    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return !f.alive; }), foes_.end());

    if (!won_ && mode_ == Mode::Watch && bell_ && watch_ >= BELL_END) loseWatch("MISSED THE BELL");
}

void Game::tickFx(float dt) {
    if (slam_ > 0.0f) slam_ = std::max(0.0f, slam_ - dt);
    if (chainT_ > 0.0f) chainT_ = std::max(0.0f, chainT_ - dt);
    if (dropCd_ > 0.0f && mode_ != Mode::Watch) dropCd_ = std::max(0.0f, dropCd_ - dt);
    if (mode_ == Mode::Watch && dropCd_ > 0.0f) dropCd_ = std::max(0.0f, dropCd_ - dt);
    if (shake_ > 0.0f) shake_ = std::max(0.0f, shake_ - dt * 1.6f);
    for (Puff& p : puffs_) p.t -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0.0f; }), puffs_.end());
    for (Pop& p : pops_) p.t -= dt;
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0.0f; }), pops_.end());
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.5f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.y > gs::SCREEN_H + 48 || s.x + s.w < -48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::shadow(float cx, float cy, float w) {
    if (w < 4.0f) return;
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
    float adv = 16.0f * scale;
    if (!s.empty() && s.size() * adv > 300.0f) {
        scale *= 300.0f / (float(s.size()) * adv);
        adv = 16.0f * scale;
    }
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, float(g.h) * scale, pal, false);
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
    v.A.enabled = false;
    v.B.enabled = false;

    float warm = 0.12f;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) warm = 0.12f + 0.55f * std::clamp(watch_ / BELL_AT, 0.0f, 1.0f);
    if (bell_ || mode_ == Mode::Victory) warm = 1.0f;
    if (mode_ == Mode::Over) warm = 0.2f;
    uint16_t skyTop = mix(gs::rgb4(1, 1, 4), gs::rgb4(4, 3, 6), warm * 0.4f);
    uint16_t skyHor = mix(gs::rgb4(5, 3, 3), gs::rgb4(13, 7, 2), warm);
    uint16_t fogC = mix(gs::rgb4(2, 2, 3), gs::rgb4(8, 5, 2), warm * 0.7f);
    if (mode_ == Mode::Over) {
        skyTop = gs::rgb4(3, 1, 1);
        skyHor = gs::rgb4(8, 2, 1);
        fogC = gs::rgb4(4, 1, 1);
    }
    v.setFogColor(fogC);

    float shx = shake_ > 0.0f ? std::sin(t_ * 90.0f) * 5.0f * shake_ : 0.0f;
    float shy = shake_ > 0.0f ? std::cos(t_ * 70.0f) * 3.0f * shake_ : 0.0f;
    const int horizon = int(HORIZON);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < horizon) {
            v.lineBackdrop[y] = mix(skyTop, skyHor, float(y) / float(horizon));
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - horizon) + 1.0f;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.0f + shx * 0.4f;
        r.hw = row * 1.22f;
        r.v = 2400.0f / row + t_ * 28.0f;
        r.pal = PAL_FIELD;
        r.band = (int(std::floor(r.v / 55.0f)) & 1) ? 1 : 0;
        r.style = gs::ROAD_ROCKY;
        r.left = r.right = gs::GROUND_LAND;
        float fog = std::clamp(1.0f - row / 72.0f, 0.0f, 1.0f);
        v.lineFog[y] = uint8_t(fog * (bell_ ? 6.0f : 10.0f));
        v.lineBackdrop[y] = skyHor;
    }

    auto fogAt = [](float z) { return std::clamp(int((z - 14.0f) / 2.6f), 0, 12); };

    if (mode_ == Mode::Title) text("YARD RELIEF", 160, 108, 1.15f, PAL_FX);
    else if (mode_ == Mode::Pause) text("PAUSED", 160, 100, 1.0f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("HELD", 160, 96, 1.2f, PAL_BELL);
    else if (mode_ == Mode::Over) text(reason_, 160, 96, 0.95f, mode_ == Mode::Over && won_ ? PAL_BELL : PAL_SCRAP);

    float swing = 0.0f;
    if (bell_ || mode_ == Mode::Victory) {
        float rate = charge_ > 0.05f || mode_ == Mode::Victory ? 14.0f : 7.0f;
        float amp = charge_ > 0.05f || mode_ == Mode::Victory ? 8.0f : 4.0f;
        swing = std::sin(t_ * rate) * amp;
    }
    float dip = slam_ > 0.0f ? std::sin((1.0f - slam_ / 0.18f) * 3.14159f) * 26.0f : 0.0f;
    float lift = charge_ > 0.0f ? std::clamp(charge_ / CHARGE_NEED, 0.0f, 1.0f) * 28.0f : 0.0f;
    float magY = 118.0f + std::sin(t_ * 2.1f) * 2.0f + dip - lift + shy;
    float sMag = FOCAL / 7.2f;
    float magX = 160.0f + px_ * LANE_X * sMag + shx;

    spr(art_.magnet, magX, magY, 36, PAL_CRANE, false);
    float cableTop = 30.0f;
    float cableBot = magY - 16.0f;
    if (cableBot > cableTop + 8.0f) spr(art_.cable, magX, (cableTop + cableBot) * 0.5f, cableBot - cableTop, PAL_CRANE, false);
    spr(art_.bell, 160.0f + swing + shx, 42, 30, PAL_BELL, false);
    int pose = int(t_ * 3.0f) & 1;
    spr(art_.hand[pose], 46, 64, 28, PAL_HAND, false);
    spr(art_.cab, 40, 78, 108, PAL_CRANE, false);
    spr(art_.shack, 286, 96, 100, PAL_PROP, false);
    spr(art_.beam, 160 + shx * 0.2f, 18, 20, PAL_CRANE, false);
    spr(art_.moon, 214, 52, 18, PAL_FX, false);

    if (chainT_ > 0.0f) {
        for (int i = 0; i < 5; i++) {
            float u = float(i + 1) / 6.0f;
            float z = 6.4f + (chainZ_ - 6.4f) * u;
            float sx, sy, s;
            project(float(chainLane_) * LANE_X, z, sx, sy, s);
            spr(art_.link, sx + shx, sy - 0.4f * s + shy, std::max(6.0f, 0.35f * s), PAL_CRANE, false, fogAt(z));
        }
    }

    struct Item {
        float z;
        int kind;
        int i;
    };
    std::vector<Item> items;
    items.reserve(24);
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[size_t(i)].alive) items.push_back({foes_[size_t(i)].z, 0, i});
    for (int i = 0; i < 8; i++) items.push_back({kPiles[i].z, 1, i});
    for (int i = 0; i < 4; i++) items.push_back({kDrums[i].z, 2, i});
    for (int i = 0; i < 4; i++) items.push_back({kLamps[i].z, 3, i});
    for (int lane = -1; lane <= 1; lane++) items.push_back({6.8f, 4, lane + 1});
    for (int i = 0; i < int(puffs_.size()); i++) items.push_back({puffs_[size_t(i)].z, 5, i});
    if (mode_ == Mode::Title) items.push_back({14.0f + std::sin(t_ * 0.5f), 6, 0});
    if (mode_ == Mode::Victory) {
        float z = std::max(6.6f, 12.0f - endT_ * 1.4f);
        items.push_back({z, 7, 0});
        items.push_back({z + 1.1f, 7, 1});
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    for (const Item& it : items) {
        if (it.kind == 0) {
            const Foe& f = foes_[size_t(it.i)];
            float sx, sy, s;
            project(float(f.lane) * LANE_X, f.z, sx, sy, s);
            sx += shx;
            sy += shy;
            int fog = fogAt(f.z);
            float body = f.kind == WRECK ? 1.55f : f.kind == DOG ? 0.95f : 1.85f;
            float h = body * s;
            int pal = f.kind == DOG ? PAL_DOG : f.kind == WRECK ? PAL_WRECK : PAL_SCRAP;
            shadow(sx, sy, h * (f.kind == WRECK ? 1.3f : 0.7f));
            spr(foeImg(f), sx, sy, h, pal, f.lane > 0, fog, true);
            if (f.flash > 0.0f) spr(art_.spark, sx, sy - h * 0.55f, h * 0.45f, PAL_FX, false, fog);
        } else if (it.kind == 1) {
            const Dress& d = kPiles[it.i];
            float sx, sy, s;
            project(d.x, d.z, sx, sy, s);
            float h = 2.3f * s;
            shadow(sx + shx, sy + shy, h * 0.8f);
            spr(art_.pile[d.kind], sx + shx, sy + shy, h, PAL_PROP, d.x > 0, fogAt(d.z), true);
            if ((it.i & 1) == 0) {
                float puff = 0.5f + 0.5f * std::sin(t_ * 1.3f + d.z);
                spr(art_.smoke, sx + shx, sy - h * 0.92f + shy - puff * 4.0f, h * 0.28f, PAL_FX, false, fogAt(d.z));
            }
        } else if (it.kind == 2) {
            const Dress& d = kDrums[it.i];
            float sx, sy, s;
            project(d.x, d.z, sx, sy, s);
            spr(art_.drum, sx + shx, sy + shy, 0.7f * s, PAL_PROP, false, fogAt(d.z), true);
        } else if (it.kind == 3) {
            const Dress& d = kLamps[it.i];
            float sx, sy, s;
            project(d.x, d.z, sx, sy, s);
            float h = 1.7f * s;
            spr(art_.lamp, sx + shx, sy + shy, h, PAL_PROP, false, fogAt(d.z), true);
            if ((int(t_ * 8.0f) + it.i) % 9 != 0)
                spr(art_.spark, sx + shx, sy - h * 0.92f + shy, h * 0.28f, PAL_FX, false, fogAt(d.z));
        } else if (it.kind == 4) {
            int lane = it.i - 1;
            float sx, sy, s;
            project(float(lane) * LANE_X, 6.8f, sx, sy, s);
            spr(art_.pad, sx + shx, sy + shy, 0.28f * s, PAL_PROP, false, 0);
            if (lane == nearestLane() && (mode_ == Mode::Watch || mode_ == Mode::Pause))
                spr(art_.spark, sx + shx, sy - 4.0f + shy, 8, PAL_FX, false);
        } else if (it.kind == 5) {
            const Puff& p = puffs_[size_t(it.i)];
            float sx, sy, s;
            project(p.lane, p.z, sx, sy, s);
            float k = std::clamp(p.t / 0.35f, 0.0f, 1.0f);
            spr(art_.puff, sx + shx, sy - 0.4f * s + shy, (0.6f + (1.0f - k)) * s * 0.5f, PAL_FX, false, fogAt(p.z));
        } else if (it.kind == 6) {
            float z = it.z;
            float sx, sy, s;
            project(0.35f, z, sx, sy, s);
            spr(art_.dog[int(t_ * 6.0f) & 1], sx + shx, sy + shy, 0.95f * s, PAL_DOG, false, fogAt(z), true);
        } else {
            float x = it.i == 0 ? -0.35f : 0.45f;
            float sx, sy, s;
            project(x, it.z, sx, sy, s);
            spr(art_.relief[int(t_ * 6.0f + it.i) & 1], sx + shx, sy + shy, 1.7f * s, PAL_HAND, false, fogAt(it.z), true);
        }
    }

    static const float kStars[][2] = {{36, 50}, {78, 42}, {128, 58}, {236, 46}, {268, 64}};
    for (int i = 0; i < 5; i++) {
        if ((int(t_ * 2.0f) + i * 2) % 7 == 0) continue;
        spr(art_.spark, kStars[i][0], kStars[i][1], 4, PAL_FX, false);
    }

    for (const Pop& p : pops_) {
        float sx, sy, s;
        project(p.lane, p.z, sx, sy, s);
        char buf[16];
        std::snprintf(buf, sizeof buf, "+%d", p.pts);
        text(buf, sx, sy - s * 0.8f - (0.7f - p.t) * 18.0f, 0.5f, PAL_FX);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(16, "HOLD UNTIL THE RELIEF BELL", PAL_FX);
        hudC(21, "ARROWS SLIDE THE MAGNET", PAL_HUD);
        hudC(22, "DOWN OR Z DROPS IT", PAL_HUD);
        hudC(23, "UP ANSWERS THE BELL", PAL_BELL);
        if (int(t_ * 2.0f) % 2 == 0) hudC(25, "PRESS START", PAL_HUD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 1, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        hud(1, 1, "YARD", yard_ <= 2 ? PAL_SCRAP : PAL_HUD);
        for (int i = 0; i < YARD_MAX; i++) {
            int pal = i < yard_ ? (yard_ <= 2 ? PAL_SCRAP : PAL_BELL) : PAL_HUD;
            hud(6 + i, 1, i < yard_ ? "=" : "-", pal);
        }
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(39 - int(std::strlen(buf)), 1, buf, PAL_HUD);

        const Foe* near = nullptr;
        for (const Foe& f : foes_) {
            if (!f.alive) continue;
            if (!near || f.z < near->z) near = &f;
        }
        if (bell_) {
            hudC(20, std::fabs(px_) < 0.42f ? "HOLD UP ON THE BELL" : "CENTER THE MAGNET", PAL_FX);
            int n = std::clamp(int(std::lround(charge_ / CHARGE_NEED * 10.0f)), 0, 10);
            std::string meter = "BELL ";
            for (int i = 0; i < 10; i++) meter += i < n ? "=" : "-";
            hudC(22, meter, charge_ > 0.0f ? PAL_BELL : PAL_FX);
        } else if (near && near->z <= Z_REACH) {
            hudC(20, near->hp > 1 ? "DROP TWICE" : "DROP", PAL_SCRAP);
            std::snprintf(buf, sizeof buf, "%s  %s", sideOf(near->lane), whoOf(near->kind));
            hudC(22, buf, PAL_HUD);
        } else if (near) {
            std::snprintf(buf, sizeof buf, "%s  %s", sideOf(near->lane), whoOf(near->kind));
            hudC(20, buf, PAL_HUD);
        } else {
            hudC(20, "THE YARD IS QUIET", PAL_HUD);
        }
        if (!bell_) {
            hud(1, 26, "WATCH", PAL_HUD);
            int n = std::clamp(int(std::lround(watch_ / BELL_AT * 10.0f)), 0, 10);
            for (int i = 0; i < 10; i++) hud(8 + i, 26, i < n ? "=" : "-", i < n ? PAL_FX : PAL_HUD);
        }
        if (mode_ == Mode::Pause) {
            hudC(24, "START RESUMES", PAL_HUD);
            hudC(26, "ESC LEAVES THE WATCH", PAL_HUD);
        }
    } else if (mode_ == Mode::Victory) {
        hudC(20, "RELIEF HAS THE YARD", PAL_BELL);
        hudC(22, "THE YARD HELD UNTIL THE RELIEF BELL", PAL_HUD);
        std::snprintf(buf, sizeof buf, "STOPPED %d   SCORE %d", stopped_, score_);
        hudC(24, buf, PAL_HUD);
        hudC(26, "START", PAL_HUD);
    } else if (mode_ == Mode::Over) {
        hudC(20, "THE WATCH IS OVER", PAL_SCRAP);
        std::snprintf(buf, sizeof buf, "YARD %d   SCORE %d", yard_, score_);
        hudC(22, buf, PAL_HUD);
        hudC(24, reason_, PAL_FX);
        hudC(26, "START TRIES AGAIN", PAL_HUD);
    }

    if (beep_ > 0.0f) {
        beep_ -= DT;
        if (beep_ <= 0.0f) sys_->apu.tone(0, 0, 0);
    }
    if (engineOn_) {
        sys_->apu.setFreq(0, 46.0f + std::min(watch_, 36.0f) * 0.35f);
        float vol = mode_ == Mode::Watch ? 0.07f : 0.04f;
        if (bell_) vol = 0.05f;
        sys_->apu.setVol(0, vol);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {330.0f, 415.0f, 494.0f, 659.0f};
        fanT_ += DT;
        if (fanT_ > 0.18f) {
            if (fanStep_ < 4) sys_->apu.keyOn(2, notes[fanStep_], 0.18f);
            else sys_->apu.keyOff(2);
            fanStep_++;
            fanT_ = 0;
            if (fanStep_ > 8) fanStep_ = -1;
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.18f, 0.28f, 0.16f);
    sys.apu.setPatch(0, dronePatch());
    sys.apu.setPatch(1, bellPatch());
    sys.apu.setPatch(2, hornPatch());
    sys.apu.keyOn(0, 46.0f, 0.05f);
    engineOn_ = true;
    if (bot_) beginWatch();
    else {
        mode_ = Mode::Title;
        sys.setLight(80, 50, 20);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    tickFx(DT);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        px_ = std::sin(t_ * 0.8f) * 0.55f;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            blip(660.0f, 0.06f);
            beginWatch();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Watch) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(DT);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Watch;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            foes_.clear();
            bell_ = false;
            over_ = false;
            won_ = false;
        }
    } else if (mode_ == Mode::Victory || mode_ == Mode::Over) {
        endT_ += DT;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            if (mode_ == Mode::Over) beginWatch();
            else {
                mode_ = Mode::Title;
                over_ = false;
                won_ = false;
                bell_ = false;
                foes_.clear();
            }
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            bell_ = false;
            foes_.clear();
        }
    }

    draw();
}

}  // namespace yard
