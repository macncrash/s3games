#include "game/maga.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace rmaga {
namespace {

constexpr float RAID_LEN = 22.f;
constexpr int MAG = 6;
constexpr int FACES = 4;
constexpr float DT = 1.f / 60.f;
constexpr float FACE_X[FACES] = {46.f, 112.f, 208.f, 274.f};

struct Plan {
    float t;
    int face;
    int storm;
};

// Stormers cross the gabion line. Scouts break on the glacis if left alone.
constexpr Plan kPlan[] = {
    {1.1f, 0, 1}, {2.6f, 2, 0}, {3.2f, 3, 0}, {5.0f, 1, 1}, {7.2f, 0, 0},
    {8.6f, 3, 1}, {10.4f, 2, 0}, {12.0f, 0, 1}, {14.2f, 1, 0}, {15.6f, 2, 1},
    {17.8f, 3, 0},
};
constexpr int NPLAN = int(sizeof(kPlan) / sizeof(kPlan[0]));

}  // namespace

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "MAGAZINE SPENT";
    if (fail_ == Fail::Gabion) return "OVER THE GABION";
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
        uint16_t sky = y < 36 ? gs::rgb4(3, 2, 5) : y < 70 ? gs::rgb4(8, 5, 4) : gs::rgb4(4, 5, 2);
        if (y >= 70) {
            int band = (y / 10) & 1;
            sky = band ? gs::rgb4(5, 5, 2) : gs::rgb4(4, 4, 2);
        }
        sys.vdp.lineBackdrop[y] = sky;
        sys.vdp.lineFog[y] = y < 80 ? uint8_t(4) : 0;
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
    face_ = 1;
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
    sys_->apu.tone(1, 587.f, 0.1f);
}

Game::Foe* Game::aimed() {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.phase != Phase::Walk || f.face != face_) continue;
        if (f.z < 0.18f || f.z > 0.62f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    return best;
}

void Game::fire() {
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
    sys_->apu.noiseBurst(0.5f, 2100.f, 0.07f);
    sys_->apu.tone(2, 110.f, 0.12f);
    if (Foe* f = aimed()) {
        f->phase = Phase::Down;
        f->age = 0;
        stopped_++;
    }
}

void Game::botAct() {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.kind != Kind::Storm || f.phase != Phase::Walk) continue;
        if (f.z < 0.22f || f.z > 0.55f) continue;
        if (!best || f.z < best->z) best = &f;
    }
    if (!best || rounds_ <= 1) return;
    face_ = best->face;
    fire();
}

void Game::humanAct() {
    gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) face_ = (face_ + FACES - 1) % FACES;
    if (p.pressed(gs::BTN_RIGHT)) face_ = (face_ + 1) % FACES;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_B)) fire();
}

void Game::updateRaid(float dt) {
    if (bot_) botAct();
    else humanAct();
    cool_ = std::max(0.f, cool_ - dt);
    flash_ = std::max(0.f, flash_ - dt);
    shake_ *= 0.86f;

    while (next_ < NPLAN && raidT_ >= kPlan[next_].t) {
        Foe f;
        f.kind = kPlan[next_].storm ? Kind::Storm : Kind::Scout;
        f.face = kPlan[next_].face;
        f.z = 1.08f;
        foes_.push_back(f);
        next_++;
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.age += dt;
        float sp = f.kind == Kind::Storm ? 0.155f : 0.13f;
        if (f.phase == Phase::Walk) {
            f.z -= sp * dt;
            if (f.kind == Kind::Scout && f.z < 0.46f) f.phase = Phase::Leave;
            if (f.kind == Kind::Storm && f.z < 0.08f) {
                lose(Fail::Gabion);
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
    if (raidT_ >= RAID_LEN) {
        if (rounds_ > 0) win();
        else lose(Fail::Spent);
    }
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
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
        beginRaid();
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
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        if (c < 32 || c > 127) c = 32;
        gs::Image img = art_.glyph[c - 32];
        int w = std::max(1, art_.gw[c - 32]);
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

    spr(art_.flag, 160 + sx, 52, 46, PAL_FLAG, false, true);
    spr(art_.chest, 160 + sx, 198, 28, PAL_GAB, false, true);
    for (int i = 0; i < FACES; i++) {
        float x = FACE_X[i] + sx;
        spr(art_.gabion, x - 18, 168, 40, PAL_GAB, false, true);
        spr(art_.gabion, x + 18, 168, 40, PAL_GAB, true, true);
    }

    const gs::Mipped& sentry = art_.you[int(t_ * 3) & 1];
    float aim = FACE_X[face_] + sx;
    spr(sentry, aim, 186, 42, PAL_YOU, face_ >= 2, true);
    spr(art_.chev, aim, 128, 10, PAL_HUD, false, false);
    if (flash_ > 0) spr(art_.puff, aim, 150, 18, PAL_FX, false, false);

    std::vector<int> order;
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].on) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z < foes_[b].z; });
    for (int i : order) {
        const Foe& f = foes_[i];
        float near = std::clamp(1.f - f.z, 0.f, 1.f);
        float cx = FACE_X[f.face] + (f.face < 2 ? -1.f : 1.f) * (1.f - near) * 18.f + sx;
        float cy = 78.f + near * 72.f;
        int pal = f.kind == Kind::Storm ? PAL_STORM : PAL_SCOUT;
        if (f.phase == Phase::Down) {
            spr(art_.down, cx, cy, 14.f + near * 8.f, pal, false, true);
            spr(art_.puff, cx, cy - 10, 12, PAL_FX, false, false);
        } else {
            const gs::Mipped& img = (f.kind == Kind::Storm ? art_.storm : art_.scout)[int(f.age * 6) & 1];
            bool flip = f.phase == Phase::Leave ? f.face < 2 : f.face >= 2;
            spr(img, cx, cy, 14.f + near * 36.f, pal, flip, true);
        }
    }

    text("MAG", 8, 8, 1.f, PAL_HUD);
    for (int i = 0; i < MAG; i++) {
        if (i < rounds_) spr(art_.brass, 42.f + i * 10.f, 18, 14, PAL_BRASS, false, false);
    }
    int secs = std::max(0, int(std::ceil(RAID_LEN - raidT_ - 0.001f)));
    char clock[24];
    std::snprintf(clock, sizeof clock, "HORN %02d", mode_ == Mode::Title ? int(RAID_LEN) : secs);
    text(clock, 214, 8, 1.f, secs <= 5 && mode_ == Mode::Raid ? PAL_ALERT : PAL_HUD);

    if (mode_ == Mode::Title) {
        text("S3 REDOUBT MAGA", 86, 36, 1.f, PAL_HUD);
        text("MAKE THE MAGAZINE LAST", 62, 52, 1.f, PAL_OK);
        text("SCOUTS TURN  STORMERS DO NOT", 48, 68, 1.f, PAL_HUD);
        text("LEFT RIGHT FACE   A FIRE", 64, 208, 1.f, PAL_HUD);
        if (int(t_ * 2) & 1) text("START", 140, 96, 1.f, PAL_BRASS);
    } else if (mode_ == Mode::Pause) {
        text("HELD", 146, 96, 1.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("THE MAGAZINE OUTLASTS THE RAID", 28, 40, 1.f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text(fail_ == Fail::Spent ? "MAGAZINE SPENT" : "OVER THE GABION", 78, 40, 1.f, PAL_ALERT);
    } else {
        text(rounds_ <= 2 ? "LAST CARTRIDGES" : "HOLD THE FACES", 96, 208, 1.f,
             rounds_ <= 2 ? PAL_ALERT : PAL_HUD);
    }
}

}  // namespace rmaga
