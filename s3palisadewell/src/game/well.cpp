#include "game/well.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace palisade {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float WALK_Y = 148.f;
constexpr float BREACH_Y = 206.f;
constexpr float WELL_X = 160.f;

const char* WAVE_NAME[] = {"FIRST WATCH", "SECOND WATCH", "LAST WATCH"};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Watch) return 1;
    if (mode_ == Mode::Banner) return 2;
    if (mode_ == Mode::Won) return 3;
    return 4;
}

void Game::blip(float freq) { sys_->apu.tone(0, freq, 0.12f); }

void Game::hudText(int col, int row, const char* s) {
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c > 127) c = '?';
        sys_->vdp.HUD.set(col + i, row, gs::entry(art_.font[c - 32], Pal::PAL_HUD));
    }
}

void Game::spr(const gs::Image& img, float x, float y, int w, int h, int pal, bool flip, int fog) {
    gs::Sprite s;
    s.x = int16_t(x + (shake_ > 0 ? std::sin(t_ * 40.f) * shake_ : 0));
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    sys.vdp.A.enabled = false;
    sys.apu.setMaster(0.8f);
    mode_ = Mode::Title;
}

void Game::beginWatch() {
    wave_ = 0;
    score_ = 0;
    wellHp_ = 100;
    px_ = WELL_X;
    face_ = 1;
    cool_ = 0;
    shake_ = 0;
    hurt_ = 0;
    foes_.clear();
    bolts_.clear();
    won_ = false;
    over_ = false;
    openWave();
}

void Game::openWave() {
    script_.clear();
    spawnIx_ = 0;
    foes_.clear();
    bolts_.clear();
    t_ = 0;
    float t = 0.6f;
    auto add = [&](float gap, float x, int kind) {
        t += gap;
        script_.push_back({t, x, kind});
    };
    if (wave_ == 0) {
        const float xs[] = {150, 176, 132, 198, 118, 168, 142, 186};
        for (float x : xs) add(1.15f, x, 0);
    } else if (wave_ == 1) {
        const float xs[] = {100, 210, 80, 230, 140, 190, 60, 250, 160, 120, 200, 170};
        for (float x : xs) add(1.05f, x, 0);
    } else {
        const float xs[] = {160, 120, 200, 90, 230, 150, 70, 250, 180, 110, 210, 140, 40, 280};
        for (int i = 0; i < 14; i++) add(i == 4 || i == 9 ? 1.35f : 0.95f, xs[i], (i == 4 || i == 9 || i == 13) ? 1 : 0);
    }
    left_ = int(script_.size());
    mode_ = Mode::Banner;
    bannerT_ = 0;
}

void Game::shoot() {
    if (cool_ > 0) return;
    cool_ = 0.16f;
    bolts_.push_back({px_, WALK_Y - 6});
    blip(face_ > 0 ? 740.f : 680.f);
}

void Game::stick(float& dir, bool& fire) {
    dir = 0;
    fire = false;
    if (bot_) {
        const Foe* best = nullptr;
        for (const Foe& f : foes_) {
            if (!best || f.y > best->y) best = &f;
        }
        if (best) {
            float dx = best->x - px_;
            if (std::fabs(dx) > 5.f) dir = dx > 0 ? 1.f : -1.f;
            fire = std::fabs(dx) < 11.f;
        } else if (px_ > WELL_X + 4) {
            dir = -1;
        } else if (px_ < WELL_X - 4) {
            dir = 1;
        }
        return;
    }
    const gs::Pad& pad = sys_->pad;
    if (pad.down(gs::BTN_LEFT) || pad.axisX < -0.3f) dir = -1;
    if (pad.down(gs::BTN_RIGHT) || pad.axisX > 0.3f) dir = 1;
    fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C);
}

void Game::update(float dt) {
    if (mode_ == Mode::Banner) {
        bannerT_ += dt;
        if (bannerT_ > 1.5f) {
            mode_ = Mode::Watch;
            t_ = 0;
        }
        return;
    }
    if (mode_ != Mode::Watch) return;
    t_ += dt;
    cool_ = std::max(0.f, cool_ - dt);
    shake_ = std::max(0.f, shake_ - dt * 8.f);
    hurt_ = std::max(0.f, hurt_ - dt);

    float dir = 0;
    bool fire = false;
    stick(dir, fire);
    if (dir) face_ = dir;
    px_ = std::clamp(px_ + dir * 210.f * dt, 18.f, 302.f);
    if (fire) shoot();

    while (spawnIx_ < int(script_.size()) && t_ >= script_[spawnIx_].t) {
        const Slot& s = script_[spawnIx_++];
        float spd = (wave_ == 0 ? 32.f : wave_ == 1 ? 42.f : 40.f);
        if (s.kind == 1) spd = 28.f;
        foes_.push_back({s.x, -18.f, s.kind == 1 ? 3.f : 1.f, 0.f, s.kind, spd});
    }

    for (Foe& f : foes_) {
        f.y += f.speed * dt;
        f.flash = std::max(0.f, f.flash - dt);
    }
    for (Bolt& b : bolts_) b.y -= 320.f * dt;

    for (Bolt& b : bolts_) {
        if (b.y < -20) continue;
        for (Foe& f : foes_) {
            if (f.hp <= 0) continue;
            float reach = f.kind ? 16.f : 12.f;
            if (std::fabs(b.x - f.x) < reach && b.y < f.y + 8 && b.y > f.y - 26) {
                f.hp -= 1;
                f.flash = 0.12f;
                b.y = -100;
                sys_->apu.noiseBurst(0.25f, 0.45f, 0.08f);
                if (f.hp <= 0) {
                    score_ += f.kind ? 250 : 100;
                    left_--;
                    blip(f.kind ? 220.f : 520.f);
                }
                break;
            }
        }
    }
    bolts_.erase(std::remove_if(bolts_.begin(), bolts_.end(), [](const Bolt& b) { return b.y < -20; }), bolts_.end());

    for (Foe& f : foes_) {
        if (f.hp <= 0) continue;
        if (f.y >= BREACH_Y) {
            int dmg = f.kind ? 28 : 14;
            wellHp_ = std::max(0, wellHp_ - dmg);
            f.hp = 0;
            left_--;
            shake_ = 6;
            hurt_ = 0.4f;
            sys_->apu.noiseBurst(0.55f, 0.15f, 0.25f);
            sys_->apu.tone(1, 90.f, 0.3f);
        }
    }
    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return f.hp <= 0; }), foes_.end());

    if (wellHp_ <= 0) {
        mode_ = Mode::Lost;
        over_ = true;
        won_ = false;
        sys_->apu.tone(0, 110.f, 0.2f);
        return;
    }
    if (spawnIx_ >= int(script_.size()) && foes_.empty()) {
        score_ += 500;
        if (wave_ >= 2) {
            score_ += wellHp_ * 10;
            mode_ = Mode::Won;
            won_ = true;
            over_ = true;
            sys_->apu.tone(0, 523.f, 0.18f);
            sys_->apu.tone(1, 659.f, 0.18f);
            sys_->apu.tone(2, 784.f, 0.18f);
        } else {
            wave_++;
            openWave();
        }
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    int topR = 1, topG = 2, topB = 6;
    int horR = 10, horG = 6, horB = 3;
    if (wave_ == 1) {
        topR = 1;
        topG = 1;
        topB = 5;
        horR = 8;
        horG = 4;
        horB = 3;
    } else if (wave_ >= 2) {
        topR = 0;
        topG = 1;
        topB = 3;
        horR = 4;
        horG = 3;
        horB = 6;
    }
    if (mode_ == Mode::Title) {
        topR = 1;
        topG = 2;
        topB = 5;
        horR = 9;
        horG = 5;
        horB = 2;
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        float k = std::min(1.f, u * 1.35f);
        int r = int(topR + (horR - topR) * k);
        int g = int(topG + (horG - topG) * k);
        int b = int(topB + (horB - topB) * k);
        if (hurt_ > 0 && y > 150) r = std::min(15, r + 4);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = y < 40 ? uint8_t(6 - y / 8) : 0;
    }
    if (wave_ >= 2 || mode_ == Mode::Title) spr(art_.moon, 250, 18, 18, 18, Pal::PAL_FX);

    for (int i = 0; i < 17; i++) {
        float x = 6.f + i * 19.f;
        if (std::fabs(x - WELL_X) < 22.f) continue;
        int h = 48 + ((i * 5) % 10);
        spr(art_.post, x, 118.f - (h - 48) * 0.3f, 12, h, Pal::PAL_WOOD, false, 0);
    }
    float wy = 118.f + (shake_ > 0 ? shake_ * 0.4f : 0);
    spr(art_.well, WELL_X - 28, wy, 56, 52, Pal::PAL_STONE);

    std::vector<Foe> order = foes_;
    std::sort(order.begin(), order.end(), [](const Foe& a, const Foe& b) { return a.y < b.y; });
    for (int i = int(order.size()) - 1; i >= 0; i--) {
        const Foe& f = order[i];
        float grow = std::clamp(f.y / 220.f, 0.f, 1.f);
        int fog = int((1.f - grow) * 8.f);
        if (f.flash > 0) fog = 0;
        if (f.kind == 0) {
            int h = int(26 + grow * 12);
            spr(art_.raider, f.x - 10, f.y - h, 20, h, f.flash > 0 ? Pal::PAL_FX : Pal::PAL_RAIDER, f.x < WELL_X, fog);
        } else {
            int h = int(32 + grow * 12);
            spr(art_.brute, f.x - 14, f.y - h, 28, h, f.flash > 0 ? Pal::PAL_FX : Pal::PAL_BRUTE, f.x > WELL_X, fog);
        }
    }
    for (const Bolt& b : bolts_) spr(art_.arrow, b.x - 2, b.y - 12, 5, 14, Pal::PAL_FX);
    if (mode_ == Mode::Watch || mode_ == Mode::Banner || mode_ == Mode::Title)
        spr(art_.player, px_ - 12, WALK_Y - 40, 24, 40, Pal::PAL_PLAYER, face_ < 0);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudText(9, 3, "PALISADE WELL");
        hudText(6, 6, "KEEP THE WELL STANDING");
        hudText(8, 8, "THROUGH THREE WAVES");
        hudText(7, 12, "ARROWS  A     MOVE");
        hudText(10, 22, "PRESS START");
    } else if (mode_ == Mode::Banner) {
        hudText(10, 4, WAVE_NAME[wave_]);
        hudText(9, 6, "HOLD THE WELL");
    } else if (mode_ == Mode::Won) {
        hudText(8, 4, "THE WELL STANDS");
        hudText(7, 6, "THE WATCH IS KEPT");
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudText(10, 9, buf);
    } else if (mode_ == Mode::Lost) {
        hudText(7, 4, "THE WATCH IS OVER");
        hudText(8, 6, "THE WELL HAS FALLEN");
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudText(10, 9, buf);
    }
    if (mode_ != Mode::Title) {
        std::snprintf(buf, sizeof buf, "WAVE %d", wave_ + 1);
        hudText(1, 26, buf);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudText(16, 26, buf);
        hudText(1, 24, "WELL");
        int pips = (wellHp_ + 9) / 10;
        for (int i = 0; i < 10; i++) {
            gs::Sprite s;
            s.img = art_.pip;
            s.x = int16_t(48 + i * 9);
            s.y = 24 * 8 + 0;
            s.w = 8;
            s.h = 8;
            s.pal = uint8_t(i < pips ? (wellHp_ < 30 ? Pal::PAL_RAIDER : Pal::PAL_GRASS) : Pal::PAL_WOOD);
            v.sprite(s);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START)) beginWatch();
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            over_ = false;
        }
    } else {
        update(DT);
    }
    if (cool_ <= 0.02f) sys.apu.tone(0, 0, 0);
    draw();
}

}  // namespace palisade
