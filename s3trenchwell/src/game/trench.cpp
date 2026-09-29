#include "trench.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../version.h"

namespace trench {

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Banner) return 2;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Play) return 1;
    return 3;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(5, 5, 4));
    sys.apu.setMaster(0.35f);
    mode_ = Mode::Title;
    age_ = 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    wave_ = 0;
    well_ = 8;
    score_ = 0;
    px_ = 96.f;
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
    const int gap = wave_ == 0 ? 72 : wave_ == 1 ? 58 : 50;
    const int n = wave_ == 0 ? 6 : wave_ == 1 ? 8 : 9;
    for (int i = 0; i < n; i++) {
        Spawn s;
        s.frame = 20 + i * gap;
        s.kind = 0;
        if (wave_ == 1 && (i % 3) == 2) s.kind = 1;
        if (wave_ == 2 && (i % 4) == 3) s.kind = 2;
        else if (wave_ == 2 && (i % 2) == 0) s.kind = 1;
        rng_ = rng_ * 1664525u + 1013904223u;
        float side = (i % 2 == 0) ? -1.f : 1.f;
        s.x = 160.f + side * (40.f + float(rng_ % 90));
        s.speed = (wave_ == 0 ? 0.72f : wave_ == 1 ? 0.88f : 1.02f) + (s.kind == 2 ? 0.28f : 0.f);
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
    sys_->apu.tone(ch, freq, 0.06f);
    toneLeft_ = 5;
}

void Game::shoot() {
    if (cool_ > 0) return;
    cool_ = 7;
    for (Bolt& b : bolts_) {
        if (b.life > 0) continue;
        b.x = px_ + 10.f;
        b.y = PARA - 22.f;
        b.life = 36;
        flash_ = 3;
        blip(0, 740.f);
        return;
    }
}

void Game::act(float& move, bool& fire) {
    move = 0;
    fire = false;
    const Foe* best = nullptr;
    float bestY = -1.f;
    for (const Foe& f : foes_) {
        if (!f.alive) continue;
        if (f.y > bestY) {
            bestY = f.y;
            best = &f;
        }
    }
    if (!best) return;
    float aimX = best->x;
    float d = aimX - (px_ + 10.f);
    if (d > 6.f) move = 1;
    else if (d < -6.f) move = -1;
    fire = std::fabs(d) < 14.f && best->y > 50.f;
}

void Game::playTick() {
    float move = 0;
    bool fire = false;
    if (bot_) act(move, fire);
    else {
        if (sys_->pad.down(gs::BTN_LEFT)) move = -1;
        if (sys_->pad.down(gs::BTN_RIGHT)) move = 1;
        fire = sys_->pad.down(gs::BTN_Z) || sys_->pad.down(gs::BTN_C) || sys_->pad.down(gs::BTN_A);
    }
    px_ = std::clamp(px_ + move * 2.4f, 16.f, 292.f);
    if (cool_ > 0) cool_--;
    if (flash_ > 0) flash_--;
    if (fire) shoot();

    waveTime_++;
    while (spawnIx_ < int(spawns_.size()) && spawns_[spawnIx_].frame <= waveTime_) {
        const Spawn& s = spawns_[spawnIx_++];
        Foe f;
        f.kind = s.kind;
        f.hp = s.kind == 2 ? 3 : s.kind == 1 ? 2 : 1;
        f.points = s.kind == 2 ? 250 : s.kind == 1 ? 160 : 100;
        f.x = std::clamp(s.x, 20.f, 300.f);
        f.y = 42.f;
        f.speed = s.speed;
        foes_.push_back(f);
    }

    for (Bolt& b : bolts_) {
        if (b.life <= 0) continue;
        b.y -= 6.2f;
        b.life--;
        if (b.y < 20.f) b.life = 0;
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age++;
        f.y += f.speed;
        f.x += (WELL_X - f.x) * 0.008f;
        for (Bolt& b : bolts_) {
            if (b.life <= 0) continue;
            if (std::fabs(b.x - f.x) < 12.f && std::fabs(b.y - (f.y - 8.f)) < 14.f) {
                b.life = 0;
                f.hp--;
                blip(1, 220.f);
                if (f.hp <= 0) {
                    f.alive = false;
                    score_ += f.points;
                    blip(2, 180.f);
                }
            }
        }
        if (f.alive && f.y >= PARA - 4.f) {
            f.alive = false;
            int hit = f.kind == 2 ? 2 : 1;
            well_ = std::max(0, well_ - hit);
            blip(1, 90.f);
            if (well_ <= 0) {
                mode_ = Mode::Lost;
                over_ = true;
                won_ = false;
                return;
            }
        }
    }

    if (spawnIx_ >= int(spawns_.size()) && !anyAlive()) {
        calm_++;
        if (calm_ > 30) {
            if (wave_ >= 2) {
                mode_ = Mode::Won;
                won_ = true;
                over_ = true;
                score_ += well_ * 200;
            } else {
                wave_++;
                banner_ = 0;
                mode_ = Mode::Banner;
            }
        }
    }
}

void Game::input() {
    age_++;
    if (toneLeft_ > 0 && --toneLeft_ == 0 && sys_) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
    if (mode_ == Mode::Title) {
        if (bot_ || sys_->pad.pressed(gs::BTN_START)) begin();
        return;
    }
    if (mode_ == Mode::Banner) {
        banner_++;
        if (banner_ > 45) {
            loadWave();
            mode_ = Mode::Play;
        }
        return;
    }
    if (mode_ == Mode::Won || mode_ == Mode::Lost) return;
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
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int fog = 0;
        if (y < 36) fog = (36 - y) / 6;
        v.lineBackdrop[y] = gs::rgb4(3 + (y < 40 ? 2 : 0), 4, 5);
        v.lineFog[y] = uint8_t(std::min(fog, 8));
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        text("S3 TRENCH WELL", 160, 52, 0.72f, PAL_AMBER);
        text("HOLD THE LINE", 160, 76, 0.55f, PAL_HUD);
    } else if (mode_ == Mode::Banner) {
        text("WAVE DOWN", 160, 48, 0.75f, PAL_OK);
    } else if (mode_ == Mode::Won) {
        text("THE WELL STANDS", 160, 44, 0.62f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("THE WELL FALLS", 160, 44, 0.68f, PAL_ALERT);
    }

    float scale = mode_ == Mode::Title ? 0.85f : 1.f;
    sprite(art_.well, WELL_X, WELL_Y, 58.f * scale, PAL_WELL, false, 0);
    if (well_ < 5 && mode_ != Mode::Title)
        sprite(art_.crack, WELL_X + 6, WELL_Y + 6, 14.f + (5 - well_) * 2.f, PAL_FX, false, 0);

    if (mode_ != Mode::Title) {
        for (const Foe& f : foes_) {
            if (!f.alive) continue;
            float near = std::clamp((f.y - 42.f) / 120.f, 0.f, 1.f);
            float h = 14.f + near * (f.kind == 2 ? 26.f : 18.f);
            int fog = int((1.f - near) * 9.f);
            const gs::Mipped* img = &art_.raider[(f.age / 7) & 1];
            int pal = PAL_FOE;
            if (f.kind == 1) {
                img = &art_.sapper[(f.age / 7) & 1];
                pal = PAL_SAP;
            } else if (f.kind == 2) {
                img = &art_.shell[(f.age / 6) & 1];
                pal = PAL_SHELL;
            }
            sprite(*img, f.x, f.y, h, pal, f.x > WELL_X, fog);
        }
        if (flash_ > 0) sprite(art_.flash, px_ + 16.f, PARA - 28.f, 10, PAL_FX, false, 0);
        for (const Bolt& b : bolts_) {
            if (b.life <= 0) continue;
            sprite(art_.shot, b.x, b.y, 7, PAL_FX, false, 0);
        }
        int fr = (age_ / 8) & 1;
        sprite(art_.rifle[fr], px_, PARA - 8.f, 36, PAL_YOU, false, 0);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(23, "THE TRENCH IS YOURS.", PAL_HUD);
        hudC(24, "KEEP THE WELL THROUGH THREE WAVES.", PAL_AMBER);
        hudC(26, "LEFT RIGHT   Z FIRE   ENTER", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        std::snprintf(line, sizeof(line), "WELL %d", well_);
        hud(1, 26, line, well_ <= 3 ? PAL_ALERT : PAL_OK);
        std::snprintf(line, sizeof(line), "WAVE %d/3", wave_ + 1);
        hud(14, 26, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "SCORE %d", score_);
        hud(26, 26, line, PAL_AMBER);
        if (mode_ == Mode::Lost) hudC(24, "THE TRENCH IS LOST.", PAL_ALERT);
        if (mode_ == Mode::Won) hudC(24, "IT STILL STANDS.", PAL_OK);
    }
}

}  // namespace trench
