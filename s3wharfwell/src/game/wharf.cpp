#include "game/wharf.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace wharf {
namespace {
constexpr float DT = 1.f / 60.f;
constexpr float WELL_X = 160.f;
constexpr float DECK_Y = 150.f;
constexpr int WELL_MAX = 10;
}  // namespace

void Game::blip(bool high) { sys_->apu.tone(0, high ? 680.f : 420.f, 0.08f); }

void Game::hitSound() {
    sys_->apu.noiseBurst(0.18f, 2400.f, 0.08f);
    sys_->apu.tone(1, 180.f, 0.1f);
}

void Game::fanfare() {
    sys_->apu.tone(0, 523.f, 0.12f);
    sys_->apu.tone(1, 659.f, 0.1f);
    sys_->apu.tone(2, 784.f, 0.1f);
}

void Game::resetRun() {
    wave_ = 0;
    wellHp_ = WELL_MAX;
    score_ = 0;
    won_ = false;
    over_ = false;
    px_ = WELL_X;
    face_ = 1;
    cool_ = 0;
    swingT_ = 0;
    hurt_ = 0;
    foes_.clear();
    queue_.clear();
    spawned_ = 0;
    queueWave();
    mode_ = Mode::Banner;
    bannerT_ = 1.15f;
    waveT_ = 0;
    endT_ = 0;
}

void Game::queueWave() {
    queue_.clear();
    spawned_ = 0;
    foes_.clear();
    auto add = [&](float t, Kind k, int side) {
        queue_.push_back(Spawn{t, k, side});
    };
    if (wave_ == 0) {
        add(0.2f, Crab, -1);
        add(1.5f, Crab, 1);
        add(2.8f, Crab, -1);
        add(4.1f, Crab, 1);
        add(5.5f, Crab, -1);
    } else if (wave_ == 1) {
        add(0.2f, Crab, -1);
        add(1.3f, Barrel, 1);
        add(2.8f, Crab, 1);
        add(4.0f, Barrel, -1);
        add(5.5f, Crab, -1);
        add(6.8f, Crab, 1);
    } else {
        add(0.15f, Crab, -1);
        add(0.55f, Crab, 1);
        add(2.0f, Brute, -1);
        add(3.6f, Barrel, 1);
        add(5.0f, Brute, 1);
        add(6.5f, Crab, -1);
        add(7.0f, Barrel, -1);
        add(8.6f, Crab, 1);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(2, 4, 8));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.2f, 0.12f);
    mode_ = Mode::Title;
    t_ = 0;
    if (bot_) resetRun();
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    if (mode_ == Mode::Title) {
        px_ = WELL_X + std::sin(t_ * 0.8f) * 36.f;
        face_ = std::cos(t_ * 0.8f) > 0 ? 1 : -1;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            blip(true);
            resetRun();
        }
        return;
    }
    if (mode_ == Mode::Banner) {
        bannerT_ -= dt;
        if (bannerT_ <= 0) {
            mode_ = Mode::Play;
            waveT_ = 0;
        }
        return;
    }
    if (mode_ == Mode::End) {
        endT_ += dt;
        if (bot_ && endT_ > 1.5f) over_ = true;
        else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            mode_ = Mode::Title;
            t_ = 0;
        }
        return;
    }

    waveT_ += dt;
    while (spawned_ < int(queue_.size()) && queue_[spawned_].t <= waveT_) {
        const Spawn& s = queue_[spawned_];
        Foe f;
        f.kind = s.kind;
        f.side = s.side;
        f.x = s.side < 0 ? 6.f : 314.f;
        f.alive = true;
        f.hit = -1;
        if (s.kind == Crab) {
            f.hp = 1;
            f.spd = 34.f;
            f.dmg = 1;
        } else if (s.kind == Barrel) {
            f.hp = 1;
            f.spd = 58.f;
            f.dmg = 2;
        } else {
            f.hp = 2;
            f.spd = 26.f;
            f.dmg = 2;
        }
        foes_.push_back(f);
        spawned_++;
    }

    float ax = 0;
    bool want = false;
    if (bot_) {
        const Foe* best = nullptr;
        float bestD = 1e9f;
        for (const Foe& f : foes_) {
            if (!f.alive) continue;
            float d = std::fabs(f.x - WELL_X);
            if (d < bestD) {
                bestD = d;
                best = &f;
            }
        }
        if (best) {
            face_ = best->x >= px_ ? 1 : -1;
            float gap = best->x - px_;
            if (std::fabs(gap) > 22.f) ax = gap > 0 ? 1.f : -1.f;
            if (std::fabs(gap) < 40.f && ((face_ > 0 && best->x >= px_) || (face_ < 0 && best->x <= px_))) want = true;
        }
    } else {
        if (pad.down(gs::BTN_LEFT)) ax = -1;
        if (pad.down(gs::BTN_RIGHT)) ax = 1;
        if (pad.axisX > 0.3f || pad.axisX < -0.3f) ax = pad.axisX > 0 ? 1.f : -1.f;
        if (ax > 0) face_ = 1;
        else if (ax < 0) face_ = -1;
        want = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C);
    }

    px_ += ax * 150.f * dt;
    if (px_ < 18) px_ = 18;
    if (px_ > 302) px_ = 302;
    if (ax != 0) walk_ += dt * 10.f;
    cool_ -= dt;
    if (swingT_ > 0) swingT_ -= dt;
    hurt_ -= dt;

    if (want && cool_ <= 0) {
        cool_ = 0.26f;
        swingT_ = 0.12f;
        swingId_++;
        blip(false);
        for (Foe& f : foes_) {
            if (!f.alive || f.hit == swingId_) continue;
            float dx = f.x - px_;
            bool facing = face_ > 0 ? dx >= -6 : dx <= 6;
            if (!facing || std::fabs(dx) > 42.f) continue;
            f.hit = swingId_;
            f.hp--;
            hitSound();
            if (f.hp <= 0) {
                f.alive = false;
                score_ += f.kind == Brute ? 50 : (f.kind == Barrel ? 30 : 20);
            }
        }
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        float dir = (WELL_X > f.x) ? 1.f : -1.f;
        f.x += dir * f.spd * dt;
        if (std::fabs(f.x - WELL_X) < 16.f) {
            f.alive = false;
            wellHp_ -= f.dmg;
            hurt_ = 0.35f;
            sys_->apu.noiseBurst(0.28f, 700.f, 0.16f);
            if (wellHp_ < 0) wellHp_ = 0;
        }
    }
    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return !f.alive; }), foes_.end());

    if (wellHp_ <= 0) {
        mode_ = Mode::End;
        won_ = false;
        endT_ = 0;
        sys_->apu.tone(0, 110.f, 0.16f);
        return;
    }
    if (spawned_ >= int(queue_.size()) && foes_.empty()) {
        if (wave_ >= 2) {
            mode_ = Mode::End;
            won_ = true;
            endT_ = 0;
            fanfare();
        } else {
            wave_++;
            queueWave();
            mode_ = Mode::Banner;
            bannerT_ = 1.15f;
            waveT_ = 0;
            blip(true);
        }
    }
}

void Game::blit(const gs::Image& img, float x, float y, int pal, bool flip) {
    if (img.w <= 0 || img.h <= 0) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = img.w;
    s.h = img.h;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, int pal) {
    for (char ch : s) {
        int i = int(static_cast<unsigned char>(ch)) - 32;
        if (i < 0 || i >= 96) i = 0;
        blit(art_.glyph[i], x, y, pal, false);
        x += float(art_.advance);
    }
}

void Game::textC(const std::string& s, float y, int pal) {
    float w = float(s.size()) * float(art_.advance);
    text(s, (gs::SCREEN_W - w) * 0.5f, y, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    v.A.enabled = false;
    v.B.enabled = false;
    v.HUD.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        uint16_t c;
        if (y < 100) {
            int k = y / 14;
            c = gs::rgb4(2 + k / 4, 4 + k / 2, 8 + k);
        } else if (y < 118) {
            c = gs::rgb4(7, 9, 10);
        } else {
            int band = ((y + int(t_ * 36.f)) / 5) & 1;
            int deep = (y - 118) / 16;
            if (deep > 6) deep = 6;
            c = gs::rgb4(1, 3 + band, 6 + (6 - deep) / 2);
        }
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    backdrop();
    const char* waveName = wave_ == 0 ? "CRABS" : wave_ == 1 ? "BARRELS" : "SURGE";

    if (mode_ == Mode::Title) {
        textC("WHARF WELL", 28, PAL_GOLD);
        textC("KEEP THE WELL STANDING", 52, PAL_TEXT);
        textC("THREE WAVES", 70, PAL_TEXT);
        if (int(t_ * 2) % 2 == 0) textC("PRESS START", 188, PAL_GOLD);
        text("ARROWS MOVE   A SWINGS", 28, 206, PAL_TEXT);
    } else {
        std::string hp = "WELL ";
        for (int i = 0; i < WELL_MAX; i++) hp += (i < wellHp_) ? "|" : ".";
        text(hp, 8, 6, wellHp_ <= 3 ? PAL_ALERT : PAL_TEXT);
        std::string wv = "WAVE " + std::to_string(wave_ + 1) + "/3";
        text(wv, 214, 6, PAL_GOLD);
    }

    if (mode_ == Mode::Banner) textC(waveName, 78, PAL_GOLD);
    if (mode_ == Mode::End) {
        textC(won_ ? "THE WELL STANDS" : "THE WELL FALLS", 72, won_ ? PAL_GOLD : PAL_ALERT);
        if (!bot_ && int(t_ * 2) % 2 == 0) textC("START", 96, PAL_TEXT);
    }

    for (int i = 0; i < 4; i++) {
        float gy = 36.f + (i % 2) * 10.f;
        float gx = std::fmod(40.f + i * 78.f + t_ * (12.f + i * 3.f), 360.f) - 20.f;
        blit(art_.gull, gx, gy, PAL_GULL, (i & 1) != 0);
    }

    bool showWell = hurt_ <= 0 || int(hurt_ * 20) % 2 == 0;
    if (showWell) blit(art_.well, WELL_X - 26, DECK_Y - 62, PAL_STONE, false);

    blit(art_.deck, 8, DECK_Y, PAL_WOOD, false);
    for (int i = 0; i < 6; i++) blit(art_.pile, 24.f + i * 50.f, DECK_Y + 16, PAL_WOOD, false);

    for (int i = 0; i < 8; i++) {
        float fx = std::fmod(i * 40.f + t_ * 18.f, 340.f) - 10.f;
        float fy = 168.f + ((i * 17 + int(t_ * 8)) % 28);
        blit(art_.foam, fx, fy, PAL_FOAM, false);
    }

    for (const Foe& f : foes_) {
        bool hop = int(t_ * 8 + f.x) % 2 == 0;
        if (f.kind == Crab) blit(art_.crab, f.x - 18, DECK_Y - 18 - (hop ? 1 : 0), PAL_CRAB, f.side > 0);
        else if (f.kind == Barrel) blit(art_.barrel, f.x - 9, DECK_Y - 22, PAL_BARREL, false);
        else blit(art_.brute, f.x - 20, DECK_Y - 24 - (hop ? 1 : 0), PAL_BRUTE, f.side > 0);
    }

    int pose = 0;
    if (swingT_ > 0) pose = 2;
    else if (mode_ == Mode::Play && std::fabs(std::sin(walk_)) > 0.2f) pose = int(walk_) & 1;
    float bob = (pose == 1) ? -1.f : 0.f;
    blit(art_.keeper[pose], px_ - 16, DECK_Y - 40 + bob, PAL_KEEPER, face_ < 0);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (!music_) {
        sys.apu.noise(0.03f, 900.f, false);
        music_ = true;
    } else if (int(t_ * 60) % 30 == 0) {
        sys.apu.noise(0.03f, 700.f + std::sin(t_) * 200.f, false);
    }
    update(DT);
    // Quiet the blip so it does not drone.
    if (cool_ < 0.12f) sys.apu.tone(0, 0, 0);
    if (swingT_ <= 0) sys.apu.tone(1, 0, 0);
    draw();
}

}  // namespace wharf
