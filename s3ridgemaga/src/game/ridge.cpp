#include "game/ridge.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace rmaga {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float HORIZON = 56.f;
constexpr float ZSCALE = 561.f;
constexpr float Z_SPAWN = 15.6f;
constexpr float Z_FIRE = 7.2f;
constexpr float Z_LINE = 4.32f;
constexpr float Z_REACH = 11.5f;
constexpr float SPEED = 1.05f;
constexpr float RAID_LEN = 24.2f;
constexpr float HIT = 0.18f;
constexpr float BOLT = 0.46f;
constexpr float PACE = 1.65f;
constexpr float BOT_PACE = 4.2f;
constexpr int MAG = 5;
constexpr float FEET = 214.f;
constexpr float BODY = 76.f;

// Path raiders must be stopped on the shelf. Shoulder runners peel off and are not worth a round.
// Windows do not overlap. The last necessary shot leaves one round, and the raid clock is still running.
struct Plan {
    float t, u, peelZ;
    int peel;
};

const Plan kPlan[] = {
    {0.20f, -0.90f, 8.6f, 1},
    {0.45f, -0.56f, 0.f, 0},
    {4.20f, 0.18f, 0.f, 0},
    {5.50f, 0.92f, 8.5f, 1},
    {7.95f, -0.20f, 0.f, 0},
    {9.30f, -0.92f, 8.4f, 1},
    {11.70f, 0.58f, 0.f, 0},
    {13.10f, 0.90f, 8.3f, 1},
    {17.40f, -0.88f, 8.2f, 1},
};
constexpr int NPLAN = int(sizeof kPlan / sizeof kPlan[0]);

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [](int p, int q, float u) { return int(std::lround(p + (q - p) * u)); };
    return gs::rgb4(ch(ar, br, t), ch(ag, bg, t), ch(ab, bb, t));
}

gs::FMPatch crackPatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.2f;
    p.op[0] = {1.0f, 1.0f, 0.001f, 0.06f, 0.0f, 0.05f};
    p.op[1] = {2.2f, 0.35f, 0.001f, 0.05f, 0.0f, 0.04f};
    p.op[2] = {0.5f, 0.8f, 0.001f, 0.09f, 0.0f, 0.06f};
    p.op[3] = {3.1f, 0.2f, 0.001f, 0.04f, 0.0f, 0.04f};
    p.vol = 0.28f;
    p.drive = 0.45f;
    p.tone = 1800;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.18f;
    p.op[0] = {1, 1, 0.02f, 0.18f, 0.55f, 0.22f};
    p.op[1] = {2, 0.35f, 0.02f, 0.2f, 0.4f, 0.2f};
    p.op[2] = {3, 0.2f, 0.03f, 0.22f, 0.3f, 0.2f};
    p.op[3] = {1, 0.3f, 0.02f, 0.22f, 0.45f, 0.22f};
    p.vol = 0.16f;
    return p;
}

gs::FMPatch windPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.4f;
    p.op[0] = {1, 0.6f, 0.5f, 0.9f, 0.8f, 0.6f};
    p.op[1] = {2.01f, 0.25f, 0.4f, 0.8f, 0.7f, 0.5f};
    p.op[2] = {0.5f, 0.4f, 0.55f, 0.85f, 0.75f, 0.55f};
    p.op[3] = {1, 0.18f, 0.45f, 0.7f, 0.55f, 0.5f};
    p.vol = 0.04f;
    p.tone = 420;
    return p;
}

}  // namespace

float Game::rowAt(float z) const { return ZSCALE / std::max(z, 0.85f); }

float Game::bend(float row) const { return std::sin(row * 0.013f + 0.4f) * (4.f + row * 0.03f); }

float Game::halfW(float row) const { return 34.f + row * 0.78f; }

Game::Spot Game::spot(float u, float z, float base) const {
    float row = rowAt(z);
    float t = std::clamp(row / rowAt(Z_LINE), 0.04f, 1.2f);
    float h = std::max(8.f, base * std::pow(t, 0.7f));
    float along = std::clamp((z - Z_LINE) / (Z_SPAWN - Z_LINE), 0.f, 1.f);
    float lift = std::pow(along, 1.2f) * h * 0.28f;
    float foot = std::max(2.f, row - lift);
    Spot s;
    s.h = h;
    s.x = 160.f + bend(foot) + u * halfW(foot) + shx_;
    s.y = HORIZON + foot + shy_;
    s.fog = int(std::clamp(along * 9.f, 0.f, 9.f));
    return s;
}

bool Game::hittable(const Foe& f) const {
    return f.on && (f.phase == Phase::Walk || f.phase == Phase::Peel) && f.z > Z_LINE && f.z <= Z_REACH;
}

Game::Foe* Game::acquire() {
    Foe* best = nullptr;
    float bz = 1e9f;
    for (Foe& f : foes_) {
        if (!hittable(f)) continue;
        if (std::fabs(f.u - u_) > HIT) continue;
        if (f.z < bz) {
            bz = f.z;
            best = &f;
        }
    }
    return best;
}

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "MAGAZINE SPENT";
    if (fail_ == Fail::Through) return "THEY TOOK THE RIDGE";
    return "THE RIDGE IS LOST";
}

const char* Game::hint() const {
    bool shelf = false, shoulder = false;
    for (const Foe& f : foes_) {
        if (!f.on || f.phase == Phase::Down) continue;
        if (f.kind == Kind::Commit && f.phase == Phase::Walk && f.z <= Z_FIRE && f.z > Z_LINE) shelf = true;
        if (f.kind == Kind::Peel && hittable(f)) shoulder = true;
    }
    if (rounds_ <= 1 && !shelf) return "HOLD THE LAST";
    if (shelf) return "ON THE SHELF";
    if (shoulder) return "SHOULDER";
    return "HOLD";
}

int Game::marker() const {
    if (over_) return 2;
    if (mode_ == Mode::Raid || mode_ == Mode::Pause) return 1;
    return 0;
}

std::string Game::trace() const {
    std::string s;
    char b[120];
    std::snprintf(b, sizeof b, "trace t=%.2f rounds=%d stopped=%d next=%d u=%.2f men=%zu\n", raidT_, rounds_, stopped_,
                  next_, u_, foes_.size());
    s += b;
    for (const Foe& f : foes_) {
        const char* ph = "W";
        if (f.phase == Phase::Peel) ph = "P";
        else if (f.phase == Phase::Down) ph = "D";
        std::snprintf(b, sizeof b, "  %c %s z=%.2f u=%.2f\n", f.kind == Kind::Peel ? 'S' : 'C', ph, f.z, f.u);
        s += b;
    }
    return s;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.18f, 0.22f, 0.12f);
    sys.apu.setPatch(0, crackPatch());
    sys.apu.setPatch(1, hornPatch());
    sys.apu.setPatch(2, windPatch());
    rounds_ = MAG;
    u_ = 0;
    if (bot_) beginRaid();
    else mode_ = Mode::Title;
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
    bolt_ = 0.15f;
    kick_ = flash_ = shake_ = fanT_ = 0;
    u_ = 0;
    foes_.clear();
    puffs_.clear();
    casings_.clear();
    sys_->apu.keyOn(2, 49.f, 0.04f);
    sys_->setLight(48, 28, 16);
}

void Game::lose(Fail why) {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    fail_ = why;
    shake_ = why == Fail::Through ? 1.2f : 0.55f;
    sys_->apu.keyOff(2);
    sys_->apu.keyOn(1, why == Fail::Through ? 70.f : 92.f, 0.22f);
    sys_->apu.noiseBurst(0.35f, 180.f, 0.22f);
    sys_->setLight(72, 16, 12);
    sys_->rumble(0.7f, 0.35f, 180);
}

void Game::win() {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Won;
    over_ = true;
    won_ = true;
    fanStep_ = 0;
    fanT_ = 0;
    sys_->apu.keyOff(2);
    sys_->setLight(24, 48, 20);
}

void Game::shotSound(bool hit) {
    sys_->apu.keyOn(0, hit ? 98.f : 150.f, hit ? 0.32f : 0.16f);
    sys_->apu.noiseBurst(hit ? 0.45f : 0.22f, hit ? 1400.f : 4200.f, hit ? 0.07f : 0.04f);
    sys_->rumble(hit ? 0.4f : 0.15f, hit ? 0.2f : 0.05f, 40);
}

void Game::pull() {
    if (mode_ != Mode::Raid || rounds_ <= 0 || bolt_ > 0) return;
    Foe* hit = acquire();
    rounds_--;
    bolt_ = BOLT;
    flash_ = 0.06f;
    kick_ = 1.f;
    shake_ = hit ? 0.4f : 0.2f;
    Casing c;
    c.x = 168;
    c.y = 196;
    c.vx = 36;
    c.vy = -28;
    c.t = 0.42f;
    casings_.push_back(c);
    if (hit) {
        hit->phase = Phase::Down;
        hit->age = 0;
        stopped_++;
        puffs_.push_back({hit->u, hit->z, 0.38f});
        shotSound(true);
    } else {
        shotSound(false);
    }
    if (rounds_ <= 0) lose(Fail::Spent);
}

void Game::botAct(float dt) {
    const Foe* shoot = nullptr;
    const Foe* track = nullptr;
    for (const Foe& f : foes_) {
        if (!f.on || f.kind != Kind::Commit || f.phase != Phase::Walk) continue;
        if (f.z <= Z_LINE) continue;
        if (!track || f.z < track->z) track = &f;
        if (f.z <= Z_FIRE && (!shoot || f.z < shoot->z)) shoot = &f;
    }
    const Foe* aim = shoot ? shoot : track;
    if (aim) {
        float du = aim->u - u_;
        float step = BOT_PACE * dt;
        if (std::fabs(du) <= step) u_ = aim->u;
        else u_ += std::copysign(step, du);
    }
    if (!shoot || bolt_ > 0) return;
    if (std::fabs(u_ - shoot->u) > 0.04f) return;
    if (acquire() != shoot) return;
    pull();
}

void Game::humanAct(float dt) {
    const gs::Pad& pad = sys_->pad;
    float ax = 0;
    if (std::fabs(pad.axisX) > 0.18f) ax = pad.axisX;
    if (pad.down(gs::BTN_LEFT)) ax -= 1;
    if (pad.down(gs::BTN_RIGHT)) ax += 1;
    ax = std::clamp(ax, -1.f, 1.f);
    u_ = std::clamp(u_ + ax * PACE * dt, -0.84f, 0.84f);
    if (wantFire_ && bolt_ <= 0) {
        wantFire_ = false;
        pull();
    }
}

void Game::updateRaid(float dt) {
    if (bolt_ > 0) bolt_ -= dt;
    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return !f.on; }), foes_.end());

    while (next_ < NPLAN && raidT_ >= kPlan[next_].t) {
        const Plan& p = kPlan[next_++];
        Foe f;
        f.kind = p.peel ? Kind::Peel : Kind::Commit;
        f.u = p.u;
        f.z = Z_SPAWN;
        f.peelZ = p.peelZ;
        f.side = p.u >= 0 ? 1.f : -1.f;
        f.age = 0;
        foes_.push_back(f);
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        if (f.phase == Phase::Down) {
            f.age += dt;
            if (f.age > 3.4f) f.on = false;
            continue;
        }
        if (f.phase == Phase::Walk) {
            f.z -= SPEED * dt;
            f.age += dt;
            if (f.kind == Kind::Peel && f.z <= f.peelZ) f.phase = Phase::Peel;
            if (f.kind == Kind::Commit && f.z <= Z_LINE) {
                lose(Fail::Through);
                return;
            }
        } else if (f.phase == Phase::Peel) {
            f.age += dt;
            f.u += f.side * 2.8f * dt;
            f.z -= SPEED * 0.45f * dt;
            if (std::fabs(f.u) > 1.25f || f.z < Z_LINE) f.on = false;
        }
    }
    if (mode_ != Mode::Raid) return;

    for (Foe& f : foes_) {
        if (f.kind == Kind::Commit && f.phase == Phase::Walk && !f.warned && f.z <= Z_FIRE) {
            f.warned = true;
            sys_->apu.keyOn(1, 196.f, 0.08f);
        }
    }

    if (bot_) botAct(dt);
    else humanAct(dt);
    if (mode_ != Mode::Raid) return;

    raidT_ += dt;
    if (raidT_ >= RAID_LEN && rounds_ > 0) win();
}

void Game::tickFx(float dt) {
    if (flash_ > 0) flash_ -= dt;
    if (kick_ > 0) kick_ = std::max(0.f, kick_ - dt * 3.4f);
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt * 2.4f);
    for (Puff& p : puffs_) p.t -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0; }), puffs_.end());
    for (Casing& c : casings_) {
        c.t -= dt;
        c.vy += 160.f * dt;
        c.x += c.vx * dt;
        c.y += c.vy * dt;
    }
    casings_.erase(std::remove_if(casings_.begin(), casings_.end(), [](const Casing& c) { return c.t <= 0; }),
                   casings_.end());
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

void Game::shadow(float cx, float cy, float w) {
    if (w < 6.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 6L, 420L));
    s.h = int16_t(std::max(4L, std::lround(double(w) * 0.16)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = art_.shadow.pick(float(s.h));
    s.pal = 0;
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.0f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false, 0, false);
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
        shx_ = std::sin(t_ * 90.f) * 3.2f * shake_;
        shy_ = std::cos(t_ * 70.f) * 2.0f * shake_;
    }

    const uint16_t skyTop = gs::rgb4(2, 2, 6);
    const uint16_t skyMid = gs::rgb4(6, 4, 9);
    const uint16_t skyHor = gs::rgb4(13, 7, 4);
    const uint16_t deep = gs::rgb4(1, 1, 2);
    const uint16_t haze = gs::rgb4(6, 3, 3);
    v.setFogColor(skyHor);
    const int horizon = std::clamp(int(std::lround(HORIZON + shy_)), 40, 90);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& r = v.road[y];
        if (y < horizon) {
            float u = y / float(std::max(horizon, 1));
            v.lineBackdrop[y] = u < 0.55f ? mix(skyTop, skyMid, u / 0.55f) : mix(skyMid, skyHor, (u - 0.55f) / 0.45f);
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y - horizon) + 0.5f;
        float z = ZSCALE / std::max(row, 1.f);
        r.on = true;
        r.cx = 160.f + bend(row) + shx_;
        r.hw = halfW(row);
        r.v = z * 48.f;
        r.pal = PAL_FIELD;
        r.band = (int(std::floor(z * 0.45f)) & 1) ? 1 : 0;
        r.style = gs::ROAD_ROCKY;
        r.left = r.right = gs::GROUND_DROP;
        v.lineFog[y] = uint8_t(std::clamp(int(12.f - row * 0.09f), 0, 12));
        float dropT = std::clamp(row / 150.f, 0.f, 1.f);
        v.lineBackdrop[y] = mix(haze, deep, dropT);
    }

    if (mode_ == Mode::Title) {
        text("S3 RIDGE MAGA", 160, 28, 0.92f, PAL_AMBER);
        text("OUTLAST THE RAID", 160, 46, 0.55f, PAL_INK);
    } else if (mode_ == Mode::Won) {
        text("MAGAZINE HELD", 160, 36, 0.9f, PAL_GREEN);
        text("IT OUTLASTS THE RAID", 160, 54, 0.52f, PAL_AMBER);
    } else if (mode_ == Mode::Lost) {
        text("THE RIDGE IS LOST", 160, 36, 0.72f, PAL_RED);
        text(fail_ == Fail::Through ? "THEY TOOK THE RIDGE" : "MAGAZINE SPENT", 160, 54, 0.5f, PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 40, 1.0f, PAL_INK);
    }

    if (mode_ != Mode::Title) {
        for (int i = 0; i < MAG; i++) {
            float x = 160.f - (MAG - 1) * 7.f + float(i) * 14.f;
            bool live = i < rounds_;
            spr(live ? art_.round : art_.spent, x, 18, 16, PAL_FX, false, 0, false);
        }
    }
    for (const Casing& c : casings_) spr(art_.round, c.x, c.y, 8, PAL_FX, false, 0, false);

    float px = 160.f + u_ * 36.f + shx_;
    float py = FEET + shy_ - kick_ * 6.f;
    int step = (std::fabs(u_) > 0.04f && (int(t_ * 8.f) & 1)) ? 1 : 0;
    if (flash_ > 0) spr(art_.flash, px + 10.f, py - 62.f, 16, PAL_FX, false, 0, false);
    spr(art_.you[step], px, py, BODY, PAL_YOU, u_ < -0.05f, 0, true);
    shadow(px, py - 4.f, 46.f);

    float beadZ = Z_FIRE;
    const Foe* beadOn = nullptr;
    for (const Foe& f : foes_) {
        if (!hittable(f)) continue;
        if (!beadOn || f.z < beadOn->z) beadOn = &f;
    }
    if (beadOn) beadZ = std::clamp(beadOn->z, Z_LINE + 0.3f, Z_SPAWN);
    if (mode_ == Mode::Raid || mode_ == Mode::Pause || mode_ == Mode::Title) {
        Spot b = spot(u_, beadZ, 14.f);
        spr(art_.bead, b.x, b.y - b.h * 0.15f, std::max(8.f, b.h), PAL_FX, false, 0, false);
    }

    std::vector<Blob> blobs;
    auto add = [&](const gs::Mipped& img, float u, float z, float base, int pal, bool flip) {
        blobs.push_back({z, u, base, &img, pal, flip, true});
    };
    add(art_.stake, -0.82f, Z_FIRE + 0.15f, 52.f, PAL_STONE, false);
    add(art_.stake, 0.82f, Z_FIRE + 0.15f, 52.f, PAL_STONE, true);
    add(art_.cairn, -0.78f, 10.4f, 40.f, PAL_STONE, false);
    add(art_.cairn, 0.8f, 12.2f, 36.f, PAL_STONE, false);
    add(art_.post, -0.7f, 5.1f, 64.f, PAL_STONE, false);
    add(art_.post, 0.72f, 5.3f, 60.f, PAL_STONE, true);
    add(art_.banner, 0.02f, 13.4f, 70.f, PAL_CLOTH, false);

    if (mode_ == Mode::Title) {
        const float zu[3] = {6.2f, 9.4f, 12.8f};
        const float uu[3] = {-0.42f, 0.55f, -0.7f};
        for (int i = 0; i < 3; i++) {
            float z = zu[i] + std::sin(t_ * 0.4f + i) * 0.2f;
            bool peel = i == 2;
            int fr = int(t_ * 6.f + i) & 1;
            add(peel ? art_.peel[fr] : art_.raider[fr], uu[i], z, peel ? 64.f : 78.f, peel ? PAL_PEEL : PAL_RAID,
                uu[i] > 0);
        }
    }

    for (const Foe& f : foes_) {
        if (!f.on) continue;
        int fr = int(f.age * 6.f) & 1;
        bool flip = f.u > 0;
        int pal = f.kind == Kind::Peel ? PAL_PEEL : PAL_RAID;
        if (f.phase == Phase::Down) add(art_.fallen, f.u, f.z, 36.f, pal, flip);
        else if (f.kind == Kind::Peel) add(art_.peel[fr], f.u, f.z, 66.f, pal, flip);
        else add(art_.raider[fr], f.u, f.z, 80.f, pal, flip);
    }
    for (const Puff& p : puffs_) {
        float k = std::clamp(p.t / 0.38f, 0.f, 1.f);
        Spot s = spot(p.u, p.z, 28.f);
        spr(art_.dust, s.x, s.y - s.h * 0.4f, 14.f + (1.f - k) * 22.f, PAL_FX, false, int((1.f - k) * 6.f), false);
    }

    std::sort(blobs.begin(), blobs.end(), [](const Blob& a, const Blob& b) { return a.z < b.z; });
    for (const Blob& b : blobs) {
        Spot s = spot(b.u, b.z, b.base);
        if (s.fog < 7) shadow(s.x, s.y, s.h * 0.42f);
        spr(*b.img, s.x, s.y, s.h, b.pal, b.flip, s.fog, true);
    }

    float drift = std::fmod(t_ * 10.f, 380.f);
    spr(art_.cloud, drift - 40.f, 18.f, 16, PAL_INK, false, 3, false);
    spr(art_.cloud, std::fmod(drift + 190.f, 380.f) - 30.f, 30.f, 12, PAL_INK, true, 4, false);
    spr(art_.mount[0], 48 + shx_ * 0.2f, HORIZON + shy_ + 6, 58, PAL_MOUNT, false, 2, true);
    spr(art_.mount[1], 268 + shx_ * 0.2f, HORIZON + shy_ + 10, 48, PAL_MOUNT, false, 3, true);
    spr(art_.sun, 236, 22, 22, PAL_FX, false, 0, false);

    char buf[32];
    if (mode_ == Mode::Title) {
        hudC(12, "THEY COME OVER THE CREST", PAL_AMBER);
        hudC(14, "SHOOT THE PATH  NOT THE SHOULDER", PAL_INK);
        hudC(16, "A MISS SPENDS A ROUND", PAL_INK);
        hudC(18, "ONE ROUND MUST REMAIN", PAL_AMBER);
        hudC(20, "ARROWS TRAVERSE    C FIRES", PAL_INK);
        if ((int(t_ * 2.f) & 1) == 0) hudC(23, "PRESS START", PAL_GREEN);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_INK);
    } else if (mode_ == Mode::Raid || mode_ == Mode::Pause) {
        int secs = int(std::ceil(RAID_LEN - raidT_));
        if (secs < 0) secs = 0;
        std::snprintf(buf, sizeof buf, "RAID %02d", secs);
        hud(1, 0, buf, secs <= 5 ? PAL_RED : PAL_INK);
        std::snprintf(buf, sizeof buf, "MAG %d", rounds_);
        hud(33, 0, buf, rounds_ <= 1 ? PAL_RED : PAL_AMBER);
        if (mode_ == Mode::Raid) hudC(26, hint(), rounds_ <= 1 ? PAL_RED : PAL_AMBER);
        hud(1, 26, bolt_ <= 0 ? "READY" : "BOLT", bolt_ <= 0 ? PAL_GREEN : PAL_INK);
    } else if (mode_ == Mode::Won) {
        hud(1, 0, "RAID OVER", PAL_GREEN);
        std::snprintf(buf, sizeof buf, "MAG %d", rounds_);
        hud(33, 0, buf, PAL_GREEN);
        std::snprintf(buf, sizeof buf, "STOPPED %d", stopped_);
        hudC(16, buf, PAL_INK);
        if ((int(t_ * 2.f) & 1) == 0) hudC(23, "START", PAL_AMBER);
    } else if (mode_ == Mode::Lost) {
        hud(1, 0, "RIDGE LOST", PAL_RED);
        std::snprintf(buf, sizeof buf, "MAG %d", rounds_);
        hud(33, 0, buf, rounds_ > 0 ? PAL_AMBER : PAL_RED);
        hudC(16, fail_ == Fail::Through ? "THEY CROSSED THE LIP" : "THE MAGAZINE DIED FIRST", PAL_INK);
        if ((int(t_ * 2.f) & 1) == 0) hudC(23, "START", PAL_INK);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = DT;
    if (mode_ != Mode::Pause) t_ += dt;
    tickFx(dt);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Won) {
        fanT_ += dt;
        const float when[4] = {0.f, 0.14f, 0.32f, 0.54f};
        const float note[4] = {196.f, 247.f, 294.f, 392.f};
        while (fanStep_ >= 0 && fanStep_ < 4 && fanT_ >= when[fanStep_]) {
            sys.apu.keyOn(1, note[fanStep_], 0.18f);
            fanStep_++;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        u_ = std::sin(t_ * 0.7f) * 0.42f;
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
            if (!bot_ && (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z) ||
                          pad.pressed(gs::BTN_TURBO)))
                wantFire_ = true;
            updateRaid(dt);
        }
    } else if (!bot_ && mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Raid;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            sys.apu.keyOff(2);
        }
    } else if (!bot_ && (mode_ == Mode::Won || mode_ == Mode::Lost)) {
        if (pad.pressed(gs::BTN_START)) beginRaid();
        else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    }

    draw();
}

}  // namespace rmaga
