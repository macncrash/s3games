#include "game/maga.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace maga {
namespace {

struct Spawn {
    float t;
    int lane;
    bool real;
};

// Six rifles that will take the deck, and six grey coats that wheel off.
// A full magazine is seven. One round has to be in the gun when the raid ends.
constexpr int kSpawnCount = 12;
constexpr Spawn kScript[kSpawnCount] = {
    {2.0f, 1, false}, {3.5f, 0, true},  {5.2f, 2, false}, {6.8f, 2, true},
    {8.5f, 1, false}, {10.0f, 0, true}, {12.0f, 1, true}, {14.2f, 0, false},
    {15.5f, 2, true}, {18.0f, 1, false}, {19.5f, 0, true}, {22.5f, 2, false},
};

constexpr int kHor = 92;
constexpr float kSpeed = 0.16f;

float projectY(float z) {
    float d = std::clamp(1.f - z, 0.f, 1.f);
    return float(kHor) + std::pow(d, 1.12f) * 118.f;
}

float laneX(int lane, float z) {
    float d = 1.f - z;
    return 160.f + float(lane - 1) * (26.f + d * 78.f);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Raid) return 1;
    return 2;
}

bool Game::anyReal() const {
    for (int i = 0; i < spawned_; i++)
        if (men_[i].alive && men_[i].real && !men_[i].peeled) return true;
    return false;
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::resetRaid() {
    mode_ = Mode::Raid;
    over_ = false;
    won_ = false;
    breached_ = false;
    rounds_ = kMag;
    aim_ = 1;
    flash_ = 0;
    t_ = 0;
    cd_ = 0;
    spawned_ = 0;
    result_ = 0;
    for (auto& m : men_) m = {};
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.tone(0, 0, 0);
    mode_ = Mode::Title;
    titleWait_ = 0;
}

void Game::tryFire() {
    if (cd_ > 0 || rounds_ <= 0 || mode_ != Mode::Raid) return;
    rounds_--;
    cd_ = 0.32f;
    flash_ = 5;
    sys_->apu.noiseBurst(0.45f, 2400.f, 0.06f);
    int best = -1;
    float bestZ = 2.f;
    for (int i = 0; i < spawned_; i++) {
        Raider& m = men_[i];
        if (!m.alive || m.peeled || m.lane != aim_) continue;
        if (m.z < 0.14f || m.z > 0.92f) continue;
        if (m.z < bestZ) {
            bestZ = m.z;
            best = i;
        }
    }
    if (best >= 0) {
        men_[best].alive = false;
        men_[best].hit = 8;
        sys_->apu.tone(1, 180.f, 0.12f);
    }
}

void Game::botAct() {
    int best = -1;
    float bestZ = 2.f;
    for (int i = 0; i < spawned_; i++) {
        const Raider& m = men_[i];
        if (!m.alive || !m.real || m.peeled) continue;
        if (m.z > 0.58f || m.z < 0.18f) continue;
        if (m.z < bestZ) {
            bestZ = m.z;
            best = i;
        }
    }
    if (best < 0) return;
    aim_ = men_[best].lane;
    tryFire();
}

void Game::updateRaid() {
    const float dt = 1.f / 60.f;
    t_ += dt;
    scroll_ += dt * 3.5f;
    if (cd_ > 0) cd_ -= dt;
    if (flash_ > 0) {
        flash_--;
        if (flash_ == 0) sys_->apu.tone(1, 0, 0);
    }

    while (spawned_ < kSpawns && t_ >= kScript[spawned_].t) {
        Raider& m = men_[spawned_];
        m.lane = kScript[spawned_].lane;
        m.real = kScript[spawned_].real;
        m.z = 1.f;
        m.alive = true;
        m.peeled = false;
        spawned_++;
    }

    for (int i = 0; i < spawned_; i++) {
        Raider& m = men_[i];
        if (m.hit > 0) m.hit--;
        if (!m.alive || m.peeled) continue;
        m.z -= kSpeed * dt;
        if (!m.real && m.z <= 0.50f) {
            m.peeled = true;
            sys_->apu.noiseBurst(0.12f, 500.f, 0.08f);
            continue;
        }
        if (m.real && m.z <= 0.10f) {
            m.alive = false;
            breached_ = true;
        }
    }

    if (bot_) botAct();
    else {
        if (sys_->pad.pressed(gs::BTN_LEFT)) aim_ = std::max(0, aim_ - 1);
        if (sys_->pad.pressed(gs::BTN_RIGHT)) aim_ = std::min(2, aim_ + 1);
        if (sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_C)) tryFire();
    }

    if (breached_) {
        result_ = 3;
        mode_ = Mode::Over;
        over_ = true;
        won_ = false;
        sys_->apu.noiseBurst(0.6f, 200.f, 0.4f);
        return;
    }
    if (rounds_ <= 0) {
        result_ = 2;
        mode_ = Mode::Over;
        over_ = true;
        won_ = false;
        sys_->apu.tone(0, 90.f, 0.2f);
        return;
    }
    if (t_ >= kRaid && !anyReal()) {
        result_ = 1;
        mode_ = Mode::Over;
        over_ = true;
        won_ = true;
        sys_->apu.tone(0, 523.f, 0.16f);
        sys_->apu.tone(1, 659.f, 0.14f);
    }
}

void Game::drawRaider(const Raider& r) {
    if (r.peeled && r.hit == 0) return;
    float z = r.z;
    float sway = 0;
    if (!r.real && r.alive) sway = std::sin(t_ * 6.f + r.lane) * (8.f * (1.f - z));
    float x = laneX(r.lane, z) + sway;
    float y = projectY(z);
    float h = 18.f + (1.f - z) * 56.f;
    const gs::Mipped& img = r.real ? art_.real : art_.feint;
    float w = h * float(img.w) / float(img.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(x - w * 0.5f));
    s.y = int16_t(std::lround(y - h));
    s.img = img.pick(h);
    s.pal = uint8_t(r.real ? PAL_REAL : PAL_FEINT);
    s.fog = uint8_t(std::clamp(int(z * 12.f), 0, 12));
    if (!r.alive) s.pal = PAL_FX;
    sys_->vdp.sprite(s);
}

void Game::drawWorld() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.clear();
    vdp.B.clear();

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < kHor) {
            int u = y * 12 / kHor;
            vdp.lineBackdrop[y] = gs::rgb4(2 + u / 5, 3 + u / 4, 8 + u / 3);
            vdp.lineFog[y] = 0;
            vdp.road[y].on = false;
        } else {
            float d = float(y - kHor) / float(gs::SCREEN_H - kHor);
            float dist = 6.5f / (d + 0.07f);
            vdp.lineBackdrop[y] = gs::rgb4(1, 2, 4);
            vdp.lineFog[y] = uint8_t((1.f - d) * 9.f);
            gs::RoadLine& r = vdp.road[y];
            r.on = true;
            r.cx = 160.f;
            r.hw = 14.f + d * 132.f;
            r.v = scroll_ + dist * 0.22f;
            r.pal = PAL_FIELD;
            r.band = (int(dist * 0.5f) & 1);
            r.style = gs::ROAD_ROCKY;
            r.left = gs::GROUND_DROP;
            r.right = gs::GROUND_DROP;
        }
    }

    gs::Sprite gun;
    gun.w = 120;
    gun.h = 48;
    gun.x = 100;
    gun.y = 176;
    gun.img = art_.gun.pick(48);
    gun.pal = PAL_GUN;
    vdp.sprite(gun);
    if (flash_ > 0) {
        gs::Sprite f;
        f.w = 28;
        f.h = 18;
        f.x = 196;
        f.y = 168;
        f.img = art_.flash.pick(18);
        f.pal = PAL_FX;
        vdp.sprite(f);
    }

    if (mode_ != Mode::Title) {
        float cx = laneX(aim_, 0.22f);
        float cy = projectY(0.22f);
        gs::Sprite mark;
        mark.w = 22;
        mark.h = 14;
        mark.x = int16_t(std::lround(cx - 11));
        mark.y = int16_t(std::lround(cy - 6));
        mark.img = art_.chev.pick(14);
        mark.pal = PAL_FX;
        vdp.sprite(mark);

        int order[12];
        int n = spawned_;
        for (int i = 0; i < n; i++) order[i] = i;
        std::sort(order, order + n, [&](int a, int b) { return men_[a].z < men_[b].z; });
        for (int i = 0; i < n; i++) drawRaider(men_[order[i]]);
    }

    for (int i = 1; i <= 5; i++) {
        float z = std::fmod(scroll_ * 0.08f + i * 0.18f, 0.9f);
        if (z < 0.05f) continue;
        float y = projectY(z);
        float h = 28.f + (1.f - z) * 90.f;
        float w = h * float(art_.arch.w) / float(art_.arch.h);
        float half = 20.f + (1.f - z) * 150.f;
        for (int side = -1; side <= 1; side += 2) {
            gs::Sprite s;
            s.w = int16_t(std::lround(w));
            s.h = int16_t(std::lround(h));
            s.x = int16_t(std::lround(160.f + side * half - w * 0.5f));
            s.y = int16_t(std::lround(y - h * 0.92f));
            s.img = art_.arch.pick(h);
            s.pal = PAL_STONE;
            s.fog = uint8_t(std::clamp(int(z * 14.f), 0, 14));
            s.hflip = side < 0;
            vdp.sprite(s);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        titleWait_++;
        scroll_ += 1.f / 60.f;
        bool go = sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || (bot_ && titleWait_ > 24);
        if (go) resetRaid();
    } else if (mode_ == Mode::Raid) {
        updateRaid();
    }

    drawWorld();

    if (mode_ == Mode::Title) {
        hudC(3, "VIADUCT MAGA", PAL_HUD);
        hudC(6, "ONE MAGAZINE. ONE RAID.", PAL_HUD);
        hudC(9, "RED RIFLES TAKE THE DECK.", PAL_HUD);
        hudC(10, "GREY COATS PEEL AWAY.", PAL_HUD);
        hudC(12, "DO NOT EMPTY THE GUN", PAL_HUD);
        hudC(13, "BEFORE THE RAID IS OVER.", PAL_HUD);
        hudC(16, "ARROWS AIM    A FIRES", PAL_HUD);
        if ((titleWait_ / 30) % 2 == 0) hudC(20, "START", PAL_HUD);
    } else if (mode_ == Mode::Raid) {
        char buf[40];
        std::snprintf(buf, sizeof(buf), "MAG %d", rounds_);
        hud(1, 1, buf, rounds_ <= 2 ? PAL_REAL : PAL_HUD);
        int left = std::max(0, int(std::ceil(kRaid - t_)));
        std::snprintf(buf, sizeof(buf), "RAID %02d", left);
        hud(30, 1, buf, PAL_HUD);
        const char* lanes = aim_ == 0 ? "LANE  LEFT" : aim_ == 1 ? "LANE  MID" : "LANE  RIGHT";
        hud(1, 26, lanes, PAL_HUD);
    } else {
        if (result_ == 1) {
            hudC(8, "MAGAZINE HELD", PAL_HUD);
            char buf[40];
            std::snprintf(buf, sizeof(buf), "%d ROUNDS STILL IN THE GUN", rounds_);
            hudC(11, buf, PAL_HUD);
            hudC(14, "THE RAID IS OVER.", PAL_HUD);
        } else if (result_ == 2) {
            hudC(8, "MAGAZINE SPENT", PAL_HUD);
            hudC(11, "THE RAID OUTLASTED THE GUN.", PAL_HUD);
        } else {
            hudC(8, "THE DECK FELL", PAL_HUD);
            hudC(11, "A RIFLE CROSSED THE VIADUCT.", PAL_HUD);
        }
    }
}

}  // namespace maga
