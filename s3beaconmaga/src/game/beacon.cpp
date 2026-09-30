#include "game/beacon.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace beaconmaga {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FOCAL = 220.f;
constexpr float HORIZON = 84.f;
constexpr float EYE = 1.62f;
constexpr float MAN_H = 1.74f;
constexpr float FALL_H = 0.38f;
constexpr float CHEST = 1.12f;
constexpr float MOUTH_Z = 2.25f;
constexpr float SPAWN_Z = 17.4f;
constexpr float WALK = 2.15f;
constexpr float RAID_LEN = 28.f;
constexpr float BOLT = 0.28f;
constexpr float BEAM_HALF = 0.62f;
constexpr float CAUSE_HALF = 1.85f;
constexpr float BEAM_SLEW = 3.6f;
constexpr int MAG = 6;
constexpr int NMEN = 5;

struct Plan {
    float t, lane;
};

// Five figures up the causeway. Each one has to be stopped inside the beam.
// One round is left when the clock dies. A sixth pull spends the magazine.
const Plan kPlan[NMEN] = {
    {0.55f, -1.15f}, {5.15f, 1.05f}, {9.70f, 0.15f}, {14.20f, -0.85f}, {18.55f, 1.20f},
};

const float kPosts[][2] = {{-2.35f, 4.4f}, {2.35f, 5.2f}, {-2.45f, 9.0f}, {2.45f, 9.8f},
                            {-2.55f, 13.6f}, {2.55f, 14.4f}};
const float kRocks[][2] = {{-3.1f, 7.2f}, {3.2f, 11.4f}, {-3.3f, 16.0f}, {2.9f, 3.6f}};

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
    p.op[1] = {2.2f, 0.3f, 0.001f, 0.04f, 0.0f, 0.03f};
    p.op[2] = {0.5f, 0.7f, 0.001f, 0.08f, 0.0f, 0.05f};
    p.op[3] = {3.0f, 0.18f, 0.001f, 0.04f, 0.0f, 0.03f};
    p.vol = 0.26f;
    p.drive = 0.4f;
    p.tone = 1600;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.18f;
    p.op[0] = {1, 1, 0.01f, 0.2f, 0.5f, 0.2f};
    p.op[1] = {2, 0.3f, 0.01f, 0.18f, 0.35f, 0.18f};
    p.op[2] = {3, 0.16f, 0.02f, 0.2f, 0.25f, 0.16f};
    p.op[3] = {0.5f, 0.4f, 0.02f, 0.22f, 0.4f, 0.2f};
    p.vol = 0.14f;
    return p;
}

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.25f;
    p.op[0] = {1, 0.55f, 0.4f, 0.7f, 0.75f, 0.45f};
    p.op[1] = {2, 0.2f, 0.3f, 0.6f, 0.6f, 0.35f};
    p.op[2] = {0.5f, 0.35f, 0.4f, 0.7f, 0.7f, 0.4f};
    p.op[3] = {1, 0.15f, 0.3f, 0.5f, 0.5f, 0.35f};
    p.vol = 0.035f;
    p.tone = 280;
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
    aimY_ = 120;
    rounds_ = MAG;
    if (bot_) beginRaid();
    else mode_ = Mode::Title;
}

const char* Game::result() const {
    if (won_) return "the magazine outlasts the raid";
    if (fail_ == Fail::Spent) return "magazine spent";
    if (fail_ == Fail::Lamp) return "they took the lamp";
    return "the lamp is lost";
}

int Game::marker() const {
    if (over_) return 2;
    if (mode_ == Mode::Raid || mode_ == Mode::Pause) return 1;
    return 0;
}

std::string Game::trace() const {
    std::string s;
    char b[96];
    std::snprintf(b, sizeof b, "trace t=%.2f beam=%.2f rounds=%d stopped=%d next=%d men=%zu\n", raidT_, beamX_, rounds_,
                  stopped_, next_, men_.size());
    s += b;
    for (const Man& m : men_) {
        std::snprintf(b, sizeof b, "  %c z=%.2f x=%.2f\n", m.down ? 'X' : 'W', m.z, m.x);
        s += b;
    }
    return s;
}

bool Game::lit(const Man& m) const { return std::fabs(m.x - beamX_) <= BEAM_HALF; }

bool Game::anyone() const {
    for (const Man& m : men_)
        if (!m.down) return true;
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
    aimY_ = 120;
    beamX_ = 0;
    men_.clear();
    sparks_.clear();
    sys_->apu.keyOn(2, 55.f, 0.035f);
}

void Game::lose(Fail why) {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    fail_ = why;
    shake_ = why == Fail::Lamp ? 1.15f : 0.5f;
    sys_->apu.keyOff(2);
    sys_->apu.keyOn(1, why == Fail::Lamp ? 66.f : 98.f, 0.2f);
    sys_->apu.noiseBurst(0.32f, 180.f, 0.22f);
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

void Game::pull() {
    if (mode_ != Mode::Raid || rounds_ <= 0 || bolt_ > 0) return;
    Man* best = nullptr;
    float bestD = 1e9f;
    float hx = aimX_, hy = aimY_;
    for (Man& m : men_) {
        if (m.down || !lit(m)) continue;
        float sx, sy, s;
        world(m.x, CHEST, m.z, sx, sy, s);
        float rad = std::max(16.f, MAN_H * s * 0.38f);
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
    shake_ = best ? 0.32f : 0.14f;
    if (!bot_) aimY_ = std::max(40.f, aimY_ - 6.f);
    if (best) {
        best->down = true;
        best->dead = 0;
        stopped_++;
        sparks_.push_back({hx, hy, 0.16f});
        sys_->apu.keyOn(0, 98.f, 0.28f);
        sys_->apu.noiseBurst(0.42f, 1200.f, 0.06f);
    } else {
        sparks_.push_back({aimX_, aimY_, 0.1f});
        sys_->apu.keyOn(0, 180.f, 0.12f);
        sys_->apu.noiseBurst(0.2f, 4200.f, 0.03f);
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
    beamX_ = std::clamp(beamX_ + ax * BEAM_SLEW * dt, -1.7f, 1.7f);
    aimY_ = std::clamp(aimY_ + ay * 220.f * dt, 40.f, 176.f);
}

void Game::botAct(float dt) {
    const Man* threat = nullptr;
    for (const Man& m : men_) {
        if (m.down) continue;
        if (!threat || m.z < threat->z) threat = &m;
    }
    if (!threat) return;
    float dx = threat->x - beamX_;
    float step = BEAM_SLEW * dt;
    if (std::fabs(dx) <= step) beamX_ = threat->x;
    else beamX_ += std::copysign(step, dx);
    beamX_ = std::clamp(beamX_, -1.7f, 1.7f);

    float sx, sy, s;
    world(threat->x, CHEST, threat->z, sx, sy, s);
    float dy = sy - aimY_;
    float ystep = 1400.f * dt;
    if (std::fabs(dy) <= ystep) aimY_ = sy;
    else aimY_ += std::copysign(ystep, dy);

    float d = std::hypot(aimX_ - sx, aimY_ - sy);
    float rad = std::max(16.f, MAN_H * s * 0.38f);
    if (lit(*threat) && threat->z < 10.5f && d <= rad && bolt_ <= 0) pull();
}

void Game::updateRaid(float dt) {
    men_.erase(std::remove_if(men_.begin(), men_.end(), [](const Man& m) { return m.down && m.dead > 5.f; }), men_.end());
    if (bolt_ > 0) bolt_ -= dt;

    while (next_ < NMEN && raidT_ >= kPlan[next_].t) {
        const Plan& p = kPlan[next_++];
        Man m;
        m.lane = p.lane;
        m.x = p.lane;
        m.z = SPAWN_Z;
        m.bob = float(next_) * 0.9f;
        m.coat = next_ & 1;
        men_.push_back(m);
    }

    for (Man& m : men_) {
        if (m.down) {
            m.dead += dt;
            continue;
        }
        m.z -= WALK * dt;
        m.x = m.lane + std::sin(raidT_ * 1.7f + m.bob) * 0.06f;
        if (m.z < MOUTH_Z) {
            lose(Fail::Lamp);
            return;
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

struct Blob {
    float z, x, worldH;
    const gs::Mipped* img;
    int pal, fog;
    bool flip;
};

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.setFogColor(gs::rgb4(1, 1, 3));
    v.roadTime = int(t_ * 60.f);
    shx_ = shy_ = 0;
    if (shake_ > 0) {
        shx_ = std::sin(t_ * 90.f) * 2.4f * shake_;
        shy_ = std::cos(t_ * 70.f) * 1.4f * shake_;
    }

    float bsx, bsy, bs;
    world(beamX_, 0.2f, 6.5f, bsx, bsy, bs);
    aimX_ = bsx;

    const uint16_t skyTop = gs::rgb4(0, 0, 2);
    const uint16_t skyMid = gs::rgb4(1, 1, 4);
    const uint16_t skyHor = gs::rgb4(3, 2, 4);
    const int horizon = std::clamp(int(std::lround(HORIZON + shy_)), 64, 110);
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
        r.hw = CAUSE_HALF * row / EYE;
        r.cx = 160.f + shx_;
        r.v = z * 36.f;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(z / 2.2f)) & 1) ? 1 : 0;
        r.style = 1;
        r.left = r.right = gs::GROUND_WATER;
        v.lineFog[y] = uint8_t(std::clamp(int(13.f - row * 0.11f), 1, 13));
        v.lineBackdrop[y] = gs::rgb4(1, 1, 3);
    }

    auto fogZ = [](float z) { return std::clamp(int((z - 3.2f) * 0.85f), 0, 14); };

    if (mode_ == Mode::Title) {
        text("S3 BEACON MAGA", 160, 36, 0.86f, PAL_AMBER);
        text("MAKE THE MAGAZINE LAST", 160, 56, 0.5f, PAL_HUD);
        text("LONGER THAN THE RAID", 160, 70, 0.5f, PAL_HUD);
        text("ARROWS SWEEP THE BEAM", 160, 148, 0.48f, PAL_AMBER);
        text("A ROUND ONLY LANDS IN THE LIGHT", 160, 162, 0.42f, PAL_HUD);
        text("ONE ROUND MUST REMAIN", 160, 176, 0.46f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 198, 0.62f, PAL_GREEN);
        hud(1, 0, "THE LAMP", PAL_AMBER);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("MAGAZINE HELD", 160, 42, 0.86f, PAL_GREEN);
        text("IT OUTLASTS THE RAID", 160, 64, 0.54f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 196, 0.54f, PAL_HUD);
        hud(1, 0, "RAID OVER", PAL_GREEN);
    } else if (mode_ == Mode::Lost) {
        text("THE LAMP IS LOST", 160, 42, 0.72f, PAL_RED);
        text(fail_ == Fail::Lamp ? "THEY TOOK THE LAMP" : "MAGAZINE SPENT", 160, 64, 0.5f, PAL_AMBER);
        if (int(t_ * 2.f) & 1) text("START", 160, 196, 0.54f, PAL_HUD);
        hud(1, 0, "LAMP LOST", PAL_RED);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 42, 0.9f, PAL_HUD);
    } else if (mode_ == Mode::Raid && !anyone()) {
        text("HOLD THE BEAM", 160, 44, 0.58f, PAL_AMBER);
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
        sprBox(art_.bar, 160, 22, 150, 4, PAL_IRON);
        if (remain > 0.02f) {
            float w = 150.f * remain;
            sprBox(art_.bar, 85.f + w * 0.5f, 22, w, 4, PAL_RED);
        }
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "MAG %02d", rounds_);
        hud(32, 0, buf, rounds_ > 0 ? PAL_GREEN : PAL_RED);
    }

    for (int i = 0; i < 7; i++) {
        float z = 3.2f + float(i) * 1.7f;
        float sx, sy, sc;
        world(beamX_, 0.35f, z, sx, sy, sc);
        float h = (26.f - float(i) * 2.4f) * (0.85f + 0.15f * std::sin(t_ * 9.f + i));
        spr(art_.beam, sx + shx_, sy + shy_, h, PAL_LAMP, false, std::min(12, i + 2));
    }

    std::vector<Blob> blobs;
    auto add = [&](const gs::Mipped& img, float x, float z, float h, int pal, bool flip) {
        blobs.push_back({z, x, h, &img, pal, fogZ(z), flip});
    };
    for (const float* p : kPosts) add(art_.post, p[0], p[1], 2.4f, PAL_STONE, false);
    for (const float* p : kRocks) add(art_.rock, p[0], p[1], 0.7f, PAL_STONE, p[0] > 0);
    if (mode_ == Mode::Title) add(art_.walker, -1.15f, 8.4f, MAN_H, PAL_COAT, false);

    for (const Man& m : men_) {
        int pal = m.coat ? PAL_COATB : PAL_COAT;
        if (m.down) add(art_.fallen, m.x, m.z, FALL_H, pal, m.lane > 0);
        else add(art_.walker, m.x, m.z, MAN_H, pal, m.lane > 0);
        if (!m.down && lit(m)) {
            float sx, sy, sc;
            world(m.x, CHEST, m.z, sx, sy, sc);
            spr(art_.glow, sx + shx_, sy + shy_, 18.f + 4.f * std::sin(t_ * 14.f), PAL_LAMP, false, fogZ(m.z));
        }
    }
    std::sort(blobs.begin(), blobs.end(), [](const Blob& a, const Blob& b) { return a.z > b.z; });
    for (const Blob& b : blobs) {
        float sx, sy, s;
        world(b.x, 0, b.z, sx, sy, s);
        spr(*b.img, sx + shx_, sy + shy_, b.worldH * s, b.pal, b.flip, b.fog, true);
    }

    if (mode_ == Mode::Title || mode_ == Mode::Raid || mode_ == Mode::Pause)
        spr(art_.sight, aimX_ + shx_, aimY_ + shy_, 18, PAL_FX);

    for (const Spark& s : sparks_) spr(art_.spark, s.x + shx_, s.y + shy_, 8 + s.t * 16.f, PAL_FX);

    float rifleX = 248.f + (aimX_ - 160.f) * 0.05f;
    float rifleY = 186.f - kick_ * 10.f;
    if (flash_ > 0) spr(art_.flash, rifleX - 6, rifleY - 40, 14, PAL_FX);
    spr(art_.rifle, rifleX, rifleY, 64, PAL_IRON);

    for (int i = 0; i < MAG; i++) {
        float x = 108.f + float(i) * 16.f;
        spr(i < rounds_ ? art_.round : art_.spent, x, 208, 13, PAL_FX);
    }

    float flick = 0.8f + 0.2f * std::sin(t_ * 8.f);
    spr(art_.glow, 160, 196, 28.f * flick, PAL_LAMP);
    spr(art_.cage, 160, 198, 34, PAL_LAMP);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = DT;
    if (mode_ != Mode::Pause) t_ += dt;
    tickFx(dt);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Won) {
        fanT_ += dt;
        const float when[4] = {0.f, 0.16f, 0.34f, 0.58f};
        const float note[4] = {220.f, 277.f, 330.f, 440.f};
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

}  // namespace beaconmaga
