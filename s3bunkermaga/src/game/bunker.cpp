#include "game/bunker.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace bmaga {
namespace {

constexpr float RAID_LEN = 26.f;
constexpr int MAG = 8;
constexpr float DT = 1.f / 60.f;

struct Plan {
    float t;
    int lane;
    int peel;
};

// Five men who will reach the slit. The rest peel if you do not spend a round.
constexpr Plan kPlan[] = {
    {1.6f, 1, 0}, {3.1f, 0, 1}, {3.4f, 2, 1}, {6.4f, 0, 0}, {8.2f, 2, 1},
    {8.8f, 1, 1}, {11.2f, 2, 0}, {13.5f, 0, 1}, {15.6f, 1, 0}, {17.4f, 2, 1},
    {18.0f, 0, 1}, {20.2f, 0, 0},
};
constexpr int NPLAN = int(sizeof(kPlan) / sizeof(kPlan[0]));

}  // namespace

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "MAGAZINE SPENT";
    if (fail_ == Fail::Slit) return "THEY REACHED THE SLIT";
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
        sys.vdp.lineBackdrop[y] = y < 28 ? gs::rgb4(3, 3, 4) : y < 78 ? gs::rgb4(6, 8, 11) : gs::rgb4(4, 5, 3);
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
    lane_ = 1;
    next_ = 0;
    raidT_ = 0;
    cool_ = 0.4f;
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
    sys_->apu.noiseBurst(0.4f, 900.f, 0.25f);
}

void Game::win() {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Won;
    over_ = true;
    won_ = true;
    sys_->apu.tone(0, 523.f, 0.12f);
    sys_->apu.tone(1, 784.f, 0.08f);
}

Game::Foe* Game::inLane(int lane) {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.phase != Phase::Walk || f.lane != lane) continue;
        if (f.z < 0.16f || f.z > 0.64f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    return best;
}

void Game::pull() {
    if (mode_ != Mode::Raid || cool_ > 0) return;
    cool_ = 0.32f;
    flash_ = 0.08f;
    shake_ = 3.5f;
    if (rounds_ <= 1) {
        rounds_ = 0;
        lose(Fail::Spent);
        return;
    }
    rounds_--;
    sys_->apu.noiseBurst(0.55f, 2400.f, 0.08f);
    sys_->apu.tone(2, 140.f, 0.15f);
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
        if (f.z < 0.22f || f.z > 0.52f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    if (!best || rounds_ <= 1) return;
    lane_ = best->lane;
    pull();
}

void Game::humanAct() {
    gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) lane_ = (lane_ + 2) % 3;
    if (p.pressed(gs::BTN_RIGHT)) lane_ = (lane_ + 1) % 3;
    if (p.axisX < -0.4f && cool_ > 0.28f) lane_ = std::max(0, lane_ - 1);
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
        f.kind = kPlan[next_].peel ? Kind::Peel : Kind::Commit;
        f.lane = kPlan[next_].lane;
        f.z = 1.05f;
        foes_.push_back(f);
        next_++;
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.age += dt;
        float sp = f.kind == Kind::Commit ? 0.172f : 0.145f;
        if (f.phase == Phase::Walk) {
            f.z -= sp * dt;
            if (f.kind == Kind::Peel && f.z < 0.50f) f.phase = Phase::Leave;
            if (f.kind == Kind::Commit && f.z < 0.07f) {
                lose(Fail::Slit);
                return;
            }
        } else if (f.phase == Phase::Leave) {
            f.z += sp * 1.3f * dt;
            if (f.z > 1.2f) f.on = false;
        } else {
            if (f.age > 0.7f) f.on = false;
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
    if (mode_ != Mode::Raid && t_ > 0.4f) {
        sys.apu.tone(2, 0, 0);
    }
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
    float sx = (shake_ > 0.4f) ? std::sin(t_ * 90.f) * shake_ : 0.f;

    float aimX = 160.f + (lane_ - 1) * 54.f;
    spr(art_.chev, aimX, 150, 10, PAL_HUD, false, false);
    if (flash_ > 0) spr(art_.flash, aimX, 138, 16, PAL_FX, false, false);
    text("MAG", 16, 168, 1.f, PAL_HUD);
    for (int i = 0; i < MAG; i++) {
        if (i < rounds_) spr(art_.brass, 58.f + i * 12.f, 180, 16, PAL_BRASS, false, false);
    }
    int secs = std::max(0, int(std::ceil(RAID_LEN - raidT_ - 0.001f)));
    char clock[16];
    std::snprintf(clock, sizeof clock, "RAID %02d", mode_ == Mode::Title ? int(RAID_LEN) : secs);
    text(clock, 200, 168, 1.f, secs <= 5 && mode_ == Mode::Raid ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Title) {
        text("S3 BUNKER MAGA", 78, 40, 1.f, PAL_HUD);
        text("MAKE THE MAGAZINE LAST", 58, 58, 1.f, PAL_OK);
        text("A ROUND MUST OUTLIVE THE RAID", 42, 74, 1.f, PAL_HUD);
        text("LEFT RIGHT AIM   A FIRE", 70, 196, 1.f, PAL_HUD);
        if (int(t_ * 2) & 1) text("START", 140, 128, 1.f, PAL_BRASS);
    } else if (mode_ == Mode::Pause) {
        text("HELD", 144, 90, 1.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("THE MAGAZINE OUTLASTS THE RAID", 36, 48, 1.f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text(fail_ == Fail::Spent ? "MAGAZINE SPENT" : "THEY REACHED THE SLIT", 56, 48, 1.f, PAL_ALERT);
    } else {
        text(rounds_ <= 2 ? "LAST ROUNDS" : "HOLD THE SLIT", 108, 196, 1.f, rounds_ <= 2 ? PAL_ALERT : PAL_HUD);
    }

    // Front layer first: concrete covers anyone who walks into the wall.
    for (int i = 0; i < 8; i++) spr(art_.stone, 22.f + i * 40.f + sx, 14, 26, PAL_STONE, false, false);
    spr(art_.pillar, 16 + sx, 90, 120, PAL_STONE, false, false);
    spr(art_.pillar, 304 + sx, 90, 120, PAL_STONE, true, false);
    for (int i = 0; i < 6; i++) spr(art_.stone, 28.f + i * 52.f, 192, 64, PAL_STONE, false, false);

    std::vector<int> order;
    order.reserve(foes_.size());
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].on) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z > foes_[b].z; });
    // Later sprites sit under earlier ones, so near men are submitted first.
    std::reverse(order.begin(), order.end());

    for (int i : order) {
        const Foe& f = foes_[i];
        float near = std::clamp(1.f - f.z, 0.f, 1.f);
        float cx = 160.f + (f.lane - 1) * (28.f + near * 62.f) + sx;
        float cy = 78.f + near * 62.f;
        int fogPal = f.kind == Kind::Commit ? PAL_FOE : PAL_PEEL;
        if (f.phase == Phase::Down) {
            spr(art_.down, cx, cy, 12.f + near * 10.f, fogPal, false, true);
            spr(art_.puff, cx, cy - 8, 14, PAL_FX, false, false);
        } else {
            const gs::Mipped& img = (f.kind == Kind::Commit ? art_.foe : art_.peel)[int(f.age * 6) & 1];
            float h = 12.f + near * 40.f;
            bool flip = f.phase == Phase::Leave ? (f.lane != 2) : false;
            spr(img, cx, cy, h, fogPal, flip, true);
        }
    }

    for (int i = 0; i < 5; i++) spr(art_.wire, 78.f + i * 38.f + sx, 128, 30, PAL_FIELD, i & 1, true);
    spr(art_.field, 160 + sx, 112, 100, PAL_FIELD, false, false);
}

}  // namespace bmaga
