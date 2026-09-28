#include "game/well.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace quarry {
namespace {

constexpr float kWellX = 160.f;
constexpr int kWaves = 3;

int waveCount(int w) { return w == 0 ? 8 : w == 1 ? 10 : 12; }
int waveGap(int w) { return w == 0 ? 78 : w == 1 ? 62 : 50; }
float waveSpeed(int w) { return w == 0 ? 1.05f : w == 1 ? 1.35f : 1.55f; }

}  // namespace

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    tone_ = 6;
}

void Game::hush() {
    if (tone_ > 0 && --tone_ == 0) sys_->apu.tone(0, 0, 0);
}

void Game::spr(const gs::Image& img, float x, float y, float w, float h, int pal, bool flip) {
    if (w < 1 || h < 1) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    float w = 0;
    for (unsigned char ch : s) {
        if (ch < 32 || ch > 126) ch = '?';
        w += (art_.glyph[ch - 32].w * scale) + scale;
    }
    if (align == 1) x -= w * 0.5f;
    else if (align == 2) x -= w;
    for (unsigned char ch : s) {
        if (ch < 32 || ch > 126) ch = '?';
        const gs::Image& g = art_.glyph[ch - 32];
        spr(g, x, y, g.w * scale, g.h * scale, pal);
        x += g.w * scale + scale;
    }
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    wave_ = 0;
    score_ = 0;
    hp_ = 100;
    px_ = kWellX;
    lean_ = 0;
    shake_ = 0;
    over_ = false;
    won_ = false;
    openWave();
}

void Game::openWave() {
    threats_.clear();
    spawned_ = 0;
    spawnWait_ = 20;
    swingT_ = 0;
    swingCd_ = 0;
    mode_ = Mode::Watch;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.5f);
    if (bot_) beginWatch();
}

void Game::think(float& ax, bool& swing) {
    ax = 0;
    swing = false;
    const Threat* best = nullptr;
    float bestT = 1e9f;
    for (const Threat& t : threats_) {
        if (!t.alive) continue;
        float dist = std::fabs(t.x - kWellX) - 22.f;
        if (dist < 0) dist = 0;
        float eta = dist / std::max(0.2f, std::fabs(t.vx));
        if (eta < bestT) {
            bestT = eta;
            best = &t;
        }
    }
    if (!best) {
        ax = (kWellX - px_) * 0.08f;
        return;
    }
    float dx = best->x - px_;
    if (std::fabs(dx) > 10.f) ax = dx > 0 ? 1.f : -1.f;
    if (std::fabs(dx) < 20.f && swingCd_ == 0 && swingT_ == 0) swing = true;
}

void Game::updateWatch() {
    float ax = 0;
    bool swing = false;
    if (bot_) think(ax, swing);
    else {
        if (sys_->pad.down(gs::BTN_LEFT)) ax -= 1;
        if (sys_->pad.down(gs::BTN_RIGHT)) ax += 1;
        if (std::fabs(sys_->pad.axisX) > 0.25f) ax = sys_->pad.axisX;
        swing = sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_B) || sys_->pad.pressed(gs::BTN_C);
    }
    if (ax < 0) facing_ = -1;
    else if (ax > 0) facing_ = 1;
    px_ += ax * 2.9f;
    px_ = std::max(28.f, std::min(292.f, px_));

    if (swing && swingCd_ == 0 && swingT_ == 0) {
        swingT_ = 12;
        swingCd_ = 16;
        swingId_++;
        blip(210.f, 0.12f);
    }
    if (swingT_ > 0) swingT_--;
    if (swingCd_ > 0) swingCd_--;

    if (spawned_ < waveCount(wave_)) {
        if (spawnWait_ > 0) spawnWait_--;
        else {
            Threat t;
            bool left = (spawned_ % 2) == 0;
            int slot = spawned_ % 4;
            if (wave_ == 0) t.kind = Kind::Rock;
            else if (wave_ == 2 && slot == 3) t.kind = Kind::Boulder;
            else if (slot == 1) t.kind = Kind::Cart;
            else t.kind = Kind::Rock;
            t.hp = t.kind == Kind::Boulder ? 3 : 1;
            t.dmg = t.kind == Kind::Boulder ? 28 : t.kind == Kind::Cart ? 16 : 12;
            float sp = waveSpeed(wave_) * (t.kind == Kind::Boulder ? 0.72f : t.kind == Kind::Cart ? 1.12f : 1.f);
            t.x = left ? -24.f : 344.f;
            t.vx = left ? sp : -sp;
            t.swingHit = -1;
            t.alive = true;
            threats_.push_back(t);
            spawned_++;
            spawnWait_ = waveGap(wave_);
        }
    }

    bool any = false;
    for (Threat& t : threats_) {
        if (!t.alive) continue;
        any = true;
        t.x += t.vx;
        if (swingT_ > 2 && swingT_ < 11 && t.swingHit != swingId_) {
            float hx = px_ + facing_ * 16.f;
            if (std::fabs(t.x - hx) < 26.f) {
                t.swingHit = swingId_;
                t.hp--;
                blip(t.hp > 0 ? 140.f : 320.f, 0.16f);
                sys_->apu.noiseBurst(0.25f, 0.45f, 0.08f);
                if (t.hp <= 0) {
                    t.alive = false;
                    int pts = t.kind == Kind::Boulder ? 250 : t.kind == Kind::Cart ? 150 : 100;
                    score_ += pts;
                }
            }
        }
        if (!t.alive) continue;
        if (std::fabs(t.x - kWellX) < 16.f) {
            t.alive = false;
            hp_ -= t.dmg;
            lean_ += t.vx > 0 ? 1.2f : -1.2f;
            shake_ = 8;
            blip(70.f, 0.22f);
            sys_->apu.noiseBurst(0.4f, 0.2f, 0.15f);
            if (hp_ < 0) hp_ = 0;
        }
    }
    lean_ *= 0.96f;
    if (shake_ > 0) shake_ -= 1;

    if (hp_ <= 0) {
        mode_ = Mode::Fail;
        banner_ = 70;
        return;
    }
    if (spawned_ >= waveCount(wave_) && !any) {
        score_ += 200 + hp_;
        if (wave_ + 1 >= kWaves) {
            mode_ = Mode::Win;
            won_ = true;
            banner_ = 50;
        } else {
            mode_ = Mode::Banner;
            banner_ = 80;
        }
    }
}

void Game::scene() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < 64) v.lineBackdrop[y] = gs::rgb4(5, 7, 9);
        else if (y < 100) v.lineBackdrop[y] = gs::rgb4(7, 6, 5);
        else if (y < 148) v.lineBackdrop[y] = gs::rgb4(6 + (y / 30), 5, 3);
        else v.lineBackdrop[y] = gs::rgb4(5, 4, 3);
        v.lineFog[y] = y < 80 ? uint8_t(4) : 0;
        gs::RoadLine& r = v.road[y];
        r = gs::RoadLine{};
        if (y >= 150) {
            r.on = true;
            r.cx = 160;
            r.hw = 28.f + (y - 150) * 1.35f;
            r.v = float(y) * 3.f + float(sys_->frame) * 0.15f;
            r.pal = PAL_ROAD;
            r.band = ((y / 6) & 1) ? 1 : 0;
            r.style = gs::ROAD_ROCKY;
            r.left = gs::GROUND_LAND;
            r.right = gs::GROUND_LAND;
        }
    }
    v.roadTime = int(sys_->frame / 2);
}

void Game::draw() {
    sys_->vdp.clearSprites();
    scene();
    float sh = shake_ > 0 ? ((int(sys_->frame) & 1) ? 1.f : -1.f) : 0;
    const char* waveWord = wave_ == 0 ? "FIRST CUT" : wave_ == 1 ? "HAUL" : "LAST FACE";

    if (mode_ == Mode::Title) {
        text("QUARRY WELL", 160, 78, 2, PAL_WELL, 1);
        text("KEEP THE WELL STANDING", 160, 100, 1, PAL_HUD, 1);
        text("THROUGH THREE WAVES", 160, 112, 1, PAL_HUD, 1);
        text("MISS THAT AND THE", 160, 126, 1, PAL_HUD, 1);
        text("WATCH IS OVER", 160, 138, 1, PAL_CART, 1);
        text("START", 160, 156, 1, PAL_HUD, 1);
    } else if (mode_ == Mode::Banner) {
        text("WAVE CLEAR", 160, 78, 2, PAL_HUD, 1);
        text("SHORE THE WELL", 160, 100, 1, PAL_WELL, 1);
    } else if (mode_ == Mode::Fail) {
        text("WATCH OVER", 160, 70, 2, PAL_CART, 1);
        text("THE WELL IS DOWN", 160, 92, 1, PAL_HUD, 1);
    } else if (mode_ == Mode::Win) {
        text("WATCH KEPT", 160, 70, 2, PAL_HUD, 1);
        text("THE WELL STANDS", 160, 92, 1, PAL_WELL, 1);
    }

    text("WELL", 8, 6, 1, PAL_HUD, 0);
    spr(art_.pip, 40, 8, std::max(1.f, hp_ * 0.7f), 5, hp_ > 35 ? PAL_HUD : PAL_CART);
    text(std::to_string(score_), 312, 6, 1, PAL_HUD, 2);
    text(waveWord, 160, 6, 1, PAL_HUD, 1);

    spr(art_.cliff, 0, 48, 40, 110, PAL_CLIFF, false);
    spr(art_.cliff, 280, 48, 40, 110, PAL_CLIFF, true);

    float wy = 92 + sh + std::fabs(lean_) * 2.f;
    if (mode_ == Mode::Fail) wy += float(70 - banner_) * 0.6f;
    spr(art_.well, kWellX - 24 + lean_ * 3.f, wy, 48, 72, PAL_WELL, lean_ < 0);

    for (const Threat& t : threats_) {
        if (!t.alive) continue;
        if (t.kind == Kind::Rock) spr(art_.rock, t.x - 10, 132, 20, 20, PAL_ROCK, t.vx < 0);
        else if (t.kind == Kind::Cart) spr(art_.cart, t.x - 16, 128, 32, 20, PAL_CART, t.vx > 0);
        else spr(art_.boulder, t.x - 18, 118, 36, 34, PAL_BOULDER, false);
    }

    bool flip = facing_ < 0;
    spr(art_.man, px_ - 12, 118, 24, 34, PAL_MAN, flip);
    spr(art_.lamp, px_ - 3, 112, 6, 6, PAL_CART, false);
    if (swingT_ > 0) {
        float tx = px_ + facing_ * 8.f;
        spr(art_.timber, tx, 128, 22, 6, PAL_WELL, facing_ < 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    hush();
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) beginWatch();
    } else if (mode_ == Mode::Watch) {
        updateWatch();
    } else if (mode_ == Mode::Banner) {
        if (--banner_ <= 0) {
            wave_++;
            openWave();
        }
    } else if (mode_ == Mode::Fail) {
        if (--banner_ <= 0) over_ = true;
        else if (!bot_ && sys.pad.pressed(gs::BTN_START)) beginWatch();
    } else if (mode_ == Mode::Win) {
        if (--banner_ <= 0) over_ = true;
    }
    draw();
}

}  // namespace quarry
