#include "game/harbor.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace harbormaga {
namespace {

constexpr int MAG = 7;
constexpr int NBOAT = 5;
constexpr float SPAWN_Z = 16.f;
constexpr float QUAY_Z = 2.6f;
constexpr float SPEED = 1.62f;
constexpr float BOLT = 0.22f;
constexpr float HORIZON = 76.f;

struct Plan {
    float t;
    float x;
};

constexpr Plan kPlan[NBOAT] = {
    {0.45f, -1.05f}, {2.15f, 0.85f}, {3.85f, -0.35f}, {5.55f, 1.05f}, {7.35f, 0.15f},
};

}  // namespace

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "THE MAGAZINE RAN DRY";
    if (fail_ == Fail::Quay) return "A CUTTER TOOK THE QUAY";
    return "THE RAID IS NOT DONE";
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Raid) return 1;
    return 2;
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
    sunk_ = 0;
    next_ = 0;
    raidT_ = 0;
    bolt_ = 0.16f;
    kick_ = shake_ = 0;
    aimX_ = 160;
    aimY_ = 128;
    boats_.clear();
    splashes_.clear();
    sys_->apu.keyOn(1, 196.f, 0.1f);
    sys_->setLight(40, 90, 140);
}

void Game::lose(Fail why) {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    fail_ = why;
    shake_ = why == Fail::Quay ? 1.1f : 0.5f;
    sys_->apu.keyOn(1, why == Fail::Quay ? 70.f : 98.f, 0.18f);
    sys_->apu.noiseBurst(0.32f, 180.f, 0.2f);
    sys_->rumble(0.45f, 0.15f, 120);
    sys_->setLight(180, 30, 24);
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
    sys_->apu.noiseBurst(0.2f, 120.f, 0.1f);
    sys_->apu.keyOn(2, 523.f, 0.12f);
    sys_->setLight(40, 170, 90);
}

void Game::project(float x, float z, float& sx, float& sy, float& s) const {
    float d = std::max(0.4f, z);
    s = 78.f / (d + 2.4f);
    sy = HORIZON + 390.f / (d + 1.15f);
    sx = 160.f + x * (240.f / (d * 0.55f + 1.4f));
}

void Game::pull() {
    if (mode_ != Mode::Raid || rounds_ <= 0 || bolt_ > 0) return;
    Boat* best = nullptr;
    float bestD = 1e9f;
    float hx = aimX_, hy = aimY_;
    for (Boat& b : boats_) {
        if (b.sunk) continue;
        float sx, sy, s;
        project(b.x, b.z, sx, sy, s);
        float rad = std::max(20.f, 22.f * s);
        float d = std::hypot(aimX_ - sx, aimY_ - sy);
        if (d <= rad && d < bestD) {
            bestD = d;
            best = &b;
            hx = sx;
            hy = sy;
        }
    }
    rounds_--;
    bolt_ = BOLT;
    kick_ = 1.f;
    shake_ = best ? 0.4f : 0.18f;
    Splash sp;
    sp.x = hx;
    sp.y = hy;
    sp.t = 0.28f;
    splashes_.push_back(sp);
    if (best) {
        best->sunk = true;
        best->dead = 0;
        sunk_++;
        sys_->apu.keyOn(0, 110.f, 0.28f);
        sys_->apu.noiseBurst(0.5f, 900.f, 0.08f);
        sys_->rumble(0.3f, 0.65f, 50);
    } else {
        sys_->apu.keyOn(0, 170.f, 0.12f);
        sys_->apu.noiseBurst(0.18f, 3200.f, 0.04f);
    }
    bool live = false;
    for (const Boat& b : boats_)
        if (!b.sunk) live = true;
    if (!live && next_ >= NBOAT && rounds_ > 0) win();
    else if (rounds_ <= 0) lose(Fail::Spent);
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
    aimY_ += ay * 220.f * dt;
    aimX_ = std::clamp(aimX_, 24.f, 296.f);
    aimY_ = std::clamp(aimY_, 86.f, 190.f);
}

void Game::botAct(float dt) {
    const Boat* threat = nullptr;
    for (const Boat& b : boats_) {
        if (b.sunk) continue;
        if (!threat || b.z < threat->z) threat = &b;
    }
    if (!threat) return;
    float sx, sy, s;
    project(threat->x, threat->z, sx, sy, s);
    float dx = sx - aimX_, dy = sy - aimY_;
    float d = std::hypot(dx, dy);
    float step = 2200.f * dt;
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
    if (threat->z < 12.5f && d <= 10.f) pull();
    else if (threat->z < 5.5f) {
        aimX_ = sx;
        aimY_ = sy;
        pull();
    }
}

void Game::updateRaid(float dt) {
    if (bolt_ > 0) bolt_ -= dt;
    while (next_ < NBOAT && raidT_ >= kPlan[next_].t) {
        Boat b;
        b.x = kPlan[next_].x;
        b.z = SPAWN_Z;
        boats_.push_back(b);
        next_++;
    }
    for (Boat& b : boats_) {
        if (b.sunk) {
            b.dead += dt;
            b.z += 0.15f * dt;
            continue;
        }
        b.z -= SPEED * dt;
        if (b.z < QUAY_Z) {
            lose(Fail::Quay);
            return;
        }
    }
    if (mode_ != Mode::Raid) return;
    boats_.erase(std::remove_if(boats_.begin(), boats_.end(),
                                [](const Boat& b) { return b.sunk && b.dead > 2.4f; }),
                 boats_.end());
    if (bot_) botAct(dt);
    else if (wantFire_ && bolt_ <= 0) {
        wantFire_ = false;
        pull();
    }
    if (mode_ != Mode::Raid) return;
    raidT_ += dt;
    bool live = false;
    for (const Boat& b : boats_)
        if (!b.sunk) live = true;
    if (!live && next_ >= NBOAT && rounds_ > 0 && sunk_ >= NBOAT) win();
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
    float ox = std::sin(float(sys_->frame) * 1.8f) * shake_ * 4.f;
    v.roadTime = int(sys_->frame);
    float scroll = mode_ == Mode::Title ? float(sys_->frame) * 0.4f : raidT_ * 18.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(HORIZON)) {
            float t = float(y) / HORIZON;
            v.lineBackdrop[y] = gs::rgb4(2 + int(4 * t), 3 + int(3 * t), 8 - int(2 * t));
            v.road[y].on = false;
            v.lineFog[y] = 0;
            continue;
        }
        float ny = (float(y) - HORIZON) / float(gs::SCREEN_H - HORIZON);
        gs::RoadLine& ln = v.road[y];
        ln.on = true;
        ln.cx = 160.f + ox + std::sin(scroll * 0.02f + ny * 3.f) * 6.f;
        ln.hw = 28.f + ny * ny * 150.f;
        ln.v = scroll + (1.f - ny) * 40.f;
        ln.pal = PAL_WATER;
        ln.style = 2;
        ln.band = uint8_t((y / 6) & 1);
        ln.left = 0;
        ln.right = 0;
        v.lineBackdrop[y] = gs::rgb4(2, 4, 6);
        v.lineFog[y] = ny < 0.18f ? 8 : (ny < 0.4f ? 3 : 0);
    }

    auto fogZ = [](float z) { return z > 12.f ? 8 : (z > 8.f ? 3 : 0); };

    std::vector<int> order(boats_.size());
    for (int i = 0; i < int(boats_.size()); i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return boats_[a].z > boats_[b].z; });
    for (int i : order) {
        const Boat& b = boats_[i];
        float sx, sy, s;
        project(b.x, b.z, sx, sy, s);
        sx += ox;
        float h = (b.sunk ? 16.f : 26.f) * s * 3.2f;
        int fog = fogZ(b.z);
        if (b.sunk) spr(art_.wreck, sx, sy + 4, h * 0.7f, PAL_WAKE, b.x < 0, fog);
        else spr(art_.cutter, sx, sy, h, PAL_HULL, b.x > 0, fog);
    }

    spr(art_.light, 36 + ox * 0.2f, 118, 78, PAL_LIGHT, false, 4);
    spr(art_.crane, 286, 132, 62, PAL_PIER, false, 2);
    spr(art_.shed, 168, 198, 52, PAL_PIER);
    spr(art_.buoy, 78, 150, 18, PAL_RED, false, 2);
    spr(art_.buoy, 248, 162, 22, PAL_RED, false, 1);

    for (const Splash& sp : splashes_) {
        if (sp.t > 0) spr(art_.splash, sp.x, sp.y, 18.f + (0.28f - sp.t) * 40.f, PAL_CREAM);
    }

    if (mode_ == Mode::Raid || mode_ == Mode::Title) {
        float bob = std::sin(kick_ * 3.1f) * kick_ * 6.f;
        spr(art_.sight, aimX_, aimY_ + 8 + bob, 16, PAL_RED);
    }

    text("HARBOR", 8, 6, 1, PAL_HUD);
    char buf[40];
    std::snprintf(buf, sizeof buf, "SUNK %d/%d", sunk_, NBOAT);
    text(buf, 228, 6, 1, PAL_HUD);
    for (int i = 0; i < MAG; i++) {
        float x = 108.f + float(i) * 12.f;
        spr(i < rounds_ ? art_.round : art_.spent, x, 20, 12, i < rounds_ ? PAL_BRASS : PAL_HUD);
    }

    if (mode_ == Mode::Title) {
        textC("S3 HARBOR MAGA", 70, 2, PAL_CREAM);
        textC("OUTLAST THE RAID", 96, 1, PAL_HUD);
        textC("AIM  FIRE  KEEP A ROUND", 168, 1, PAL_BRASS);
        textC("START", 188, 1, PAL_CREAM);
    } else if (mode_ == Mode::Won) {
        textC("MAGAZINE HOLDS", 78, 2, PAL_CREAM);
        std::snprintf(buf, sizeof buf, "%d ROUNDS LEFT", rounds_);
        textC(buf, 104, 1, PAL_BRASS);
    } else if (mode_ == Mode::Lost) {
        textC(fail_ == Fail::Quay ? "QUAY LOST" : "MAGAZINE DRY", 78, 2, PAL_RED);
        textC(result(), 104, 1, PAL_HUD);
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
    for (Splash& s : splashes_) s.t -= dt;
    splashes_.erase(std::remove_if(splashes_.begin(), splashes_.end(), [](const Splash& s) { return s.t <= 0; }),
                    splashes_.end());

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

}  // namespace harbormaga
