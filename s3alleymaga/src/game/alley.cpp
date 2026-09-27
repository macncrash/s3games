#include "game/alley.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace alleymaga {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FOCAL = 210.f;
constexpr float HORIZON = 78.f;
constexpr float EYE = 1.42f;
constexpr float MAN_H = 1.72f;
constexpr float CROUCH_H = 1.05f;
constexpr float FALL_H = 0.42f;
constexpr float CHEST = 1.15f;
constexpr float MOUTH_Z = 2.15f;
constexpr float SPAWN_Z = 17.2f;
constexpr float SLIP = 3.05f;
constexpr float RUSH_V = 3.55f;
constexpr float RUSH_X = 2.8f;
constexpr float RAID_LEN = 28.2f;
constexpr float BOLT = 0.30f;
constexpr float ALLEY_HALF = 1.55f;
constexpr int MAG = 8;
constexpr int NMEN = 7;

struct Plan {
    float t, side, hideZ, hold;
};

// Doorways along the brick. A man slips to the bin, waits, then steps into the alley.
// A shot only counts on that step. The last rush is cleared before the clock dies.
const Plan kPlan[NMEN] = {
    {0.40f, -1.35f, 8.6f, 0.55f},
    {3.90f, 1.35f, 9.4f, 0.50f},
    {7.50f, -1.42f, 10.2f, 0.62f},
    {11.20f, 1.28f, 8.2f, 0.48f},
    {14.80f, -1.22f, 9.8f, 0.58f},
    {18.40f, 1.40f, 11.0f, 0.52f},
    {22.10f, -1.30f, 8.9f, 0.46f},
};

const float kBricks[][2] = {{-2.15f, 4.2f}, {2.15f, 4.6f}, {-2.25f, 8.4f}, {2.25f, 8.8f},
                             {-2.35f, 13.2f}, {2.35f, 13.6f}, {-2.45f, 18.0f}, {2.45f, 18.4f}};
const float kEscapes[][2] = {{-2.05f, 6.4f}, {2.05f, 11.6f}, {-2.15f, 16.2f}};

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

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.3f;
    p.op[0] = {1, 0.65f, 0.4f, 0.75f, 0.8f, 0.5f};
    p.op[1] = {2, 0.25f, 0.3f, 0.65f, 0.65f, 0.4f};
    p.op[2] = {0.5f, 0.4f, 0.45f, 0.75f, 0.75f, 0.45f};
    p.op[3] = {1, 0.18f, 0.35f, 0.55f, 0.55f, 0.4f};
    p.vol = 0.04f;
    p.tone = 420;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.2f;
    p.op[0] = {1, 1, 0.01f, 0.15f, 0.45f, 0.18f};
    p.op[1] = {2, 0.35f, 0.01f, 0.16f, 0.3f, 0.16f};
    p.op[2] = {3, 0.2f, 0.02f, 0.18f, 0.22f, 0.16f};
    p.op[3] = {1, 0.28f, 0.01f, 0.18f, 0.35f, 0.18f};
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
    aimY_ = 112;
    rounds_ = MAG;
    if (bot_) beginRaid();
    else mode_ = Mode::Title;
}

const char* Game::result() const {
    if (won_) return "the magazine outlasts the raid";
    if (fail_ == Fail::Spent) return "magazine spent";
    if (fail_ == Fail::Mouth) return "they reached the mouth";
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
        if (m.phase == Phase::Door) ph = "D";
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
    bolt_ = 0.18f;
    kick_ = flash_ = shake_ = fanT_ = 0;
    aimX_ = 160;
    aimY_ = 112;
    men_.clear();
    sparks_.clear();
    casings_.clear();
    sys_->apu.keyOn(2, 49.f, 0.04f);
}

void Game::lose(Fail why) {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    fail_ = why;
    shake_ = why == Fail::Mouth ? 1.2f : 0.55f;
    sys_->apu.keyOff(2);
    sys_->apu.keyOn(1, why == Fail::Mouth ? 70.f : 96.f, 0.2f);
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
        if (m.phase != Phase::Rush) continue;
        float sx, sy, s;
        world(m.x, CHEST, m.z, sx, sy, s);
        float rad = std::max(18.f, MAN_H * s * 0.42f);
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
    Casing c;
    c.x = 250;
    c.y = 170;
    c.vx = 48;
    c.vy = -40;
    c.t = 0.36f;
    casings_.push_back(c);
    if (best) {
        kill(*best);
        sparks_.push_back({hx, hy, 0.15f});
        sys_->apu.keyOn(0, 104.f, 0.3f);
        sys_->apu.noiseBurst(0.48f, 1400.f, 0.06f);
    } else {
        sparks_.push_back({aimX_, aimY_, 0.12f});
        sys_->apu.keyOn(0, 164.f, 0.16f);
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
    aimX_ += ax * 250.f * dt;
    aimY_ += ay * 250.f * dt;
    aimX_ = std::clamp(aimX_, 40.f, 280.f);
    aimY_ = std::clamp(aimY_, 32.f, 180.f);
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
    float step = 1100.f * dt;
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
    men_.erase(std::remove_if(men_.begin(), men_.end(), [](const Man& m) { return m.phase == Phase::Dead && m.dead > 6.f; }),
               men_.end());
    if (bolt_ > 0) bolt_ -= dt;

    while (next_ < NMEN && raidT_ >= kPlan[next_].t) {
        const Plan& p = kPlan[next_++];
        Man m;
        m.side = p.side;
        m.x = p.side;
        m.z = SPAWN_Z;
        m.hideZ = p.hideZ;
        m.holdDur = p.hold;
        m.ph = float(next_) * 1.17f;
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
            float lean = m.side * (0.35f + 0.65f * std::clamp((m.z - m.hideZ) / 6.f, 0.f, 1.f));
            m.x += (m.side * 1.55f - lean - m.x) * std::min(1.f, dt * 4.f);
            if (m.z <= m.hideZ) {
                m.z = m.hideZ;
                m.x = m.side * 1.55f;
                m.phase = Phase::Door;
                m.cover = m.holdDur;
            }
        } else if (m.phase == Phase::Door) {
            m.x = m.side * 1.55f;
            m.cover -= dt;
            if (m.cover <= 0) {
                m.phase = Phase::Rush;
                sys_->apu.keyOn(1, 148.f, 0.08f);
            }
        } else if (m.phase == Phase::Rush) {
            float dx = -m.x * 0.15f - m.x;
            float step = RUSH_X * dt;
            if (std::fabs(dx) <= step) m.x = 0;
            else m.x += std::copysign(step, dx);
            m.z -= RUSH_V * dt;
            if (m.z < MOUTH_Z) {
                lose(Fail::Mouth);
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
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt * 2.8f);
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
    v.setFogColor(gs::rgb4(2, 2, 4));
    shx_ = shy_ = 0;
    if (shake_ > 0) {
        shx_ = std::sin(t_ * 91.f) * 2.6f * shake_;
        shy_ = std::cos(t_ * 67.f) * 1.6f * shake_;
    }

    const uint16_t skyTop = gs::rgb4(1, 1, 3);
    const uint16_t skyMid = gs::rgb4(2, 2, 5);
    const uint16_t skyHor = gs::rgb4(6, 3, 2);
    const int horizon = std::clamp(int(std::lround(HORIZON + shy_)), 60, 110);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < horizon) {
            float u = y / float(horizon);
            v.lineBackdrop[y] = u < 0.7f ? lerp4(skyTop, skyMid, u / 0.7f) : lerp4(skyMid, skyHor, (u - 0.7f) / 0.3f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - horizon) + 0.5f;
        float z = EYE * FOCAL / row;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.hw = ALLEY_HALF * row / EYE;
        r.cx = 160.f + shx_ + std::sin(z * 0.15f) * 6.f;
        r.v = z * 40.f;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(z / 1.6f)) & 1) ? 1 : 0;
        r.style = 0;
        r.left = r.right = 0;
        v.lineFog[y] = uint8_t(std::clamp(int(12.f - row * 0.12f), 0, 12));
        v.lineBackdrop[y] = gs::rgb4(2, 2, 3);
    }

    auto fogZ = [](float z) { return std::clamp(int((z - 3.5f) * 0.9f), 0, 14); };

    if (mode_ == Mode::Title) {
        text("S3 ALLEY MAGA", 160, 40, 0.95f, PAL_AMBER);
        text("MAKE THE MAGAZINE LAST", 160, 60, 0.55f, PAL_HUD);
        text("LONGER THAN THE RAID", 160, 74, 0.55f, PAL_HUD);
        text("ARROWS AIM    C FIRES", 160, 150, 0.52f, PAL_AMBER);
        text("THE DOORWAY DOES NOT COUNT", 160, 164, 0.48f, PAL_HUD);
        text("ONE ROUND MUST REMAIN", 160, 178, 0.48f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 196, 0.66f, PAL_GREEN);
        hud(1, 0, "THE ALLEY", PAL_AMBER);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("MAGAZINE HELD", 160, 46, 0.9f, PAL_GREEN);
        text("IT OUTLASTS THE RAID", 160, 68, 0.58f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 196, 0.56f, PAL_HUD);
        hud(1, 0, "RAID OVER", PAL_GREEN);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160, 46, 0.74f, PAL_RED);
        text(fail_ == Fail::Mouth ? "THEY REACHED THE MOUTH" : "MAGAZINE SPENT", 160, 68, 0.52f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 196, 0.56f, PAL_HUD);
        hud(1, 0, "WATCH OVER", PAL_RED);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 46, 0.95f, PAL_HUD);
    } else if (mode_ == Mode::Raid && !rushing()) {
        text(anyone() ? "HOLD" : "HOLD FIRE", 160, 48, 0.68f, PAL_AMBER);
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
        sprBox(art_.bar, 160, 26, 140, 4, PAL_IRON);
        if (remain > 0.02f) {
            float w = 140.f * remain;
            sprBox(art_.bar, 90.f + w * 0.5f, 26, w, 4, PAL_RED);
        }
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "MAG %02d", rounds_);
        hud(32, 0, buf, rounds_ > 0 ? PAL_GREEN : PAL_RED);
    }

    if (mode_ == Mode::Title || mode_ == Mode::Raid || mode_ == Mode::Pause)
        spr(art_.sight, aimX_ + shx_, aimY_ + shy_, 20, PAL_FX);

    for (const Spark& s : sparks_) spr(art_.spark, s.x + shx_, s.y + shy_, 9 + s.t * 18.f, PAL_FX);
    for (const Casing& c : casings_) spr(art_.round, c.x, c.y, 7, PAL_FX);

    float rifleX = 246.f + (aimX_ - 160.f) * 0.06f;
    float rifleY = 182.f - kick_ * 12.f;
    if (flash_ > 0) spr(art_.flash, rifleX - 4, rifleY - 44, 16, PAL_FX);
    spr(art_.rifle, rifleX, rifleY, 70, PAL_IRON);

    for (int i = 0; i < MAG; i++) {
        float x = 86.f + float(i) * 14.f;
        bool live = i < rounds_;
        spr(live ? art_.round : art_.spent, x, 208, 14, PAL_FX);
    }

    float flick = 0.78f + 0.22f * std::sin(t_ * 11.f);
    spr(art_.glow, 160, 36, 22.f * flick, PAL_LAMP);
    spr(art_.lamp, 160, 34, 18, PAL_LAMP);

    std::vector<Blob> blobs;
    auto add = [&](const gs::Mipped& img, float x, float z, float h, int pal, bool flip) {
        blobs.push_back({z, x, h, &img, pal, fogZ(z), flip});
    };
    for (const float* p : kBricks) add(art_.brick, p[0], p[1], 3.4f, PAL_BRICK, p[0] > 0);
    for (const float* p : kEscapes) add(art_.escape, p[0], p[1], 2.6f, PAL_IRON, p[0] > 0);
    for (const Plan& p : kPlan) add(art_.bin, p.side * 1.55f, p.hideZ + 0.15f, 0.85f, PAL_IRON, p.side > 0);
    add(art_.puddle, 0.2f, 5.4f, 0.18f, PAL_IRON, false);
    add(art_.puddle, -0.4f, 9.6f, 0.16f, PAL_IRON, false);
    if (mode_ == Mode::Title) add(art_.crouch, -1.35f * 1.55f, 8.6f, CROUCH_H, PAL_MAN, false);

    for (const Man& m : men_) {
        int pal = m.coat ? PAL_MANB : PAL_MAN;
        float x = m.x;
        if (m.phase == Phase::Slip) x += std::sin(t_ * 4.f + m.ph) * 0.08f;
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
        const float when[4] = {0.f, 0.18f, 0.36f, 0.6f};
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

}  // namespace alleymaga
