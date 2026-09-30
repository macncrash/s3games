#include "wharf.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace wharfmaga {
namespace {

constexpr float RAID_LEN = 24.f;
constexpr int MAG = 7;
constexpr float DT = 1.f / 60.f;
constexpr int HORIZON = 72;

struct Plan {
    float t;
    int berth;
    int swing;
};

// Five men stay on the planks. The rest drop a line into a dinghy.
constexpr Plan kPlan[] = {
    {1.1f, 0, 0},  {2.3f, -1, 1}, {3.1f, 1, 1},  {4.4f, 1, 0},  {6.0f, -1, 1}, {6.8f, 0, 1},
    {8.0f, -1, 0}, {9.6f, 1, 1},  {10.6f, 0, 1}, {12.2f, 0, 0}, {14.0f, -1, 1}, {15.2f, 1, 1},
    {16.6f, 1, 0}, {18.4f, 0, 1}, {20.2f, -1, 1},
};
constexpr int NPLAN = int(sizeof(kPlan) / sizeof(kPlan[0]));

}  // namespace

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "MAGAZINE SPENT";
    if (fail_ == Fail::Taken) return "THEY TOOK THE WHARF";
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
    berth_ = 0;
    next_ = 0;
    raidT_ = 0;
    cool_ = 0.28f;
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
    sys_->apu.tone(0, 523.f, 0.1f);
    sys_->apu.tone(1, 784.f, 0.08f);
}

Game::Foe* Game::inBerth(int berth) {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.phase != Phase::Walk || f.berth != berth) continue;
        if (f.z < 0.18f || f.z > 0.56f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    return best;
}

void Game::pull() {
    if (mode_ != Mode::Raid || cool_ > 0) return;
    cool_ = 0.28f;
    flash_ = 0.06f;
    shake_ = 2.8f;
    if (rounds_ <= 1) {
        rounds_ = 0;
        lose(Fail::Spent);
        return;
    }
    rounds_--;
    sys_->apu.noiseBurst(0.48f, 1900.f, 0.06f);
    sys_->apu.tone(2, 90.f, 0.1f);
    if (Foe* f = inBerth(berth_)) {
        f->phase = Phase::Down;
        f->age = 0;
        stopped_++;
    }
}

void Game::botAct() {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.kind != Kind::Board || f.phase != Phase::Walk) continue;
        if (f.z < 0.22f || f.z > 0.48f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    if (!best || rounds_ <= 1) return;
    berth_ = best->berth;
    pull();
}

void Game::humanAct() {
    gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) berth_ = std::max(-1, berth_ - 1);
    if (p.pressed(gs::BTN_RIGHT)) berth_ = std::min(1, berth_ + 1);
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.accel > 0.55f) pull();
}

void Game::updateRaid(float dt) {
    if (bot_) botAct();
    else humanAct();
    cool_ = std::max(0.f, cool_ - dt);
    flash_ = std::max(0.f, flash_ - dt);
    shake_ *= 0.84f;

    while (next_ < NPLAN && raidT_ >= kPlan[next_].t) {
        Foe f;
        f.kind = kPlan[next_].swing ? Kind::Swing : Kind::Board;
        f.berth = kPlan[next_].berth;
        f.side = f.berth == 0 ? ((next_ & 1) ? 1 : -1) : (f.berth < 0 ? -1 : 1);
        f.z = 1.06f;
        foes_.push_back(f);
        next_++;
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.age += dt;
        float sp = f.kind == Kind::Board ? 0.148f : 0.128f;
        if (f.phase == Phase::Walk) {
            f.z -= sp * dt;
            if (f.kind == Kind::Swing && f.z < 0.55f) f.phase = Phase::Drop;
            if (f.kind == Kind::Board && f.z < 0.07f) {
                lose(Fail::Taken);
                return;
            }
        } else if (f.phase == Phase::Drop) {
            f.z += sp * 0.2f * dt;
            if (f.age > 1.8f) f.on = false;
        } else if (f.age > 0.55f) {
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
    float depth = std::clamp((1.06f - f.z) / 1.06f, 0.f, 1.f);
    float spread = 18.f + depth * depth * 86.f;
    float drift = 0.f;
    if (f.phase == Phase::Drop) drift = float(f.side) * std::min(1.f, f.age * 0.8f) * (48.f + depth * 80.f);
    x = 160.f + float(f.berth) * spread + drift;
    y = float(HORIZON) + depth * 112.f;
    h = 9.f + depth * 34.f;
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

void Game::pier() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 60.f);
    float swell = std::sin(t_ * 0.7f) * 6.f;
    float scroll = raidT_ * 28.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float sky = float(y) / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(1 + int(sky * 2), 2 + int(sky * 3), 5 + int((1.f - sky) * 5));
            v.lineFog[y] = uint8_t(7 - int(sky * 4));
            v.road[y].on = false;
            continue;
        }
        float depth = float(y - HORIZON) / float(gs::SCREEN_H - HORIZON);
        gs::RoadLine& rl = v.road[y];
        rl = {};
        rl.on = true;
        rl.cx = 160.f + swell * (1.f - depth);
        rl.hw = 8.f + depth * depth * 132.f;
        rl.v = 2200.f / (depth + 0.16f) - scroll;
        rl.pal = 12;
        rl.band = (int(std::floor(rl.v / 64.f)) & 1) ? 1 : 0;
        rl.style = 0;
        rl.left = gs::GROUND_WATER;
        rl.right = gs::GROUND_WATER;
        v.lineBackdrop[y] = gs::rgb4(1, 3, 6);
        v.lineFog[y] = uint8_t(std::clamp(int((1.f - depth) * 10.f), 0, 10));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    pier();

    for (int i = 0; i < 5; i++) {
        float near = 0.12f + float(i) * 0.18f;
        float h = 16.f + near * 28.f;
        float y = float(HORIZON) + near * 100.f;
        float spread = 28.f + near * 130.f;
        int fog = int((1.f - near) * 9);
        spr(art_.pile, 160.f - spread, y, h, PAL_PILE, false, fog);
        spr(art_.pile, 160.f + spread, y, h, PAL_PILE, true, fog);
        if (i == 2 || i == 4) spr(art_.crate, 160.f + (i == 2 ? -18.f : 22.f), y + 6.f, h * 0.45f, PAL_CRATE, false, fog);
    }
    for (int g = 0; g < 3; g++) {
        float gx = 40.f + std::fmod(t_ * (18.f + g * 7.f) + g * 90.f, 280.f);
        float gy = 28.f + float(g) * 14.f + std::sin(t_ * 1.4f + g) * 4.f;
        spr(art_.gull[int(t_ * 6.f + g) & 1], gx, gy, 10.f, PAL_GULL, false, 2);
    }

    std::vector<int> order;
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].on) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z > foes_[b].z; });
    for (int idx : order) {
        const Foe& f = foes_[idx];
        float x, y, h;
        place(f, x, y, h);
        int fog = int(std::clamp((f.z - 0.15f) * 11.f, 0.f, 12.f));
        if (f.phase == Phase::Drop) spr(art_.dinghy, x, y + h * 0.5f, h * 0.42f, PAL_DINGHY, f.side < 0, fog);
        if (f.phase != Phase::Down) {
            int fr = int(f.age * 7.f) & 1;
            if (f.kind == Kind::Swing)
                spr(art_.swinger[fr], x, y, h, PAL_SWING, f.side < 0, fog);
            else
                spr(art_.boarder[fr], x, y, h, PAL_BOARD, f.berth < 0, fog);
        }
    }

    float px = 160.f + float(berth_) * 42.f + std::sin(t_ * 80.f) * (shake_ > 0.4f ? shake_ : 0.f);
    int fr = int(t_ * 7.f) & 1;
    if (flash_ > 0) spr(art_.flash, px + 14.f, 156.f, 14.f, PAL_FX, false, 0);
    if (mode_ != Mode::Title) spr(art_.keeper[fr], px, 164.f, 52.f, PAL_KEEP, berth_ < 0, 0);
    else {
        spr(art_.boarder[fr], 96.f, 96.f, 36.f, PAL_BOARD, false, 2);
        spr(art_.swinger[fr], 230.f, 108.f, 30.f, PAL_SWING, true, 1);
        spr(art_.dinghy, 236.f, 132.f, 16.f, PAL_DINGHY, true, 1);
        spr(art_.keeper[fr], 160.f, 148.f, 54.f, PAL_KEEP, false, 0);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(22, "YOU HAVE THE WHARF.", PAL_HUD);
        hudC(23, "MAKE THE MAGAZINE LAST.", PAL_OK);
        hudC(24, "LEFT RIGHT THE BERTHS   A FIRES", PAL_HUD);
        hudC(25, "SWINGERS ARE NOT A ROUND.", PAL_HUD);
        hudC(26, "ANYTHING ELSE IS A LOSS.", PAL_ALERT);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
        text("S3 WHARF", 160, 16, 0.85f, PAL_HUD);
    } else {
        int secs = std::max(0, int(std::ceil(RAID_LEN - raidT_ - 0.001f)));
        std::snprintf(line, sizeof line, "RAID %02d", secs);
        hud(1, 0, line, secs <= 5 && mode_ == Mode::Raid ? PAL_ALERT : PAL_HUD);
        std::snprintf(line, sizeof line, "MAG %d", rounds_);
        hud(32, 0, line, rounds_ <= 2 ? PAL_ALERT : PAL_BRASS);
        const char* name = berth_ < 0 ? "PORT" : berth_ > 0 ? "STARBOARD" : "MID";
        std::snprintf(line, sizeof line, "BERTH %s", name);
        hud(1, 27, line, PAL_HUD);
        for (int i = 0; i < rounds_; i++) spr(art_.brass, 200.f + float(i) * 11.f, 10.f, 11.f, PAL_BRASS, false, 0);
        if (mode_ == Mode::Pause) hudC(24, "PAUSED", PAL_HUD);
        else if (mode_ == Mode::Won) {
            hudC(24, "THE MAGAZINE OUTLASTS THE RAID", PAL_OK);
            text("HELD", 160, 30, 1.f, PAL_OK);
        } else if (mode_ == Mode::Lost) {
            hudC(24, result(), PAL_ALERT);
            text("LOST", 160, 30, 1.f, PAL_ALERT);
        } else {
            hudC(26, "KEEP A ROUND", PAL_HUD);
        }
    }
}

}  // namespace wharfmaga
