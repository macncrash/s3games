#include "game/reli.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace reli {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kHorizon = 60.f;
constexpr float kZScale = 268.f;
constexpr float kZLine = 2.55f;
constexpr float kZSpawn = 17.2f;
constexpr float kMove = 2.4f;
constexpr float kBoltV = 21.f;
constexpr float kBoltCool = 0.2f;
constexpr float kPikeCool = 0.28f;
constexpr float kPikeZ = 6.2f;
constexpr float kBellAt = 34.2f;
constexpr float kBellEnd = 48.f;
constexpr float kRope = 1.22f;
constexpr float kBellU = -0.74f;
constexpr float kBellReach = 0.12f;

enum { Skirm = 0, Porter = 1, Scout = 2 };

struct Row {
    float arr;
    int kind;
    float u;
};

// Arrival at the spine. Spawn time is arrival minus the walk down.
const Row kRows[] = {
    {6.2f, Skirm, -0.30f}, {9.5f, Scout, 0.55f},  {12.7f, Skirm, -0.60f}, {16.4f, Porter, 0.16f},
    {20.0f, Scout, 0.70f}, {23.3f, Skirm, -0.40f}, {26.6f, Scout, 0.34f},  {29.8f, Skirm, -0.08f},
    {32.8f, Scout, 0.62f}, {39.8f, Skirm, 0.20f},  {44.2f, Scout, -0.50f},
};

float speedOf(int kind) { return kind == Porter ? 1.78f : kind == Scout ? 3.40f : 2.72f; }
int hpOf(int kind) { return kind == Porter ? 2 : 1; }
int ptsOf(int kind) { return kind == Porter ? 250 : kind == Scout ? 150 : 100; }
float radOf(int kind) { return kind == Porter ? 0.24f : kind == Scout ? 0.15f : 0.18f; }
float tallOf(int kind) { return kind == Porter ? 98.f : kind == Scout ? 76.f : 86.f; }
int palOf(int kind) { return kind == Porter ? PAL_PORTER : kind == Scout ? PAL_SCOUT : PAL_SKIRM; }

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch windPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.38f;
    p.op[0] = {1.f, 0.8f, 0.45f, 1.1f, 0.75f, 0.4f};
    p.op[1] = {0.5f, 0.35f, 0.4f, 0.9f, 0.55f, 0.35f};
    p.op[2] = {2.f, 0.18f, 0.25f, 0.6f, 0.35f, 0.25f};
    p.op[3] = {3.1f, 0.12f, 0.2f, 0.5f, 0.25f, 0.2f};
    p.vol = 0.1f;
    p.tone = 380.f;
    p.drive = 0.08f;
    return p;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.08f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.45f, 0.12f, 0.92f};
    p.op[1] = {2.02f, 0.4f, 0.004f, 0.34f, 0.07f, 0.78f};
    p.op[2] = {3.71f, 0.22f, 0.006f, 0.28f, 0.04f, 0.66f};
    p.op[3] = {5.15f, 0.11f, 0.008f, 0.22f, 0.02f, 0.5f};
    p.vol = 0.24f;
    p.echo = 0.42f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (over_ || mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (mode_ == Mode::Watch && bell_) return 2;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) return 1;
    return 0;
}

bool Game::atBell() const { return std::fabs(u_ - kBellU) <= kBellReach; }

float Game::bendAt(float row) const { return std::sin(row * 0.017f + 0.4f) * (3.f + row * 0.022f); }

float Game::halfAt(float row) const { return 22.f + row * 0.56f; }

Game::Spot Game::spot(float u, float z, float base) const {
    float row = kZScale / std::max(z, 0.8f);
    float feet = kZScale / kZLine;
    float t = std::clamp(row / feet, 0.02f, 1.25f);
    float along = std::clamp((z - kZLine) / (kZSpawn - kZLine), 0.f, 1.f);
    Spot s;
    s.h = std::max(11.f, base * std::pow(t, 0.62f));
    float lift = std::pow(along, 1.25f) * s.h * 0.55f;
    s.x = 160.f + bendAt(row) + u * halfAt(row) + shx_;
    s.y = kHorizon + row - lift + shy_;
    s.fog = int(std::clamp(along * 4.2f, 0.f, 6.f));
    return s;
}

const gs::Mipped& Game::foeImg(int kind, int frame) const {
    if (kind == Porter) return art_.porter[frame & 1];
    if (kind == Scout) return art_.scout[frame & 1];
    return art_.skirm[frame & 1];
}

void Game::beginWatch() {
    mode_ = bot_ ? Mode::Watch : Mode::Brief;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "THE WATCH RAN OUT";
    score_ = 0;
    stopped_ = 0;
    spawnAt_ = 0;
    fanStep_ = -1;
    u_ = 0;
    face_ = 1;
    watch_ = 0;
    modeT_ = 0;
    rope_ = 0;
    fireCd_ = pikeCd_ = swing_ = flash_ = shake_ = beep_ = fanT_ = bellTick_ = 0;
    foes_.clear();
    bolts_.clear();
    puffs_.clear();
    pops_.clear();
    script_.clear();
    const float span = kZSpawn - kZLine;
    for (const Row& r : kRows) {
        Spawn s;
        s.t = r.arr - span / speedOf(r.kind);
        s.kind = r.kind;
        s.u = r.u;
        script_.push_back(s);
    }
    std::sort(script_.begin(), script_.end(), [](const Spawn& a, const Spawn& b) { return a.t < b.t; });
    if (!sys_) return;
    sys_->apu.keyOn(0, 49.f, 0.055f);
    sys_->setLight(36, 28, 48);
}

void Game::hurt(Foe& f) {
    if (!f.on) return;
    f.hp -= 1;
    f.flash = 0.1f;
    puffs_.push_back({f.u, f.z, 0.35f});
    if (puffs_.size() > 8) puffs_.erase(puffs_.begin());
    if (f.hp > 0) {
        sys_->apu.tone(0, 310.f, 0.045f);
        beep_ = 0.05f;
        return;
    }
    f.on = false;
    score_ += f.points;
    stopped_++;
    Spot s = spot(f.u, std::max(f.z, kZLine), tallOf(f.kind));
    pops_.push_back({s.x, s.y - s.h * 0.55f, 0.7f, f.points});
    if (pops_.size() > 4) pops_.erase(pops_.begin());
    sys_->apu.tone(0, 620.f, 0.05f);
    sys_->apu.noiseBurst(0.16f, 900.f, 0.06f);
    beep_ = 0.05f;
}

void Game::looseBolt() {
    if (fireCd_ > 0 || bolts_.size() >= 4 || won_) return;
    fireCd_ = kBoltCool;
    flash_ = 0.07f;
    Bolt b;
    b.u = u_;
    b.z = b.prev = kZLine + 0.28f;
    b.on = true;
    bolts_.push_back(b);
    sys_->apu.tone(0, 740.f, 0.04f);
    beep_ = 0.04f;
}

void Game::pike() {
    if (pikeCd_ > 0 || won_) return;
    bool any = false;
    for (Foe& f : foes_) {
        if (!f.on || f.z > kPikeZ || f.z <= kZLine) continue;
        if (std::fabs(f.u - u_) > 0.26f) continue;
        hurt(f);
        any = true;
    }
    if (!any) return;
    pikeCd_ = kPikeCool;
    swing_ = 0.12f;
    sys_->apu.noiseBurst(0.22f, 700.f, 0.05f);
    sys_->apu.tone(0, 180.f, 0.05f);
    beep_ = 0.05f;
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE RIDGE HELD UNTIL THE RELIEF BELL";
    fanStep_ = 0;
    fanT_ = 0;
    sys_->apu.keyOff(1);
    sys_->apu.setVol(0, 0.03f);
    sys_->rumble(0.3f, 0.55f, 180);
    sys_->setLight(36, 150, 64);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    shake_ = 0.7f;
    sys_->apu.keyOff(0);
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.48f, 240.f, 0.32f);
    sys_->apu.tone(0, 92.f, 0.1f);
    beep_ = 0.2f;
    sys_->rumble(0.8f, 0.4f, 220);
    sys_->setLight(160, 24, 18);
}

void Game::tickAudio() {
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
    if (mode_ == Mode::Watch && bell_ && !won_) {
        bellTick_ += DT;
        if (bellTick_ >= 0.92f) {
            bellTick_ = 0;
            sys_->apu.keyOn(1, 494.f, 0.22f);
        } else if (bellTick_ >= 0.14f && bellTick_ < 0.14f + DT) {
            sys_->apu.keyOn(1, 740.f, 0.12f);
        }
    }
    if (fanStep_ < 0) return;
    fanT_ += DT;
    static const float notes[] = {392.f, 523.3f, 659.3f, 784.f, 1046.5f};
    if (fanT_ < 0.13f) return;
    if (fanStep_ < 5) sys_->apu.tone(1, notes[fanStep_], 0.08f);
    else sys_->apu.tone(1, 0, 0);
    fanStep_++;
    fanT_ = 0;
    if (fanStep_ > 7) fanStep_ = -1;
}

void Game::update(float dt) {
    watch_ += dt;
    if (!bell_ && watch_ >= kBellAt) {
        bell_ = true;
        bellTick_ = 0.92f;
        sys_->apu.keyOn(1, 494.f, 0.26f);
        sys_->rumble(0.25f, 0.5f, 140);
        sys_->setLight(150, 96, 32);
    }

    while (spawnAt_ < int(script_.size()) && script_[size_t(spawnAt_)].t <= watch_) {
        const Spawn& s = script_[size_t(spawnAt_++)];
        Foe f;
        f.kind = s.kind;
        f.u = s.u;
        f.z = kZSpawn;
        f.age = 0;
        f.flash = 0;
        f.hp = hpOf(s.kind);
        f.points = ptsOf(s.kind);
        f.on = true;
        foes_.push_back(f);
        sys_->apu.tone(2, 146.f, 0.035f);
        beep_ = std::max(beep_, 0.04f);
    }

    int bi = -1;
    float eta = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[size_t(i)].on) continue;
        float e = (foes_[size_t(i)].z - kZLine) / speedOf(foes_[size_t(i)].kind);
        if (e < eta) {
            eta = e;
            bi = i;
        }
    }
    bool close = bi >= 0 && foes_[size_t(bi)].z < 6.0f;
    float travel = std::fabs(u_ - kBellU) / kMove;
    float need = std::max(0.f, kRope - rope_) + (atBell() ? 0.f : travel);
    bool fight = false;
    if (!bell_) fight = bi >= 0;
    else if (close) fight = true;
    else if (bi >= 0 && !(need + 0.4f < eta) && !((kBellEnd - watch_) < need + 0.6f && eta > 0.5f))
        fight = true;
    bool haul = bell_ && !fight && watch_ < kBellEnd;

    float dir = 0;
    bool wantBolt = false;
    bool wantPike = false;
    bool ropeHeld = false;
    if (bot_) {
        float dest = (fight && bi >= 0) ? foes_[size_t(bi)].u : kBellU;
        float du = dest - u_;
        if (!haul || !atBell()) {
            if (du > 0.035f) dir = 1.f;
            else if (du < -0.035f) dir = -1.f;
        }
        if (fight && bi >= 0 && std::fabs(du) <= 0.05f) {
            if (foes_[size_t(bi)].z <= kPikeZ) wantPike = true;
            if (foes_[size_t(bi)].z > 4.6f) wantBolt = true;
        }
        ropeHeld = haul && atBell();
    } else {
        const gs::Pad& pad = sys_->pad;
        ropeHeld = bell_ && atBell() && pad.down(gs::BTN_UP);
        if (!ropeHeld) {
            if (std::fabs(pad.axisX) > 0.16f) dir = std::clamp(pad.axisX, -1.f, 1.f);
            else dir = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
            wantPike = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B);
            wantBolt = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO) || pad.accel > 0.45f;
        }
    }

    if (dir > 0.2f) face_ = 1.f;
    else if (dir < -0.2f) face_ = -1.f;

    if (ropeHeld && !won_) {
        rope_ += dt;
        if (rope_ >= kRope) winWatch();
    } else {
        u_ = std::clamp(u_ + dir * kMove * dt, -0.90f, 0.90f);
        if (rope_ > 0) rope_ = std::max(0.f, rope_ - 0.8f * dt);
    }

    if (!won_ && wantBolt) looseBolt();
    if (!won_ && wantPike) pike();

    if (!won_) {
        for (Bolt& b : bolts_) {
            if (!b.on) continue;
            b.prev = b.z;
            b.z += kBoltV * dt;
            int hit = -1;
            for (int i = 0; i < int(foes_.size()); i++) {
                Foe& f = foes_[size_t(i)];
                if (!f.on) continue;
                if (std::fabs(b.u - f.u) > radOf(f.kind)) continue;
                if (b.prev - 0.05f <= f.z && b.z + 0.05f >= f.z) {
                    if (hit < 0 || f.z < foes_[size_t(hit)].z) hit = i;
                }
            }
            if (hit >= 0) {
                b.on = false;
                hurt(foes_[size_t(hit)]);
            } else if (b.z > kZSpawn + 1.2f) {
                b.on = false;
            }
        }
    }

    if (!won_) {
        for (Foe& f : foes_) {
            if (!f.on) continue;
            f.age += dt;
            if (f.flash > 0) f.flash -= dt;
            f.z -= speedOf(f.kind) * dt;
            if (f.z > kZLine) continue;
            f.on = false;
            puffs_.push_back({f.u, kZLine, 0.4f});
            loseWatch("THE RIDGE FELL");
            break;
        }
    }

    if (!won_ && mode_ == Mode::Watch && watch_ >= kBellEnd) loseWatch("MISSED THE BELL");

    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return !f.on; }), foes_.end());
    bolts_.erase(std::remove_if(bolts_.begin(), bolts_.end(), [](const Bolt& b) { return !b.on; }), bolts_.end());
    for (Puff& p : puffs_) p.t -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0; }), puffs_.end());
    for (Pop& p : pops_) {
        p.t -= dt;
        p.y -= 18.f * dt;
    }
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0; }), pops_.end());
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 2.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
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
    if (w < 6.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 6L, 420L));
    s.h = int16_t(std::max(5L, std::lround(double(w) * 0.16)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = art_.shadow.pick(float(s.h));
    s.pal = 0;
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.f * scale;
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, float(g.h) * scale, pal, false, 0, false);
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
    shx_ = shy_ = 0;
    if (shake_ > 0) {
        shx_ = std::sin(modeT_ * 71.f) * 5.f * shake_;
        shy_ = std::cos(modeT_ * 53.f) * 3.f * shake_;
    }

    float dawn = 0.25f;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause || mode_ == Mode::Victory || mode_ == Mode::Over)
        dawn = std::clamp(watch_ / kBellAt, 0.f, 1.f);
    if (bell_ || mode_ == Mode::Victory) dawn = 1.f;
    uint16_t skyTop = mix(gs::rgb4(1, 2, 6), gs::rgb4(4, 5, 10), dawn);
    uint16_t skyHor = mix(gs::rgb4(7, 4, 6), gs::rgb4(14, 8, 3), dawn * dawn);
    uint16_t valley = mix(gs::rgb4(1, 1, 3), gs::rgb4(3, 2, 2), dawn * 0.5f);
    v.setFogColor(mix(gs::rgb4(2, 2, 5), skyHor, 0.45f));

    int hor = std::clamp(int(std::lround(kHorizon + shy_)), 36, 88);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& r = v.road[y];
        if (y <= hor) {
            float t = float(y) / float(std::max(hor, 1));
            v.lineBackdrop[y] = mix(skyTop, skyHor, t * t);
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y - hor);
        float z = kZScale / std::max(row, 1.f);
        r.on = true;
        r.cx = 160.f + bendAt(row) + shx_;
        r.hw = halfAt(row);
        float drift = (mode_ == Mode::Title || mode_ == Mode::Brief) ? modeT_ * 16.f : 0.f;
        r.v = z * 96.f - drift;
        r.pal = PAL_FIELD;
        r.band = (int(std::floor(z * 1.7f)) & 1) ? 1 : 0;
        r.style = gs::ROAD_ROCKY;
        r.left = r.right = gs::GROUND_DROP;
        v.lineFog[y] = uint8_t(std::clamp(int(10.f - row * 0.1f), 0, 9));
        v.lineBackdrop[y] = mix(valley, gs::rgb4(0, 0, 1), std::clamp(row / 110.f, 0.f, 1.f));
    }

    if (mode_ == Mode::Title) text("S3 RIDGE RELIEF", 160, 32, 0.92f, PAL_AMBER);
    else if (mode_ == Mode::Brief) text("THE WATCH", 160, 36, 1.05f, PAL_AMBER);
    else if (mode_ == Mode::Victory) text("ANSWERED", 160, 36, 1.05f, PAL_GREEN);
    else if (mode_ == Mode::Over) text(reason_, 160, 40, 0.78f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSE", 160, 40, 1.1f, PAL_HUD);
    for (const Pop& p : pops_) text("+" + std::to_string(p.pts), p.x, p.y, 0.55f, PAL_AMBER);
    if ((mode_ == Mode::Watch || mode_ == Mode::Pause) && bell_ && !atBell() && !won_) {
        Spot tag = spot(kBellU, 3.35f, 118.f);
        text("BELL", tag.x, tag.y - tag.h - 6.f, 0.62f, PAL_AMBER);
    }

    auto drawFoe = [&](const Foe& f) {
        Spot s = spot(f.u, f.z, tallOf(f.kind));
        int frame = int(f.age * (f.kind == Scout ? 11.f : 7.f)) & 1;
        if (s.fog < 5) shadow(s.x, s.y, s.h * 0.38f);
        spr(foeImg(f.kind, frame), s.x, s.y, s.h, palOf(f.kind), f.u > 0, s.fog, true);
        if (f.flash > 0) spr(art_.glint, s.x, s.y - s.h * 0.55f, s.h * 0.28f, PAL_FX, false, 0, false);
    };

    int fr = (mode_ == Mode::Title || mode_ == Mode::Watch) ? int(modeT_ * 8.f) & 1 : 0;
    Spot you = spot(u_, kZLine, 86.f);
    float lunge = swing_ > 0 ? face_ * 8.f : 0.f;
    spr(art_.sentry[fr & 1], you.x + lunge, you.y, swing_ > 0 ? 80.f : 88.f, PAL_SENTRY, face_ < 0, 0, true);
    shadow(you.x, you.y, 36.f);
    if (flash_ > 0)
        spr(art_.glint, you.x + face_ * 16.f, you.y - 58.f, 16.f, PAL_FX, false, 0, false);

    Spot post = spot(kBellU, 3.35f, 118.f);
    float swing = std::sin(modeT_ * (bell_ ? 9.5f : 2.2f)) * (bell_ || rope_ > 0.05f ? 9.f : 3.f);
    spr(art_.yoke, post.x, post.y, post.h, PAL_STONE, false, post.fog, true);
    float bellH = post.h * 0.38f;
    float bellY = post.y - post.h * 0.72f;
    spr(art_.bell, post.x + swing, bellY, bellH, PAL_BELL, false, 0, false);
    spr(art_.rope, post.x + swing * 0.35f, bellY + bellH * 0.85f, 28.f + rope_ * 18.f, PAL_STONE, false, 0, false);
    Spot pad = spot(kBellU, 2.85f, 22.f);
    spr(art_.pad, pad.x, pad.y, pad.h, PAL_STONE, false, 0, true);

    for (const Bolt& b : bolts_) {
        Spot s = spot(b.u, b.z, 28.f);
        spr(art_.bolt, s.x, s.y, std::max(10.f, s.h), PAL_FX, false, s.fog, true);
    }
    for (const Puff& p : puffs_) {
        float k = std::clamp(p.t / 0.4f, 0.f, 1.f);
        Spot s = spot(p.u, p.z, 34.f);
        spr(art_.dust, s.x, s.y - s.h * 0.2f, 18.f + (1.f - k) * 22.f, PAL_FX, false, int((1.f - k) * 6.f), false);
    }
    std::vector<int> near, far;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[size_t(i)].on) continue;
        if (foes_[size_t(i)].z < 4.2f) near.push_back(i);
        else far.push_back(i);
    }
    auto byZ = [&](int a, int b) { return foes_[size_t(a)].z < foes_[size_t(b)].z; };
    std::sort(near.begin(), near.end(), byZ);
    std::sort(far.begin(), far.end(), byZ);
    for (int i : near) drawFoe(foes_[size_t(i)]);

    auto prop = [&](float u, float z, float base, const gs::Mipped& img, int pal) {
        Spot s = spot(u, z, base);
        spr(img, s.x, s.y, s.h, pal, u < 0, std::max(0, s.fog - 1), true);
    };
    prop(0.86f, 6.2f, 52.f, art_.cairn, PAL_STONE);
    prop(-0.82f, 8.4f, 46.f, art_.cairn, PAL_STONE);
    prop(0.78f, 11.5f, 70.f, art_.banner, PAL_SKIRM);
    prop(0.7f, 11.2f, 40.f, art_.cairn, PAL_STONE);

    for (int i : far) drawFoe(foes_[size_t(i)]);

    if (bell_ || mode_ == Mode::Victory) {
        float march = std::max(0.f, watch_ - kBellAt) * 0.22f;
        const float us[3] = {-0.52f, -0.30f, -0.08f};
        for (int i = 0; i < 3; i++) {
            float z = std::max(9.5f, 15.2f - march - i * 0.8f);
            Spot s = spot(us[i], z, 80.f);
            int frame = int(modeT_ * 6.f + i) & 1;
            spr(art_.sentry[frame], s.x, s.y, s.h, PAL_RELIEF, false, std::max(2, s.fog), true);
            spr(art_.lantern, s.x + s.h * 0.16f, s.y - s.h * 0.42f, s.h * 0.22f, PAL_FX, false, s.fog, false);
        }
    }

    if (mode_ == Mode::Title || mode_ == Mode::Brief) {
        const float us[3] = {-0.42f, 0.12f, 0.58f};
        const int ks[3] = {Skirm, Scout, Porter};
        for (int i = 0; i < 3; i++) {
            float z = 7.5f + i * 2.6f;
            Spot s = spot(us[i], z, tallOf(ks[i]));
            spr(foeImg(ks[i], int(modeT_ * 7.f + i) & 1), s.x, s.y, s.h, palOf(ks[i]), us[i] > 0, s.fog, true);
        }
    }

    spr(art_.peakL, 58.f + shx_ * 0.25f, kHorizon + shy_ + 6.f, 64.f, PAL_MOUNT, false, 2, true);
    spr(art_.peakR, 262.f + shx_ * 0.25f, kHorizon + shy_ + 10.f, 54.f, PAL_MOUNT, false, 3, true);
    float driftC = std::fmod(modeT_ * 10.f, 380.f);
    spr(art_.cloud, driftC - 40.f, 20.f, 16.f, PAL_HUD, false, 3, false);
    spr(art_.cloud, std::fmod(driftC + 190.f, 380.f) - 30.f, 34.f, 12.f, PAL_HUD, true, 4, false);
    int moonPal = (bell_ || mode_ == Mode::Victory) ? PAL_AMBER : PAL_MOUNT;
    spr(art_.moon, bell_ ? 118.f : 214.f, bell_ ? 28.f : 22.f, bell_ ? 20.f : 26.f, moonPal, false, 0, false);
    if (!bell_ && mode_ != Mode::Victory) {
        const float sx[8] = {18, 46, 78, 104, 150, 188, 250, 292};
        const float sy[8] = {12, 28, 16, 40, 14, 26, 12, 34};
        for (int i = 0; i < 8; i++) spr(art_.star, sx[i], sy[i], 5.f, PAL_HUD, false, 0, false);
    }

    char buf[48];
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        if (bell_) std::snprintf(buf, sizeof buf, "ANSWER");
        else std::snprintf(buf, sizeof buf, "BELL %d", int(std::ceil(std::max(0.f, kBellAt - watch_))));
        hud(1, 1, buf, bell_ ? PAL_AMBER : PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(39 - int(std::strlen(buf)), 1, buf, PAL_AMBER);
        const Foe* threat = nullptr;
        for (const Foe& f : foes_) {
            if (!f.on) continue;
            if (!threat || f.z < threat->z) threat = &f;
        }
        if (threat && std::fabs(threat->u - u_) > 0.22f && threat->z < 7.f)
            hudC(3, "ON THE SPINE", PAL_RED);
        else if (bell_ && !atBell())
            hudC(3, "GET TO THE BELL", (int(modeT_ * 5.f) & 1) ? PAL_AMBER : PAL_HUD);
        else if (bell_ && atBell() && rope_ < kRope)
            hudC(3, "HOLD UP", PAL_AMBER);
        if (bell_ || rope_ > 0.02f) {
            int n = std::clamp(int(std::lround(rope_ / kRope * 10.f)), 0, 10);
            std::string meter = "ROPE ";
            meter.append(size_t(n), '#');
            meter.append(size_t(10 - n), '.');
            hud(1, 26, meter, rope_ > 0 ? PAL_AMBER : PAL_HUD);
        } else {
            hud(1, 26, "HOLD THE RIDGE", PAL_HUD);
        }
    }

    if (mode_ == Mode::Title) {
        hudC(8, "HOLD UNTIL THE RELIEF BELL", PAL_AMBER);
        hudC(10, "ANYTHING ELSE IS A LOSS", PAL_HUD);
        hudC(16, "ARROWS MOVE   Z BOLT   X PIKE", PAL_HUD);
        hudC(18, "THE BELL STANDS ON THE LEFT", PAL_HUD);
        hudC(20, "UP HAULS THE ROPE", PAL_AMBER);
        if ((int(modeT_ * 2.f) & 1) == 0) hudC(23, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Brief) {
        hudC(12, "THEY COME DOWN THE SPINE", PAL_HUD);
        hudC(14, "HOLD UNTIL THE BELL RINGS", PAL_AMBER);
        hudC(16, "THEN HAUL THE ROPE", PAL_HUD);
        hudC(18, "MISS IT AND THE WATCH IS LOST", PAL_RED);
        if ((int(modeT_ * 2.f) & 1) == 0) hudC(22, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(16, "START  RESUME", PAL_HUD);
        hudC(18, "ESC    TITLE", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(12, "THE RIDGE HELD", PAL_GREEN);
        hudC(14, "UNTIL THE RELIEF BELL", PAL_AMBER);
        std::snprintf(buf, sizeof buf, "STOPPED %d", stopped_);
        hudC(17, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(19, buf, PAL_AMBER);
        hudC(23, "START", PAL_HUD);
    } else if (mode_ == Mode::Over) {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(14, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "STOPPED %d", stopped_);
        hudC(16, buf, PAL_HUD);
        hudC(20, "START", PAL_HUD);
    }

    if (mode_ == Mode::Title) sys_->setLight(36, 28, 64);
    else if (mode_ == Mode::Victory) sys_->setLight(36, 150, 64);
    else if (mode_ == Mode::Over) sys_->setLight(160, 24, 18);
    else if (bell_) sys_->setLight(150, 96, 32);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.86f);
    sys.apu.setEcho(0.18f, 0.3f, 0.15f);
    sys.apu.setPatch(0, windPatch());
    sys.apu.setPatch(1, bellPatch());
    u_ = 0;
    if (bot_) beginWatch();
    else {
        mode_ = Mode::Title;
        sys.setLight(36, 28, 64);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    modeT_ += DT;
    if (fireCd_ > 0) fireCd_ -= DT;
    if (pikeCd_ > 0) pikeCd_ -= DT;
    if (flash_ > 0) flash_ -= DT;
    if (swing_ > 0) swing_ -= DT;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT);

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        u_ = std::sin(modeT_ * 0.75f) * 0.4f;
        face_ = std::cos(modeT_ * 0.75f) >= 0 ? 1.f : -1.f;
        if (!bot_ && pad.pressed(gs::BTN_START)) beginWatch();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Brief) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Watch;
            modeT_ = 0;
            sys.apu.tone(0, 523.f, 0.05f);
            beep_ = 0.05f;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            modeT_ = 0;
            sys.apu.silence();
            sys.apu.setPatch(0, windPatch());
            sys.apu.setPatch(1, bellPatch());
        }
    } else if (mode_ == Mode::Watch) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            modeT_ = 0;
        } else {
            update(DT);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Watch;
            modeT_ = 0;
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            modeT_ = 0;
            bell_ = false;
            foes_.clear();
            bolts_.clear();
            sys.apu.silence();
            sys.apu.setPatch(0, windPatch());
            sys.apu.setPatch(1, bellPatch());
        }
    } else if (mode_ == Mode::Victory || mode_ == Mode::Over) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
            mode_ = Mode::Title;
            modeT_ = 0;
            over_ = false;
            won_ = false;
            bell_ = false;
            foes_.clear();
            bolts_.clear();
            pops_.clear();
            sys.apu.silence();
            sys.apu.setPatch(0, windPatch());
            sys.apu.setPatch(1, bellPatch());
        }
    }

    tickAudio();
    draw();
}

}  // namespace reli
