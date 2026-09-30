#include "granary.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace grmaga {
namespace {

constexpr float RAID_LEN = 24.f;
constexpr int MAG = 6;
constexpr float DT = 1.f / 60.f;
constexpr int HORIZON = 72;

struct Plan {
    float t;
    int lane;
    int sack;
};

// Four ram-men stay on a lane all the way to the grain door.
// Sack-carriers turn into the ricks and are not worth a round.
constexpr Plan kPlan[] = {
    {1.1f, -1, 1}, {2.2f, 0, 0},  {3.8f, 1, 1},  {5.6f, -1, 0}, {7.4f, 0, 1},
    {9.2f, 1, 0},  {11.4f, -1, 1}, {13.2f, 0, 0}, {15.6f, 1, 1}, {18.0f, -1, 1}, {20.4f, 1, 1},
};
constexpr int NPLAN = int(sizeof(kPlan) / sizeof(kPlan[0]));

}  // namespace

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "MAGAZINE SPENT";
    if (fail_ == Fail::Door) return "THEY REACHED THE GRAIN DOOR";
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
    sys_->apu.noiseBurst(0.4f, 640.f, 0.3f);
}

void Game::win() {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Won;
    over_ = true;
    won_ = true;
    sys_->apu.tone(0, 392.f, 0.1f);
    sys_->apu.tone(1, 587.f, 0.08f);
}

Game::Foe* Game::inLane(int lane) {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.phase != Phase::Walk || f.lane != lane) continue;
        if (f.z < 0.22f || f.z > 0.56f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    return best;
}

void Game::fire() {
    if (mode_ != Mode::Raid || cool_ > 0) return;
    cool_ = 0.32f;
    flash_ = 0.08f;
    shake_ = 3.4f;
    if (rounds_ <= 1) {
        rounds_ = 0;
        lose(Fail::Spent);
        return;
    }
    rounds_--;
    sys_->apu.noiseBurst(0.48f, 1800.f, 0.07f);
    sys_->apu.tone(2, 96.f, 0.1f);
    if (Foe* f = inLane(lane_)) {
        f->phase = Phase::Down;
        f->age = 0;
        stopped_++;
    }
}

void Game::botAct() {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.kind != Kind::Ram || f.phase != Phase::Walk) continue;
        if (f.z < 0.30f || f.z > 0.48f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    if (!best || rounds_ <= 1) return;
    lane_ = best->lane;
    fire();
}

void Game::humanAct() {
    gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) lane_ = std::max(-1, lane_ - 1);
    if (p.pressed(gs::BTN_RIGHT)) lane_ = std::min(1, lane_ + 1);
    if (p.axisX < -0.45f) lane_ = -1;
    else if (p.axisX > 0.45f) lane_ = 1;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.accel > 0.55f) fire();
}

void Game::updateRaid(float dt) {
    if (bot_) botAct();
    else humanAct();
    cool_ = std::max(0.f, cool_ - dt);
    flash_ = std::max(0.f, flash_ - dt);
    shake_ *= 0.86f;

    while (next_ < NPLAN && raidT_ >= kPlan[next_].t) {
        Foe f;
        f.kind = kPlan[next_].sack ? Kind::Sack : Kind::Ram;
        f.lane = kPlan[next_].lane;
        f.side = f.lane == 0 ? ((next_ & 1) ? 1 : -1) : (f.lane < 0 ? -1 : 1);
        f.z = 1.05f;
        foes_.push_back(f);
        next_++;
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.age += dt;
        float sp = f.kind == Kind::Ram ? 0.16f : 0.14f;
        if (f.phase == Phase::Walk) {
            f.z -= sp * dt;
            if (f.kind == Kind::Sack && f.z < 0.50f) f.phase = Phase::Peel;
            if (f.kind == Kind::Ram && f.z < 0.08f) {
                lose(Fail::Door);
                return;
            }
        } else if (f.phase == Phase::Peel) {
            f.z -= sp * 0.15f * dt;
            if (f.age > 2.2f) f.on = false;
        } else if (f.age > 0.7f) {
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
    float spread = 18.f + depth * depth * 86.f;
    float drift = 0.f;
    if (f.phase == Phase::Peel) drift = float(f.side) * std::min(1.f, f.age) * (48.f + depth * 50.f);
    x = 160.f + float(f.lane) * spread + drift;
    y = float(HORIZON) + depth * 100.f;
    h = 12.f + depth * 34.f;
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

void Game::yard() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 40.f);
    float sway = std::sin(t_ * 0.22f) * 6.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float sky = float(y) / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(4 + int((1.f - sky) * 6), 3 + int(sky * 3), 6 + int((1.f - sky) * 5));
            v.lineFog[y] = uint8_t(4);
            v.road[y].on = false;
            continue;
        }
        float depth = float(y - HORIZON) / float(gs::SCREEN_H - HORIZON);
        gs::RoadLine& rl = v.road[y];
        rl = {};
        rl.on = true;
        rl.cx = 160.f + sway * (1.f - depth);
        rl.hw = 14.f + depth * depth * 130.f;
        rl.v = 1800.f / (depth + 0.16f);
        rl.pal = 12;
        rl.band = (int(std::floor(rl.v / 90.f)) & 1) ? 1 : 0;
        rl.style = 0;
        rl.left = gs::GROUND_LAND;
        rl.right = gs::GROUND_LAND;
        v.lineBackdrop[y] = gs::rgb4(4, 5, 2);
        v.lineFog[y] = uint8_t(std::clamp(int((1.f - depth) * 8.f), 0, 8));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    yard();

    float px = 160.f + float(lane_) * 42.f + std::sin(t_ * 80.f) * (shake_ > 0.4f ? shake_ : 0.f);
    int fr = int(t_ * 7.f) & 1;
    if (mode_ != Mode::Title) {
        if (flash_ > 0) spr(art_.flash, px + (lane_ < 0 ? -16.f : 16.f), 150.f, 12.f, PAL_FLASH, lane_ < 0, 0);
        spr(art_.watcher[fr], px, 164.f, 52.f, PAL_WATCH, lane_ < 0, 0);
    }

    std::vector<int> order;
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].on) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z < foes_[b].z; });
    for (int idx : order) {
        const Foe& f = foes_[idx];
        float x, y, h;
        place(f, x, y, h);
        int fog = int(std::clamp((f.z - 0.15f) * 12.f, 0.f, 12.f));
        if (f.phase != Phase::Down) {
            if (f.kind == Kind::Sack || f.phase == Phase::Peel)
                spr(art_.sack, x + 6.f, y + h * 0.35f, h * 0.42f, PAL_SACK, false, fog);
            spr(art_.raider[(int(f.age * 8.f) & 1)], x, y, h, PAL_RAIDER, f.side < 0, fog);
        }
    }

    spr(art_.silo, 36.f, 28.f, 92.f, PAL_TIMBER, false, 3);
    spr(art_.door, 160.f, 48.f, 46.f, PAL_TIMBER, false, 5);
    spr(art_.silo, 286.f, 36.f, 78.f, PAL_TIMBER, true, 3);

    char line[64];
    if (mode_ == Mode::Title) {
        spr(art_.raider[fr], 96.f, 108.f, 36.f, PAL_RAIDER, false, 2);
        spr(art_.sack, 118.f, 122.f, 16.f, PAL_SACK, false, 1);
        spr(art_.watcher[fr], 200.f, 140.f, 56.f, PAL_WATCH, false, 0);
        hudC(21, "THE LOFT. ONE MAGAZINE.", PAL_HUD);
        hudC(22, "MAKE IT LAST THE RAID.", PAL_HELD);
        hudC(23, "LEFT RIGHT AIM    A FIRES", PAL_HUD);
        hudC(24, "SACKS PEEL INTO THE RICKS.", PAL_HUD);
        hudC(25, "A SPENT MAGAZINE ENDS THE WATCH.", PAL_LOST);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
        const char* title = "S3 GRANARY MAGA";
        int tn = int(std::strlen(title));
        float adv = 11.f;
        float left = 160.f - tn * adv * 0.5f;
        for (int i = 0; title[i]; i++) {
            unsigned char c = static_cast<unsigned char>(title[i]);
            if (c < 33 || c > 126) continue;
            spr(art_.glyph[c - 32], left + float(i) * adv + adv * 0.5f, 12.f, 14.f, PAL_HUD, false, 0);
        }
    } else {
        int secs = std::max(0, int(std::ceil(RAID_LEN - raidT_ - 0.001f)));
        std::snprintf(line, sizeof line, "RAID %02d", secs);
        hud(1, 0, line, secs <= 5 && mode_ == Mode::Raid ? PAL_LOST : PAL_HUD);
        std::snprintf(line, sizeof line, "RND %d", rounds_);
        hud(32, 0, line, rounds_ <= 2 ? PAL_LOST : PAL_GRAIN);
        const char* laneName = lane_ < 0 ? "LEFT RICK" : lane_ > 0 ? "RIGHT RICK" : "GRAIN DOOR";
        std::snprintf(line, sizeof line, "%s", laneName);
        hud(1, 27, line, PAL_HUD);
        std::snprintf(line, sizeof line, "STOP %d", stopped_);
        hud(30, 27, line, PAL_HUD);
        for (int i = 0; i < rounds_; i++) spr(art_.round, 228.f + float(i) * 12.f, 10.f, 14.f, PAL_GRAIN, false, 0);
        if (mode_ == Mode::Pause) hudC(24, "PAUSED", PAL_HUD);
        else if (mode_ == Mode::Won) {
            hudC(23, "THE MAGAZINE OUTLASTS THE RAID", PAL_HELD);
        } else if (mode_ == Mode::Lost) {
            hudC(23, result(), PAL_LOST);
        } else {
            hudC(25, "KEEP A ROUND", PAL_HUD);
        }
    }
}

}  // namespace grmaga
