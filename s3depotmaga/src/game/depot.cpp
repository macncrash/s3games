#include "game/depot.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace depotmaga {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FOCAL = 200.f;
constexpr float HORIZON = 108.f;
constexpr float EYE = 1.22f;
constexpr float MAN_H = 1.70f;
constexpr float FALL_H = 0.48f;
constexpr float BODY_Y = 1.00f;
constexpr float SPAWN_Z = 18.f;
constexpr float DOOR_Z = 3.20f;
constexpr float PEEL_Z = 8.60f;
constexpr float WALK = 2.20f;
constexpr float PEEL_X = 3.40f;
constexpr float RAID_LEN = 28.f;
constexpr float BOLT = 0.38f;
constexpr float ROAD_HALF = 2.70f;
constexpr int MAG = 6;

struct Plan {
    float t;
    float x;
    int crate;
};

const Plan kPlan[] = {
    {0.70f, -0.72f, 1}, {3.40f, -1.72f, 0}, {6.90f, 0.78f, 1}, {9.60f, 1.68f, 0},
    {13.10f, -0.38f, 1}, {15.80f, -1.58f, 0}, {19.50f, 0.58f, 1},
};
constexpr int NPLAN = int(sizeof kPlan / sizeof kPlan[0]);

enum PropKind { BOX, CRANE, TOWER, DRUM, SACK, LAMP, FREIGHT };

struct Prop {
    float x, z, h;
    int kind;
    bool flip;
};

const Prop kProps[] = {
    {-3.55f, 7.2f, 2.50f, BOX, false},
    {-3.45f, 11.4f, 2.50f, BOX, false},
    {-3.35f, 15.8f, 2.35f, BOX, true},
    {3.45f, 8.6f, 2.45f, BOX, true},
    {3.40f, 13.2f, 2.45f, BOX, false},
    {-3.10f, 17.2f, 4.40f, CRANE, false},
    {3.15f, 16.0f, 5.20f, TOWER, false},
    {2.45f, 6.2f, 0.85f, DRUM, false},
    {-2.55f, 6.8f, 0.85f, DRUM, true},
    {2.15f, 4.7f, 0.70f, SACK, false},
    {-2.25f, 5.1f, 0.75f, SACK, true},
    {-2.90f, 9.6f, 2.30f, LAMP, false},
    {2.85f, 11.8f, 2.30f, LAMP, true},
    {2.35f, 4.3f, 0.95f, FREIGHT, false},
    {-2.40f, 4.9f, 0.95f, FREIGHT, true},
};

uint16_t lerp4(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t + 0.5f), int(ag + (bg - ag) * t + 0.5f), int(ab + (bb - ab) * t + 0.5f));
}

gs::FMPatch crackPatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.12f;
    p.op[0] = {1.0f, 1.0f, 0.001f, 0.05f, 0.0f, 0.04f};
    p.op[1] = {2.4f, 0.35f, 0.001f, 0.04f, 0.0f, 0.03f};
    p.op[2] = {0.5f, 0.8f, 0.001f, 0.08f, 0.0f, 0.05f};
    p.op[3] = {3.2f, 0.2f, 0.001f, 0.04f, 0.0f, 0.03f};
    p.vol = 0.28f;
    p.drive = 0.45f;
    p.tone = 1800;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.2f;
    p.op[0] = {1, 1, 0.01f, 0.18f, 0.45f, 0.22f};
    p.op[1] = {2, 0.35f, 0.012f, 0.2f, 0.3f, 0.2f};
    p.op[2] = {3, 0.18f, 0.02f, 0.22f, 0.2f, 0.2f};
    p.op[3] = {1, 0.3f, 0.012f, 0.2f, 0.35f, 0.22f};
    p.vol = 0.16f;
    return p;
}

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.3f;
    p.op[0] = {1, 0.65f, 0.4f, 0.8f, 0.85f, 0.5f};
    p.op[1] = {2, 0.25f, 0.35f, 0.7f, 0.7f, 0.45f};
    p.op[2] = {0.5f, 0.4f, 0.5f, 0.8f, 0.8f, 0.5f};
    p.op[3] = {1, 0.2f, 0.4f, 0.6f, 0.55f, 0.4f};
    p.vol = 0.04f;
    p.tone = 420;
    return p;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setPatch(0, crackPatch());
    sys.apu.setPatch(1, hornPatch());
    sys.apu.setPatch(2, dronePatch());
    rounds_ = MAG;
    aimX_ = 160;
    aimY_ = 118;
    if (bot_) beginRaid();
    else mode_ = Mode::Title;
}

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "MAGAZINE SPENT";
    if (fail_ == Fail::Door) return "THEY REACHED THE DOOR";
    return "NOT DONE";
}

int Game::marker() const {
    if (over_) return 2;
    if (mode_ == Mode::Raid || mode_ == Mode::Pause) return 1;
    return 0;
}

std::string Game::trace() const {
    char b[120];
    std::snprintf(b, sizeof b, "trace t=%.2f rounds=%d stopped=%d next=%d men=%zu\n", raidT_, rounds_, stopped_, next_,
                  men_.size());
    std::string s = b;
    for (const Man& m : men_) {
        const char* ph = "W";
        if (m.phase == Phase::Peel) ph = "P";
        else if (m.phase == Phase::Clear) ph = "C";
        else if (m.phase == Phase::Dead) ph = "D";
        std::snprintf(b, sizeof b, "  %s %s z=%.2f x=%.2f\n", m.kind == Kind::Crate ? "crate" : "lamp", ph, m.z, m.x);
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
    bolt_ = 0.18f;
    kick_ = flash_ = shake_ = fanT_ = 0;
    aimX_ = 160;
    aimY_ = 118;
    men_.clear();
    casings_.clear();
    sparks_.clear();
    sys_->apu.keyOn(1, 220.f, 0.12f);
    sys_->apu.keyOn(2, 49.f, 0.035f);
    sys_->setLight(210, 120, 40);
}

void Game::lose(Fail why) {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    fail_ = why;
    shake_ = why == Fail::Door ? 1.15f : 0.55f;
    fanStep_ = -1;
    sys_->apu.keyOff(2);
    sys_->apu.keyOn(1, why == Fail::Door ? 82.f : 110.f, 0.2f);
    sys_->apu.noiseBurst(0.35f, 220.f, 0.22f);
    sys_->rumble(0.5f, 0.2f, 140);
    sys_->setLight(200, 30, 24);
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
    sys_->apu.noiseBurst(0.22f, 140.f, 0.12f);
    sys_->setLight(40, 190, 70);
}

void Game::kill(Man& m) {
    if (m.phase == Phase::Dead || m.phase == Phase::Clear) return;
    if (m.kind == Kind::Crate) stopped_++;
    m.phase = Phase::Dead;
    m.dead = 0;
}

void Game::pull(bool cratesOnly) {
    if (mode_ != Mode::Raid || rounds_ <= 0 || bolt_ > 0) return;
    Man* best = nullptr;
    float bestD = 1e9f;
    float hx = aimX_, hy = aimY_;
    for (Man& m : men_) {
        if (m.phase != Phase::Walk && m.phase != Phase::Peel) continue;
        if (m.kind == Kind::Crate && m.phase != Phase::Walk) continue;
        if (cratesOnly && m.kind != Kind::Crate) continue;
        float sx, sy, s;
        world(m.x, BODY_Y, m.z, sx, sy, s);
        float rad = std::max(18.f, MAN_H * s * 0.48f);
        float d = std::hypot(aimX_ - sx, aimY_ - sy);
        if (d <= rad && d < bestD) {
            bestD = d;
            best = &m;
            hx = sx;
            hy = sy;
        }
    }
    rounds_--;
    bolt_ = BOLT;
    kick_ = 1.f;
    flash_ = 0.06f;
    flashX_ = hx;
    flashY_ = hy;
    shake_ = best ? 0.42f : 0.2f;
    Casing c;
    c.x = 286;
    c.y = 168;
    c.vx = 36;
    c.vy = -48;
    c.t = 0.42f;
    casings_.push_back(c);
    if (best) {
        kill(*best);
        sparks_.push_back({hx, hy, 0.16f});
        sys_->apu.keyOn(0, 118.f, 0.3f);
        sys_->apu.noiseBurst(0.48f, 1400.f, 0.07f);
        sys_->rumble(0.35f, 0.7f, 60);
    } else {
        sparks_.push_back({aimX_, aimY_, 0.12f});
        sys_->apu.keyOn(0, 180.f, 0.16f);
        sys_->apu.noiseBurst(0.22f, 4200.f, 0.04f);
    }
    if (!bot_) aimY_ = std::max(72.f, aimY_ - 6.f);
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
    aimY_ += ay * 240.f * dt;
    aimX_ = std::clamp(aimX_, 28.f, 292.f);
    aimY_ = std::clamp(aimY_, 64.f, 176.f);
}

void Game::botAct(float dt) {
    const Man* threat = nullptr;
    for (const Man& m : men_) {
        if (m.kind != Kind::Crate || m.phase != Phase::Walk) continue;
        if (!threat || m.z < threat->z) threat = &m;
    }
    if (!threat) return;
    float sx, sy, s;
    world(threat->x, BODY_Y, threat->z, sx, sy, s);
    float dx = sx - aimX_, dy = sy - aimY_;
    float d = std::hypot(dx, dy);
    float step = 1600.f * dt;
    if (d <= step) {
        aimX_ = sx;
        aimY_ = sy;
        d = 0;
    } else if (d > 0.001f) {
        aimX_ += dx / d * step;
        aimY_ += dy / d * step;
        d -= step;
    }
    if (mode_ != Mode::Raid || bolt_ > 0) return;
    if (threat->z < 13.2f && d <= 8.f) pull(true);
    else if (threat->z < 6.f) {
        aimX_ = sx;
        aimY_ = sy;
        pull(true);
    }
}

void Game::updateRaid(float dt) {
    if (bolt_ > 0) bolt_ -= dt;
    while (next_ < NPLAN && raidT_ >= kPlan[next_].t) {
        const Plan& p = kPlan[next_++];
        Man m;
        m.kind = p.crate ? Kind::Crate : Kind::Lamp;
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
        if (m.kind == Kind::Crate) {
            m.z -= WALK * dt;
            m.walk += WALK * dt;
            if (m.z < DOOR_Z) {
                lose(Fail::Door);
                return;
            }
        } else if (m.phase == Phase::Walk) {
            m.z -= WALK * dt;
            m.walk += WALK * dt;
            if (m.z <= PEEL_Z) m.phase = Phase::Peel;
        } else {
            float side = std::copysign(1.f, m.x < 0.f ? -1.f : 1.f);
            m.x += side * PEEL_X * dt;
            m.z -= 0.25f * dt;
            m.walk += dt;
            if (std::fabs(m.x) > 4.05f || m.z < 4.2f) m.phase = Phase::Clear;
        }
    }
    if (mode_ != Mode::Raid) return;

    men_.erase(std::remove_if(men_.begin(), men_.end(),
                              [](const Man& m) { return m.phase == Phase::Clear || (m.phase == Phase::Dead && m.dead > 5.f); }),
               men_.end());

    if (bot_) botAct(dt);
    else if (wantFire_ && bolt_ <= 0) {
        wantFire_ = false;
        pull(false);
    }
    if (mode_ != Mode::Raid) return;

    raidT_ += dt;
    if (raidT_ >= RAID_LEN && rounds_ > 0) win();
}

void Game::tickFx(float dt) {
    if (flash_ > 0) flash_ -= dt;
    if (kick_ > 0) kick_ = std::max(0.f, kick_ - dt * 3.4f);
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt * 2.4f);
    for (Spark& s : sparks_) s.t -= dt;
    sparks_.erase(std::remove_if(sparks_.begin(), sparks_.end(), [](const Spark& s) { return s.t <= 0; }), sparks_.end());
    for (Casing& c : casings_) {
        c.t -= dt;
        c.vy += 220.f * dt;
        c.x += c.vx * dt;
        c.y += c.vy * dt;
    }
    casings_.erase(std::remove_if(casings_.begin(), casings_.end(), [](const Casing& c) { return c.t <= 0; }), casings_.end());
    if (casings_.size() > 6) casings_.erase(casings_.begin(), casings_.end() - 6);
}

bool Game::fireEdge() {
    const gs::Pad& pad = sys_->pad;
    bool trig = pad.accel > 0.55f;
    bool edge = trig && !trigWas_;
    trigWas_ = trig;
    return edge || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_X) ||
           pad.pressed(gs::BTN_Y) || pad.pressed(gs::BTN_Z) || pad.pressed(gs::BTN_TURBO);
}

const char* Game::hint() const {
    bool door = false, crate = false, lamp = false;
    for (const Man& m : men_) {
        if (m.phase == Phase::Dead || m.phase == Phase::Clear) continue;
        if (m.kind == Kind::Crate && m.phase == Phase::Walk) {
            if (m.z < 5.6f) door = true;
            else if (m.z < 14.f) crate = true;
        } else if (m.z < 12.f) {
            lamp = true;
        }
    }
    if (door) return "THE DOOR";
    if (crate) return "SHOOT THE CRATE";
    if (lamp) return "LET THE LAMP GO";
    return "HOLD THE DEPOT";
}

int Game::hintPal() const {
    const char* h = hint();
    if (h[0] == 'T') return PAL_RED;
    if (h[0] == 'L') return PAL_GREEN;
    return PAL_AMBER;
}

float Game::bendPx(float z) const { return std::sin(z * 0.11f) * 14.f; }

int Game::fogFor(float z) const { return std::clamp(int((z - 7.f) * 0.8f), 0, 13); }

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
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    shx_ = shy_ = 0;
    if (shake_ > 0) {
        shx_ = std::sin(t_ * 91.f) * 3.1f * std::min(shake_, 1.f);
        shy_ = std::cos(t_ * 73.f) * 1.6f * std::min(shake_, 1.f);
    }

    uint16_t skyTop = gs::rgb4(2, 3, 6);
    uint16_t skyMid = gs::rgb4(7, 5, 7);
    uint16_t skyHor = gs::rgb4(14, 8, 4);
    if (mode_ == Mode::Lost) skyHor = gs::rgb4(12, 3, 2);
    else if (mode_ == Mode::Won) skyHor = gs::rgb4(12, 9, 5);
    v.setFogColor(mode_ == Mode::Lost ? gs::rgb4(8, 3, 2) : gs::rgb4(7, 4, 3));
    const int horizon = int(HORIZON);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& rd = v.road[y];
        if (y < horizon) {
            rd.on = false;
            float u = float(y) / float(horizon);
            v.lineBackdrop[y] = u < 0.55f ? lerp4(skyTop, skyMid, u / 0.55f) : lerp4(skyMid, skyHor, (u - 0.55f) / 0.45f);
            v.lineFog[y] = 0;
            continue;
        }
        float row = float(y) + 0.5f - float(horizon);
        row = std::max(row, 1.f);
        float z = EYE * FOCAL / row;
        rd.on = true;
        rd.cx = 160.f + bendPx(z) + shx_;
        rd.hw = std::max(4.f, ROAD_HALF * row / EYE);
        rd.v = z * 36.f;
        rd.pal = uint8_t(PAL_YARD);
        rd.style = 0;
        rd.band = (int(std::floor(z * 0.45f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((z - 9.f) / 16.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 11.f);
        v.lineBackdrop[y] = gs::rgb4(5, 4, 3);
    }

    if (mode_ == Mode::Title) {
        text("S3 DEPOT MAGA", 160, 16, 0.72f, PAL_AMBER);
        text("ONE DEPOT", 160, 32, 0.46f, PAL_HUD);
        text("MAKE THE MAGAZINE LAST", 160, 44, 0.42f, PAL_HUD);
        text("LONGER THAN THE RAID", 160, 54, 0.42f, PAL_HUD);
        text("THEN IT IS DONE", 160, 66, 0.46f, PAL_AMBER);
        text("CRATES ONLY    Z FIRES", 160, 78, 0.4f, PAL_GREEN);
    } else if (mode_ == Mode::Won) {
        text("IT IS DONE", 160, 36, 1.05f, PAL_GREEN);
        text("THE MAGAZINE OUTLASTS THE RAID", 160, 58, 0.42f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 78, 0.5f, PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        text("NOT DONE", 160, 36, 1.0f, PAL_RED);
        text(fail_ == Fail::Door ? "THEY REACHED THE DOOR" : "MAGAZINE SPENT", 160, 58, 0.5f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 78, 0.5f, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 40, 0.9f, PAL_HUD);
    } else if (mode_ == Mode::Raid) {
        text(hint(), 160, 22, 0.5f, hintPal());
    }

    if (mode_ == Mode::Title || mode_ == Mode::Raid || mode_ == Mode::Pause)
        spr(art_.sight, aimX_, aimY_, 20, PAL_AMBER);

    for (const Spark& s : sparks_) spr(art_.spark, s.x, s.y, 8.f + s.t * 28.f, PAL_FX);
    if (flash_ > 0) spr(art_.flash, flashX_, flashY_, 16.f + (0.06f - flash_) * 80.f, PAL_FX);

    for (int i = 0; i < MAG; i++) {
        float x = 160.f - (MAG - 1) * 7.f + float(i) * 14.f;
        bool live = i < rounds_;
        spr(live ? art_.round : art_.spent, x, 208, live ? 20.f : 16.f, PAL_FX);
    }
    for (const Casing& c : casings_) spr(art_.round, c.x, c.y, 8, PAL_FX);

    float rifleX = 292.f + (aimX_ - 160.f) * 0.04f;
    float rifleY = 228.f - kick_ * 12.f;
    if (flash_ > 0) spr(art_.flash, rifleX - 6.f, rifleY - 62.f, 16, PAL_FX);
    spr(art_.rifle, rifleX, rifleY, 70, PAL_METAL, false, 0, true);

    sprBox(art_.sill, 160, 216, 250, 16, PAL_WOOD);
    spr(art_.jamb, 22, 224, 148, PAL_BRICK, false, 0, true);
    spr(art_.jamb, 298, 224, 148, PAL_BRICK, true, 0, true);
    spr(art_.sign, 58, 124, 22, PAL_WOOD);
    spr(art_.clock, 278, 30, 20, PAL_METAL);

    struct Blob {
        float z;
        float x;
        float y;
        float h;
        const gs::Mipped* img;
        int pal;
        bool flip;
        bool feet;
        int fog;
    };
    std::vector<Blob> blobs;
    auto add = [&](const gs::Mipped& img, float x, float y, float z, float h, int pal, bool flip, bool feet) {
        Blob b;
        b.z = z;
        b.x = x;
        b.y = y;
        b.h = h;
        b.img = &img;
        b.pal = pal;
        b.flip = flip;
        b.feet = feet;
        b.fog = fogFor(z);
        blobs.push_back(b);
    };

    for (const Prop& p : kProps) {
        const gs::Mipped* img = &art_.boxcar;
        int pal = PAL_BRICK;
        if (p.kind == CRANE) {
            img = &art_.crane;
            pal = PAL_METAL;
        } else if (p.kind == TOWER) {
            img = &art_.tower;
            pal = PAL_METAL;
        } else if (p.kind == DRUM) {
            img = &art_.drum;
            pal = PAL_METAL;
        } else if (p.kind == SACK) {
            img = &art_.sack;
            pal = PAL_WOOD;
        } else if (p.kind == LAMP) {
            img = &art_.lamp;
            pal = PAL_METAL;
        } else if (p.kind == FREIGHT) {
            img = &art_.pile;
            pal = PAL_AMBER;
        }
        add(*img, p.x, 0, p.z, p.h, pal, p.flip, true);
    }

    auto addMan = [&](const Man& m) {
        if (m.phase == Phase::Clear) return;
        int fr = int(m.walk * 2.4f) & 1;
        bool lamp = m.kind == Kind::Lamp;
        bool dead = m.phase == Phase::Dead;
        const gs::Mipped& body = dead ? art_.fallen : (lamp ? art_.shunt[fr] : art_.lift[fr]);
        float h = dead ? FALL_H : MAN_H;
        float bob = (!dead && m.phase == Phase::Walk) ? std::sin(m.walk * 7.f) * 0.04f : 0.f;
        add(body, m.x, bob, m.z, h, lamp ? PAL_SHUNT : PAL_LIFT, m.x > 0, true);
        if (!dead && m.kind == Kind::Crate) {
            float side = m.x >= 0 ? 0.26f : -0.26f;
            add(art_.crate, m.x + side, 1.28f, m.z - 0.02f, 0.52f, PAL_AMBER, false, false);
        }
        if (!dead && lamp) add(art_.glow, m.x, 1.62f, m.z - 0.03f, 0.36f, PAL_AMBER, false, false);
    };
    for (const Man& m : men_) addMan(m);
    if (mode_ == Mode::Title) {
        Man show;
        show.kind = Kind::Crate;
        show.x = -0.55f;
        show.z = 8.6f;
        show.walk = t_ * 1.4f;
        addMan(show);
        Man lamp;
        lamp.kind = Kind::Lamp;
        lamp.phase = Phase::Peel;
        lamp.x = 1.45f;
        lamp.z = 7.1f;
        lamp.walk = t_ * 1.1f;
        addMan(lamp);
    }

    std::sort(blobs.begin(), blobs.end(), [](const Blob& a, const Blob& b) {
        if (a.z != b.z) return a.z < b.z;
        return a.y > b.y;
    });
    for (const Blob& b : blobs) {
        float sx, sy, s;
        world(b.x, b.feet ? 0.f : b.y, b.z, sx, sy, s);
        float hh = b.h * s;
        if (!b.feet) {
            float hh = b.h * s;
            float minH = b.img == &art_.crate ? 8.f : (b.img == &art_.glow ? 6.f : 2.f);
            spr(*b.img, sx, sy, std::max(hh, minH), b.pal, b.flip, b.fog, false);
        } else {
            float fsy = sy - b.y * s;
            bool person = b.pal == PAL_LIFT || b.pal == PAL_SHUNT;
            spr(*b.img, sx, fsy, hh, b.pal, b.flip, b.fog, true);
            if (person) spr(art_.shadow, sx, fsy + 2.f, std::max(6.f, hh * 0.28f), PAL_FX, false, 0, false, true);
        }
    }

    float lineZ = DOOR_Z + 0.15f;
    float lx, ly, ls;
    world(0, 0, lineZ, lx, ly, ls);
    sprBox(art_.chevron, lx, ly - 2.f, std::min(220.f, ROAD_HALF * 1.55f * ls), std::max(4.f, ls * 0.12f), PAL_AMBER, 0);

    float drift = std::sin(t_ * 0.8f) * 6.f;
    spr(art_.steam, 78.f + drift, 34.f, 16, PAL_SOOT, false, 3);
    spr(art_.steam, 96.f + drift * 0.5f, 26.f, 12, PAL_SOOT, true, 5);
    spr(art_.sun, 214, HORIZON - 8.f, 22, PAL_FX, false, 2);

    char buf[20];
    if (mode_ == Mode::Title) {
        hud(1, 0, "ONE DEPOT", PAL_AMBER);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Raid || mode_ == Mode::Pause) {
        int secs = int(std::ceil(RAID_LEN - raidT_ - 0.001f));
        if (secs < 0) secs = 0;
        std::snprintf(buf, sizeof buf, "RAID %02d", secs);
        hud(1, 0, buf, PAL_RED);
        std::snprintf(buf, sizeof buf, "MAG %02d", rounds_);
        hud(32, 0, buf, rounds_ <= 2 ? PAL_RED : PAL_AMBER);
        if (mode_ == Mode::Pause) hud(13, 1, "START RESUMES", PAL_HUD);
    } else if (mode_ == Mode::Won) {
        hud(1, 0, "IT IS DONE", PAL_GREEN);
        std::snprintf(buf, sizeof buf, "MAG %02d", rounds_);
        hud(32, 0, buf, PAL_GREEN);
    } else {
        hud(1, 0, "NOT DONE", PAL_RED);
        std::snprintf(buf, sizeof buf, "MAG %02d", rounds_);
        hud(32, 0, buf, rounds_ > 0 ? PAL_AMBER : PAL_RED);
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
        const float note[4] = {262.f, 330.f, 392.f, 523.f};
        while (fanStep_ >= 0 && fanStep_ < 4 && fanT_ >= when[fanStep_]) {
            sys.apu.keyOn(1, note[fanStep_], 0.18f);
            fanStep_++;
        }
    }

    bool fire = fireEdge();
    bool start = sys.pad.pressed(gs::BTN_START);
    bool back = sys.pad.pressed(gs::BTN_MODE);

    if (mode_ == Mode::Title) {
        aimX_ = 160.f + std::sin(t_ * 0.7f) * 28.f;
        aimY_ = 116.f + std::sin(t_ * 0.5f) * 5.f;
        if (!bot_ && (start || fire)) beginRaid();
        else if (!bot_ && back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Raid) {
        if (!bot_ && start) {
            wantFire_ = false;
            mode_ = Mode::Pause;
        } else {
            if (!bot_ && fire) wantFire_ = true;
            if (!bot_) humanAim(DT);
            updateRaid(DT);
        }
    } else if (mode_ == Mode::Pause) {
        if (!bot_ && start) mode_ = Mode::Raid;
        else if (!bot_ && back) {
            mode_ = Mode::Title;
            sys.apu.keyOff(2);
        }
    } else if (!bot_ && (mode_ == Mode::Won || mode_ == Mode::Lost)) {
        if (start) beginRaid();
        else if (back) {
            mode_ = Mode::Title;
            sys.apu.keyOff(2);
        }
    }

    if (mode_ == Mode::Raid) {
        if (rounds_ <= 2) sys.setLight(200, 60, 30);
        else sys.setLight(210, 120, 40);
    }

    draw();
}

}  // namespace depotmaga
