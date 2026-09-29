#include "game/well.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace well {
namespace {
constexpr float DT = 1.0f / 60.0f;
constexpr float WELL_X = 160.0f;
constexpr float STRIKE = 36.0f;
constexpr int WAVES = 3;
}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Play) return 1;
    if (mode_ == Mode::Banner) return 2;
    return 3;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    over_ = false;
    won_ = false;
    if (bot_) bootWatch();
    else mode_ = Mode::Title;
}

void Game::bootWatch() {
    wave_ = 0;
    score_ = 0;
    hp_ = hpMax_ = 8;
    px_ = WELL_X;
    face_ = -1;
    cool_ = 0;
    t_ = 0;
    hold_ = 0;
    shake_ = 0;
    foes_.clear();
    drops_.clear();
    over_ = false;
    won_ = false;
    armWave();
    mode_ = Mode::Play;
}

void Game::armWave() {
    script_.clear();
    spawnAt_ = 0;
    t_ = 0;
    // Gaps are wide enough to cross the coping, tight enough that standing still loses the well.
    if (wave_ == 0) {
        for (int i = 0; i < 4; i++) script_.push_back({0.6f + i * 1.35f, -1, 1, 0, 34.0f});
        for (int i = 0; i < 3; i++) script_.push_back({6.4f + i * 1.25f, 1, 1, 0, 36.0f});
    } else if (wave_ == 1) {
        for (int i = 0; i < 8; i++) {
            int dir = (i % 2 == 0) ? -1 : 1;
            script_.push_back({0.4f + i * 1.45f, dir, i == 5 ? 2 : 1, 0, 40.0f + (i % 3) * 4.0f});
        }
    } else {
        for (int i = 0; i < 6; i++) {
            int dir = (i % 2 == 0) ? 1 : -1;
            script_.push_back({0.35f + i * 1.35f, dir, 2, 0, 44.0f});
        }
        script_.push_back({2.2f, -1, 4, 1, 26.0f});
        script_.push_back({6.4f, 1, 4, 1, 26.0f});
        script_.push_back({9.2f, -1, 2, 0, 52.0f});
    }
}

void Game::blip(bool high) {
    sys_->apu.tone(0, high ? 740.0f : 420.0f, 0.06f);
    beep_ = 0.05f;
}

void Game::crack() { sys_->apu.noiseBurst(0.4f, 500.0f, 0.22f); }

void Game::fanfare() {
    fanStep_ = 0;
    fanT_ = 0;
}

void Game::spray() {
    if (cool_ > 0) return;
    Drop d;
    d.x = px_ + face_ * 10.0f;
    d.vx = face_ * 280.0f;
    d.life = 0.42f;
    drops_.push_back(d);
    cool_ = 0.14f;
    blip(true);
}

void Game::botAim(bool& fire) {
    fire = false;
    const Foe* best = nullptr;
    float bestT = 1e9f;
    for (const Foe& f : foes_) {
        float gate = f.vx > 0 ? WELL_X - STRIKE : WELL_X + STRIKE;
        float dist = std::fabs(gate - f.x);
        float eta = dist / std::max(8.0f, std::fabs(f.vx));
        if (eta < bestT) {
            bestT = eta;
            best = &f;
        }
    }
    if (!best) return;
    float goal = std::clamp(best->x, 28.0f, 292.0f);
    if (px_ < goal - 3) {
        px_ += 200.0f * DT;
        face_ = 1;
    } else if (px_ > goal + 3) {
        px_ -= 200.0f * DT;
        face_ = -1;
    } else {
        face_ = best->x < px_ ? -1 : 1;
    }
    if (std::fabs(px_ - best->x) < 108.0f) fire = true;
}

void Game::update(float dt) {
    if (beep_ > 0) {
        beep_ -= dt;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (fanStep_ >= 0) {
        fanT_ -= dt;
        if (fanT_ <= 0) {
            static const float notes[] = {523.0f, 659.0f, 784.0f, 1046.0f};
            if (fanStep_ < 4) {
                sys_->apu.tone(1, notes[fanStep_], 0.12f);
                fanT_ = 0.16f;
                fanStep_++;
            } else {
                sys_->apu.tone(1, 0, 0);
                fanStep_ = -1;
            }
        }
    }
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - dt);

    if (mode_ == Mode::Title) {
        if (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A)) bootWatch();
        return;
    }
    if (mode_ == Mode::Dead || mode_ == Mode::Victory) {
        hold_ += dt;
        if (!bot_ && hold_ > 0.4f && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A))) {
            mode_ = Mode::Title;
            over_ = false;
        }
        return;
    }
    if (mode_ == Mode::Banner) {
        hold_ -= dt;
        if (hold_ <= 0) {
            if (wave_ + 1 >= WAVES) {
                won_ = true;
                over_ = true;
                mode_ = Mode::Victory;
                hold_ = 0;
                score_ += hp_ * 50;
                fanfare();
            } else {
                wave_++;
                armWave();
                mode_ = Mode::Play;
            }
        }
        return;
    }

    bool fire = false;
    if (bot_) botAim(fire);
    else {
        float ax = sys_->pad.axisX;
        if (sys_->pad.down(gs::BTN_LEFT) || ax < -0.3f) {
            px_ -= 150.0f * dt;
            face_ = -1;
        }
        if (sys_->pad.down(gs::BTN_RIGHT) || ax > 0.3f) {
            px_ += 150.0f * dt;
            face_ = 1;
        }
        fire = sys_->pad.down(gs::BTN_A) || sys_->pad.down(gs::BTN_B);
    }
    px_ = std::clamp(px_, 22.0f, 298.0f);
    cool_ = std::max(0.0f, cool_ - dt);
    if (fire) spray();

    t_ += dt;
    while (spawnAt_ < int(script_.size()) && script_[spawnAt_].t <= t_) {
        const Spawn& s = script_[spawnAt_];
        Foe f;
        f.x = s.dir < 0 ? -18.0f : 338.0f;
        f.vx = (s.dir < 0 ? 1.0f : -1.0f) * s.spd;
        f.hp = s.hp;
        f.kind = s.kind;
        f.flash = 0;
        foes_.push_back(f);
        spawnAt_++;
    }

    for (Drop& d : drops_) {
        d.x += d.vx * dt;
        d.life -= dt;
    }
    for (Foe& f : foes_) {
        f.x += f.vx * dt;
        if (f.flash > 0) f.flash -= dt;
        for (Drop& d : drops_) {
            if (d.life <= 0) continue;
            if (std::fabs(d.x - f.x) < (f.kind ? 22.0f : 16.0f)) {
                d.life = 0;
                f.hp--;
                f.flash = 0.08f;
                f.x += (f.x < WELL_X ? -12.0f : 12.0f);
                blip(false);
                if (f.hp <= 0) score_ += f.kind ? 250 : 100;
            }
        }
    }
    drops_.erase(std::remove_if(drops_.begin(), drops_.end(), [](const Drop& d) { return d.life <= 0 || d.x < -20 || d.x > 340; }),
                 drops_.end());

    for (Foe& f : foes_) {
        if (f.hp <= 0) continue;
        bool hit = std::fabs(f.x - WELL_X) < STRIKE;
        if (!hit) continue;
        f.hp = 0;
        hp_--;
        shake_ = 0.25f;
        crack();
    }
    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return f.hp <= 0; }), foes_.end());

    if (hp_ <= 0) {
        hp_ = 0;
        mode_ = Mode::Dead;
        over_ = true;
        won_ = false;
        hold_ = 0;
        return;
    }
    if (spawnAt_ >= int(script_.size()) && foes_.empty()) {
        score_ += 500;
        mode_ = Mode::Banner;
        hold_ = 1.15f;
        if (hp_ < hpMax_) hp_++;
        fanfare();
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    float sh = shake_ > 0 ? std::sin(shake_ * 80.0f) * 3.0f : 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        int yy = y;
        if (y < 148) {
            float u = y / 148.0f;
            int r = int(3 + u * 8);
            int g = int(5 + u * 6);
            int b = int(10 - u * 2);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else {
            int band = ((yy + int(sh)) / 4) & 1;
            v.lineBackdrop[y] = band ? gs::rgb4(6, 8, 3) : gs::rgb4(5, 7, 3);
        }
    }
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

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    x -= w * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    float shx = shake_ > 0 ? std::sin(shake_ * 90.0f) * 4.0f : 0;

    spr(art_.reed, 28, 150, 36, PAL_EARTH, false);
    spr(art_.reed, 292, 148, 40, PAL_EARTH, true);
    spr(art_.reed, 54, 156, 28, PAL_EARTH, false);
    spr(art_.well, WELL_X + shx, 150, 108, PAL_STONE, false);
    spr(art_.mouth, WELL_X + shx, 112, 18, PAL_STONE, false);

    for (const Foe& f : foes_) {
        const gs::Mipped& m = f.kind ? art_.brute : art_.raider;
        int pal = f.flash > 0 ? PAL_FX : (f.kind ? PAL_BRUTE : PAL_RAIDER);
        spr(m, f.x, f.kind ? 150 : 156, f.kind ? 64 : 48, pal, f.vx > 0);
    }
    for (const Drop& d : drops_) {
        if (d.life < 0.08f) spr(art_.splash, d.x, 150, 20, PAL_WATER, false);
        else spr(art_.drop, d.x, 148, 12, PAL_WATER, d.vx < 0);
    }
    spr(art_.keeper, px_ + shx, 132, 46, PAL_KEEPER, face_ < 0);

    std::string bars;
    for (int i = 0; i < hpMax_; i++) bars += (i < hp_) ? '#' : '-';
    hud(1, 1, "WELL", PAL_HUD);
    hud(6, 1, bars, PAL_HUD);
    hud(28, 1, "WAVE " + std::to_string(wave_ + 1) + "/3", PAL_HUD);
    hud(1, 26, "SCORE " + std::to_string(score_), PAL_HUD);

    if (mode_ == Mode::Title) {
        text("CISTERN WELL", 160, 48, 1.0f, PAL_HUD);
        hudC(12, "KEEP THE WELL STANDING", PAL_HUD);
        hudC(14, "THREE WAVES", PAL_HUD);
        hudC(18, "ARROWS MOVE    A SPRAYS", PAL_HUD);
        hudC(21, "PRESS START", PAL_HUD);
    } else if (mode_ == Mode::Banner) {
        hudC(8, wave_ + 1 >= WAVES ? "LAST STONE HELD" : "WAVE HELD", PAL_HUD);
    } else if (mode_ == Mode::Dead) {
        text("THE WELL FELL", 160, 48, 1.0f, PAL_FX);
        hudC(20, "PRESS START", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        text("THE WELL STANDS", 160, 44, 1.0f, PAL_WATER);
        hudC(16, "THREE WAVES HELD", PAL_HUD);
        hudC(20, "PRESS START", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    update(DT);
    draw();
}

}  // namespace well
