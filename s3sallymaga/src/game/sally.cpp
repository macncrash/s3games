#include "game/sally.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace sallymaga {
namespace {

constexpr int MAG = 6;
constexpr int NMAN = 5;
constexpr float SPAWN_Z = 17.f;
constexpr float GATE_Z = 3.1f;
constexpr float SPEED = 1.85f;
constexpr float BOLT = 0.2f;
constexpr float HORIZON = 70.f;
constexpr float RAID_LEN = 15.5f;

struct Plan {
    float t;
    float x;
};

constexpr Plan kPlan[NMAN] = {
    {0.4f, -0.9f}, {2.1f, 0.75f}, {3.9f, -0.25f}, {5.6f, 0.95f}, {7.4f, 0.1f},
};

}  // namespace

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "THE MAGAZINE RAN DRY";
    if (fail_ == Fail::Gate) return "THE SALLY IS FORCED";
    return "THE RAID IS NOT DONE";
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Raid) return 1;
    return 2;
}

bool Game::anyLive() const {
    for (const Man& m : men_)
        if (!m.down) return true;
    return false;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.clear();
    sys.vdp.B.clear();
    sys.vdp.HUD.clear();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.8f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
}

void Game::beginRaid() {
    mode_ = Mode::Raid;
    over_ = false;
    won_ = false;
    fail_ = Fail::None;
    wantFire_ = false;
    rounds_ = MAG;
    down_ = 0;
    next_ = 0;
    raidT_ = 0;
    bolt_ = 0.18f;
    kick_ = shake_ = 0;
    aimX_ = 160;
    aimY_ = 124;
    men_.clear();
    puffs_.clear();
    sys_->apu.keyOn(1, 146.f, 0.1f);
    sys_->setLight(90, 50, 30);
}

void Game::lose(Fail why) {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    fail_ = why;
    shake_ = why == Fail::Gate ? 1.1f : 0.5f;
    sys_->apu.keyOn(1, why == Fail::Gate ? 64.f : 90.f, 0.18f);
    sys_->apu.noiseBurst(0.32f, 160.f, 0.2f);
    sys_->rumble(0.5f, 0.2f, 140);
    sys_->setLight(160, 28, 18);
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
    sys_->apu.noiseBurst(0.16f, 90.f, 0.08f);
    sys_->apu.keyOn(2, 392.f, 0.12f);
    sys_->setLight(40, 140, 70);
}

void Game::project(float x, float z, float& sx, float& sy, float& s) const {
    float d = std::max(0.45f, z);
    s = 72.f / (d + 2.2f);
    sy = HORIZON + 360.f / (d + 1.05f);
    sx = 160.f + x * (220.f / (d * 0.5f + 1.35f));
}

void Game::pull() {
    if (mode_ != Mode::Raid || rounds_ <= 0 || bolt_ > 0) return;
    Man* best = nullptr;
    float bestD = 1e9f;
    float hx = aimX_, hy = aimY_;
    for (Man& m : men_) {
        if (m.down) continue;
        float sx, sy, sc;
        project(m.x, m.z, sx, sy, sc);
        float rad = std::max(18.f, 20.f * sc * 3.f);
        float d = std::hypot(aimX_ - sx, aimY_ - (sy - 10.f));
        if (d <= rad && d < bestD) {
            bestD = d;
            best = &m;
            hx = sx;
            hy = sy - 8.f;
        }
    }
    rounds_--;
    bolt_ = BOLT;
    kick_ = 1.f;
    shake_ = best ? 0.35f : 0.15f;
    Puff p;
    p.x = hx;
    p.y = hy;
    p.t = 0.26f;
    puffs_.push_back(p);
    if (best) {
        best->down = true;
        best->dead = 0;
        down_++;
        sys_->apu.keyOn(0, 98.f, 0.26f);
        sys_->apu.noiseBurst(0.45f, 700.f, 0.07f);
        sys_->rumble(0.25f, 0.55f, 45);
    } else {
        sys_->apu.keyOn(0, 180.f, 0.1f);
        sys_->apu.noiseBurst(0.16f, 2800.f, 0.04f);
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
    aimY_ += ay * 200.f * dt;
    aimX_ = std::clamp(aimX_, 28.f, 292.f);
    aimY_ = std::clamp(aimY_, 78.f, 188.f);
}

void Game::botAct(float dt) {
    const Man* threat = nullptr;
    for (const Man& m : men_) {
        if (m.down) continue;
        if (!threat || m.z < threat->z) threat = &m;
    }
    if (!threat) return;
    float sx, sy, sc;
    project(threat->x, threat->z, sx, sy, sc);
    sy -= 10.f;
    float dx = sx - aimX_, dy = sy - aimY_;
    float d = std::hypot(dx, dy);
    float step = 2400.f * dt;
    if (d <= step) {
        aimX_ = sx;
        aimY_ = sy;
        d = 0;
    } else if (d > 0.001f) {
        aimX_ += dx / d * step;
        aimY_ += dy / d * step;
        d -= step;
    }
    if (bolt_ > 0) return;
    if (threat->z < 13.f && d <= 8.f) pull();
    else if (threat->z < 5.2f) {
        aimX_ = sx;
        aimY_ = sy;
        pull();
    }
}

void Game::updateRaid(float dt) {
    if (bolt_ > 0) bolt_ -= dt;
    while (next_ < NMAN && raidT_ >= kPlan[next_].t) {
        Man m;
        m.x = kPlan[next_].x;
        m.z = SPAWN_Z;
        m.flip = kPlan[next_].x > 0;
        men_.push_back(m);
        next_++;
    }
    for (Man& m : men_) {
        if (m.down) {
            m.dead += dt;
            continue;
        }
        m.z -= SPEED * dt;
        if (m.z < GATE_Z) {
            lose(Fail::Gate);
            return;
        }
    }
    if (mode_ != Mode::Raid) return;
    men_.erase(std::remove_if(men_.begin(), men_.end(), [](const Man& m) { return m.down && m.dead > 2.2f; }),
               men_.end());
    if (bot_) botAct(dt);
    else if (wantFire_ && bolt_ <= 0) {
        wantFire_ = false;
        pull();
    }
    if (mode_ != Mode::Raid) return;
    raidT_ += dt;
    if (!anyLive() && next_ >= NMAN && rounds_ > 0 && down_ >= NMAN && raidT_ >= RAID_LEN) win();
    else if (raidT_ >= RAID_LEN && anyLive()) lose(Fail::Gate);
}

bool Game::fireEdge() {
    const gs::Pad& pad = sys_->pad;
    bool trig = pad.accel > 0.45f;
    bool edge = (trig && !trigWas_) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
    trigWas_ = trig;
    return edge;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (m.h <= 0 || h < 1.5f) return;
    const gs::Image& img = m.pick(h);
    if (img.h == 0) return;
    float sc = h / float(img.h);
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::max(1, int(std::lround(float(img.w) * sc))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float h = float(art_.cellH) * scale;
    float a = float(art_.cellW) * scale;
    for (unsigned char ch : s) {
        if (ch >= 32 && ch < 127 && art_.glyph[ch - 32].w) {
            const gs::Image& img = art_.glyph[ch - 32];
            gs::Sprite sp;
            sp.img = img;
            sp.w = int16_t(std::lround(float(img.w) * scale));
            sp.h = int16_t(std::lround(h));
            sp.x = int16_t(std::lround(x));
            sp.y = int16_t(std::lround(y));
            sp.pal = uint8_t(pal);
            sys_->vdp.sprite(sp);
        }
        x += a;
    }
}

void Game::textC(const std::string& s, float y, float scale, int pal) {
    float w = float(s.size()) * float(art_.cellW) * scale;
    text(s, 160.f - w * 0.5f, y, scale, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    float ox = std::sin(float(sys_->frame) * 1.7f) * shake_ * 3.5f;
    v.roadTime = int(sys_->frame);
    float scroll = mode_ == Mode::Title ? float(sys_->frame) * 0.35f : raidT_ * 14.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(HORIZON)) {
            float t = float(y) / HORIZON;
            v.lineBackdrop[y] = gs::rgb4(3 + int(3 * t), 2 + int(t), 6 - int(2 * t));
            v.road[y].on = false;
            v.lineFog[y] = y < 18 ? 6 : 0;
            continue;
        }
        float ny = (float(y) - HORIZON) / float(gs::SCREEN_H - HORIZON);
        gs::RoadLine& ln = v.road[y];
        ln.on = true;
        ln.cx = 160.f + ox + std::sin(ny * 2.4f) * 10.f;
        ln.hw = 22.f + ny * ny * 138.f;
        ln.v = scroll + (1.f - ny) * 36.f;
        ln.pal = PAL_ROAD;
        ln.style = 0;
        ln.band = uint8_t((y / 8) & 1);
        ln.left = 0;
        ln.right = 0;
        v.lineBackdrop[y] = gs::rgb4(3, 4, 2);
        v.lineFog[y] = ny < 0.16f ? 7 : (ny < 0.35f ? 2 : 0);
    }

    auto fogZ = [](float z) { return z > 12.f ? 7 : (z > 8.f ? 3 : 0); };

    std::vector<int> order(men_.size());
    for (int i = 0; i < int(men_.size()); i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return men_[a].z > men_[b].z; });
    for (int i : order) {
        const Man& m = men_[i];
        float sx, sy, sc;
        project(m.x, m.z, sx, sy, sc);
        sx += ox;
        int fog = fogZ(m.z);
        if (m.down) spr(art_.fallen, sx, sy + 2, 18.f * sc * 2.6f, PAL_FALL, m.flip, fog);
        else spr(art_.raider, sx, sy, 30.f * sc * 2.8f, PAL_MAN, m.flip, fog);
    }

    spr(art_.tower, 28 + ox * 0.15f, 214, 118, PAL_STONE, false, 1);
    spr(art_.tower, 292 + ox * 0.15f, 214, 118, PAL_STONE, true, 1);
    spr(art_.arch, 160, 168, 40, PAL_STONE);
    spr(art_.banner, 28, 96, 28, PAL_RED);
    spr(art_.banner, 292, 96, 28, PAL_RED, true);
    float flick = 22.f + std::sin(float(sys_->frame) * 0.45f) * 2.f;
    spr(art_.torch, 48, 150, flick, PAL_FIRE);
    spr(art_.torch, 272, 150, flick + 1.f, PAL_FIRE);

    for (const Puff& p : puffs_) {
        if (p.t > 0) spr(art_.puff, p.x, p.y, 14.f + (0.26f - p.t) * 36.f, PAL_CREAM);
    }

    if (mode_ == Mode::Raid || mode_ == Mode::Title) {
        float bob = std::sin(kick_ * 3.1f) * kick_ * 5.f;
        spr(art_.sight, aimX_, aimY_ + 8 + bob, 14, PAL_RED);
    }

    text("SALLY", 8, 6, 1, PAL_HUD);
    char buf[48];
    std::snprintf(buf, sizeof buf, "DOWN %d/%d", down_, NMAN);
    text(buf, 220, 6, 1, PAL_HUD);
    for (int i = 0; i < MAG; i++) {
        float x = 108.f + float(i) * 12.f;
        spr(i < rounds_ ? art_.round : art_.spent, x, 22, 12, i < rounds_ ? PAL_BRASS : PAL_HUD);
    }

    if (mode_ == Mode::Title) {
        textC("S3 SALLY MAGA", 64, 2, PAL_CREAM);
        textC("OUTLAST THE RAID", 90, 1, PAL_HUD);
        textC("AIM  FIRE  KEEP A ROUND", 164, 1, PAL_BRASS);
        textC("START", 184, 1, PAL_CREAM);
    } else if (mode_ == Mode::Won) {
        textC("MAGAZINE HOLDS", 72, 2, PAL_CREAM);
        std::snprintf(buf, sizeof buf, "%d ROUNDS LEFT", rounds_);
        textC(buf, 100, 1, PAL_BRASS);
    } else if (mode_ == Mode::Lost) {
        textC(fail_ == Fail::Gate ? "SALLY FORCED" : "MAGAZINE DRY", 72, 2, PAL_RED);
        textC(result(), 100, 1, PAL_HUD);
    } else if (rounds_ <= 2) {
        textC("MAGAZINE LOW", 28, 1, PAL_RED);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    t_ += dt;
    if (kick_ > 0) kick_ = std::max(0.f, kick_ - dt * 3.2f);
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt * 2.2f);
    for (Puff& p : puffs_) p.t -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0; }), puffs_.end());

    if (mode_ == Mode::Title) {
        if (bot_ && t_ > 0.35f) beginRaid();
        else if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) beginRaid();
    } else if (mode_ == Mode::Raid) {
        if (fireEdge()) wantFire_ = true;
        if (!bot_) humanAim(dt);
        updateRaid(dt);
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
        t_ = 0;
        mode_ = Mode::Title;
        over_ = false;
    }
    draw();
}

}  // namespace sallymaga
