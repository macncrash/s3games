#include "beacon.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../version.h"

namespace bwell {
namespace {
constexpr int LANES = 3;
constexpr float LANE_X[LANES] = {78.f, 160.f, 242.f};
constexpr float RAIL_Y = 58.f;
constexpr float WELL_Y = 96.f;
constexpr float RIM = 118.f;
constexpr float SPAWN_Y = 210.f;
constexpr float SPARK_V = 6.2f;
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
    sys.vdp.setFogColor(gs::rgb4(1, 2, 4));
    mode_ = Mode::Title;
    age_ = 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    wave_ = 0;
    well_ = 5;
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
    const int n = wave_ == 0 ? 6 : wave_ == 1 ? 8 : 10;
    const int gap = wave_ == 0 ? 96 : wave_ == 1 ? 82 : 72;
    const int lanes[10] = {1, 0, 2, 1, 2, 0, 1, 2, 0, 1};
    for (int i = 0; i < n; i++) {
        Spawn s;
        s.frame = 20 + i * gap;
        s.lane = lanes[i];
        s.kind = 0;
        s.speed = wave_ == 0 ? 0.52f : wave_ == 1 ? 0.64f : 0.76f;
        if (wave_ >= 1 && (i % 4) == 3) {
            s.kind = 1;
            s.speed *= 0.82f;
        }
        if (wave_ == 2 && (i % 5) == 4) {
            s.kind = 0;
            s.speed = 0.95f;
        }
        spawns_.push_back(s);
    }
}

bool Game::anyAlive() const {
    for (const Foe& f : foes_)
        if (f.alive) return true;
    return false;
}

void Game::tone(int ch, float freq) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, 0.07f);
}

void Game::shoot() {
    if (cool_ > 0) return;
    cool_ = 8;
    for (Spark& b : sparks_) {
        if (b.life > 0) continue;
        b.x = LANE_X[lane_];
        b.y = RAIL_Y + 16.f;
        b.lane = lane_;
        b.life = 28;
        flash_ = 4;
        tone(0, 720.f);
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
    fire = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_Z);
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
        f.points = s.kind == 0 ? 100 : 160;
        foes_.push_back(f);
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age++;
        f.y -= f.speed;
        if (f.y <= RIM) {
            f.alive = false;
            well_--;
            tone(1, 80.f);
            sys_->apu.noiseBurst(0.2f, 900.f, 0.16f);
            if (well_ <= 0) {
                well_ = 0;
                mode_ = Mode::Lost;
                over_ = true;
                won_ = false;
                return;
            }
        }
    }

    for (Spark& b : sparks_) {
        if (b.life <= 0) continue;
        b.life--;
        b.y += SPARK_V;
        for (Foe& f : foes_) {
            if (!f.alive || b.life <= 0 || f.lane != b.lane) continue;
            if (std::fabs(b.y - f.y) > 14.f) continue;
            b.life = 0;
            f.hp--;
            if (f.hp <= 0) {
                f.alive = false;
                score_ += f.points;
                tone(2, 280.f);
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
        if (banner_ > 42) {
            loadWave();
            mode_ = Mode::Play;
        }
        return;
    }
    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        wonAge_++;
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
    int lamp = (age_ / 8) & 1 ? gs::rgb4(15, 14, 7) : gs::rgb4(15, 11, 3);
    v.setColor(PAL_LAMP * 16 + 6, lamp);
    v.setColor(PAL_SPARK * 16 + 1, lamp);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = 1 + (y < 90 ? 0 : (y - 90) / 40);
        if (g > 4) g = 4;
        v.lineBackdrop[y] = y < 150 ? gs::rgb4(1, 1, 2 + (y > 70)) : gs::rgb4(1, 2 + g / 2, 3 + g);
        v.road[y].on = false;
        v.lineFog[y] = y > 180 ? 3 : 0;
    }

    if (mode_ == Mode::Title) {
        text("S3 BEACON WELL", 160, 22, 0.62f, PAL_LAMP);
        text("ONE BEACON", 160, 42, 0.42f, PAL_HUD);
    } else if (mode_ == Mode::Banner) {
        text("WAVE OUT", 160, 22, 0.7f, PAL_SEA);
    } else if (mode_ == Mode::Won) {
        text("THE WELL STANDS", 160, 18, 0.55f, PAL_LAMP);
    } else if (mode_ == Mode::Lost) {
        text("THE WELL FALLS", 160, 18, 0.58f, PAL_WICK);
    } else if (mode_ == Mode::Pause) {
        text("HOLD", 160, 18, 0.7f, PAL_HUD);
    }

    sprite(art_.tower, 160, 40, 70, PAL_LAMP, false, 0);
    for (int i = 0; i < LANES; i++) sprite(art_.rock, LANE_X[i], 168, 12, PAL_STONE, i == 2, 2);
    sprite(art_.well, 160, WELL_Y, 46, PAL_STONE, false, 0);

    for (const Foe& f : foes_) {
        if (!f.alive) continue;
        float near = std::clamp((SPAWN_Y - f.y) / (SPAWN_Y - RIM), 0.f, 1.f);
        float h = 20.f + near * 14.f;
        int fog = int((1.f - near) * 5.f);
        const gs::Mipped* img = &art_.tide[(f.age / 8) & 1];
        int pal = PAL_TIDE;
        if (f.kind == 1) {
            img = &art_.wick[(f.age / 9) & 1];
            pal = PAL_WICK;
            h += 4.f;
        }
        sprite(*img, LANE_X[f.lane], f.y, h, pal, f.lane == 0, fog);
    }

    float sx = LANE_X[lane_];
    if (flash_ > 0) sprite(art_.beam, sx, RAIL_Y + 48.f, 72, PAL_SPARK, false, 0);
    sprite(art_.keeper[(age_ / 10) & 1], sx, RAIL_Y, 32, PAL_KEEPER, lane_ == 0, 0);
    for (const Spark& b : sparks_) {
        if (b.life <= 0) continue;
        sprite(art_.spark, b.x, b.y, 8, PAL_SPARK, false, 0);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(22, "KEEP THE WELL STANDING.", PAL_HUD);
        hudC(23, "THREE WAVES FROM THE TIDE.", PAL_LAMP);
        hudC(25, "LEFT RIGHT   Z FIRE   ENTER", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        std::snprintf(line, sizeof(line), "WELL %d", well_);
        hud(1, 26, line, well_ <= 2 ? PAL_WICK : PAL_SEA);
        std::snprintf(line, sizeof(line), "WAVE %d/3", wave_ + 1);
        hud(14, 26, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "SCORE %d", score_);
        hud(26, 26, line, PAL_LAMP);
        if (mode_ == Mode::Lost) hudC(24, "THE LAMP IS OUT.", PAL_WICK);
        if (mode_ == Mode::Won) hudC(24, "THE BEACON HELD.", PAL_LAMP);
    }
}

}  // namespace bwell
