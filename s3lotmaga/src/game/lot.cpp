#include "game/lot.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace lot {
namespace {

constexpr float RAID_LEN = 22.f;
constexpr int MAG = 6;
constexpr float DT = 1.f / 60.f;

struct Plan {
    float t;
    int aisle;
    int peel;
};

// Four runners will hit the store door. The rest break for a parked car.
constexpr Plan kPlan[] = {
    {1.2f, 0, 1}, {2.0f, 1, 0}, {3.6f, 2, 1}, {5.2f, 0, 0}, {6.8f, 2, 1},
    {8.0f, 1, 1}, {9.4f, 2, 0}, {11.2f, 0, 1}, {12.6f, 1, 0}, {14.4f, 2, 1},
    {16.0f, 0, 1}, {17.6f, 1, 1},
};
constexpr int NPLAN = int(sizeof(kPlan) / sizeof(kPlan[0]));

}  // namespace

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "MAGAZINE SPENT";
    if (fail_ == Fail::Door) return "THEY REACHED THE DOOR";
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
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t sky = y < 36 ? gs::rgb4(1, 1, 3) : y < 90 ? gs::rgb4(2, 2, 4) : gs::rgb4(3, 3, 4);
        if (y > 150) sky = gs::rgb4(2, 2, 3);
        sys.vdp.lineBackdrop[y] = sky;
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.4f);
}

void Game::beginRaid() {
    mode_ = Mode::Raid;
    fail_ = Fail::None;
    over_ = false;
    won_ = false;
    rounds_ = MAG;
    stopped_ = 0;
    aisle_ = 1;
    next_ = 0;
    raidT_ = 0;
    cool_ = 0.35f;
    flash_ = 0;
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
    sys_->apu.tone(0, 440.f, 0.12f);
    sys_->apu.tone(1, 660.f, 0.08f);
}

Game::Foe* Game::inAisle(int aisle) {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.phase != Phase::Run || f.aisle != aisle) continue;
        if (f.z < 0.14f || f.z > 0.66f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    return best;
}

void Game::pull() {
    if (mode_ != Mode::Raid || cool_ > 0) return;
    cool_ = 0.34f;
    flash_ = 0.08f;
    if (rounds_ <= 1) {
        rounds_ = 0;
        lose(Fail::Spent);
        return;
    }
    rounds_--;
    sys_->apu.noiseBurst(0.5f, 2100.f, 0.07f);
    sys_->apu.tone(2, 120.f, 0.12f);
    if (Foe* f = inAisle(aisle_)) {
        f->phase = Phase::Down;
        f->age = 0;
        stopped_++;
    }
}

void Game::botAct() {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.kind != Kind::Commit || f.phase != Phase::Run) continue;
        if (f.z < 0.20f || f.z > 0.52f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    if (!best || rounds_ <= 1) return;
    aisle_ = best->aisle;
    pull();
}

void Game::humanAct() {
    gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) aisle_ = (aisle_ + 2) % 3;
    if (p.pressed(gs::BTN_RIGHT)) aisle_ = (aisle_ + 1) % 3;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C)) pull();
}

void Game::updateRaid(float dt) {
    if (bot_) botAct();
    else humanAct();
    cool_ = std::max(0.f, cool_ - dt);
    flash_ = std::max(0.f, flash_ - dt);

    while (next_ < NPLAN && raidT_ >= kPlan[next_].t) {
        Foe f;
        f.kind = kPlan[next_].peel ? Kind::Peel : Kind::Commit;
        f.aisle = kPlan[next_].aisle;
        f.z = 1.05f;
        foes_.push_back(f);
        next_++;
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.age += dt;
        float sp = f.kind == Kind::Commit ? 0.18f : 0.15f;
        if (f.phase == Phase::Run) {
            f.z -= sp * dt;
            if (f.kind == Kind::Peel && f.z < 0.48f) f.phase = Phase::Leave;
            if (f.kind == Kind::Commit && f.z < 0.08f) {
                lose(Fail::Door);
                return;
            }
        } else if (f.phase == Phase::Leave) {
            f.z += sp * 1.25f * dt;
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
    draw();
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

    float aimX = 160.f + (aisle_ - 1) * 70.f;
    spr(art_.chev, aimX, 156, 10, PAL_HUD, false, false);
    if (flash_ > 0) spr(art_.flash, aimX, 146, 14, PAL_FX, false, false);
    text("MAG", 8, 8, 1.f, PAL_HUD);
    for (int i = 0; i < MAG; i++) {
        if (i < rounds_) spr(art_.brass, 42.f + i * 10.f, 16, 12, PAL_BRASS, false, false);
    }
    int secs = std::max(0, int(std::ceil(RAID_LEN - raidT_ - 0.001f)));
    char clock[16];
    std::snprintf(clock, sizeof clock, "RAID %02d", mode_ == Mode::Title ? int(RAID_LEN) : secs);
    text(clock, 230, 8, 1.f, secs <= 5 && mode_ == Mode::Raid ? PAL_ALERT : PAL_HUD);

    if (mode_ == Mode::Title) {
        text("S3 LOT MAGA", 112, 36, 1.f, PAL_HUD);
        text("ONE LOT  ONE MAGAZINE", 76, 52, 1.f, PAL_OK);
        text("MAKE IT LAST THE RAID", 80, 66, 1.f, PAL_HUD);
        text("LEFT RIGHT AIM   A FIRE", 72, 200, 1.f, PAL_HUD);
        if (int(t_ * 2) & 1) text("START", 142, 96, 1.f, PAL_BRASS);
    } else if (mode_ == Mode::Pause) {
        text("HELD", 146, 88, 1.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("THE MAGAZINE OUTLASTS THE RAID", 36, 36, 1.f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text(fail_ == Fail::Spent ? "MAGAZINE SPENT" : "THEY REACHED THE DOOR", 64, 36, 1.f, PAL_ALERT);
    } else {
        text(rounds_ <= 2 ? "LAST ROUNDS" : "HOLD THE DOOR", 108, 200, 1.f, rounds_ <= 2 ? PAL_ALERT : PAL_HUD);
    }

    spr(art_.lamp, 28, 48, 40, PAL_LOT, false, true);
    spr(art_.lamp, 292, 48, 40, PAL_LOT, false, true);
    for (int i = 0; i < 4; i++) spr(art_.stall, 70.f + i * 46.f, 120, 70, PAL_LOT, false, false);

    for (int row = 0; row < 2; row++) {
        float cy = 70.f + row * 28.f;
        spr(art_.car, 36, cy, 16, PAL_CAR, false, false);
        spr(art_.car, 284, cy, 16, PAL_CAR, true, false);
    }

    std::vector<int> order;
    order.reserve(foes_.size());
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].on) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z > foes_[b].z; });
    std::reverse(order.begin(), order.end());

    for (int i : order) {
        const Foe& f = foes_[i];
        float near = std::clamp(1.f - f.z, 0.f, 1.f);
        float cx = 160.f + (f.aisle - 1) * (36.f + near * 48.f);
        float cy = 62.f + near * 78.f;
        int pal = f.kind == Kind::Commit ? PAL_COMMIT : PAL_PEEL;
        if (f.phase == Phase::Down) {
            spr(art_.down, cx, cy, 12.f + near * 8.f, pal, false, true);
        } else if (f.phase == Phase::Leave) {
            spr(art_.bag, cx + (f.aisle == 0 ? -10.f : 10.f), cy - 6, 10, PAL_PEEL, f.aisle != 0, false);
            const gs::Mipped& img = art_.peel[int(f.age * 6) & 1];
            spr(img, cx, cy, 14.f + near * 22.f, pal, f.aisle != 2, true);
        } else {
            const gs::Mipped& img = (f.kind == Kind::Commit ? art_.commit : art_.peel)[int(f.age * 6) & 1];
            spr(img, cx, cy, 16.f + near * 36.f, pal, false, true);
        }
    }

    spr(art_.door, 160, 188, 36, PAL_STORE, false, true);
    spr(art_.clerk, aimX, 176, 28, PAL_STORE, false, true);
}

}  // namespace lot
