#include "well.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../version.h"

namespace rwell {
namespace {
constexpr float CREST = 112.f;
constexpr float SPAWN_Y = 208.f;
constexpr float BOLT_V = 6.6f;
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
    sys.vdp.setFogColor(gs::rgb4(3, 4, 3));
    mode_ = Mode::Title;
    age_ = 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    wave_ = 0;
    well_ = 6;
    score_ = 0;
    lane_ = 1;
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
    const int n = wave_ == 0 ? 5 : wave_ == 1 ? 7 : 9;
    const int gap = wave_ == 0 ? 78 : wave_ == 1 ? 68 : 60;
    const int lanes[9] = {0, 2, 1, 3, 2, 0, 3, 1, 2};
    for (int i = 0; i < n; i++) {
        Spawn s;
        s.frame = 16 + i * gap;
        s.lane = lanes[i];
        s.kind = 0;
        s.speed = wave_ == 0 ? 0.72f : wave_ == 1 ? 0.88f : 1.02f;
        if (wave_ == 1 && (i % 3) == 2) {
            s.kind = 1;
            s.speed = 0.62f;
        }
        if (wave_ == 2 && (i % 4) == 3) {
            s.kind = 2;
            s.speed = 1.28f;
        } else if (wave_ == 2 && (i % 3) == 1) {
            s.kind = 1;
            s.speed = 0.74f;
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
    sys_->apu.tone(ch, freq, 0.08f);
}

void Game::shoot() {
    if (cool_ > 0) return;
    cool_ = 9;
    for (Bolt& b : bolts_) {
        if (b.life > 0) continue;
        b.x = LANE_X[lane_];
        b.y = STEP_Y + 10.f;
        b.lane = lane_;
        b.life = 26;
        flash_ = 3;
        blip(0, 640.f);
        return;
    }
}

void Game::act(int& step, bool& fire) {
    step = 0;
    fire = false;
    if (bot_) {
        const Foe* best = nullptr;
        for (const Foe& f : foes_) {
            if (!f.alive) continue;
            if (!best || f.y < best->y) best = &f;
        }
        if (!best) return;
        if (best->lane < lane_) step = -1;
        else if (best->lane > lane_) step = 1;
        fire = best->lane == lane_;
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT) || p.axisX < -0.45f) step = -1;
    if (p.pressed(gs::BTN_RIGHT) || p.axisX > 0.45f) step = 1;
    fire = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO);
}

void Game::playTick() {
    int step = 0;
    bool fire = false;
    act(step, fire);
    if (step < 0 && lane_ > 0) lane_--;
    if (step > 0 && lane_ < LANES - 1) lane_++;
    if (cool_ > 0) cool_--;
    if (fire) shoot();
    if (flash_ > 0) flash_--;
    sys_->apu.tone(0, 0, 0);

    while (spawnIx_ < int(spawns_.size()) && spawns_[spawnIx_].frame <= waveTime_) {
        const Spawn& s = spawns_[spawnIx_++];
        Foe f;
        f.kind = s.kind;
        f.lane = s.lane;
        f.y = SPAWN_Y;
        f.speed = s.speed;
        f.hp = s.kind == 1 ? 2 : 1;
        f.points = s.kind == 0 ? 100 : s.kind == 1 ? 180 : 140;
        foes_.push_back(f);
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age++;
        f.y -= f.speed;
        if (f.y <= CREST) {
            f.alive = false;
            well_--;
            blip(1, 90.f);
            sys_->apu.noiseBurst(0.22f, 1600.f, 0.18f);
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
        b.y += BOLT_V;
        for (Foe& f : foes_) {
            if (!f.alive || b.life <= 0 || f.lane != b.lane) continue;
            if (std::fabs(b.y - f.y) > 16.f) continue;
            b.life = 0;
            f.hp--;
            if (f.hp <= 0) {
                f.alive = false;
                score_ += f.points;
                blip(2, 240.f);
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
    if (mode_ == Mode::Title) {
        bool go = bot_ ? age_ > 16 : sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A);
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
    int water = (age_ / 10) & 1 ? gs::rgb4(3, 8, 12) : gs::rgb4(2, 6, 9);
    v.setColor(PAL_WELL * 16 + 5, water);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineBackdrop[y] = gs::rgb4(2, 3, 4);
        v.road[y].on = false;
        v.lineFog[y] = 0;
    }

    if (mode_ == Mode::Title) {
        text("S3 REDOUBT WELL", 160, 28, 0.72f, PAL_AMBER);
        text("KEEP THE WELL", 160, 48, 0.5f, PAL_HUD);
    } else if (mode_ == Mode::Banner) {
        text("WAVE DOWN", 160, 28, 0.7f, PAL_OK);
    } else if (mode_ == Mode::Won) {
        text("THE WELL STANDS", 160, 24, 0.62f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("THE WELL FALLS", 160, 24, 0.66f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("HOLD", 160, 24, 0.7f, PAL_HUD);
    }

    if (well_ < 4) sprite(art_.crack, WELL_X + 8, WELL_Y + 6, 14.f + (4 - well_) * 2.f, PAL_FX, false, 0);
    sprite(art_.well, WELL_X, WELL_Y, 52, PAL_WELL, false, 0);

    for (const Foe& f : foes_) {
        if (!f.alive) continue;
        float near = std::clamp((SPAWN_Y - f.y) / (SPAWN_Y - CREST), 0.f, 1.f);
        float h = 22.f + near * 16.f;
        int fog = int((1.f - near) * 6.f);
        const gs::Mipped* img = &art_.musket[(f.age / 7) & 1];
        int pal = PAL_FOE;
        if (f.kind == 1) {
            img = &art_.sapper[(f.age / 8) & 1];
            pal = PAL_SAP;
        } else if (f.kind == 2) {
            img = &art_.runner[(f.age / 5) & 1];
            pal = PAL_RUN;
        }
        sprite(*img, LANE_X[f.lane], f.y, h, pal, f.lane >= 2, fog);
    }

    int fr = (age_ / 8) & 1;
    float sx = LANE_X[lane_];
    sprite(art_.sentry[fr], sx, STEP_Y - 4.f, 34, PAL_YOU, lane_ >= 2, 0);
    if (flash_ > 0) sprite(art_.muzzle, sx, STEP_Y + 12.f, 12, PAL_FX, false, 0);
    for (const Bolt& b : bolts_) {
        if (b.life <= 0) continue;
        sprite(art_.shot, b.x, b.y, 6, PAL_FX, false, 0);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(23, "ONE REDOUBT. ONE WELL.", PAL_HUD);
        hudC(24, "THREE WAVES.", PAL_AMBER);
        hudC(25, "LEFT RIGHT STEP   Z FIRE   ENTER", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        std::snprintf(line, sizeof(line), "WELL %d", well_);
        hud(1, 26, line, well_ <= 2 ? PAL_ALERT : PAL_OK);
        std::snprintf(line, sizeof(line), "WAVE %d/3", wave_ + 1);
        hud(14, 26, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "SCORE %d", score_);
        hud(26, 26, line, PAL_AMBER);
        if (mode_ == Mode::Lost) hudC(24, "THE PARADE IS DRY.", PAL_ALERT);
        if (mode_ == Mode::Won) hudC(24, "IT STILL STANDS.", PAL_OK);
    }
}

}  // namespace rwell
