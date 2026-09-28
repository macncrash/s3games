#include "game/tower.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace towermaga {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FOCAL = 220.f;
constexpr float HORIZON = 70.f;
constexpr float EYE = 4.6f;
constexpr float MAN_H = 1.72f;
constexpr float CROUCH_H = 1.05f;
constexpr float FALL_H = 0.42f;
constexpr float CHEST = 1.15f;
constexpr float STAIR_Z = 2.35f;
constexpr float SPAWN_Z = 22.0f;
constexpr float SLIP = 3.4f;
constexpr float RUSH_V = 3.85f;
constexpr float RUSH_X = 2.6f;
constexpr float RAID_LEN = 32.4f;
constexpr float BOLT = 0.28f;
constexpr float SPAN_HALF = 2.15f;
constexpr int MAG = 7;
constexpr int NMEN = 6;

struct Plan {
    float t, side, hideZ, hold;
};

// Piers along the span. A man slips to cover, waits, then sprints the open.
// A shot only counts on that sprint. The last one falls before the clock dies.
const Plan kPlan[NMEN] = {
    {0.55f, -1.0f, 9.2f, 0.48f},
    {5.55f, 1.0f, 10.4f, 0.44f},
    {10.55f, -1.0f, 8.6f, 0.52f},
    {15.55f, 1.0f, 11.0f, 0.42f},
    {20.55f, -1.0f, 9.6f, 0.50f},
    {25.40f, 1.0f, 8.8f, 0.40f},
};

const float kPiers[][2] = {{-2.7f, 5.2f}, {2.7f, 6.4f}, {-2.85f, 10.6f}, {2.85f, 12.2f}, {-3.0f, 16.4f}, {3.0f, 18.2f}};

uint16_t lerp4(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t + 0.5f), int(ag + (bg - ag) * t + 0.5f), int(ab + (bb - ab) * t + 0.5f));
}

gs::FMPatch crackPatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.1f;
    p.op[0] = {1.0f, 1.0f, 0.001f, 0.05f, 0.0f, 0.04f};
    p.op[1] = {2.2f, 0.32f, 0.001f, 0.04f, 0.0f, 0.03f};
    p.op[2] = {0.5f, 0.7f, 0.001f, 0.08f, 0.0f, 0.05f};
    p.op[3] = {3.0f, 0.18f, 0.001f, 0.04f, 0.0f, 0.03f};
    p.vol = 0.26f;
    p.drive = 0.4f;
    p.tone = 1600;
    return p;
}

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.25f;
    p.op[0] = {1, 0.55f, 0.5f, 0.8f, 0.8f, 0.55f};
    p.op[1] = {2, 0.2f, 0.35f, 0.7f, 0.7f, 0.45f};
    p.op[2] = {0.5f, 0.35f, 0.5f, 0.8f, 0.8f, 0.5f};
    p.op[3] = {1, 0.14f, 0.4f, 0.6f, 0.6f, 0.45f};
    p.vol = 0.035f;
    p.tone = 280;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.18f;
    p.op[0] = {1, 1, 0.01f, 0.16f, 0.5f, 0.2f};
    p.op[1] = {2, 0.3f, 0.01f, 0.16f, 0.32f, 0.16f};
    p.op[2] = {3, 0.18f, 0.02f, 0.18f, 0.22f, 0.16f};
    p.op[3] = {1, 0.24f, 0.01f, 0.18f, 0.35f, 0.18f};
    p.vol = 0.15f;
    return p;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setPatch(0, crackPatch());
    sys.apu.setPatch(1, hornPatch());
    sys.apu.setPatch(2, dronePatch());
    aimX_ = 160;
    aimY_ = 100;
    rounds_ = MAG;
    if (bot_) beginRaid();
    else mode_ = Mode::Title;
}

const char* Game::result() const {
    if (won_) return "the magazine outlasts the raid";
    if (fail_ == Fail::Spent) return "magazine spent";
    if (fail_ == Fail::Stair) return "they reached the stair";
    return "the watch is over";
}

int Game::marker() const {
    if (over_) return 2;
    if (mode_ == Mode::Raid || mode_ == Mode::Pause) return 1;
    return 0;
}

std::string Game::trace() const {
    std::string s;
    char b[96];
    std::snprintf(b, sizeof b, "trace t=%.2f rounds=%d stopped=%d next=%d men=%zu\n", raidT_, rounds_, stopped_, next_,
                  men_.size());
    s += b;
    for (const Man& m : men_) {
        const char* ph = "S";
        if (m.phase == Phase::Cover) ph = "C";
        else if (m.phase == Phase::Rush) ph = "R";
        else if (m.phase == Phase::Dead) ph = "X";
        std::snprintf(b, sizeof b, "  %s z=%.2f x=%.2f\n", ph, m.z, m.x);
        s += b;
    }
    return s;
}

bool Game::rushing() const {
    for (const Man& m : men_)
        if (m.phase == Phase::Rush) return true;
    return false;
}

bool Game::anyone() const {
    for (const Man& m : men_)
        if (m.phase != Phase::Dead) return true;
    return false;
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
    bolt_ = 0.16f;
    kick_ = flash_ = shake_ = fanT_ = 0;
    aimX_ = 160;
    aimY_ = 100;
    men_.clear();
    sparks_.clear();
    casings_.clear();
    sys_->apu.keyOn(2, 41.f, 0.035f);
}

void Game::lose(Fail why) {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    fail_ = why;
    shake_ = why == Fail::Stair ? 1.2f : 0.5f;
    sys_->apu.keyOff(2);
    sys_->apu.keyOn(1, why == Fail::Stair ? 66.f : 92.f, 0.2f);
    sys_->apu.noiseBurst(0.32f, 200.f, 0.2f);
}

void Game::win() {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Won;
    over_ = true;
    won_ = true;
    fanStep_ = 0;
    fanT_ = 0;
    sys_->apu.keyOff(2);
}

void Game::kill(Man& m) {
    if (m.phase == Phase::Dead) return;
    m.phase = Phase::Dead;
    m.dead = 0;
    stopped_++;
}

void Game::pull() {
    if (mode_ != Mode::Raid || rounds_ <= 0 || bolt_ > 0) return;
    Man* best = nullptr;
    float bestD = 1e9f;
    float hx = aimX_, hy = aimY_;
    for (Man& m : men_) {
        if (m.phase != Phase::Rush) continue;
        float sx, sy, s;
        world(m.x, CHEST, m.z, sx, sy, s);
        float rad = std::max(16.f, MAN_H * s * 0.45f);
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
    flash_ = 0.05f;
    kick_ = 1;
    shake_ = best ? 0.32f : 0.16f;
    if (!bot_) aimY_ = std::max(40.f, aimY_ - 6.f);
    Casing c;
    c.x = 248;
    c.y = 168;
    c.vx = 42;
    c.vy = -36;
    c.t = 0.34f;
    casings_.push_back(c);
    if (best) {
        kill(*best);
        sparks_.push_back({hx, hy, 0.14f});
        sys_->apu.keyOn(0, 98.f, 0.28f);
        sys_->apu.noiseBurst(0.44f, 1200.f, 0.055f);
    } else {
        sparks_.push_back({aimX_, aimY_, 0.1f});
        sys_->apu.keyOn(0, 150.f, 0.14f);
        sys_->apu.noiseBurst(0.22f, 4200.f, 0.03f);
    }
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
    aimX_ += ax * 240.f * dt;
    aimY_ += ay * 240.f * dt;
    aimX_ = std::clamp(aimX_, 36.f, 284.f);
    aimY_ = std::clamp(aimY_, 28.f, 176.f);
}

void Game::botAct(float dt) {
    const Man* threat = nullptr;
    const Man* watch = nullptr;
    for (const Man& m : men_) {
        if (m.phase == Phase::Dead) continue;
        if (!watch || m.z < watch->z) watch = &m;
        if (m.phase == Phase::Rush && (!threat || m.z < threat->z)) threat = &m;
    }
    const Man* aim = threat ? threat : watch;
    if (!aim) return;
    float sx, sy, s;
    world(aim->x, CHEST, aim->z, sx, sy, s);
    float dx = sx - aimX_, dy = sy - aimY_;
    float d = std::hypot(dx, dy);
    float step = 1400.f * dt;
    if (d <= step) {
        aimX_ = sx;
        aimY_ = sy;
    } else if (d > 0.001f) {
        aimX_ += dx / d * step;
        aimY_ += dy / d * step;
    }
    if (threat && d <= step && bolt_ <= 0) pull();
}

void Game::updateRaid(float dt) {
    men_.erase(std::remove_if(men_.begin(), men_.end(), [](const Man& m) { return m.phase == Phase::Dead && m.dead > 5.f; }),
               men_.end());
    if (bolt_ > 0) bolt_ -= dt;

    while (next_ < NMEN && raidT_ >= kPlan[next_].t) {
        const Plan& p = kPlan[next_++];
        Man m;
        m.side = p.side;
        m.x = p.side * 0.4f;
        m.z = SPAWN_Z;
        m.hideZ = p.hideZ;
        m.holdDur = p.hold;
        m.ph = float(next_) * 1.31f;
        m.coat = next_ & 1;
        m.phase = Phase::Slip;
        men_.push_back(m);
    }

    for (Man& m : men_) {
        if (m.phase == Phase::Dead) {
            m.dead += dt;
            continue;
        }
        if (m.phase == Phase::Slip) {
            m.z -= SLIP * dt;
            float edge = m.side * 2.05f;
            float u = std::clamp((m.z - m.hideZ) / 8.f, 0.f, 1.f);
            float want = edge * (1.f - 0.35f * u);
            m.x += (want - m.x) * std::min(1.f, dt * 3.5f);
            if (m.z <= m.hideZ) {
                m.z = m.hideZ;
                m.x = m.side * 2.05f;
                m.phase = Phase::Cover;
                m.cover = m.holdDur;
            }
        } else if (m.phase == Phase::Cover) {
            m.x = m.side * 2.05f;
            m.cover -= dt;
            if (m.cover <= 0) {
                m.phase = Phase::Rush;
                sys_->apu.keyOn(1, 132.f, 0.07f);
            }
        } else if (m.phase == Phase::Rush) {
            float step = RUSH_X * dt;
            if (std::fabs(m.x) <= step) m.x = 0;
            else m.x -= std::copysign(step, m.x);
            m.z -= RUSH_V * dt;
            if (m.z < STAIR_Z) {
                lose(Fail::Stair);
                return;
            }
        }
    }
    if (mode_ != Mode::Raid) return;

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
    if (kick_ > 0) kick_ = std::max(0.f, kick_ - dt * 3.2f);
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt * 2.6f);
    for (Spark& s : sparks_) s.t -= dt;
    sparks_.erase(std::remove_if(sparks_.begin(), sparks_.end(), [](const Spark& s) { return s.t <= 0; }), sparks_.end());
    for (Casing& c : casings_) {
        c.t -= dt;
        c.vy += 140.f * dt;
        c.x += c.vx * dt;
        c.y += c.vy * dt;
    }
    casings_.erase(std::remove_if(casings_.begin(), casings_.end(), [](const Casing& c) { return c.t <= 0; }), casings_.end());
}

void Game::world(float x, float y, float z, float& sx, float& sy, float& s) const {
    float zz = std::max(0.45f, z);
    s = FOCAL / zz;
    sx = 160.f + x * s;
    sy = HORIZON + (EYE - y) * s;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.1f || m.h < 1) return;
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
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
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
    const float adv = 16.0f * scale;
    x -= float(s.size()) * adv * 0.5f;
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

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.setFogColor(gs::rgb4(1, 1, 3));
    shx_ = shy_ = 0;
    if (shake_ > 0) {
        shx_ = std::sin(t_ * 88.f) * 2.4f * shake_;
        shy_ = std::cos(t_ * 63.f) * 1.4f * shake_;
    }

    const uint16_t skyTop = gs::rgb4(0, 0, 2);
    const uint16_t skyMid = gs::rgb4(1, 1, 4);
    const uint16_t skyHor = gs::rgb4(5, 3, 2);
    const int horizon = std::clamp(int(std::lround(HORIZON + shy_)), 52, 100);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < horizon) {
            float u = y / float(horizon);
            v.lineBackdrop[y] = u < 0.65f ? lerp4(skyTop, skyMid, u / 0.65f) : lerp4(skyMid, skyHor, (u - 0.65f) / 0.35f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - horizon) + 0.5f;
        float z = EYE * FOCAL / row;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.hw = SPAN_HALF * row / EYE;
        r.cx = 160.f + shx_;
        r.v = z * 28.f;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(z / 2.2f)) & 1) ? 1 : 0;
        r.style = 1;
        r.left = r.right = 0;
        v.lineFog[y] = uint8_t(std::clamp(int(14.f - row * 0.1f), 0, 13));
        v.lineBackdrop[y] = gs::rgb4(1, 1, 2);
    }

    auto fogZ = [](float z) { return std::clamp(int((z - 4.f) * 0.75f), 0, 14); };

    if (mode_ == Mode::Title) {
        text("S3 TOWER MAGA", 160, 36, 0.9f, PAL_AMBER);
        text("MAKE THE MAGAZINE LAST", 160, 56, 0.52f, PAL_HUD);
        text("LONGER THAN THE RAID", 160, 70, 0.52f, PAL_HUD);
        text("ARROWS AIM    C FIRES", 160, 148, 0.5f, PAL_AMBER);
        text("THE PIER DOES NOT COUNT", 160, 162, 0.46f, PAL_HUD);
        text("ONE ROUND MUST REMAIN", 160, 176, 0.46f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 198, 0.62f, PAL_GREEN);
        hud(1, 0, "THE TOWER", PAL_AMBER);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("MAGAZINE HELD", 160, 42, 0.88f, PAL_GREEN);
        text("IT OUTLASTS THE RAID", 160, 64, 0.54f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 196, 0.54f, PAL_HUD);
        hud(1, 0, "RAID OVER", PAL_GREEN);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160, 42, 0.7f, PAL_RED);
        text(fail_ == Fail::Stair ? "THEY REACHED THE STAIR" : "MAGAZINE SPENT", 160, 64, 0.48f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 196, 0.54f, PAL_HUD);
        hud(1, 0, "WATCH OVER", PAL_RED);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 42, 0.9f, PAL_HUD);
    } else if (mode_ == Mode::Raid && !rushing()) {
        text(anyone() ? "HOLD" : "HOLD FIRE", 160, 44, 0.64f, PAL_AMBER);
    }

    if (mode_ == Mode::Raid || mode_ == Mode::Pause) {
        int secs = int(std::ceil(RAID_LEN - raidT_));
        if (secs < 0) secs = 0;
        char buf[16];
        std::snprintf(buf, sizeof buf, "RAID %02d", secs);
        hud(1, 0, buf, PAL_RED);
        std::snprintf(buf, sizeof buf, "MAG %02d", rounds_);
        hud(32, 0, buf, rounds_ <= 2 ? PAL_RED : PAL_AMBER);
        float remain = std::clamp(1.f - raidT_ / RAID_LEN, 0.f, 1.f);
        sprBox(art_.bar, 160, 22, 148, 4, PAL_IRON);
        if (remain > 0.02f) {
            float w = 148.f * remain;
            sprBox(art_.bar, 86.f + w * 0.5f, 22, w, 4, PAL_RED);
        }
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "MAG %02d", rounds_);
        hud(32, 0, buf, rounds_ > 0 ? PAL_GREEN : PAL_RED);
    }

    if (mode_ == Mode::Title || mode_ == Mode::Raid || mode_ == Mode::Pause)
        spr(art_.sight, aimX_ + shx_, aimY_ + shy_, 18, PAL_FX);

    for (const Spark& s : sparks_) spr(art_.spark, s.x + shx_, s.y + shy_, 8 + s.t * 16.f, PAL_FX);
    for (const Casing& c : casings_) spr(art_.round, c.x, c.y, 6, PAL_FX);

    float rifleX = 250.f + (aimX_ - 160.f) * 0.05f;
    float rifleY = 186.f - kick_ * 10.f;
    if (flash_ > 0) spr(art_.flash, rifleX - 2, rifleY - 48, 14, PAL_FX);
    spr(art_.rifle, rifleX, rifleY, 74, PAL_IRON);

    for (int i = 0; i < MAG; i++) {
        float x = 78.f + float(i) * 16.f;
        bool live = i < rounds_;
        spr(live ? art_.round : art_.spent, x, 210, 13, PAL_FX);
    }

    float flick = 0.75f + 0.25f * std::sin(t_ * 9.f);
    spr(art_.glow, 28, 30, 16.f * flick, PAL_LAMP);
    spr(art_.lamp, 28, 28, 16, PAL_LAMP);
    spr(art_.glow, 292, 30, 16.f * flick, PAL_LAMP);
    spr(art_.lamp, 292, 28, 16, PAL_LAMP);

    std::vector<Blob> blobs;
    auto add = [&](const gs::Mipped& img, float x, float z, float h, int pal, bool flip) {
        blobs.push_back({z, x, h, &img, pal, fogZ(z), flip});
    };
    for (const float* p : kPiers) add(art_.pier, p[0], p[1], 4.2f, PAL_STONE, p[0] > 0);
    add(art_.banner, -2.55f, 5.4f, 2.4f, PAL_RED, false);
    add(art_.banner, 2.55f, 12.4f, 2.2f, PAL_RED, true);
    add(art_.flag, 0.15f, 3.1f, 0.55f, PAL_RED, false);
    add(art_.stair, 0.f, 2.6f, 0.7f, PAL_STONE, false);
    if (mode_ == Mode::Title) add(art_.crouch, -2.05f, 9.2f, CROUCH_H, PAL_MAN, false);

    for (const Man& m : men_) {
        int pal = m.coat ? PAL_MANB : PAL_MAN;
        float x = m.x;
        if (m.phase == Phase::Slip) x += std::sin(t_ * 3.6f + m.ph) * 0.06f;
        bool flip = m.side > 0;
        if (m.phase == Phase::Dead) add(art_.fallen, x, m.z, FALL_H, pal, flip);
        else if (m.phase == Phase::Rush) add(art_.stand, x, m.z, MAN_H, pal, flip);
        else add(art_.crouch, x, m.z, CROUCH_H, pal, flip);
    }
    std::sort(blobs.begin(), blobs.end(), [](const Blob& a, const Blob& b) { return a.z > b.z; });
    for (const Blob& b : blobs) {
        float sx, sy, s;
        world(b.x, 0, b.z, sx, sy, s);
        spr(*b.img, sx + shx_, sy + shy_, b.worldH * s, b.pal, b.flip, b.fog, true);
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
        const float when[4] = {0.f, 0.16f, 0.34f, 0.56f};
        const float note[4] = {174.f, 220.f, 261.f, 349.f};
        while (fanStep_ >= 0 && fanStep_ < 4 && fanT_ >= when[fanStep_]) {
            sys.apu.keyOn(1, note[fanStep_], 0.16f);
            fanStep_++;
        }
    }

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
            if (!bot_ && (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO))) wantFire_ = true;
            if (!bot_) humanAim(dt);
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

}  // namespace towermaga
