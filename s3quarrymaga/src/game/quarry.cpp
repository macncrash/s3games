#include "game/quarry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace qmaga {
namespace {

constexpr float RAID_LEN = 24.f;
constexpr int MAG = 8;
constexpr float DT = 1.f / 60.f;
constexpr float Z0 = 1.02f;
constexpr float CUT_SP = 0.16f;

struct Plan {
    float t;
    int bench;
    int dust;
};

// Cutters walk the bench to the crusher. Dusters turn on the upper shelf.
constexpr Plan kPlan[] = {
    {0.8f, 1, 1}, {1.4f, 0, 0}, {2.6f, 2, 1}, {3.4f, 2, 0}, {4.6f, 0, 1},
    {5.6f, 1, 0}, {7.0f, 1, 1}, {8.2f, 0, 0}, {9.4f, 2, 1}, {10.8f, 1, 0},
    {12.2f, 0, 1}, {13.6f, 2, 0}, {15.0f, 1, 1}, {16.4f, 0, 0}, {17.8f, 2, 1},
};
constexpr int NPLAN = int(sizeof(kPlan) / sizeof(kPlan[0]));

}  // namespace

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "MAGAZINE SPENT";
    if (fail_ == Fail::Crusher) return "THEY REACHED THE CRUSHER";
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
        uint16_t c;
        if (y < 36) c = gs::rgb4(6, 8, 12);
        else if (y < 70) c = gs::rgb4(8, 8, 7);
        else if (y < 130) c = gs::rgb4(6, 6, 5);
        else if (y < 180) c = gs::rgb4(4, 4, 3);
        else c = gs::rgb4(3, 3, 2);
        sys.vdp.lineBackdrop[y] = c;
        sys.vdp.lineFog[y] = y < 80 ? uint8_t(4) : 0;
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
    bench_ = 1;
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
    sys_->apu.tone(1, 523.f, 0.1f);
}

Game::Foe* Game::nearest(int bench, float z0, float z1, bool cutsOnly) {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.phase != Phase::Walk || f.bench != bench) continue;
        if (cutsOnly && f.kind != Kind::Cut) continue;
        if (f.z < z0 || f.z > z1) continue;
        if (!best || f.z < best->z) best = &f;
    }
    return best;
}

void Game::pull() {
    if (mode_ != Mode::Raid || cool_ > 0) return;
    cool_ = 0.34f;
    flash_ = 0.08f;
    shake_ = 3.2f;
    if (rounds_ <= 1) {
        rounds_ = 0;
        lose(Fail::Spent);
        return;
    }
    rounds_--;
    sys_->apu.noiseBurst(0.5f, 1800.f, 0.07f);
    sys_->apu.tone(2, 110.f, 0.12f);
    if (Foe* f = nearest(bench_, 0.18f, 0.58f, false)) {
        f->phase = Phase::Down;
        f->age = 0;
        stopped_++;
    }
}

void Game::botAct() {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.kind != Kind::Cut || f.phase != Phase::Walk) continue;
        if (f.z < 0.26f || f.z > 0.50f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    if (!best || rounds_ <= 1) return;
    Foe* front = nearest(best->bench, 0.18f, 0.58f, false);
    if (front != best) return;
    bench_ = best->bench;
    pull();
}

void Game::humanAct() {
    gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) bench_ = (bench_ + 2) % 3;
    if (p.pressed(gs::BTN_RIGHT)) bench_ = (bench_ + 1) % 3;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C)) pull();
}

void Game::updateRaid(float dt) {
    if (bot_) botAct();
    else humanAct();
    cool_ = std::max(0.f, cool_ - dt);
    flash_ = std::max(0.f, flash_ - dt);
    shake_ *= 0.86f;

    while (next_ < NPLAN && raidT_ >= kPlan[next_].t) {
        Foe f;
        f.kind = kPlan[next_].dust ? Kind::Dust : Kind::Cut;
        f.bench = kPlan[next_].bench;
        f.z = Z0;
        foes_.push_back(f);
        next_++;
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.age += dt;
        float sp = f.kind == Kind::Cut ? CUT_SP : 0.13f;
        if (f.phase == Phase::Walk) {
            f.z -= sp * dt;
            if (f.kind == Kind::Dust && f.z < 0.66f) f.phase = Phase::Leave;
            if (f.kind == Kind::Cut && f.z < 0.08f) {
                lose(Fail::Crusher);
                return;
            }
        } else if (f.phase == Phase::Leave) {
            f.z += sp * 1.35f * dt;
            if (f.z > 1.25f) f.on = false;
        } else if (f.age > 0.75f) {
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
    float sx = (shake_ > 0.4f) ? std::sin(t_ * 80.f) * shake_ : 0.f;

    spr(art_.crane, 210.f + sx * 0.2f, 28, 36, PAL_CRANE, false, false);
    spr(art_.crusher, 160.f + sx, 168, 28, PAL_LIME, false, true);

    const float bx[3] = {62.f, 160.f, 258.f};
    for (int i = 0; i < 3; i++) {
        float w = 54.f + (i == bench_ ? 6.f : 0.f);
        (void)w;
        spr(art_.bench, bx[i] + sx, 92, 12, PAL_LIME, false, false);
        spr(art_.bench, bx[i] + sx * 0.6f, 128, 16, PAL_LIME, false, false);
    }
    spr(art_.hardhat, 160.f + sx, 196, 28, PAL_WATCH, false, true);

    for (const Foe& f : foes_) {
        if (!f.on) continue;
        float u = std::clamp(f.z, 0.f, 1.1f);
        float y = 78.f + (1.f - u) * 86.f;
        float h = 10.f + (1.f - u) * 28.f;
        float x = bx[f.bench] + (f.bench - 1) * (1.f - u) * 18.f + sx * (1.f - u);
        bool flip = f.phase == Phase::Leave;
        if (f.phase == Phase::Down) {
            spr(art_.down, x, y + 6, h * 0.45f, f.kind == Kind::Cut ? PAL_CUT : PAL_DUST, false, true);
            spr(art_.puff, x, y - 4, 10, PAL_FX, false, false);
            continue;
        }
        int fr = int(f.age * 8.f) & 1;
        const gs::Mipped& img = f.kind == Kind::Cut ? art_.cut[fr] : art_.dust[fr];
        spr(img, x, y, h, f.kind == Kind::Cut ? PAL_CUT : PAL_DUST, flip, true);
    }

    float aimX = bx[bench_] + sx;
    spr(art_.chev, aimX, 118, 10, PAL_HUD, false, false);
    if (flash_ > 0) spr(art_.flash, aimX, 108, 14, PAL_FX, false, false);

    text("MAG", 8, 200, 1.f, PAL_HUD);
    for (int i = 0; i < MAG; i++) {
        if (i < rounds_) spr(art_.brass, 40.f + i * 10.f, 214, 14, PAL_BRASS, false, true);
    }
    int secs = std::max(0, int(std::ceil(RAID_LEN - (mode_ == Mode::Title ? 0.f : raidT_) - 0.001f)));
    char clock[24];
    std::snprintf(clock, sizeof clock, "RAID %02d", secs);
    int clockPal = (mode_ == Mode::Raid && secs <= 5) ? PAL_ALERT : PAL_HUD;
    text(clock, 236, 200, 1.f, clockPal);

    if (mode_ == Mode::Title) {
        text("S3 QUARRY MAGA", 78, 40, 1.f, PAL_HUD);
        text("ONE MAGAZINE", 96, 58, 1.f, PAL_BRASS);
        text("OUTLAST THE RAID", 72, 72, 1.f, PAL_OK);
        text("LEFT RIGHT AIM   A FIRE", 48, 150, 1.f, PAL_HUD);
        text("PRESS START", 108, 166, 1.f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("HOLD", 136, 48, 1.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("MAGAZINE HOLDS", 84, 40, 1.f, PAL_OK);
        text(result(), 36, 56, 1.f, PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        text("QUARRY LOST", 96, 40, 1.f, PAL_ALERT);
        text(result(), 48, 56, 1.f, PAL_HUD);
    }
}

}  // namespace qmaga
