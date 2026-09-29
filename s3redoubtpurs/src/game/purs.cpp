#include "game/purs.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace redoubtpurs {
namespace {

struct Spawn {
    int frame;
    int lane;
    int kind;
};

constexpr Spawn kSpawns[] = {
    {30, 1, 0}, {170, 0, 0}, {310, 2, 0}, {460, 1, 1}, {620, 0, 1}, {780, 2, 0},
};

constexpr float kLanes[3] = {78.f, 160.f, 242.f};
constexpr float kKillY = 164.f;

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Title) return 0;
    if (seize_ > 0) return 2;
    return 1;
}

float Game::laneX(int lane) const {
    if (lane < 0) lane = 0;
    if (lane > 2) lane = 2;
    return kLanes[lane];
}

bool Game::anyRunning() const {
    for (const auto& r : rivals_)
        if (r.running) return true;
    return false;
}

void Game::bootRoom() {
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = true;
    sys_->apu.setMaster(0.32f);
    sys_->apu.setEcho(0.06f, 0.12f, 0.06f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    bootRoom();
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
    reason_ = "";
}

void Game::begin() {
    mode_ = Mode::Play;
    playF_ = 0;
    lane_ = 1;
    boiler_ = 8;
    stalled_ = 0;
    spawnIx_ = 0;
    cool_ = 0;
    stokeC_ = 20;
    seize_ = 0;
    hold_ = 0;
    flash_ = 0;
    won_ = false;
    over_ = false;
    score_ = 0;
    reason_ = "";
    rivals_.clear();
    bolts_.clear();
}

void Game::fire() {
    if (cool_ > 0 || mode_ != Mode::Play) return;
    Bolt b;
    b.lane = lane_;
    b.y = 168.f;
    b.live = true;
    bolts_.push_back(b);
    cool_ = 9;
    flash_ = 4;
    sys_->apu.tone(0, 520.f, 0.12f);
    blipT_ = 4;
}

void Game::stoke() {
    if (stokeC_ > 0 || boiler_ >= 8 || mode_ != Mode::Play) return;
    boiler_++;
    stokeC_ = 26;
    sys_->apu.tone(1, 180.f, 0.08f);
    blipT_ = 5;
}

void Game::winYard() {
    if (won_ || boiler_ <= 0) return;
    mode_ = Mode::Victory;
    won_ = true;
    score_ = stalled_ * 100 + boiler_ * 10;
    reason_ = "THE REDOUBT IS THE LAST MACHINE STILL RUNNING";
    sys_->apu.tone(0, 330.f, 0.14f);
    sys_->apu.tone(1, 440.f, 0.1f);
    blipT_ = 18;
}

void Game::loseYard(const char* why) {
    if (mode_ == Mode::Fail || mode_ == Mode::Victory) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    score_ = stalled_ * 100 + std::max(0, boiler_) * 10;
    sys_->apu.noiseBurst(0.4f, 240.f, 0.2f);
}

void Game::stopRival(Rival& r, bool hitWall) {
    if (!r.running) return;
    r.running = false;
    r.hp = 0;
    stalled_++;
    seize_ = 26;
    sys_->apu.noiseBurst(0.28f, hitWall ? 400.f : 1600.f, 0.07f);
    if (hitWall) {
        boiler_ -= 3;
        if (boiler_ <= 0) {
            boiler_ = 0;
            loseYard("A MACHINE REACHED THE PARAPET");
            return;
        }
    }
    if (boiler_ > 0 && spawnIx_ >= kFleet && !anyRunning()) winYard();
}

void Game::spawnDue() {
    while (spawnIx_ < kFleet && playF_ >= kSpawns[spawnIx_].frame) {
        Rival r;
        r.lane = kSpawns[spawnIx_].lane;
        r.kind = kSpawns[spawnIx_].kind;
        r.hp = r.kind ? 3 : 2;
        r.y = 94.f;
        r.running = true;
        rivals_.push_back(r);
        spawnIx_++;
    }
}

void Game::botAct() {
    int best = -1;
    float by = -1.f;
    for (int i = 0; i < int(rivals_.size()); i++) {
        if (!rivals_[i].running) continue;
        if (rivals_[i].y > by) {
            by = rivals_[i].y;
            best = i;
        }
    }
    if (best >= 0) lane_ = rivals_[best].lane;
    if (best >= 0) fire();
    if (boiler_ < 6) stoke();
}

void Game::readPad() {
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT) || p.axisX < -0.4f) lane_--;
    if (p.pressed(gs::BTN_RIGHT) || p.axisX > 0.4f) lane_++;
    if (lane_ < 0) lane_ = 0;
    if (lane_ > 2) lane_ = 2;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_Z)) fire();
    if (p.down(gs::BTN_B) || p.down(gs::BTN_X)) stoke();
}

void Game::update() {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) {
        if (++hold_ > 36) over_ = true;
        return;
    }
    playF_++;
    if (cool_ > 0) cool_--;
    if (stokeC_ > 0) stokeC_--;
    if (seize_ > 0) seize_--;
    if (flash_ > 0) flash_--;
    if (blipT_ > 0 && --blipT_ == 0) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
    }
    if (playF_ % 100 == 0) {
        boiler_--;
        if (boiler_ <= 0) {
            boiler_ = 0;
            loseYard("YOUR MACHINE STALLED");
            return;
        }
    }
    spawnDue();
    if (bot_) botAct();
    else readPad();

    for (auto& b : bolts_) {
        if (!b.live) continue;
        b.y -= 3.4f;
        if (b.y < 78.f) b.live = false;
    }
    for (auto& r : rivals_) {
        if (!r.running) continue;
        r.age++;
        r.y += r.kind ? 0.40f : 0.52f;
        for (auto& b : bolts_) {
            if (!b.live || b.lane != r.lane) continue;
            if (b.y <= r.y + 8.f && b.y >= r.y - 12.f) {
                b.live = false;
                r.hp--;
                sys_->apu.tone(1, 240.f, 0.07f);
                blipT_ = 3;
                if (r.hp <= 0) stopRival(r, false);
                break;
            }
        }
        if (r.running && r.y >= kKillY) stopRival(r, true);
        if (mode_ != Mode::Play) return;
    }
}

void Game::road() {
    gs::VDP& v = sys_->vdp;
    const float horizon = 78.f;
    float tsec = (mode_ == Mode::Title) ? age_ / 60.f : playF_ / 60.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r = 3, g = 4, b = 7;
        if (y > 50) {
            r = 5;
            g = 5;
            b = 6;
        }
        if (y >= int(horizon)) {
            r = 3;
            g = 3;
            b = 2;
        }
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = y < 64 ? 5 : 0;
        gs::RoadLine& rl = v.road[y];
        if (y < int(horizon) || y > 196) {
            rl.on = false;
            continue;
        }
        float t = (float(y) - horizon) / (196.f - horizon);
        t = std::clamp(t, 0.02f, 1.f);
        rl.on = true;
        rl.cx = 160.f;
        rl.hw = 10.f + t * t * 148.f;
        rl.v = tsec * 280.f + 70.f / t;
        rl.pal = PAL_ROAD;
        rl.band = (int(rl.v / 160.f) & 1) ? 1 : 0;
        rl.style = gs::ROAD_RUTS;
        rl.left = gs::GROUND_LAND;
        rl.right = gs::GROUND_LAND;
    }
    v.roadTime = int(tsec * 60.f);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
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

void Game::draw() {
    road();
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.hudEnabled = true;

    spr(art_.you[(playF_ / 6) & 1], laneX(lane_), 186.f, 34.f, PAL_YOU, false, 0);
    if (flash_ > 0) spr(art_.bolt, laneX(lane_), 162.f, 14.f, PAL_BOLT, false, 0);
    for (const auto& b : bolts_) {
        if (!b.live) continue;
        spr(art_.bolt, laneX(b.lane), b.y, 12.f, PAL_BOLT, false, 0);
    }

    for (int i = int(rivals_.size()) - 1; i >= 0; i--) {
        const Rival& r = rivals_[i];
        float depth = std::clamp((r.y - 90.f) / 80.f, 0.f, 1.f);
        float h = 14.f + depth * 22.f;
        int fog = int((1.f - depth) * 8.f);
        if (!r.running) {
            spr(art_.wreck, laneX(r.lane), r.y, h * 0.7f, PAL_WRECK, false, fog);
            continue;
        }
        const gs::Mipped& img = r.kind ? art_.heavy[(r.age / 7) & 1] : art_.light[(r.age / 6) & 1];
        spr(img, laneX(r.lane), r.y, h, r.kind ? PAL_HEAVY : PAL_LIGHT, r.lane == 0, fog);
    }
    for (int s = -2; s <= 2; s++) {
        if (s == 0) continue;
        spr(art_.stake, 160.f + float(s) * 58.f, 108.f, 16.f, PAL_EARTH, false, 6);
    }
    spr(art_.berm, 160.f, 200.f, 48.f, PAL_EARTH, false, 0);

    char line[64];
    if (mode_ == Mode::Title) {
        hudC(20, "YOU HAVE THE REDOUBT.", PAL_HUD);
        hudC(21, "BE THE LAST MACHINE STILL RUNNING.", PAL_OK);
        hudC(23, "LEFT RIGHT THE BERM    A FIRE", PAL_HUD);
        hudC(24, "B STOKE THE BOILER", PAL_HUD);
        hudC(26, "START", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(22, "HOLD", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(21, "LAST MACHINE STILL RUNNING", PAL_OK);
        std::snprintf(line, sizeof(line), "SCORE %d", score_);
        hudC(23, line, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(21, reason_, PAL_HUD);
        hudC(23, "THE WATCH IS OVER", PAL_HUD);
    } else {
        std::snprintf(line, sizeof(line), "BOILER %d", boiler_);
        hud(1, 1, line, boiler_ <= 3 ? PAL_BOLT : PAL_HUD);
        std::snprintf(line, sizeof(line), "STOPPED %d/%d", stalled_, kFleet);
        hud(24, 1, line, PAL_OK);
        if (stokeC_ == 0 && boiler_ < 8) hud(1, 2, "STOKE", PAL_OK);
    }
    int n = int(std::strlen(S3_VERSION_STRING));
    hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    const gs::Pad& pad = sys.pad;
    bool start = !bot_ && (pad.pressed(gs::BTN_START) || (mode_ == Mode::Title && pad.pressed(gs::BTN_A)));
    if (!bot_ && pad.pressed(gs::BTN_MODE) && mode_ == Mode::Title) {
        if (sys.hasHome()) sys.eject();
        return;
    }
    if (mode_ == Mode::Title) {
        if (bot_ && age_ > 24) begin();
        else if (start) begin();
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Play;
        draw();
        return;
    }
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) {
        update();
        if (!bot_ && start) begin();
        draw();
        return;
    }
    if (start) {
        mode_ = Mode::Pause;
        draw();
        return;
    }
    update();
    draw();
}

}  // namespace redoubtpurs
