#include "game/trench.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace tmaga {
namespace {

constexpr float RAID_LEN = 22.5f;
constexpr int MAG = 5;
constexpr float DT = 1.f / 60.f;
constexpr int BAYS = 3;

struct Plan {
    float t;
    int bay;
    int peel;
};

// Eight rushes will cross the parapet. Five peel if you do not waste a round.
// Usable shots are MAG-1, so the lip has to be taken with the bayonet.
constexpr Plan kPlan[] = {
    {1.0f, 0, 0}, {2.1f, 1, 1}, {3.2f, 1, 0}, {4.3f, 2, 1}, {5.4f, 2, 0},
    {6.5f, 0, 1}, {7.6f, 0, 0}, {8.7f, 2, 1}, {9.8f, 2, 0}, {11.0f, 1, 1},
    {12.0f, 1, 0}, {13.1f, 0, 1}, {14.2f, 0, 0}, {16.4f, 2, 0},
};
constexpr int NPLAN = int(sizeof(kPlan) / sizeof(kPlan[0]));

}  // namespace

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "THE WATCH IS OVER";
    if (fail_ == Fail::Parapet) return "OVER THE PARAPET";
    return "THE RAID IS NOT DONE";
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Raid || mode_ == Mode::Pause) return 1;
    return 2;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.apu.setMaster(0.4f);
    field();
}

void Game::field() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t sky = y < 36 ? gs::rgb4(2, 2, 4) : y < 64 ? gs::rgb4(6, 6, 8) : gs::rgb4(8, 7, 5);
        vdp.lineBackdrop[y] = sky;
        vdp.lineFog[y] = y < 70 ? uint8_t(4) : 0;
        gs::RoadLine& r = vdp.road[y];
        r = {};
        if (y >= 68 && y <= 176) {
            float n = (y - 68) / 108.f;
            r.on = true;
            r.cx = 160.f;
            r.hw = 28.f + n * 150.f;
            r.v = (1.f - n) * 4200.f + t_ * 30.f;
            r.pal = PAL_ROAD;
            r.band = (int(r.v / 180.f) & 1) ? 1 : 0;
            r.style = gs::ROAD_MUD;
            r.left = gs::GROUND_LAND;
            r.right = gs::GROUND_LAND;
        }
    }
}

void Game::beginRaid() {
    mode_ = Mode::Raid;
    fail_ = Fail::None;
    over_ = false;
    won_ = false;
    rounds_ = MAG;
    held_ = 0;
    bay_ = 1;
    next_ = 0;
    raidT_ = 0;
    cool_ = 0.35f;
    flash_ = 0;
    stab_ = 0;
    shake_ = 0;
    foes_.clear();
}

void Game::lose(Fail why) {
    if (mode_ != Mode::Raid) return;
    fail_ = why;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    sys_->apu.noiseBurst(0.4f, 700.f, 0.28f);
}

void Game::win() {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Won;
    over_ = true;
    won_ = true;
    sys_->apu.tone(0, 392.f, 0.12f);
    sys_->apu.tone(1, 523.f, 0.08f);
}

Game::Foe* Game::nearest(Kind kind, float z0, float z1, int bay, bool anyBay) {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.phase != Phase::Walk || f.kind != kind) continue;
        if (!anyBay && f.bay != bay) continue;
        if (f.z < z0 || f.z > z1) continue;
        if (!best || f.z < best->z) best = &f;
    }
    return best;
}

void Game::fire() {
    if (mode_ != Mode::Raid || cool_ > 0) return;
    cool_ = 0.28f;
    flash_ = 0.07f;
    shake_ = 3.2f;
    if (rounds_ <= 1) {
        rounds_ = 0;
        lose(Fail::Spent);
        return;
    }
    rounds_--;
    sys_->apu.noiseBurst(0.5f, 2200.f, 0.07f);
    sys_->apu.tone(2, 110.f, 0.12f);
    if (Foe* f = nearest(Kind::Rush, 0.10f, 0.72f, bay_, false)) {
        f->phase = Phase::Down;
        f->age = 0;
        held_++;
    } else if (Foe* p = nearest(Kind::Peel, 0.20f, 0.72f, bay_, false)) {
        p->phase = Phase::Down;
        p->age = 0;
    }
}

void Game::bayonet() {
    if (mode_ != Mode::Raid || cool_ > 0) return;
    Foe* f = nearest(Kind::Rush, 0.12f, 0.26f, bay_, false);
    if (!f) {
        cool_ = 0.12f;
        return;
    }
    cool_ = 0.30f;
    stab_ = 0.12f;
    shake_ = 1.4f;
    f->phase = Phase::Down;
    f->age = 0;
    held_++;
    sys_->apu.tone(0, 180.f, 0.1f);
    sys_->apu.noiseBurst(0.25f, 500.f, 0.05f);
}

void Game::botAct() {
    if (Foe* lip = nearest(Kind::Rush, 0.13f, 0.25f, 0, true)) {
        bay_ = lip->bay;
        bayonet();
        return;
    }
    if (rounds_ <= 1) return;
    if (Foe* late = nearest(Kind::Rush, 0.08f, 0.14f, 0, true)) {
        bay_ = late->bay;
        fire();
    }
}

void Game::humanAct() {
    gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) bay_ = (bay_ + BAYS - 1) % BAYS;
    if (p.pressed(gs::BTN_RIGHT)) bay_ = (bay_ + 1) % BAYS;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) fire();
    if (p.pressed(gs::BTN_B)) bayonet();
}

void Game::updateRaid(float dt) {
    if (bot_) botAct();
    else humanAct();
    cool_ = std::max(0.f, cool_ - dt);
    flash_ = std::max(0.f, flash_ - dt);
    stab_ = std::max(0.f, stab_ - dt);
    shake_ *= 0.86f;

    while (next_ < NPLAN && raidT_ >= kPlan[next_].t) {
        Foe f;
        f.kind = kPlan[next_].peel ? Kind::Peel : Kind::Rush;
        f.bay = kPlan[next_].bay;
        f.z = 1.04f;
        foes_.push_back(f);
        next_++;
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.age += dt;
        float sp = f.kind == Kind::Rush ? 0.20f : 0.16f;
        if (f.phase == Phase::Walk) {
            f.z -= sp * dt;
            if (f.kind == Kind::Peel && f.z < 0.46f) f.phase = Phase::Leave;
            if (f.kind == Kind::Rush && f.z < 0.08f) {
                lose(Fail::Parapet);
                return;
            }
        } else if (f.phase == Phase::Leave) {
            f.z += sp * 1.35f * dt;
            if (f.z > 1.25f) f.on = false;
        } else if (f.age > 0.65f) {
            f.on = false;
        }
    }

    raidT_ += dt;
    if (mode_ != Mode::Raid) return;
    if (raidT_ >= RAID_LEN && rounds_ > 0) win();
    else if (raidT_ >= RAID_LEN) lose(Fail::Spent);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) beginRaid();
    } else if (mode_ == Mode::Raid) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else updateRaid(DT);
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Raid;
    } else if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
        beginRaid();
        over_ = false;
    }
    field();
    sys.vdp.roadTime = int(t_ * 60.f);
    draw();
    if (mode_ != Mode::Raid) sys.apu.tone(2, 0, 0);
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
    float aimX = 56.f + bay_ * 104.f;

    text("MAG", 12, 8, 1.f, PAL_HUD);
    for (int i = 0; i < MAG; i++) {
        if (i < rounds_) spr(art_.brass, 52.f + i * 14.f, 18, 18, PAL_BRASS, false, false);
    }
    int secs = std::max(0, int(std::ceil(RAID_LEN - (mode_ == Mode::Title ? 0.f : raidT_) - 0.001f)));
    char clock[24];
    std::snprintf(clock, sizeof clock, "RAID %02d", secs);
    text(clock, 220, 8, 1.f, secs <= 5 && mode_ == Mode::Raid ? PAL_ALERT : PAL_HUD);

    if (mode_ == Mode::Title) {
        text("S3 TRENCH MAGA", 92, 36, 1.f, PAL_HUD);
        text("MAKE THE MAGAZINE LAST", 62, 54, 1.f, PAL_OK);
        text("A ROUND MUST OUTLIVE THE RAID", 40, 70, 1.f, PAL_HUD);
        text("BAYONET THE LIP  FIRE THE REST", 44, 88, 1.f, PAL_BRASS);
        text("LEFT RIGHT BAY    A FIRE  B STEEL", 36, 200, 1.f, PAL_HUD);
        if (int(t_ * 2) & 1) text("START", 140, 118, 1.f, PAL_BRASS);
    } else if (mode_ == Mode::Pause) {
        text("HELD", 146, 96, 1.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("THE MAGAZINE OUTLASTS THE RAID", 36, 40, 1.f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text(result(), fail_ == Fail::Spent ? 80 : 76, 40, 1.f, PAL_ALERT);
    } else {
        text(rounds_ <= 2 ? "SAVE A ROUND" : "HOLD THE LIP", 108, 200, 1.f, rounds_ <= 2 ? PAL_ALERT : PAL_HUD);
    }

    for (int i = 0; i < 4; i++) spr(art_.bag, 40.f + i * 80.f + sx, 188, 52, PAL_BAG, false, false);
    spr(art_.plank, 160 + sx, 206, 18, PAL_MUD, false, false);
    if (stab_ > 0) spr(art_.stab, aimX, 156, 14, PAL_FX, bay_ == 0, false);
    if (flash_ > 0) spr(art_.flash, aimX, 148, 18, PAL_FX, false, false);
    spr(art_.notch, aimX, 136, 12, PAL_HUD, false, false);

    std::vector<int> order;
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].on) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z > foes_[b].z; });
    std::reverse(order.begin(), order.end());

    for (int i : order) {
        const Foe& f = foes_[i];
        float near = std::clamp(1.f - f.z, 0.f, 1.f);
        float cx = 160.f + (f.bay - 1) * (36.f + near * 78.f) + sx;
        float cy = 86.f + near * 78.f;
        int pal = f.kind == Kind::Rush ? PAL_FOE : PAL_PEEL;
        if (f.phase == Phase::Down) {
            spr(art_.down, cx, cy, 14.f + near * 12.f, pal, false, true);
        } else {
            const gs::Mipped& img = (f.kind == Kind::Rush ? art_.foe : art_.peel)[int(f.age * 6) & 1];
            bool flip = f.phase == Phase::Leave ? f.bay != 2 : f.bay == 0;
            spr(img, cx, cy, 14.f + near * 36.f, pal, flip, true);
        }
    }

    for (int i = 0; i < 4; i++) {
        float x = 48.f + i * 74.f;
        spr(art_.post, x, 108, 40, PAL_MUD, false, true);
        spr(art_.wire, x + 36.f, 96, 16, PAL_MUD, false, false);
    }
}

}  // namespace tmaga
