#include "game/foundry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace foundrymaga {
namespace {

constexpr float RAID_LEN = 16.f;
constexpr int MAG = 6;
constexpr float DT = 1.f / 60.f;
constexpr float HORIZON = 78.f;

struct Plan {
    float t;
    int lane;
    int peel;
};

// Four men will reach the pour lip. The rest turn at the crucibles if you do not fire.
constexpr Plan kPlan[] = {
    {1.1f, 1, 0}, {2.5f, 0, 1}, {2.9f, 2, 1}, {4.8f, 0, 0}, {6.6f, 2, 1},
    {7.2f, 1, 1}, {8.8f, 2, 0}, {10.6f, 0, 1}, {12.2f, 1, 0}, {13.4f, 2, 1},
};
constexpr int NPLAN = int(sizeof(kPlan) / sizeof(kPlan[0]));

uint16_t mixC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [](int c0, int c1, float u) { return int(std::lround(c0 + (c1 - c0) * u)); };
    return gs::rgb4(ch(ar, br, t), ch(ag, bg, t), ch(ab, bb, t));
}

}  // namespace

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "MAGAZINE SPENT";
    if (fail_ == Fail::Lip) return "THEY REACHED THE LIP";
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
    sys.vdp.HUD.enabled = false;
    sys.apu.setMaster(0.4f);
    floor();
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
    sys_->apu.tone(1, 523.f, 0.08f);
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
    shake_ = 3.2f;
    if (rounds_ <= 1) {
        rounds_ = 0;
        lose(Fail::Spent);
        return;
    }
    rounds_--;
    sys_->apu.noiseBurst(0.5f, 1800.f, 0.07f);
    sys_->apu.tone(2, 90.f, 0.12f);
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
        if (f.z < 0.28f || f.z > 0.48f) continue;
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
        f.z = 1.04f;
        foes_.push_back(f);
        next_++;
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.age += dt;
        float sp = f.kind == Kind::Commit ? 0.155f : 0.13f;
        if (f.phase == Phase::Walk) {
            f.z -= sp * dt;
            if (f.kind == Kind::Peel && f.z < 0.50f) f.phase = Phase::Leave;
            if (f.kind == Kind::Commit && f.z < 0.08f) {
                lose(Fail::Lip);
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
    floor();
    draw();
}

void Game::floor() {
    gs::VDP& v = sys_->vdp;
    uint16_t sky = gs::rgb4(1, 1, 2);
    uint16_t glow = gs::rgb4(12, 4, 1);
    if (mode_ == Mode::Lost) glow = gs::rgb4(8, 1, 1);
    else if (mode_ == Mode::Won) glow = gs::rgb4(4, 8, 2);
    v.setFogColor(glow);
    const float span = float(gs::SCREEN_H) - HORIZON;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(HORIZON)) {
            rd.on = false;
            float u = float(y) / HORIZON;
            v.lineBackdrop[y] = mixC(sky, glow, u * u);
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) + 0.5f - HORIZON) / span;
        t = std::max(t, 0.04f);
        float wz = 2.2f / t;
        rd.on = true;
        rd.cx = 160.f;
        rd.hw = std::max(18.f, 2.4f * 90.f * t);
        rd.v = wz * 22.f + t_ * 4.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = gs::ROAD_ROCKY;
        rd.band = (int(std::floor(wz * 0.4f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_DROP;
        rd.right = gs::GROUND_DROP;
        float fogT = std::clamp((wz - 6.f) / 14.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 10.f);
        v.lineBackdrop[y] = mixC(gs::rgb4(5, 2, 1), gs::rgb4(1, 1, 1), std::clamp((float(y) - HORIZON) / span, 0.f, 1.f));
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
    float sx = (shake_ > 0.4f) ? std::sin(t_ * 80.f) * shake_ : 0.f;
    int fr = (int(t_ * 8.f) & 1);

    float aimX = 160.f + (lane_ - 1) * 58.f;
    spr(art_.chev, aimX + sx, 146, 10, PAL_HUD, false, false);
    if (flash_ > 0) spr(art_.flash, aimX + sx, 136, 14, PAL_FX, false, false);
    text("MAG", 12, 198, 1.f, PAL_HUD);
    for (int i = 0; i < MAG; i++) {
        if (i < rounds_) spr(art_.brass, 48.f + i * 11.f, 206, 14, PAL_BRASS, false, false);
    }
    int secs = std::max(0, int(std::ceil(RAID_LEN - raidT_ - 0.001f)));
    char clock[16];
    std::snprintf(clock, sizeof clock, "RAID %02d", mode_ == Mode::Title ? int(RAID_LEN) : secs);
    text(clock, 214, 198, 1.f, secs <= 4 && mode_ == Mode::Raid ? PAL_ALERT : PAL_HUD);

    if (mode_ == Mode::Title) {
        text("S3 FOUNDRY MAGA", 86, 18, 1.f, PAL_HUD);
        text("MAKE THE MAGAZINE LAST", 70, 34, 1.f, PAL_OK);
        text("A ROUND MUST OUTLIVE THE RAID", 52, 48, 1.f, PAL_EMBER);
        text("LEFT RIGHT AIM   A FIRE", 78, 214, 1.f, PAL_HUD);
        if (int(t_ * 2) & 1) text("START", 140, 96, 1.f, PAL_BRASS);
    } else if (mode_ == Mode::Pause) {
        text("HELD", 144, 28, 1.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("THE MAGAZINE OUTLASTS THE RAID", 40, 18, 1.f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text(fail_ == Fail::Spent ? "MAGAZINE SPENT" : "THEY REACHED THE LIP", 72, 18, 1.f, PAL_ALERT);
    } else {
        text(rounds_ <= 2 ? "LAST ROUNDS" : "HOLD THE LIP", 112, 214, 1.f, rounds_ <= 2 ? PAL_ALERT : PAL_HUD);
    }

    std::vector<int> order;
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].on) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z > foes_[b].z; });
    std::reverse(order.begin(), order.end());
    for (int i : order) {
        const Foe& f = foes_[i];
        float near = std::clamp(1.f - f.z, 0.f, 1.f);
        float cx = 160.f + (f.lane - 1) * (22.f + near * 70.f) + sx;
        float cy = 92.f + near * 78.f;
        int pal = f.kind == Kind::Commit ? PAL_FOE : PAL_PEEL;
        if (f.phase == Phase::Down) {
            spr(art_.down, cx, cy, 10.f + near * 8.f, pal, false, true);
        } else {
            const gs::Mipped& img = (f.kind == Kind::Commit ? art_.apron : art_.peel)[int(f.age * 6) & 1];
            bool flip = f.phase == Phase::Leave;
            spr(img, cx, cy, 14.f + near * 36.f, pal, flip, true);
        }
    }

    spr(art_.chimney, 36 + sx, 70, 52, PAL_IRON, false, true);
    spr(art_.chimney, 284 + sx, 66, 58, PAL_IRON, false, true);
    spr(art_.furnace, 48 + sx, 118, 70, PAL_BRICK, false, true);
    spr(art_.furnace, 272 + sx, 122, 76, PAL_BRICK, true, true);
    spr(art_.flame[fr], 48 + sx, 86, 18, PAL_EMBER, false, false);
    spr(art_.flame[fr], 272 + sx, 88, 20, PAL_EMBER, false, false);
    spr(art_.ingot, 86 + sx, 150, 10, PAL_SLAG, false, false);
    spr(art_.ingot, 230 + sx, 156, 12, PAL_IRON, false, false);
    for (int lane = 0; lane < 3; lane++) {
        float cx = 160.f + (lane - 1) * 58.f + sx;
        spr(art_.crucible, cx, 132, 22, PAL_IRON, false, true);
    }
    spr(art_.lip, 160 + sx, 188, 22, PAL_BRICK, false, true);
}

}  // namespace foundrymaga
