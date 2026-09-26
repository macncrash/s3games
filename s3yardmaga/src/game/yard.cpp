#include "game/yard.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace yardmaga {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FOCAL = 200.f;
constexpr float HORIZON = 98.f;
constexpr float EYE = 1.30f;
constexpr float MAN_H = 1.72f;
constexpr float FALL_H = 0.48f;
constexpr float BODY_Y = 1.05f;
constexpr float SPAWN_Z = 18.f;
constexpr float LIGHT_Z = 9.0f;
constexpr float SILL_Z = 3.25f;
constexpr float PEEL_Z = 11.5f;
constexpr float WALK = 2.42f;
constexpr float PEEL_X = 3.6f;
constexpr float RAID_LEN = 28.f;
constexpr float BOLT = 0.36f;
constexpr float YARD_HALF = 4.4f;
constexpr int MAG = 6;

struct Plan {
    float t;
    float x;
    int cutter;
};

const Plan kPlan[] = {
    {0.70f, -1.05f, 1}, {3.10f, 1.50f, 0},  {5.60f, 0.95f, 1},  {8.40f, -1.55f, 0},
    {10.50f, -0.45f, 1}, {13.40f, 0.70f, 0}, {15.40f, 1.15f, 1}, {20.30f, -1.10f, 1},
};
constexpr int NPLAN = int(sizeof kPlan / sizeof kPlan[0]);

enum PropKind { SHED, BOXCAR, CRANE, TOWER, PALLET, DRUM, LAMP };

struct Prop {
    float x, z, h;
    int kind;
    bool flip;
};

const Prop kProps[] = {
    {-4.9f, 12.2f, 3.5f, SHED, false},
    {-4.3f, 16.0f, 5.6f, CRANE, false},
    {-3.7f, 7.6f, 1.55f, PALLET, false},
    {-3.4f, 5.6f, 1.15f, PALLET, true},
    {5.0f, 11.0f, 2.7f, BOXCAR, true},
    {4.5f, 15.4f, 5.3f, TOWER, false},
    {3.5f, 6.5f, 0.95f, DRUM, false},
    {-3.3f, 4.7f, 0.9f, DRUM, true},
    {-2.85f, 9.2f, 3.05f, LAMP, false},
    {2.85f, 9.05f, 3.05f, LAMP, true},
    {3.6f, 4.9f, 1.05f, PALLET, false},
};

const float kStars[][2] = {{28, 16}, {54, 28}, {96, 12}, {140, 22}, {188, 14}, {250, 26}, {300, 18}, {210, 36}};

uint16_t lerp4(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t + 0.5f), int(ag + (bg - ag) * t + 0.5f), int(ab + (bb - ab) * t + 0.5f));
}

gs::FMPatch crackPatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.16f;
    p.op[0] = {1.0f, 1.0f, 0.001f, 0.04f, 0.0f, 0.03f};
    p.op[1] = {2.7f, 0.28f, 0.001f, 0.03f, 0.0f, 0.02f};
    p.op[2] = {0.5f, 0.7f, 0.001f, 0.07f, 0.0f, 0.04f};
    p.op[3] = {3.6f, 0.16f, 0.001f, 0.03f, 0.0f, 0.02f};
    p.vol = 0.26f;
    p.drive = 0.4f;
    p.tone = 1600;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.18f;
    p.op[0] = {1, 1, 0.01f, 0.16f, 0.4f, 0.2f};
    p.op[1] = {2, 0.3f, 0.012f, 0.18f, 0.28f, 0.18f};
    p.op[2] = {3, 0.14f, 0.02f, 0.2f, 0.18f, 0.18f};
    p.op[3] = {1, 0.25f, 0.01f, 0.18f, 0.3f, 0.2f};
    p.vol = 0.15f;
    return p;
}

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.28f;
    p.op[0] = {1, 0.55f, 0.35f, 0.7f, 0.8f, 0.45f};
    p.op[1] = {2, 0.2f, 0.3f, 0.65f, 0.65f, 0.4f};
    p.op[2] = {0.5f, 0.35f, 0.45f, 0.75f, 0.75f, 0.45f};
    p.op[3] = {1, 0.16f, 0.35f, 0.55f, 0.5f, 0.35f};
    p.vol = 0.035f;
    p.tone = 380;
    return p;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.14f, 0.18f, 0.1f);
    sys.apu.setPatch(0, crackPatch());
    sys.apu.setPatch(1, hornPatch());
    sys.apu.setPatch(2, dronePatch());
    rounds_ = MAG;
    aimX_ = 160;
    aimY_ = 108;
    if (bot_) beginRaid();
    else {
        mode_ = Mode::Title;
        sys.setLight(90, 48, 16);
    }
}

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "MAGAZINE SPENT";
    if (fail_ == Fail::Sill) return "THEY REACHED THE SILL";
    if (fail_ == Fail::Hand) return "A HAND IS DOWN";
    return "THE WATCH IS OVER";
}

int Game::marker() const {
    if (over_) return 2;
    if (mode_ == Mode::Raid || mode_ == Mode::Pause) return 1;
    return 0;
}

std::string Game::trace() const {
    char b[128];
    std::snprintf(b, sizeof b, "trace t=%.2f rounds=%d stopped=%d next=%d men=%zu\n", raidT_, rounds_, stopped_, next_,
                  men_.size());
    std::string s = b;
    for (const Man& m : men_) {
        const char* ph = "W";
        if (m.phase == Phase::Peel) ph = "P";
        else if (m.phase == Phase::Clear) ph = "C";
        else if (m.phase == Phase::Dead) ph = "D";
        std::snprintf(b, sizeof b, "  %s %s z=%.2f x=%.2f\n", m.kind == Kind::Cutter ? "cut" : "hand", ph, m.z, m.x);
        s += b;
    }
    return s;
}

void Game::beginRaid() {
    mode_ = Mode::Raid;
    over_ = false;
    won_ = false;
    fail_ = Fail::None;
    wantFire_ = false;
    rounds_ = MAG;
    stopped_ = 0;
    next_ = 0;
    fanStep_ = -1;
    raidT_ = 0;
    bolt_ = 0.2f;
    kick_ = flash_ = shake_ = fanT_ = 0;
    aimX_ = 160;
    aimY_ = 108;
    men_.clear();
    men_.reserve(12);
    casings_.clear();
    sparks_.clear();
    sys_->apu.keyOff(2);
    sys_->apu.keyOn(1, 196.f, 0.1f);
    sys_->apu.keyOn(2, 46.f, 0.03f);
    sys_->setLight(180, 90, 24);
}

void Game::lose(Fail why) {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    fail_ = why;
    shake_ = why == Fail::Spent ? 0.5f : 1.1f;
    fanStep_ = -1;
    sys_->apu.keyOff(2);
    sys_->apu.keyOn(1, why == Fail::Hand ? 98.f : why == Fail::Sill ? 74.f : 110.f, 0.22f);
    sys_->apu.noiseBurst(0.34f, 200.f, 0.22f);
    sys_->rumble(0.55f, 0.25f, 150);
    sys_->setLight(180, 24, 18);
}

void Game::win() {
    if (mode_ != Mode::Raid) return;
    if (rounds_ <= 0) {
        lose(Fail::Spent);
        return;
    }
    mode_ = Mode::Won;
    over_ = true;
    won_ = true;
    fanStep_ = 0;
    fanT_ = 0;
    sys_->apu.keyOff(2);
    sys_->apu.noiseBurst(0.18f, 120.f, 0.1f);
    sys_->setLight(36, 170, 64);
}

void Game::kill(Man& m) {
    if (m.phase == Phase::Dead || m.phase == Phase::Clear) return;
    if (m.kind == Kind::Cutter) ++stopped_;
    m.phase = Phase::Dead;
    m.dead = 0;
}

Game::Man* Game::acquire(bool cuttersOnly) {
    Man* kill = nullptr;
    Man* hand = nullptr;
    float killD = 1e9f, handD = 1e9f;
    for (Man& m : men_) {
        if (m.phase != Phase::Walk && m.phase != Phase::Peel) continue;
        bool canKill = m.kind == Kind::Cutter && m.phase == Phase::Walk && m.z <= LIGHT_Z && m.z > SILL_Z;
        bool isHand = m.kind == Kind::Hand;
        if (!canKill && !isHand) continue;
        if (cuttersOnly && !canKill) continue;
        float sx, sy, s;
        world(m.x, BODY_Y, m.z, sx, sy, s);
        float pixH = MAN_H * s;
        float rad = std::clamp(std::max(14.f, pixH * 0.38f), 14.f, 36.f);
        float d = std::hypot(aimX_ - sx, aimY_ - sy);
        if (d > rad) continue;
        if (canKill && d < killD) {
            killD = d;
            kill = &m;
        } else if (isHand && d < handD) {
            handD = d;
            hand = &m;
        }
    }
    if (cuttersOnly) return kill;
    if (kill && (!hand || killD <= handD + 8.f)) return kill;
    return hand;
}

void Game::pull() {
    if (mode_ != Mode::Raid || rounds_ <= 0 || bolt_ > 0) return;
    Man* hit = acquire(false);
    bool canKill = hit && hit->kind == Kind::Cutter && hit->phase == Phase::Walk && hit->z <= LIGHT_Z && hit->z > SILL_Z;
    bool onHand = hit && hit->kind == Kind::Hand;
    float hx = aimX_, hy = aimY_;
    if (hit) {
        float sx, sy, s;
        world(hit->x, BODY_Y, hit->z, sx, sy, s);
        hx = sx;
        hy = sy;
    }
    --rounds_;
    bolt_ = BOLT;
    kick_ = 1.f;
    flash_ = 0.06f;
    flashX_ = hx;
    flashY_ = hy;
    shake_ = canKill ? 0.42f : 0.22f;
    Casing c;
    c.x = 278;
    c.y = 162;
    c.vx = -40;
    c.vy = -52;
    c.t = 0.42f;
    casings_.push_back(c);
    if (canKill) {
        kill(*hit);
        sparks_.push_back({hx, hy, 0.16f});
        sys_->apu.keyOn(0, 104.f, 0.32f);
        sys_->apu.noiseBurst(0.5f, 1300.f, 0.07f);
        sys_->rumble(0.35f, 0.65f, 55);
    } else if (onHand) {
        sparks_.push_back({hx, hy, 0.18f});
        sys_->apu.keyOn(0, 90.f, 0.28f);
        sys_->apu.noiseBurst(0.4f, 700.f, 0.1f);
        lose(Fail::Hand);
        return;
    } else {
        sparks_.push_back({aimX_, aimY_, 0.1f});
        sys_->apu.keyOn(0, 168.f, 0.14f);
        sys_->apu.noiseBurst(0.2f, 3800.f, 0.04f);
    }
    if (!bot_) aimY_ = std::max(70.f, aimY_ - 5.f);
    if (rounds_ <= 0) lose(Fail::Spent);
}

void Game::humanAim(float dt) {
    const gs::Pad& pad = sys_->pad;
    float ax = 0, ay = 0;
    if (std::fabs(pad.axisX) > 0.18f) ax = pad.axisX;
    if (std::fabs(pad.axisY) > 0.18f) ay = -pad.axisY;
    if (pad.down(gs::BTN_LEFT)) ax -= 1;
    if (pad.down(gs::BTN_RIGHT)) ax += 1;
    if (pad.down(gs::BTN_UP)) ay -= 1;
    if (pad.down(gs::BTN_DOWN)) ay += 1;
    float m = std::hypot(ax, ay);
    if (m > 1.f) {
        ax /= m;
        ay /= m;
    }
    aimX_ += ax * 280.f * dt;
    aimY_ += ay * 220.f * dt;
    aimX_ = std::clamp(aimX_, 36.f, 284.f);
    aimY_ = std::clamp(aimY_, 64.f, 176.f);
}

void Game::botAct(float dt) {
    Man* threat = nullptr;
    for (Man& m : men_) {
        if (m.kind != Kind::Cutter || m.phase != Phase::Walk) continue;
        if (m.z <= SILL_Z) continue;
        if (!threat || m.z < threat->z) threat = &m;
    }
    if (!threat) return;
    float sx, sy, s;
    world(threat->x, BODY_Y, threat->z, sx, sy, s);
    float dx = sx - aimX_, dy = sy - aimY_;
    float dist = std::hypot(dx, dy);
    float step = 2200.f * dt;
    if (threat->z < 5.4f || dist <= step) {
        aimX_ = sx;
        aimY_ = sy;
        dist = 0;
    } else if (dist > 0.001f) {
        aimX_ += dx / dist * step;
        aimY_ += dy / dist * step;
        dist -= step;
    }
    if (bolt_ > 0 || threat->z > LIGHT_Z - 0.02f) return;
    if (dist > 7.f && threat->z > 5.0f) return;
    aimX_ = sx;
    aimY_ = sy;
    if (acquire(true) == threat) pull();
}

void Game::updateRaid(float dt) {
    if (bolt_ > 0) bolt_ -= dt;
    while (next_ < NPLAN && raidT_ >= kPlan[next_].t) {
        const Plan& p = kPlan[next_++];
        Man m;
        m.kind = p.cutter ? Kind::Cutter : Kind::Hand;
        m.x = p.x;
        m.z = SPAWN_Z;
        m.phase = Phase::Walk;
        men_.push_back(m);
    }

    for (Man& m : men_) {
        if (m.phase == Phase::Dead) {
            m.dead += dt;
            continue;
        }
        if (m.phase == Phase::Clear) continue;
        if (m.kind == Kind::Cutter) {
            m.z -= WALK * dt;
            m.walk += WALK * dt;
            m.x += std::sin(m.walk * 3.1f) * 0.004f;
            if (m.z < SILL_Z) {
                lose(Fail::Sill);
                return;
            }
        } else if (m.phase == Phase::Walk) {
            m.z -= WALK * dt;
            m.walk += WALK * dt;
            if (m.z <= PEEL_Z) m.phase = Phase::Peel;
        } else {
            float side = m.x < 0.f ? -1.f : 1.f;
            m.x += side * PEEL_X * dt;
            m.z -= 0.35f * dt;
            m.walk += dt;
            if (std::fabs(m.x) > 5.1f || m.z < 6.2f) m.phase = Phase::Clear;
        }
    }
    if (mode_ != Mode::Raid) return;

    for (Man& m : men_) {
        if (m.kind == Kind::Cutter && m.phase == Phase::Walk && !m.warned && m.z <= LIGHT_Z) {
            m.warned = true;
            sys_->apu.keyOn(1, 174.f, 0.08f);
        }
    }

    men_.erase(std::remove_if(men_.begin(), men_.end(),
                              [](const Man& m) {
                                  return m.phase == Phase::Clear || (m.phase == Phase::Dead && m.dead > 4.5f);
                              }),
               men_.end());

    if (bot_) botAct(dt);
    else if (wantFire_ && bolt_ <= 0) {
        wantFire_ = false;
        pull();
    }
    if (mode_ != Mode::Raid) return;

    raidT_ += dt;
    if (raidT_ >= RAID_LEN && rounds_ > 0) win();
}

void Game::tickFx(float dt) {
    if (flash_ > 0) flash_ -= dt;
    if (kick_ > 0) kick_ = std::max(0.f, kick_ - dt * 3.5f);
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt * 2.5f);
    for (Spark& s : sparks_) s.t -= dt;
    sparks_.erase(std::remove_if(sparks_.begin(), sparks_.end(), [](const Spark& s) { return s.t <= 0; }), sparks_.end());
    for (Casing& c : casings_) {
        c.t -= dt;
        c.vy += 240.f * dt;
        c.x += c.vx * dt;
        c.y += c.vy * dt;
    }
    casings_.erase(std::remove_if(casings_.begin(), casings_.end(), [](const Casing& c) { return c.t <= 0; }),
                   casings_.end());
    if (casings_.size() > 6) casings_.erase(casings_.begin(), casings_.end() - 6);
}

bool Game::fireEdge() const {
    const gs::Pad& pad = sys_->pad;
    return pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_X) ||
           pad.pressed(gs::BTN_Y) || pad.pressed(gs::BTN_Z) || pad.pressed(gs::BTN_TURBO);
}

const char* Game::hint() const {
    const Man* cut = nullptr;
    const Man* hand = nullptr;
    for (const Man& m : men_) {
        if (m.phase == Phase::Dead || m.phase == Phase::Clear) continue;
        if (m.kind == Kind::Cutter && m.phase == Phase::Walk) {
            if (!cut || m.z < cut->z) cut = &m;
        } else if (m.kind == Kind::Hand && (!hand || m.z < hand->z)) {
            hand = &m;
        }
    }
    if (cut && cut->z < SILL_Z + 1.35f) return "THE SILL";
    if (cut && cut->z <= LIGHT_Z) return "IN THE LIGHT";
    if (hand && hand->z < 14.f && (!cut || hand->z < cut->z)) return "LET THE HAND PASS";
    if (cut && cut->z < 13.f) return "WAIT FOR THE LIGHT";
    return "HOLD THE YARD";
}

int Game::hintPal() const {
    const char* h = hint();
    if (h[0] == 'T' || h[0] == 'A') return PAL_RED;
    if (h[0] == 'L') return PAL_GREEN;
    if (h[0] == 'I') return PAL_AMBER;
    return PAL_HUD;
}

float Game::bendPx(float z) const { return std::sin(z * 0.07f) * 14.f; }

int Game::fogFor(float z) const { return std::clamp(int((z - 6.f) * 0.75f), 0, 12); }

void Game::world(float x, float y, float z, float& sx, float& sy, float& s) const {
    float zz = std::max(0.55f, z);
    s = FOCAL / zz;
    sx = 160.f + x * s + bendPx(z);
    sy = HORIZON + (EYE - y) * s;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (h < 1.1f || m.h < 1) return;
    cx += shx_;
    cy += shy_;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -80) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    cx += shx_;
    cy += shy_;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    auto adv = [&](unsigned char c) {
        if (c <= 32 || c >= 128) return 9.f * scale;
        const gs::Mipped& g = art_.glyph[c - 32];
        if (g.w < 1) return 9.f * scale;
        return float(g.w) * scale;
    };
    float width = 0;
    for (unsigned char c : s) width += adv(c);
    x -= width * 0.5f;
    for (unsigned char c : s) {
        float a = adv(c);
        if (c > 32 && c < 128) {
            const gs::Mipped& g = art_.glyph[c - 32];
            if (g.h > 0) spr(g, x + a * 0.5f, y, float(g.h) * scale, pal, false, 0, false);
        }
        x += a;
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

void Game::hudC(int row, const std::string& s, int pal) {
    int col = 20 - int(s.size()) / 2;
    hud(col, row, s, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    shx_ = shy_ = 0;
    if (shake_ > 0) {
        shx_ = std::sin(t_ * 91.f) * 3.0f * std::min(shake_, 1.f);
        shy_ = std::cos(t_ * 73.f) * 1.6f * std::min(shake_, 1.f);
    }

    uint16_t skyTop = gs::rgb4(1, 1, 4);
    uint16_t skyMid = gs::rgb4(4, 3, 8);
    uint16_t skyHor = gs::rgb4(14, 7, 3);
    if (mode_ == Mode::Lost) skyHor = gs::rgb4(12, 3, 2);
    else if (mode_ == Mode::Won) skyHor = gs::rgb4(13, 9, 4);
    v.setFogColor(mode_ == Mode::Lost ? gs::rgb4(8, 3, 2) : gs::rgb4(8, 4, 2));
    const int horizon = int(HORIZON);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& rd = v.road[y];
        if (y < horizon) {
            rd.on = false;
            float u = float(y) / float(horizon);
            v.lineBackdrop[y] = u < 0.55f ? lerp4(skyTop, skyMid, u / 0.55f) : lerp4(skyMid, skyHor, (u - 0.55f) / 0.45f);
            v.lineFog[y] = 0;
            continue;
        }
        float row = std::max(float(y) + 0.5f - float(horizon), 1.f);
        float z = EYE * FOCAL / row;
        rd.on = true;
        rd.cx = 160.f + bendPx(z) + shx_;
        rd.hw = std::max(4.f, YARD_HALF * row / EYE);
        rd.v = z * 42.f;
        rd.pal = uint8_t(PAL_YARD);
        rd.style = gs::ROAD_RUTS;
        rd.band = (int(std::floor(z * 0.42f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((z - 8.f) / 16.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 11.f);
        v.lineBackdrop[y] = gs::rgb4(4, 3, 2);
    }

    if (mode_ == Mode::Title) {
        text("S3 YARD MAGA", 160, 22, 0.68f, PAL_AMBER);
        text("MAKE THE MAGAZINE LAST", 160, 40, 0.44f, PAL_HUD);
        text("LONGER THAN THE RAID", 160, 52, 0.44f, PAL_HUD);
        text("CUTTERS IN THE LIGHT", 160, 68, 0.42f, PAL_GREEN);
        text("A HAND ENDS THE WATCH", 160, 80, 0.42f, PAL_RED);
    } else if (mode_ == Mode::Won) {
        text("MAGAZINE HELD", 160, 28, 0.9f, PAL_GREEN);
        text("IT OUTLASTS THE RAID", 160, 48, 0.48f, PAL_AMBER);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160, 28, 0.62f, PAL_RED);
        const char* why = "MAGAZINE SPENT";
        if (fail_ == Fail::Sill) why = "THEY REACHED THE SILL";
        else if (fail_ == Fail::Hand) why = "A HAND IS DOWN";
        text(why, 160, 48, 0.46f, PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 36, 0.9f, PAL_HUD);
    } else if (mode_ == Mode::Raid) {
        text(hint(), 160, 28, 0.48f, hintPal());
    }

    if (mode_ == Mode::Title || mode_ == Mode::Raid || mode_ == Mode::Pause) spr(art_.sight, aimX_, aimY_, 20, PAL_FX);

    for (const Spark& s : sparks_) spr(art_.spark, s.x, s.y, 8.f + s.t * 30.f, PAL_FX);
    if (flash_ > 0) spr(art_.flash, flashX_, flashY_, 14.f + (0.06f - flash_) * 70.f, PAL_FX);

    for (int i = 0; i < MAG; ++i) {
        float x = 118.f + float(i) * 14.f;
        bool live = i < rounds_;
        spr(live ? art_.round : art_.spent, x, 198, live ? 18.f : 14.f, PAL_FX);
    }
    for (const Casing& c : casings_) spr(art_.round, c.x, c.y, 8, PAL_FX);

    float rifleX = 286.f + (aimX_ - 160.f) * 0.04f;
    float rifleY = 226.f - kick_ * 12.f;
    if (flash_ > 0) spr(art_.flash, rifleX - 4.f, rifleY - 58.f, 16, PAL_FX);
    spr(art_.rifle, rifleX, rifleY, 72, PAL_IRON, false, 0, true);

    spr(art_.jamb, 14, 230, 200, PAL_IRON, false, 0, true);
    spr(art_.jamb, 306, 230, 200, PAL_IRON, true, 0, true);
    sprBox(art_.sill, 160, 214, 300, 18, PAL_WOOD);

    struct Blob {
        float z, x, y, h, w;
        const gs::Mipped* img;
        int pal, fog;
        bool flip, feet, box, shadow;
    };
    std::vector<Blob> blobs;
    auto add = [&](const gs::Mipped& img, float x, float y, float z, float h, float w, int pal, bool flip, bool feet,
                   bool box, bool shadow) {
        Blob b;
        b.z = z;
        b.x = x;
        b.y = y;
        b.h = h;
        b.w = w;
        b.img = &img;
        b.pal = pal;
        b.fog = fogFor(z);
        b.flip = flip;
        b.feet = feet;
        b.box = box;
        b.shadow = shadow;
        blobs.push_back(b);
    };

    for (const Prop& p : kProps) {
        const gs::Mipped* img = &art_.pallet;
        int pal = PAL_WOOD;
        if (p.kind == SHED) {
            img = &art_.shed;
            pal = PAL_BRICK;
        } else if (p.kind == BOXCAR) {
            img = &art_.boxcar;
            pal = PAL_BRICK;
        } else if (p.kind == CRANE) {
            img = &art_.crane;
            pal = PAL_IRON;
        } else if (p.kind == TOWER) {
            img = &art_.tower;
            pal = PAL_IRON;
        } else if (p.kind == DRUM) {
            img = &art_.drum;
            pal = PAL_IRON;
        } else if (p.kind == LAMP) {
            img = &art_.lamp;
            pal = PAL_IRON;
        }
        add(*img, p.x, 0, p.z, p.h, 0, pal, p.flip, true, false, false);
        if (p.kind == LAMP) {
            float pulse = 0.42f + 0.08f * std::sin(t_ * 11.f + p.x);
            add(art_.glow, p.x, p.h * 0.9f, p.z - 0.04f, pulse, 0, PAL_FX, false, false, false, false);
        }
    }

    add(art_.pool, 0.05f, 0, 7.5f, 0.42f, 5.4f, PAL_FX, false, true, true, false);
    add(art_.chevron, 0, 0, LIGHT_Z, 0.16f, 5.6f, PAL_AMBER, false, true, true, false);

    auto addMan = [&](Kind kind, Phase phase, float x, float z, float walk) {
        bool dead = phase == Phase::Dead;
        bool hand = kind == Kind::Hand;
        int fr = int(walk * 2.6f) & 1;
        const gs::Mipped& body = dead ? art_.fallen : (hand ? art_.hand[fr] : art_.cutter[fr]);
        float h = dead ? FALL_H : MAN_H;
        float bob = (!dead && phase == Phase::Walk) ? std::sin(walk * 7.f) * 0.03f : 0.f;
        int pal = hand ? PAL_HAND : PAL_CUT;
        add(art_.shadow, x, 0, z + 0.08f, 0.22f, 0, PAL_FX, false, true, false, true);
        add(body, x, bob, z, h, 0, pal, x > 0, true, false, false);
        if (!dead && hand) add(art_.glow, x, 1.62f, z - 0.03f, 0.32f, 0, PAL_FX, false, false, false, false);
        if (!dead && !hand && phase == Phase::Walk && z <= LIGHT_Z && z > SILL_Z)
            add(art_.glow, x, 1.85f, z - 0.03f, 0.28f, 0, PAL_AMBER, false, false, false, false);
    };
    for (const Man& m : men_) {
        if (m.phase == Phase::Clear) continue;
        addMan(m.kind, m.phase, m.x, m.z, m.walk);
    }
    if (mode_ == Mode::Title) {
        addMan(Kind::Cutter, Phase::Walk, -0.72f, 7.4f, t_ * 1.6f);
        addMan(Kind::Hand, Phase::Peel, 1.55f, 10.6f, t_ * 1.2f);
    }

    std::sort(blobs.begin(), blobs.end(), [](const Blob& a, const Blob& b) {
        if (a.z != b.z) return a.z < b.z;
        return a.y > b.y;
    });
    for (const Blob& b : blobs) {
        float sx, sy, s;
        world(b.x, b.feet ? b.y : b.y, b.z, sx, sy, s);
        if (b.box) {
            float hh = std::max(3.f, b.h * s);
            float ww = std::max(hh, b.w * s);
            float cy = b.feet ? sy - hh * 0.5f : sy;
            sprBox(*b.img, sx, cy, ww, hh, b.pal, b.fog);
        } else {
            float hh = b.h * s;
            if (b.img == &art_.glow) hh = std::max(hh, 6.f);
            spr(*b.img, sx, sy, hh, b.pal, b.flip, b.shadow ? 0 : b.fog, b.feet, b.shadow);
        }
    }

    float drift = std::sin(t_ * 0.7f) * 5.f;
    spr(art_.steam, 70.f + drift, 40.f, 14, PAL_NIGHT, false, 4);
    spr(art_.steam, 86.f + drift * 0.4f, 32.f, 10, PAL_NIGHT, true, 6);
    spr(art_.sun, 236, HORIZON - 6.f, 26, PAL_FX, false, 2);
    for (const float* st : kStars) {
        float tw = 2.5f + ((int(st[0]) + int(t_ * 2.f)) & 1 ? 0.8f : 0.f);
        spr(art_.star, st[0], st[1], tw, PAL_NIGHT);
    }

    char buf[24];
    if (mode_ == Mode::Title) {
        hud(2, 0, "THE YARD", PAL_AMBER);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
        if (int(t_ * 2.f) & 1) hudC(27, "START", PAL_GREEN);
        else hudC(27, "ARROWS AIM   C FIRES", PAL_HUD);
    } else if (mode_ == Mode::Raid || mode_ == Mode::Pause) {
        int secs = int(std::ceil(RAID_LEN - raidT_ - 0.001f));
        if (secs < 0) secs = 0;
        std::snprintf(buf, sizeof buf, "RAID %02d", secs);
        hud(3, 0, buf, PAL_RED);
        std::snprintf(buf, sizeof buf, "MAG %02d", rounds_);
        hud(28, 0, buf, rounds_ <= 2 ? PAL_RED : PAL_AMBER);
        if (mode_ == Mode::Pause) hudC(27, "START RESUMES", PAL_HUD);
    } else if (mode_ == Mode::Won) {
        hud(2, 0, "RAID OVER", PAL_GREEN);
        std::snprintf(buf, sizeof buf, "MAG %02d", rounds_);
        hud(28, 0, buf, PAL_GREEN);
        if (int(t_ * 2.f) & 1) hudC(27, "START", PAL_HUD);
    } else {
        hud(2, 0, "WATCH OVER", PAL_RED);
        std::snprintf(buf, sizeof buf, "MAG %02d", rounds_);
        hud(28, 0, buf, rounds_ > 0 ? PAL_AMBER : PAL_RED);
        if (int(t_ * 2.f) & 1) hudC(27, "START", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) {
        t_ += DT;
        tickFx(DT);
    }
    if (mode_ == Mode::Won) {
        fanT_ += DT;
        const float when[4] = {0.f, 0.16f, 0.34f, 0.56f};
        const float note[4] = {220.f, 277.f, 330.f, 440.f};
        while (fanStep_ >= 0 && fanStep_ < 4 && fanT_ >= when[fanStep_]) {
            sys.apu.keyOn(1, note[fanStep_], 0.18f);
            ++fanStep_;
        }
    }

    const gs::Pad& pad = sys.pad;
    if (!bot_ && mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) beginRaid();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Raid) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            wantFire_ = false;
            mode_ = Mode::Pause;
        } else {
            if (!bot_ && fireEdge()) wantFire_ = true;
            if (!bot_) humanAim(DT);
            updateRaid(DT);
        }
    } else if (!bot_ && mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Raid;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            sys.apu.keyOff(2);
        }
    } else if (!bot_ && (mode_ == Mode::Won || mode_ == Mode::Lost)) {
        if (pad.pressed(gs::BTN_START)) beginRaid();
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            sys.apu.keyOff(2);
        }
    }

    draw();
}

}  // namespace yardmaga
