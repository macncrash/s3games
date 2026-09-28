#include "well.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../version.h"

namespace bwell {
namespace {
constexpr float SPAWN_R = 150.f;
constexpr float HIT_R = 22.f;
constexpr float WELL_R = 34.f;
constexpr float TURN = 0.085f;

float wrap(float a) {
    while (a > 3.14159265f) a -= 6.2831853f;
    while (a < -3.14159265f) a += 6.2831853f;
    return a;
}
}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Banner) return 2;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 3;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(2, 2, 1));
    mode_ = Mode::Title;
    age_ = 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    wave_ = 0;
    well_ = 8;
    score_ = 0;
    aim_ = -1.15f;
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
    const int gap = wave_ == 0 ? 78 : wave_ == 1 ? 64 : 54;
    const int n = wave_ == 0 ? 6 : wave_ == 1 ? 8 : 10;
    for (int i = 0; i < n; i++) {
        Spawn s;
        s.frame = 24 + i * gap;
        s.kind = 0;
        if (wave_ == 1 && (i % 3) == 2) s.kind = 1;
        if (wave_ == 2 && (i % 4) == 3) s.kind = 2;
        else if (wave_ == 2 && (i % 2) == 0) s.kind = 1;
        rng_ = rng_ * 1664525u + 1013904223u;
        float side = (i % 2 == 0) ? -1.f : 1.f;
        s.ang = side * (0.35f + float(rng_ % 90) / 90.f * 1.15f) - 1.15f;
        s.speed = (wave_ == 0 ? 0.42f : wave_ == 1 ? 0.55f : 0.68f) + (s.kind == 2 ? 0.16f : 0.f);
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
    sys_->apu.tone(ch, freq, 0.08f);
}

void Game::shoot() {
    if (cool_ > 0) return;
    cool_ = 8;
    for (Bolt& b : bolts_) {
        if (b.life > 0) continue;
        float c = std::cos(aim_), s = std::sin(aim_);
        b.x = WELL_X + c * 50.f;
        b.y = WELL_Y + s * 50.f;
        b.vx = c * 5.4f;
        b.vy = s * 5.4f;
        b.life = 28;
        flash_ = 3;
        blip(0, 680.f);
        return;
    }
}

void Game::act(float& turn, bool& fire) {
    turn = 0;
    fire = false;
    if (bot_) {
        const Foe* best = nullptr;
        float bestR = 1e9f;
        for (const Foe& f : foes_) {
            if (!f.alive) continue;
            if (f.r < bestR) {
                bestR = f.r;
                best = &f;
            }
        }
        if (!best) return;
        float d = wrap(best->ang - aim_);
        if (d > 0.04f) turn = 1;
        else if (d < -0.04f) turn = -1;
        fire = std::fabs(d) < 0.11f;
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT) || p.axisX < -0.3f) turn -= 1;
    if (p.down(gs::BTN_RIGHT) || p.axisX > 0.3f) turn += 1;
    if (p.down(gs::BTN_UP)) turn -= 1;
    if (p.down(gs::BTN_DOWN)) turn += 1;
    fire = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO);
}

void Game::playTick() {
    float turn = 0;
    bool fire = false;
    act(turn, fire);
    aim_ = wrap(aim_ + turn * TURN);
    if (cool_ > 0) cool_--;
    if (fire) shoot();
    if (flash_ > 0) flash_--;
    sys_->apu.tone(0, 0, 0);

    while (spawnIx_ < int(spawns_.size()) && spawns_[spawnIx_].frame <= waveTime_) {
        const Spawn& s = spawns_[spawnIx_++];
        Foe f;
        f.kind = s.kind;
        f.ang = s.ang;
        f.r = SPAWN_R;
        f.speed = s.speed;
        f.hp = s.kind == 2 ? 2 : 1;
        f.points = s.kind == 0 ? 100 : s.kind == 1 ? 150 : 250;
        foes_.push_back(f);
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age++;
        f.r -= f.speed;
        if (f.r <= WELL_R) {
            f.alive = false;
            well_--;
            blip(1, 90.f);
            sys_->apu.noiseBurst(0.25f, 1800.f, 0.2f);
            if (well_ <= 0) {
                well_ = 0;
                mode_ = Mode::Lost;
                over_ = true;
                won_ = false;
                return;
            }
        }
    }

    for (Bolt& b : bolts_) {
        if (b.life <= 0) continue;
        b.life--;
        b.x += b.vx;
        b.y += b.vy;
        for (Foe& f : foes_) {
            if (!f.alive || b.life <= 0) continue;
            float x = WELL_X + std::cos(f.ang) * f.r;
            float y = WELL_Y + std::sin(f.ang) * f.r;
            float dx = b.x - x, dy = b.y - y;
            if (dx * dx + dy * dy > HIT_R * HIT_R) continue;
            b.life = 0;
            f.hp--;
            if (f.hp <= 0) {
                f.alive = false;
                score_ += f.points;
                blip(2, 220.f);
            }
        }
    }

    if (spawnIx_ >= int(spawns_.size()) && !anyAlive()) {
        calm_++;
        if (calm_ > 36) {
            if (wave_ >= 2) {
                mode_ = Mode::Won;
                won_ = true;
                over_ = true;
                score_ += 400 + well_ * 50;
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
    if (mode_ == Mode::Title) {
        bool go = bot_ ? age_ > 18 : sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A);
        if (go) begin();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (sys_->pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        return;
    }
    if (mode_ == Mode::Banner) {
        banner_++;
        if (banner_ > 50) {
            loadWave();
            mode_ = Mode::Play;
        }
        return;
    }
    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        wonAge_++;
        if (mode_ == Mode::Won) {
            if (wonAge_ == 1) blip(0, 392.f);
            else if (wonAge_ == 10) blip(0, 523.f);
            else if (wonAge_ == 20) blip(0, 659.f);
        }
        return;
    }
    if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        return;
    }
    playTick();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    input();
    draw();
}

void Game::sprite(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
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
        sprite(g, left + float(i) * adv + adv * 0.5f, y, g.h * scale, pal, false, 0);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    int water = (age_ / 12) & 1 ? gs::rgb4(3, 7, 10) : gs::rgb4(2, 5, 8);
    v.setColor(PAL_WELL * 16 + 5, water);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineBackdrop[y] = gs::rgb4(1, 1, 2);
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        text("S3 BUNKER WELL", 160, 48, 0.85f, PAL_AMBER);
        text("KEEP THE WELL", 160, 70, 0.62f, PAL_HUD);
    } else if (mode_ == Mode::Banner) {
        text("WAVE DOWN", 160, 40, 0.8f, PAL_OK);
    } else if (mode_ == Mode::Won) {
        text("THE WELL STANDS", 160, 36, 0.72f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("THE WELL FALLS", 160, 36, 0.78f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("HOLD", 160, 36, 0.8f, PAL_HUD);
    }

    float gx = WELL_X + std::cos(aim_) * 46.f;
    float gy = WELL_Y + std::sin(aim_) * 46.f;
    if (flash_ > 0) sprite(art_.flash, gx + std::cos(aim_) * 16.f, gy + std::sin(aim_) * 16.f, 12, PAL_FX, false, 0);
    for (const Bolt& b : bolts_) {
        if (b.life <= 0) continue;
        sprite(art_.shot, b.x, b.y, 5, PAL_FX, b.vx < 0, 0);
    }
    int fr = (age_ / 8) & 1;
    sprite(art_.gunner[fr], gx, gy - 6, 36, PAL_YOU, std::cos(aim_) < 0, 0);

    for (const Foe& f : foes_) {
        if (!f.alive) continue;
        float x = WELL_X + std::cos(f.ang) * f.r;
        float y = WELL_Y + std::sin(f.ang) * f.r;
        float near = std::clamp((SPAWN_R - f.r) / (SPAWN_R - WELL_R), 0.f, 1.f);
        float h = 18.f + near * (f.kind == 2 ? 28.f : 20.f);
        int fog = int((1.f - near) * 8.f);
        const gs::Mipped* img = &art_.raider[(f.age / 8) & 1];
        int pal = PAL_FOE;
        if (f.kind == 1) {
            img = &art_.sapper[(f.age / 8) & 1];
            pal = PAL_SAP;
        } else if (f.kind == 2) {
            img = &art_.rammer[(f.age / 8) & 1];
            pal = PAL_RAM;
        }
        sprite(*img, x, y, h, pal, std::cos(f.ang) < 0, fog);
    }

    if (well_ < 5) sprite(art_.crack, WELL_X + 10, WELL_Y + 8, 16.f + (5 - well_) * 3.f, PAL_FX, false, 0);
    sprite(art_.well, WELL_X, WELL_Y + 4, 72, PAL_WELL, false, 0);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(24, "ONE BUNKER. ONE WELL.", PAL_HUD);
        hudC(25, "THREE WAVES.", PAL_AMBER);
        hudC(26, "LEFT RIGHT ORBIT   Z FIRE   ENTER", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        std::snprintf(line, sizeof(line), "WELL %d", well_);
        hud(1, 26, line, well_ <= 3 ? PAL_ALERT : PAL_OK);
        std::snprintf(line, sizeof(line), "WAVE %d/3", wave_ + 1);
        hud(14, 26, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "SCORE %d", score_);
        hud(26, 26, line, PAL_AMBER);
        if (mode_ == Mode::Lost) hudC(24, "THE WATER IS GONE.", PAL_ALERT);
        if (mode_ == Mode::Won) hudC(24, "IT STILL STANDS.", PAL_OK);
    }
}

}  // namespace bwell
