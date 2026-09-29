#include "causeway.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cwmaga {
namespace {

constexpr float RAID_LEN = 26.f;
constexpr int MAG = 7;
constexpr float DT = 1.f / 60.f;
constexpr int HORIZON = 78;

struct Plan {
    float t;
    int lane;
    int peel;
};

// Six men stay on the stone. The rest step off into skiffs if you do not spend a round.
constexpr Plan kPlan[] = {
    {1.4f, 0, 0},  {2.8f, -1, 1}, {3.2f, 1, 1},  {5.2f, 1, 0},  {7.2f, -1, 1}, {7.8f, 0, 1},
    {9.4f, -1, 0}, {11.4f, 1, 1}, {12.0f, -1, 1}, {13.8f, 0, 0}, {16.0f, 1, 1}, {16.6f, -1, 1},
    {18.2f, 1, 0}, {20.4f, 0, 1}, {22.0f, -1, 0},
};
constexpr int NPLAN = int(sizeof(kPlan) / sizeof(kPlan[0]));

}  // namespace

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "MAGAZINE SPENT";
    if (fail_ == Fail::Taken) return "THEY TOOK THE CAUSEWAY";
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
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.4f);
    mode_ = Mode::Title;
}

void Game::beginRaid() {
    mode_ = Mode::Raid;
    fail_ = Fail::None;
    over_ = false;
    won_ = false;
    rounds_ = MAG;
    stopped_ = 0;
    lane_ = 0;
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
    sys_->apu.tone(0, 494.f, 0.1f);
    sys_->apu.tone(1, 740.f, 0.08f);
}

Game::Foe* Game::inLane(int lane) {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.phase != Phase::Walk || f.lane != lane) continue;
        if (f.z < 0.20f || f.z > 0.58f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    return best;
}

void Game::pull() {
    if (mode_ != Mode::Raid || cool_ > 0) return;
    cool_ = 0.30f;
    flash_ = 0.07f;
    shake_ = 3.2f;
    if (rounds_ <= 1) {
        rounds_ = 0;
        lose(Fail::Spent);
        return;
    }
    rounds_--;
    sys_->apu.noiseBurst(0.5f, 2200.f, 0.07f);
    sys_->apu.tone(2, 120.f, 0.12f);
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
    pull();
}

void Game::humanAct() {
    gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) lane_ = std::max(-1, lane_ - 1);
    if (p.pressed(gs::BTN_RIGHT)) lane_ = std::min(1, lane_ + 1);
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.accel > 0.55f) pull();
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
        f.side = f.lane == 0 ? ((next_ & 1) ? 1 : -1) : (f.lane < 0 ? -1 : 1);
        f.z = 1.05f;
        foes_.push_back(f);
        next_++;
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.age += dt;
        float sp = f.kind == Kind::Commit ? 0.155f : 0.135f;
        if (f.phase == Phase::Walk) {
            f.z -= sp * dt;
            if (f.kind == Kind::Peel && f.z < 0.52f) f.phase = Phase::Leave;
            if (f.kind == Kind::Commit && f.z < 0.08f) {
                lose(Fail::Taken);
                return;
            }
        } else if (f.phase == Phase::Leave) {
            f.z += sp * 0.35f * dt;
            if (f.age > 1.6f) f.on = false;
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
    const gs::Pad& pad = sys.pad;
    bool start = !bot_ && pad.pressed(gs::BTN_START);
    if (mode_ == Mode::Title) {
        if (bot_ || start || pad.pressed(gs::BTN_A)) beginRaid();
    } else if (mode_ == Mode::Raid) {
        if (start) mode_ = Mode::Pause;
        else updateRaid(DT);
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Raid;
    } else if (start || pad.pressed(gs::BTN_A)) {
        beginRaid();
    }
    draw();
}

void Game::place(const Foe& f, float& x, float& y, float& h) const {
    float depth = std::clamp((1.05f - f.z) / 1.05f, 0.f, 1.f);
    float spread = 16.f + depth * depth * 92.f;
    float drift = 0.f;
    if (f.phase == Phase::Leave) drift = float(f.side) * std::min(1.f, f.age) * (40.f + depth * 70.f);
    x = 160.f + float(f.lane) * spread + drift;
    y = float(HORIZON) + depth * 108.f;
    h = 10.f + depth * 36.f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.f || m.h < 1) return;
    float w = h * (float(m.w) / float(m.h));
    gs::Sprite s;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int x = col + i;
        if (x < 0 || x > 39 || c < 32 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    int n = int(std::strlen(s));
    float adv = 16.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, std::max(8.f, float(g.h) * scale), pal, false, 0);
    }
}

void Game::road() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 60.f);
    float sway = std::sin(t_ * 0.28f) * 10.f;
    float scroll = raidT_ * 40.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float sky = float(y) / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(2 + int(sky * 4), 2 + int(sky * 3), 6 + int((1.f - sky) * 4));
            v.lineFog[y] = uint8_t(6 - int(sky * 4));
            v.road[y].on = false;
            continue;
        }
        float depth = float(y - HORIZON) / float(gs::SCREEN_H - HORIZON);
        gs::RoadLine& rl = v.road[y];
        rl = {};
        rl.on = true;
        rl.cx = 160.f + sway * (1.f - depth);
        rl.hw = 10.f + depth * depth * 156.f;
        rl.v = 2600.f / (depth + 0.14f) - scroll;
        rl.pal = 12;
        rl.band = (int(std::floor(rl.v / 80.f)) & 1) ? 1 : 0;
        rl.style = gs::ROAD_ROCKY;
        rl.left = gs::GROUND_WATER;
        rl.right = gs::GROUND_WATER;
        v.lineBackdrop[y] = gs::rgb4(1, 3, 6);
        v.lineFog[y] = uint8_t(std::clamp(int((1.f - depth) * 9.f), 0, 9));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    road();

    for (int i = 0; i < 4; i++) {
        float near = 0.22f + float(i) * 0.2f;
        float h = 12.f + near * 20.f;
        float y = float(HORIZON) + near * 96.f;
        float spread = 36.f + near * 118.f;
        int fog = int((1.f - near) * 8);
        spr(art_.lamp, 160.f - spread, y, h, PAL_STONE, false, fog);
        spr(art_.lamp, 160.f + spread, y, h, PAL_STONE, false, fog);
    }

    std::vector<int> order;
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].on) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z > foes_[b].z; });
    for (int idx : order) {
        const Foe& f = foes_[idx];
        float x, y, h;
        place(f, x, y, h);
        int fog = int(std::clamp((f.z - 0.2f) * 10.f, 0.f, 12.f));
        if (f.phase == Phase::Leave) spr(art_.skiff, x, y + h * 0.45f, h * 0.55f, PAL_SKIFF, f.side < 0, fog);
        if (f.phase != Phase::Down)
            spr(art_.runner[(int(f.age * 8.f) & 1)], x, y, h, PAL_FOE, f.lane < 0, fog);
    }

    float px = 160.f + float(lane_) * 48.f + std::sin(t_ * 90.f) * (shake_ > 0.4f ? shake_ : 0.f);
    int fr = int(t_ * 8.f) & 1;
    if (flash_ > 0) spr(art_.flash, px + 12.f, 158.f, 12.f, PAL_FX, false, 0);
    if (mode_ != Mode::Title) spr(art_.gunner[fr], px, 168.f, 48.f, PAL_YOU, lane_ < 0, 0);
    else {
        spr(art_.runner[fr], 120.f, 100.f, 34.f, PAL_FOE, false, 2);
        spr(art_.skiff, 210.f, 118.f, 18.f, PAL_SKIFF, true, 1);
        spr(art_.gunner[fr], 160.f, 150.f, 50.f, PAL_YOU, false, 0);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(22, "YOU HAVE THE CAUSEWAY.", PAL_HUD);
        hudC(23, "MAKE THE MAGAZINE LAST.", PAL_OK);
        hudC(24, "LEFT RIGHT THE LANES    A FIRES", PAL_HUD);
        hudC(25, "SKIFFS ARE NOT A ROUND.", PAL_HUD);
        hudC(26, "ANYTHING ELSE IS A LOSS.", PAL_ALERT);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
        text("S3 CAUSEWAY", 160, 14, 0.7f, PAL_HUD);
    } else {
        int secs = std::max(0, int(std::ceil(RAID_LEN - raidT_ - 0.001f)));
        std::snprintf(line, sizeof line, "RAID %02d", secs);
        hud(1, 0, line, secs <= 5 && mode_ == Mode::Raid ? PAL_ALERT : PAL_HUD);
        std::snprintf(line, sizeof line, "MAG %d", rounds_);
        hud(30, 0, line, rounds_ <= 2 ? PAL_ALERT : PAL_BRASS);
        const char* laneName = lane_ < 0 ? "WEST" : lane_ > 0 ? "EAST" : "CROWN";
        std::snprintf(line, sizeof line, "LANE %s", laneName);
        hud(1, 27, laneName ? line : line, PAL_HUD);
        for (int i = 0; i < rounds_; i++) spr(art_.brass, 214.f + float(i) * 10.f, 8.f, 12.f, PAL_BRASS, false, 0);
        if (mode_ == Mode::Pause) hudC(24, "PAUSED", PAL_HUD);
        else if (mode_ == Mode::Won) {
            hudC(24, "THE MAGAZINE OUTLASTS THE RAID", PAL_OK);
            text("HELD", 160, 28, 1.f, PAL_OK);
        } else if (mode_ == Mode::Lost) {
            hudC(24, result(), PAL_ALERT);
            text("LOST", 160, 28, 1.f, PAL_ALERT);
        } else {
            hudC(26, "SAVE A ROUND", PAL_HUD);
        }
    }
}

}  // namespace cwmaga
