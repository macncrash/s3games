#include "game/span.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace spanmaga {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FOCAL = 240.f;
constexpr float HORIZON = 74.f;
constexpr float EYE = 1.55f;
constexpr float MAN_H = 1.72f;
constexpr float BRACE_H = 1.12f;
constexpr float FALL_H = 0.48f;
constexpr float CHEST = 1.12f;
constexpr float NEAR_Z = 2.15f;
constexpr float SPAWN_Z = 23.5f;
constexpr float WALK = 2.55f;
constexpr float CROSS_Z = 4.15f;
constexpr float CROSS_X = 2.8f;
constexpr float RAID_LEN = 27.6f;
constexpr float BOLT = 0.36f;
constexpr float DECK_HALF = 1.62f;
constexpr int MAG = 7;

struct Plan {
    float t, side, braceZ, hold;
};

// They walk the rail, brace in the truss bay, then cross the open deck.
// A shot only lands on the cross. Six crossings, seven rounds.
const Plan kPlan[] = {
    {0.40f, -1.35f, 11.0f, 0.60f}, {4.00f, 1.35f, 12.5f, 0.55f}, {8.20f, -1.42f, 10.2f, 0.50f},
    {12.00f, 1.28f, 13.0f, 0.45f},  {15.50f, -1.38f, 11.4f, 0.40f}, {18.80f, 1.40f, 12.0f, 0.35f},
};
constexpr int NRAID = int(sizeof kPlan / sizeof kPlan[0]);

const float kBays[] = {6.4f, 9.2f, 12.2f, 15.4f, 18.6f, 21.6f};
const float kStars[][2] = {{28, 28}, {64, 18}, {102, 34}, {188, 22}, {236, 40}, {280, 16}, {304, 36}};

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
    p.op[2] = {0.5f, 0.7f, 0.001f, 0.08f, 0.0f, 0.05f};
    p.op[3] = {3.2f, 0.18f, 0.001f, 0.04f, 0.0f, 0.03f};
    p.vol = 0.28f;
    p.drive = 0.45f;
    p.tone = 1800;
    return p;
}

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.3f;
    p.op[0] = {1, 0.65f, 0.5f, 0.8f, 0.85f, 0.55f};
    p.op[1] = {2, 0.25f, 0.4f, 0.7f, 0.7f, 0.45f};
    p.op[2] = {0.5f, 0.4f, 0.5f, 0.8f, 0.8f, 0.5f};
    p.op[3] = {1, 0.18f, 0.4f, 0.6f, 0.6f, 0.45f};
    p.vol = 0.045f;
    p.tone = 420;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.2f;
    p.op[0] = {1, 1, 0.01f, 0.16f, 0.5f, 0.2f};
    p.op[1] = {2, 0.35f, 0.012f, 0.18f, 0.3f, 0.18f};
    p.op[2] = {3, 0.18f, 0.02f, 0.2f, 0.22f, 0.18f};
    p.op[3] = {1, 0.28f, 0.012f, 0.2f, 0.35f, 0.2f};
    p.vol = 0.16f;
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
    if (fail_ == Fail::Across) return "they crossed the span";
    return "the span is lost";
}

bool Game::crossing() const {
    for (const Man& m : men_)
        if (m.phase == Phase::Cross) return true;
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
    bolt_ = 0.18f;
    kick_ = flash_ = shake_ = fanT_ = 0;
    aimX_ = 160;
    aimY_ = 100;
    men_.clear();
    sparks_.clear();
    casings_.clear();
    sys_->apu.keyOn(2, 48.f, 0.04f);
}

void Game::lose(Fail why) {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    fail_ = why;
    shake_ = why == Fail::Across ? 1.2f : 0.55f;
    sys_->apu.keyOff(2);
    sys_->apu.keyOn(1, why == Fail::Across ? 70.f : 96.f, 0.2f);
    sys_->apu.noiseBurst(0.35f, 220.f, 0.22f);
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
        if (m.phase != Phase::Cross) continue;
        float sx, sy, s;
        world(m.x, CHEST, m.z, sx, sy, s);
        float rad = std::max(16.f, MAN_H * s * 0.4f);
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
    shake_ = best ? 0.35f : 0.18f;
    if (!bot_) aimY_ = std::max(36.f, aimY_ - 7.f);
    casings_.push_back({274.f, 170.f, 48.f, -40.f, 0.36f});
    if (best) {
        kill(*best);
        sparks_.push_back({hx, hy, 0.16f});
        sys_->apu.keyOn(0, 104.f, 0.3f);
        sys_->apu.noiseBurst(0.48f, 1400.f, 0.06f);
    } else {
        sparks_.push_back({aimX_, aimY_, 0.12f});
        sys_->apu.keyOn(0, 168.f, 0.16f);
        sys_->apu.noiseBurst(0.25f, 4600.f, 0.035f);
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
    aimX_ += ax * 260.f * dt;
    aimY_ += ay * 260.f * dt;
    aimX_ = std::clamp(aimX_, 40.f, 280.f);
    aimY_ = std::clamp(aimY_, 32.f, 180.f);
}

void Game::botAct(float dt) {
    const Man* threat = nullptr;
    const Man* watch = nullptr;
    for (const Man& m : men_) {
        if (m.phase == Phase::Dead) continue;
        if (!watch || m.z < watch->z) watch = &m;
        if (m.phase == Phase::Cross && (!threat || m.z < threat->z)) threat = &m;
    }
    const Man* aim = threat ? threat : watch;
    if (!aim) return;
    float sx, sy, s;
    world(aim->x, CHEST, aim->z, sx, sy, s);
    float dx = sx - aimX_, dy = sy - aimY_;
    float d = std::hypot(dx, dy);
    float step = 980.f * dt;
    if (d <= step) {
        aimX_ = sx;
        aimY_ = sy;
        d = 0;
    } else if (d > 0.001f) {
        aimX_ += dx / d * step;
        aimY_ += dy / d * step;
        d -= step;
    }
    float rad = std::max(16.f, MAN_H * s * 0.4f);
    if (threat && bolt_ <= 0 && d <= rad) pull();
}

void Game::updateRaid(float dt) {
    men_.erase(std::remove_if(men_.begin(), men_.end(), [](const Man& m) { return m.phase == Phase::Dead && m.dead > 8.f; }),
               men_.end());
    if (bolt_ > 0) bolt_ -= dt;

    while (next_ < NRAID && raidT_ >= kPlan[next_].t) {
        const Plan& p = kPlan[next_++];
        Man m;
        m.side = p.side;
        m.x = p.side;
        m.z = SPAWN_Z;
        m.braceZ = p.braceZ;
        m.hold = p.hold;
        m.ph = float(next_) * 1.7f;
        m.coat = next_ & 1;
        m.phase = Phase::Walk;
        men_.push_back(m);
    }

    for (Man& m : men_) {
        if (m.phase == Phase::Dead) {
            m.dead += dt;
            continue;
        }
        if (m.phase == Phase::Walk) {
            m.z -= WALK * dt;
            m.x = m.side;
            if (m.z <= m.braceZ) {
                m.z = m.braceZ;
                m.phase = Phase::Brace;
            }
        } else if (m.phase == Phase::Brace) {
            m.hold -= dt;
            if (m.hold <= 0) {
                m.phase = Phase::Cross;
                sys_->apu.keyOn(1, 148.f, 0.08f);
            }
        } else if (m.phase == Phase::Cross) {
            float dx = -m.x;
            float step = CROSS_X * dt;
            if (std::fabs(dx) <= step) m.x = 0;
            else m.x += std::copysign(step, dx);
            m.z -= CROSS_Z * dt;
            if (m.z < NEAR_Z) {
                lose(Fail::Across);
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
    if (kick_ > 0) kick_ = std::max(0.f, kick_ - dt * 3.4f);
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt * 2.4f);
    for (Spark& s : sparks_) s.t -= dt;
    sparks_.erase(std::remove_if(sparks_.begin(), sparks_.end(), [](const Spark& s) { return s.t <= 0; }), sparks_.end());
    for (Casing& c : casings_) {
        c.t -= dt;
        c.vy += 150.f * dt;
        c.x += c.vx * dt;
        c.y += c.vy * dt;
    }
    casings_.erase(std::remove_if(casings_.begin(), casings_.end(), [](const Casing& c) { return c.t <= 0; }), casings_.end());
}

void Game::world(float x, float y, float z, float& sx, float& sy, float& s) const {
    float zz = std::max(0.4f, z);
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
    v.roadTime = int(t_ * 60.f);
    v.setFogColor(gs::rgb4(3, 5, 8));
    float shx = 0, shy = 0;
    if (shake_ > 0) {
        shx = std::sin(t_ * 90.f) * 2.6f * shake_;
        shy = std::cos(t_ * 67.f) * 1.6f * shake_;
    }

    const uint16_t skyTop = gs::rgb4(1, 2, 6);
    const uint16_t skyMid = gs::rgb4(3, 5, 9);
    const uint16_t skyHor = gs::rgb4(12, 6, 3);
    const int horizon = std::clamp(int(std::lround(HORIZON + shy)), 60, 110);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < horizon) {
            float u = y / float(horizon);
            v.lineBackdrop[y] = u < 0.62f ? lerp4(skyTop, skyMid, u / 0.62f) : lerp4(skyMid, skyHor, (u - 0.62f) / 0.38f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - horizon) + 0.5f;
        float z = EYE * FOCAL / row;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.hw = DECK_HALF * row / EYE;
        r.cx = 160.f + shx + std::sin(z * 0.08f + t_ * 0.4f) * 1.4f;
        r.v = z * 28.f;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(z / 3.1f)) & 1) ? 1 : 0;
        r.style = 1;
        r.left = r.right = gs::GROUND_WATER;
        v.lineFog[y] = uint8_t(std::clamp(int(13.f - row * 0.12f), 0, 13));
        v.lineBackdrop[y] = gs::rgb4(1, 3, 6);
    }

    auto fogZ = [](float z) { return std::clamp(int((z - 5.f) * 0.72f), 0, 13); };

    if (mode_ == Mode::Title) {
        text("S3 SPAN MAGA", 160, 36, 0.95f, PAL_AMBER);
        text("MAKE THE MAGAZINE LAST", 160, 54, 0.52f, PAL_HUD);
        text("LONGER THAN THE RAID", 160, 66, 0.52f, PAL_HUD);
        text("ARROWS AIM    C FIRES", 160, 148, 0.5f, PAL_AMBER);
        text("THE TRUSS DOES NOT COUNT", 160, 162, 0.46f, PAL_HUD);
        text("ONE ROUND MUST REMAIN", 160, 176, 0.46f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 196, 0.64f, PAL_GREEN);
        hud(1, 0, "THE SPAN", PAL_AMBER);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("MAGAZINE HELD", 160, 44, 0.9f, PAL_GREEN);
        text("IT OUTLASTS THE RAID", 160, 64, 0.55f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 196, 0.55f, PAL_HUD);
        hud(1, 0, "RAID OVER", PAL_GREEN);
    } else if (mode_ == Mode::Lost) {
        text("THE SPAN IS LOST", 160, 44, 0.78f, PAL_RED);
        text(fail_ == Fail::Across ? "THEY CROSSED" : "MAGAZINE SPENT", 160, 64, 0.55f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 196, 0.55f, PAL_HUD);
        hud(1, 0, "SPAN LOST", PAL_RED);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 44, 0.9f, PAL_HUD);
    } else if (mode_ == Mode::Raid && !crossing()) {
        text("HOLD THE SPAN", 160, 46, 0.62f, PAL_AMBER);
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
        sprBox(art_.bar, 160, 24, 140, 4, PAL_PIER);
        if (remain > 0.02f) {
            float w = 140.f * remain;
            sprBox(art_.bar, 90.f + w * 0.5f, 24, w, 4, PAL_RED);
        }
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "MAG %02d", rounds_);
        hud(32, 0, buf, rounds_ > 0 ? PAL_GREEN : PAL_RED);
    }

    if (mode_ == Mode::Title || mode_ == Mode::Raid || mode_ == Mode::Pause) spr(art_.sight, aimX_ + shx, aimY_ + shy, 20, PAL_FX);

    for (const Spark& s : sparks_) spr(art_.spark, s.x + shx, s.y + shy, 8 + s.t * 22.f, PAL_FX);
    for (const Casing& c : casings_) spr(art_.round, c.x, c.y, 7, PAL_FX);

    float rifleX = 262.f + (aimX_ - 160.f) * 0.06f;
    float rifleY = 176.f - kick_ * 12.f;
    if (flash_ > 0) spr(art_.flash, rifleX - 2, rifleY - 42, 16, PAL_FX);
    spr(art_.rifle, rifleX, rifleY, 70, PAL_STEEL);

    for (int i = 0; i < MAG; i++) {
        float x = 118.f + float(i) * 12.f;
        spr(i < rounds_ ? art_.round : art_.spent, x, 208, 14, PAL_FX);
    }

    spr(art_.abutment, 18, 120, 200, PAL_STEEL, false);
    spr(art_.abutment, 302, 120, 200, PAL_STEEL, true);
    spr(art_.lamp, 36, 58, 18, PAL_FX);
    spr(art_.lamp, 284, 58, 18, PAL_FX);

    std::vector<Blob> blobs;
    auto add = [&](const gs::Mipped& img, float x, float z, float h, int pal, bool flip) {
        blobs.push_back({z, x, h, &img, pal, fogZ(z), flip});
    };
    for (float z : kBays) {
        add(art_.truss, -2.15f, z, 3.4f, PAL_STEEL, false);
        add(art_.truss, 2.15f, z, 3.4f, PAL_STEEL, true);
        add(art_.cable, 0, z + 0.2f, 0.35f, PAL_STEEL, false);
    }
    add(art_.pier, -3.6f, 8.5f, 2.4f, PAL_PIER, false);
    add(art_.pier, 3.6f, 8.5f, 2.4f, PAL_PIER, true);
    add(art_.pier, -3.8f, 16.5f, 2.6f, PAL_PIER, false);
    add(art_.pier, 3.8f, 16.5f, 2.6f, PAL_PIER, true);

    if (mode_ == Mode::Title) add(art_.brace, -1.35f, 11.0f, BRACE_H, PAL_COAT, false);

    for (const Man& m : men_) {
        int pal = m.coat ? PAL_COATB : PAL_COAT;
        bool flip = m.side > 0;
        if (m.phase == Phase::Dead) add(art_.fallen, m.x, m.z, FALL_H, pal, flip);
        else if (m.phase == Phase::Cross) add(art_.stand, m.x, m.z, MAN_H, pal, flip);
        else add(art_.brace, m.x, m.z, BRACE_H, pal, flip);
    }
    std::sort(blobs.begin(), blobs.end(), [](const Blob& a, const Blob& b) { return a.z < b.z; });
    for (const Blob& b : blobs) {
        float sx, sy, s;
        world(b.x, 0, b.z, sx, sy, s);
        float h = b.worldH * s;
        if (b.img == &art_.cable) {
            sprBox(*b.img, sx + shx, sy + shy - h * 2.2f, std::max(8.f, 4.3f * s), std::max(2.f, h), b.pal, b.fog);
        } else {
            spr(*b.img, sx + shx, sy + shy, h, b.pal, b.flip, b.fog, true);
        }
    }

    spr(art_.moon, 248, 30, 22, PAL_DUSK);
    for (const float* st : kStars) spr(art_.star, st[0], st[1], 3, PAL_DUSK);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = DT;
    if (mode_ != Mode::Pause) t_ += dt;
    tickFx(dt);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Won) {
        fanT_ += dt;
        const float when[4] = {0.f, 0.15f, 0.32f, 0.55f};
        const float note[4] = {196.f, 247.f, 294.f, 392.f};
        while (fanStep_ >= 0 && fanStep_ < 4 && fanT_ >= when[fanStep_]) {
            sys.apu.keyOn(1, note[fanStep_], 0.18f);
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

}  // namespace spanmaga
