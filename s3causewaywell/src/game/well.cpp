#include "well.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../version.h"

namespace cwell {
namespace {
constexpr int HORIZON = 76;
constexpr float HIT_Z = 0.10f;
constexpr float SHOT_Z = 0.16f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Banner) return 2;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 4;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    age_ = 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    wave_ = 0;
    well_ = 6;
    score_ = 0;
    lane_ = 0;
    over_ = false;
    won_ = false;
    loadWave();
}

void Game::loadWave() {
    spawns_.clear();
    foes_.clear();
    spawnIx_ = 0;
    waveTime_ = 0;
    calm_ = 0;
    cool_ = 0;
    for (Shot& s : shots_) s.life = 0;
    const int n = wave_ == 0 ? 6 : wave_ == 1 ? 8 : 9;
    const int gap = wave_ == 0 ? 72 : wave_ == 1 ? 62 : 56;
    const float speed = wave_ == 0 ? 0.0056f : wave_ == 1 ? 0.0066f : 0.0074f;
    const int lanes[3] = {-1, 1, 0};
    for (int i = 0; i < n; i++) {
        Spawn s;
        s.frame = 30 + i * gap;
        s.lane = lanes[i % 3];
        s.kind = 0;
        s.speed = speed;
        if (wave_ >= 1 && (i % 4) == 3) {
            s.kind = 1;
            s.speed = speed * 0.82f;
        }
        spawns_.push_back(s);
    }
}

bool Game::anyAlive() const {
    for (const Foe& f : foes_)
        if (f.alive) return true;
    return false;
}

void Game::blip(int ch, float freq) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, 0.07f);
}

void Game::shoot() {
    if (cool_ > 0) return;
    cool_ = 9;
    for (Shot& s : shots_) {
        if (s.life > 0) continue;
        s.lane = lane_;
        s.z = SHOT_Z;
        s.life = 36;
        flash_ = 3;
        blip(0, 740.f);
        return;
    }
}

void Game::act(int& lane, bool& fire) {
    lane = lane_;
    fire = false;
    if (bot_) {
        const Foe* best = nullptr;
        for (const Foe& f : foes_) {
            if (!f.alive) continue;
            if (!best || f.z < best->z) best = &f;
        }
        if (!best) return;
        lane = best->lane;
        fire = true;
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT) || p.axisX < -0.4f) lane = std::max(-1, lane_ - 1);
    if (p.pressed(gs::BTN_RIGHT) || p.axisX > 0.4f) lane = std::min(1, lane_ + 1);
    fire = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO);
}

void Game::playTick() {
    int want = lane_;
    bool fire = false;
    act(want, fire);
    lane_ = want;
    if (cool_ > 0) cool_--;
    if (fire) shoot();
    else sys_->apu.tone(0, 0, 0);
    if (flash_ > 0) flash_--;

    while (spawnIx_ < int(spawns_.size()) && spawns_[spawnIx_].frame <= waveTime_) {
        const Spawn& s = spawns_[spawnIx_++];
        Foe f;
        f.kind = s.kind;
        f.lane = s.lane;
        f.z = 1.22f;
        f.speed = s.speed;
        f.hp = s.kind == 1 ? 2 : 1;
        f.points = s.kind == 1 ? 180 : 100;
        foes_.push_back(f);
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age++;
        f.z -= f.speed;
        if (f.z <= HIT_Z) {
            f.alive = false;
            well_--;
            blip(1, 110.f);
            sys_->apu.noiseBurst(0.22f, 900.f, 0.18f);
            if (well_ <= 0) {
                well_ = 0;
                mode_ = Mode::Lost;
                over_ = true;
                won_ = false;
                return;
            }
        }
    }

    for (Shot& s : shots_) {
        if (s.life <= 0) continue;
        s.life--;
        s.z += 0.048f;
        for (Foe& f : foes_) {
            if (!f.alive || s.life <= 0) continue;
            if (f.lane != s.lane) continue;
            if (std::fabs(f.z - s.z) > 0.07f) continue;
            s.life = 0;
            f.hp--;
            if (f.hp <= 0) {
                f.alive = false;
                score_ += f.points;
                blip(2, 240.f);
            } else {
                blip(2, 180.f);
            }
        }
    }

    if (spawnIx_ >= int(spawns_.size()) && !anyAlive()) {
        calm_++;
        if (calm_ > 28) {
            if (wave_ >= 2) {
                mode_ = Mode::Won;
                won_ = true;
                over_ = true;
                score_ += 500 + well_ * 40;
                wonAge_ = 0;
                return;
            }
            wave_++;
            mode_ = Mode::Banner;
            banner_ = 0;
        }
    }
    waveTime_++;
}

void Game::input() {
    age_++;
    sway_ += 0.02f;
    if (mode_ == Mode::Title) {
        bool go = bot_ ? age_ > 20 : sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A);
        if (go) begin();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (sys_->pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        return;
    }
    if (mode_ == Mode::Banner) {
        banner_++;
        if (banner_ > 46) {
            loadWave();
            mode_ = Mode::Play;
        }
        return;
    }
    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        wonAge_++;
        if (mode_ == Mode::Won) {
            if (wonAge_ == 1) blip(0, 392.f);
            else if (wonAge_ == 12) blip(0, 494.f);
            else if (wonAge_ == 24) blip(0, 587.f);
            else if (wonAge_ > 24) sys_->apu.tone(0, 0, 0);
        }
        return;
    }
    if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        return;
    }
    playTick();
}

void Game::place(int lane, float z, float& x, float& y, float& depth) const {
    float t = clampf((1.22f - z) / 1.12f, 0.f, 1.f);
    depth = t * t;
    float sway = std::sin(sway_) * 10.f * (1.f - depth);
    x = 160.f + sway + float(lane) * (16.f + depth * 78.f);
    y = float(HORIZON) + 8.f + depth * 118.f;
}

void Game::road() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = age_;
    float sway = std::sin(sway_) * 12.f;
    int tide = (age_ / 10) & 1;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float sky = float(y) / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(2 + int(sky * 4), 3 + int(sky * 3), 8 - int(sky * 2));
            v.lineFog[y] = uint8_t(7 - int(sky * 5));
            v.road[y].on = false;
            continue;
        }
        float depth = float(y - HORIZON) / float(gs::SCREEN_H - HORIZON);
        gs::RoadLine& rl = v.road[y];
        rl = {};
        rl.on = true;
        rl.cx = 160.f + sway * (1.f - depth);
        rl.hw = 10.f + depth * depth * 150.f;
        rl.v = 2400.f / (depth + 0.14f) + float(age_) * 0.4f;
        rl.pal = 12;
        rl.band = (int(std::floor(rl.v / 80.f)) & 1) ? 1 : 0;
        rl.style = gs::ROAD_ROCKY;
        rl.left = gs::GROUND_WATER;
        rl.right = gs::GROUND_WATER;
        v.lineBackdrop[y] = tide ? gs::rgb4(1, 3, 6) : gs::rgb4(1, 4, 7);
        v.lineFog[y] = uint8_t(std::clamp(int((1.f - depth) * 9.f), 0, 9));
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h));
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
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    int n = int(std::strlen(s));
    float adv = 18.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, g.h * scale, pal, false, 0);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    road();

    if (mode_ == Mode::Title) text("S3 CAUSEWAY WELL", 160, 36, 0.72f, PAL_HUD);
    else if (mode_ == Mode::Banner) text("WAVE DOWN", 160, 34, 0.8f, PAL_OK);
    else if (mode_ == Mode::Won) text("THE WELL STANDS", 160, 32, 0.68f, PAL_OK);
    else if (mode_ == Mode::Lost) text("THE WELL FALLS", 160, 32, 0.72f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("HOLD", 160, 32, 0.8f, PAL_HUD);

    for (int i = 0; i < 4; i++) {
        float near = 0.18f + float(i) * 0.2f;
        float h = 8.f + near * 20.f;
        float y = 86.f + near * 100.f;
        float spread = 36.f + near * 118.f;
        spr(art_.post, 160.f - spread, y, h, PAL_POST, false, int((1.f - near) * 8));
        spr(art_.post, 160.f + spread, y, h, PAL_POST, false, int((1.f - near) * 8));
    }
    float bob = std::sin(sway_ * 3.f) * 2.f;
    spr(art_.skiff, 52.f, 128.f + bob, 16.f, PAL_SKIFF, false, 2);
    spr(art_.skiff, 270.f, 146.f - bob, 18.f, PAL_SKIFF, true, 1);

    std::vector<int> order;
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].alive) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z > foes_[b].z; });
    for (int id : order) {
        const Foe& f = foes_[id];
        float x, y, depth;
        place(f.lane, f.z, x, y, depth);
        float h = (8.f + depth * (f.kind ? 34.f : 28.f));
        int fog = int((1.f - depth) * 10.f);
        if (f.kind == 1) spr(art_.hauler, x, y, h, PAL_HAUL, f.lane > 0, fog);
        else spr(art_.raider[(f.age / 8) & 1], x, y, h, PAL_FOE, f.lane < 0, fog);
    }

    for (const Shot& s : shots_) {
        if (s.life <= 0) continue;
        float x, y, depth;
        place(s.lane, s.z, x, y, depth);
        spr(art_.shot, x, y - 8.f, 6.f + depth * 4.f, PAL_FX, false, 0);
    }

    float px, py, pd;
    place(lane_, 0.08f, px, py, pd);
    int fr = (age_ / 8) & 1;
    if (flash_ > 0) spr(art_.flash, px + float(lane_) * 4.f, py - 28.f, 12.f, PAL_FX, false, 0);
    spr(art_.keeper[fr], px, py - 6.f, 48.f, PAL_YOU, lane_ < 0, 0);

    float wx, wy, wd;
    place(0, 0.02f, wx, wy, wd);
    if (well_ < 4) spr(art_.crack, wx + 8.f, wy - 10.f, 14.f + float(4 - well_) * 3.f, PAL_ALERT, false, 0);
    spr(art_.well, wx, wy + 6.f, 58.f, PAL_WELL, false, 0);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(22, "ONE CAUSEWAY. ONE WELL.", PAL_HUD);
        hudC(23, "THREE WAVES.", PAL_HUD);
        hudC(24, "KEEP THE WELL STANDING.", PAL_OK);
        hudC(26, "LEFT RIGHT   Z FIRE   ENTER", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        std::snprintf(line, sizeof(line), "WELL %d", well_);
        hud(1, 26, line, well_ <= 2 ? PAL_ALERT : PAL_OK);
        std::snprintf(line, sizeof(line), "WAVE %d/3", std::min(wave_ + 1, 3));
        hud(14, 26, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "SCORE %d", score_);
        hud(26, 26, line, PAL_HUD);
        if (mode_ == Mode::Lost) hudC(24, "THE STONE IS DOWN.", PAL_ALERT);
        if (mode_ == Mode::Won) hudC(24, "IT STILL STANDS.", PAL_OK);
        if (mode_ == Mode::Banner) hudC(24, "THE NEXT WAVE IS ON THE ROAD.", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    input();
    draw();
}

}  // namespace cwell
