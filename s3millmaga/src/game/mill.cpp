#include "game/mill.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace mmaga {
namespace {

constexpr float RAID_LEN = 28.f;
constexpr int MAG = 8;
constexpr float DT = 1.f / 60.f;

struct Plan {
    float t;
    int lane;
    int peel;
};

// Six who will reach the mill door. The rest turn off the lane if you hold fire.
constexpr Plan kPlan[] = {
    {1.1f, 1, 0}, {2.6f, 0, 1}, {3.2f, 2, 1}, {4.8f, 0, 0}, {7.6f, 2, 1},
    {8.6f, 2, 0}, {11.0f, 1, 1}, {12.4f, 1, 0}, {15.2f, 0, 1}, {16.6f, 0, 0},
    {19.4f, 2, 1}, {20.6f, 2, 0}, {23.4f, 1, 1}, {24.2f, 0, 1},
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
        int sky = y < 70 ? 1 : 0;
        sys.vdp.lineBackdrop[y] = sky ? gs::rgb4(6, 8, 11) : gs::rgb4(8, 7, 3);
        sys.vdp.lineFog[y] = y < 40 ? uint8_t(4) : 0;
        sys.vdp.road[y].on = false;
    }
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.apu.setMaster(0.4f);
}

void Game::beginRaid() {
    mode_ = Mode::Raid;
    fail_ = Fail::None;
    over_ = false;
    won_ = false;
    rounds_ = MAG;
    stopped_ = 0;
    lane_ = 1;
    next_ = 0;
    raidT_ = 0;
    cool_ = 0.35f;
    flash_ = 0;
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
    sys_->apu.tone(1, 587.f, 0.08f);
}

Game::Foe* Game::inLane(int lane) {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.phase != Phase::Walk || f.lane != lane) continue;
        if (f.z < 0.18f || f.z > 0.62f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    return best;
}

void Game::pull() {
    if (mode_ != Mode::Raid || cool_ > 0) return;
    cool_ = 0.34f;
    flash_ = 0.07f;
    shake_ = 3.f;
    if (rounds_ <= 1) {
        rounds_ = 0;
        lose(Fail::Spent);
        return;
    }
    rounds_--;
    sys_->apu.noiseBurst(0.5f, 1800.f, 0.07f);
    sys_->apu.tone(2, 110.f, 0.12f);
    if (Foe* f = inLane(lane_)) {
        f->phase = Phase::Down;
        f->age = 0;
        stopped_++;
    }
}

void Game::botAct() {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.kind != Kind::Commit || f.phase != Phase::Walk) continue;
        if (f.z < 0.24f || f.z > 0.50f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    if (!best || rounds_ <= 1) return;
    lane_ = best->lane;
    if (Foe* hit = inLane(lane_)) {
        if (hit->kind != Kind::Commit) return;
    }
    pull();
}

void Game::humanAct() {
    gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) lane_ = std::max(0, lane_ - 1);
    if (p.pressed(gs::BTN_RIGHT)) lane_ = std::min(2, lane_ + 1);
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C)) pull();
}

void Game::updateRaid(float dt) {
    if (bot_) botAct();
    else humanAct();
    cool_ = std::max(0.f, cool_ - dt);
    flash_ = std::max(0.f, flash_ - dt);
    shake_ *= 0.86f;
    sail_ += dt;

    while (next_ < NPLAN && raidT_ >= kPlan[next_].t) {
        Foe f;
        f.kind = kPlan[next_].peel ? Kind::Peel : Kind::Commit;
        f.lane = kPlan[next_].lane;
        f.z = 1.08f;
        foes_.push_back(f);
        next_++;
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.age += dt;
        float sp = f.kind == Kind::Commit ? 0.155f : 0.132f;
        if (f.phase == Phase::Walk) {
            f.z -= sp * dt;
            if (f.kind == Kind::Peel && f.z < 0.46f) f.phase = Phase::Leave;
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
    if (mode_ != Mode::Raid) return;
    if (raidT_ >= RAID_LEN && rounds_ > 0) win();
    else if (raidT_ >= RAID_LEN) lose(Fail::Spent);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    sail_ += (mode_ == Mode::Raid) ? 0.f : DT * 0.35f;
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
        if (c < 32 || c > 127) c = 32;
        int gi = int(c) - 32;
        if (gi < 0 || gi > 95) gi = 0;
        gs::Image img = art_.glyph[gi];
        int w = std::max(1, art_.gw[gi]);
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

    auto place = [&](int lane, float z, float& x, float& y, float& h) {
        float n = 1.f - std::clamp(z, 0.f, 1.2f);
        float spread = 18.f + n * 78.f;
        x = 160.f + (lane - 1) * spread + sx;
        y = 78.f + n * 78.f;
        h = 10.f + n * 28.f;
    };

    float aimX, aimY, aimH;
    place(lane_, 0.22f, aimX, aimY, aimH);
    spr(art_.mark, aimX, aimY - aimH - 4.f, 8, PAL_HUD, false, false);
    if (flash_ > 0) spr(art_.flash, aimX, aimY - aimH * 0.4f, 12, PAL_FX, false, false);

    text("MAG", 12, 200, 1.f, PAL_HUD);
    for (int i = 0; i < MAG; i++) {
        if (i < rounds_) spr(art_.round, 48.f + i * 10.f, 214, 14, PAL_BRASS, false, true);
    }
    int secs = std::max(0, int(std::ceil(RAID_LEN - raidT_ - 0.001f)));
    char clock[16];
    std::snprintf(clock, sizeof clock, "RAID %02d", mode_ == Mode::Title ? int(RAID_LEN) : secs);
    text(clock, 210, 200, 1.f, secs <= 5 && mode_ == Mode::Raid ? PAL_ALERT : PAL_HUD);

    if (mode_ == Mode::Title) {
        text("S3 MILL MAGA", 96, 16, 1.f, PAL_HUD);
        text("MAKE THE MAGAZINE LAST", 62, 32, 1.f, PAL_OK);
        text("A ROUND MUST OUTLIVE THE RAID", 46, 46, 1.f, PAL_HUD);
        text("LEFT RIGHT AIM   A FIRE", 66, 186, 1.f, PAL_HUD);
        if (int(t_ * 2) & 1) text("START", 140, 64, 1.f, PAL_BRASS);
    } else if (mode_ == Mode::Pause) {
        text("HELD", 144, 18, 1.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("THE MAGAZINE OUTLASTS THE RAID", 32, 16, 1.f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text(fail_ == Fail::Spent ? "MAGAZINE SPENT" : "THEY REACHED THE DOOR", 52, 16, 1.f, PAL_ALERT);
    } else {
        text(rounds_ <= 2 ? "LAST ROUNDS" : "HOLD THE DOOR", 104, 186, 1.f, rounds_ <= 2 ? PAL_ALERT : PAL_HUD);
    }

    spr(art_.mill, 160.f + sx, 86, 92, PAL_MILL, false, true);
    spr(art_.door, 160.f + sx, 86, 22, PAL_MILL, false, true);
    spr(art_.miller, 176.f + sx, 84, 26, PAL_MILL, false, true);
    const gs::Mipped& sail = (int(sail_ * 3.f) & 1) ? art_.sailB : art_.sailA;
    spr(sail, 160.f + sx, 28, 52, PAL_SAIL, false, false);

    std::vector<int> order;
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].on) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z < foes_[b].z; });
    for (int i : order) {
        const Foe& f = foes_[i];
        float x, y, h;
        place(f.lane, f.z, x, y, h);
        bool step = int(f.age * 6.f) & 1;
        if (f.phase == Phase::Down) {
            spr(art_.down, x, y, h * 0.45f, f.kind == Kind::Peel ? PAL_PEEL : PAL_FOE, false, true);
        } else {
            const gs::Mipped& body = f.kind == Kind::Peel ? art_.peel[step] : art_.foe[step];
            bool flip = f.phase == Phase::Leave ? (f.lane <= 1) : (f.lane == 0);
            spr(body, x, y, h, f.kind == Kind::Peel ? PAL_PEEL : PAL_FOE, flip, true);
        }
    }

    for (int lane = 0; lane < 3; lane++) {
        float x, y, h;
        place(lane, 0.72f, x, y, h);
        spr(art_.path, x, y + 8.f, 36, PAL_WHEAT, false, false);
    }
    for (int i = 0; i < 8; i++) {
        spr(art_.wheat, 18.f + i * 40.f, 150.f + (i & 1) * 8.f, 14, PAL_WHEAT, false, true);
        spr(art_.wheat, 28.f + i * 38.f, 188.f, 16, PAL_WHEAT, i & 1, true);
    }
}

}  // namespace mmaga
