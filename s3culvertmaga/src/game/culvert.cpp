#include "game/culvert.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cmaga {
namespace {

constexpr float RAID_LEN = 24.f;
constexpr int MAG = 6;
constexpr float DT = 1.f / 60.f;
constexpr float WIN_NEAR = 0.30f;
constexpr float WIN_FAR = 0.50f;

struct Plan {
    float t;
    int lamp;
    float x;
};

// Lamps walk the pipe to the grate. Shades turn before the firing window.
constexpr Plan kPlan[] = {
    {1.8f, 1, -0.35f}, {3.6f, 0, 0.40f},  {5.8f, 1, 0.15f},  {8.0f, 0, -0.25f}, {10.4f, 1, -0.10f},
    {12.6f, 0, 0.30f}, {14.6f, 1, 0.42f}, {16.6f, 0, -0.45f}, {18.4f, 1, 0.05f},
};
constexpr int NPLAN = int(sizeof(kPlan) / sizeof(kPlan[0]));

}  // namespace

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "MAGAZINE SPENT";
    if (fail_ == Fail::Grate) return "THEY REACHED THE GRATE";
    return "THE WATCH IS NOT DONE";
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) return 1;
    return 2;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = y < 36 ? 1 : y < 90 ? 2 : 1;
        sys.vdp.lineBackdrop[y] = y < 70 ? gs::rgb4(1, 2, sky + 2) : gs::rgb4(2, 3, 3);
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.4f);
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    fail_ = Fail::None;
    over_ = false;
    won_ = false;
    rounds_ = MAG;
    stopped_ = 0;
    next_ = 0;
    aim_ = 0;
    raidT_ = 0;
    cool_ = 0.35f;
    flash_ = 0;
    shake_ = 0;
    foes_.clear();
}

void Game::lose(Fail why) {
    if (mode_ != Mode::Watch) return;
    fail_ = why;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    sys_->apu.noiseBurst(0.4f, 700.f, 0.28f);
}

void Game::win() {
    if (mode_ != Mode::Watch) return;
    mode_ = Mode::Won;
    over_ = true;
    won_ = true;
    sys_->apu.tone(0, 440.f, 0.12f);
    sys_->apu.tone(1, 659.f, 0.08f);
}

Game::Foe* Game::inWindow() {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.phase != Phase::Walk) continue;
        if (f.z < WIN_NEAR || f.z > WIN_FAR) continue;
        if (std::fabs(f.x - aim_) > 0.55f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    return best;
}

void Game::squeeze() {
    if (mode_ != Mode::Watch || cool_ > 0) return;
    cool_ = 0.34f;
    flash_ = 0.07f;
    shake_ = 3.2f;
    if (rounds_ <= 1) {
        rounds_ = 0;
        lose(Fail::Spent);
        return;
    }
    rounds_--;
    sys_->apu.noiseBurst(0.5f, 1800.f, 0.07f);
    sys_->apu.tone(2, 110.f, 0.12f);
    if (Foe* f = inWindow()) {
        if (f->kind == Kind::Lamp) {
            f->phase = Phase::Down;
            f->age = 0;
            stopped_++;
        } else {
            f->phase = Phase::Turn;
        }
    }
}

void Game::botAct() {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.kind != Kind::Lamp || f.phase != Phase::Walk) continue;
        if (f.z < WIN_NEAR + 0.02f || f.z > WIN_FAR - 0.04f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    if (!best || rounds_ <= 1) return;
    aim_ = best->x;
    squeeze();
}

void Game::humanAct() {
    gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT) || p.axisX < -0.35f) aim_ = std::max(-0.7f, aim_ - 1.6f * DT);
    if (p.down(gs::BTN_RIGHT) || p.axisX > 0.35f) aim_ = std::min(0.7f, aim_ + 1.6f * DT);
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C)) squeeze();
}

void Game::updateWatch(float dt) {
    if (bot_) botAct();
    else humanAct();
    cool_ = std::max(0.f, cool_ - dt);
    flash_ = std::max(0.f, flash_ - dt);
    shake_ *= 0.86f;

    while (next_ < NPLAN && raidT_ >= kPlan[next_].t) {
        Foe f;
        f.kind = kPlan[next_].lamp ? Kind::Lamp : Kind::Shade;
        f.x = kPlan[next_].x;
        f.z = 1.08f;
        foes_.push_back(f);
        next_++;
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.age += dt;
        float sp = f.kind == Kind::Lamp ? 0.20f : 0.16f;
        if (f.phase == Phase::Walk) {
            f.z -= sp * dt;
            if (f.kind == Kind::Shade && f.z < 0.62f) f.phase = Phase::Turn;
            if (f.kind == Kind::Lamp && f.z < 0.08f) {
                lose(Fail::Grate);
                return;
            }
        } else if (f.phase == Phase::Turn) {
            f.z += sp * 1.15f * dt;
            f.x += (f.x < 0 ? -0.35f : 0.35f) * dt;
            if (f.z > 1.25f) f.on = false;
        } else if (f.age > 0.65f) {
            f.on = false;
        }
    }

    raidT_ += dt;
    if (raidT_ >= RAID_LEN && rounds_ > 0) win();
    else if (raidT_ >= RAID_LEN) lose(Fail::Spent);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) beginWatch();
    } else if (mode_ == Mode::Watch) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else updateWatch(DT);
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Watch;
    } else if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
        beginWatch();
        over_ = false;
    }
    draw();
    if (mode_ != Mode::Watch && t_ > 0.4f) sys.apu.tone(2, 0, 0);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 2.f || m.h <= 0) return;
    float s = h / float(m.h);
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.w = std::max(1, int(m.w * s));
    sp.h = std::max(1, int(h));
    sp.x = int(std::lround(cx - sp.w * 0.5f));
    sp.y = int(std::lround(feet ? cy - sp.h : cy - sp.h * 0.5f));
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float pen = x;
    float sc = std::max(1.f, scale);
    for (char ch : s) {
        unsigned c = unsigned(ch);
        if (c < 32 || c > 95) c = 32;
        gs::Image img = art_.glyph[c];
        int w = std::max(1, art_.gw[c]);
        gs::Sprite sp;
        sp.img = img;
        sp.w = int(w * sc);
        sp.h = int(art_.gh * sc);
        sp.x = int(pen);
        sp.y = int(y);
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        pen += (w + 1) * sc;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    float sx = (shake_ > 0.4f) ? std::sin(t_ * 80.f) * shake_ : 0.f;

    text("MAG", 12, 8, 1.f, PAL_HUD);
    for (int i = 0; i < MAG; i++) {
        if (i < rounds_) spr(art_.brass, 52.f + i * 10.f, 20, 14, PAL_BRASS, false, false);
    }
    int secs = std::max(0, int(std::ceil(RAID_LEN - raidT_ - 0.001f)));
    char clock[16];
    std::snprintf(clock, sizeof clock, "RAID %02d", mode_ == Mode::Title ? int(RAID_LEN) : secs);
    text(clock, 220, 8, 1.f, secs <= 5 && mode_ == Mode::Watch ? PAL_ALERT : PAL_HUD);

    if (mode_ == Mode::Title) {
        text("S3 CULVERT MAGA", 86, 28, 1.f, PAL_HUD);
        text("MAKE THE MAGAZINE LAST", 66, 46, 1.f, PAL_OK);
        text("A ROUND MUST OUTLIVE THE RAID", 48, 62, 1.f, PAL_HUD);
        text("LEFT RIGHT AIM   A FIRE", 74, 200, 1.f, PAL_HUD);
        if (int(t_ * 2) & 1) text("START", 140, 96, 1.f, PAL_BRASS);
    } else if (mode_ == Mode::Pause) {
        text("HELD", 144, 40, 1.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("THE MAGAZINE OUTLASTS THE RAID", 36, 28, 1.f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text(fail_ == Fail::Spent ? "MAGAZINE SPENT" : "THEY REACHED THE GRATE", 52, 28, 1.f, PAL_ALERT);
    } else {
        text(rounds_ <= 2 ? "LAST ROUNDS" : "HOLD THE MOUTH", 100, 200, 1.f, rounds_ <= 2 ? PAL_ALERT : PAL_HUD);
    }

    // Far rings first so the near concrete sits on top.
    for (int i = 3; i >= 0; i--) {
        float n = i / 3.f;
        float h = 36.f + n * 92.f;
        float cy = 118.f + n * 18.f;
        spr(art_.ring, 160.f + sx * (0.2f + n), cy, h, PAL_PIPE, false, false);
    }
    spr(art_.rib, 78 + sx, 150, 90, PAL_PIPE, false, true);
    spr(art_.rib, 242 + sx, 150, 90, PAL_PIPE, true, true);
    float dripY = 70.f + std::fmod(t_ * 28.f, 40.f);
    spr(art_.drip, 96, dripY, 18, PAL_WATER, false, false);
    spr(art_.drip, 210, dripY + 12.f, 16, PAL_WATER, false, false);

    std::vector<int> order;
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].on) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z > foes_[b].z; });
    std::reverse(order.begin(), order.end());
    for (int i : order) {
        const Foe& f = foes_[i];
        float near = std::clamp(1.f - f.z, 0.f, 1.f);
        float cx = 160.f + f.x * (18.f + near * 70.f) + sx;
        float cy = 108.f + near * 52.f;
        int pal = f.kind == Kind::Lamp ? PAL_LAMP : PAL_SHADE;
        if (f.phase == Phase::Down) {
            spr(art_.down, cx, cy, 10.f + near * 12.f, pal, false, true);
        } else {
            const gs::Mipped& img = (f.kind == Kind::Lamp ? art_.lamp : art_.shade)[int(f.age * 6) & 1];
            spr(img, cx, cy, 14.f + near * 36.f, pal, f.phase == Phase::Turn, true);
        }
    }

    spr(art_.water, 160 + sx, 196, 28, PAL_WATER, false, true);
    spr(art_.grate, 160 + sx, 168, 36, PAL_MOSS, false, true);
    float ax = 160.f + aim_ * 70.f + sx;
    if (flash_ > 0) spr(art_.muzzle, ax, 150, 18, PAL_BRASS, false, false);
    spr(art_.rib, ax, 188, 22, PAL_HUD, false, true);
}

}  // namespace cmaga
