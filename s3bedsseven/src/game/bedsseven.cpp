#include "game/bedsseven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace bedsseven {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float SPEED = 150.f;
constexpr float POUR = 2.35f;
constexpr float RIVAL = 0.42f;
constexpr float BX[BEDS] = {28.f, 118.f, 208.f};
constexpr float BY = 96.f;
constexpr float BW = 72.f;

int nearest(float x) {
    int best = 0;
    float d = 1e9f;
    for (int i = 0; i < BEDS; i++) {
        float c = BX[i] + BW * 0.5f;
        float ad = std::fabs(x - c);
        if (ad < d) {
            d = ad;
            best = i;
        }
    }
    return best;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win) return 2;
    if (mode_ == Mode::Lose) return 3;
    return 1;
}

void Game::tone(int ch, float hz, float vol) {
    if (!sys_) return;
    gs::FMPatch p;
    p.alg = 4;
    p.vol = vol;
    p.echo = 0.2f;
    p.op[0] = {1, 0.8f, 0.004f, 0.12f, 0.3f, 0.18f};
    p.op[1] = {2, 0.25f, 0.01f, 0.2f, 0.1f, 0.2f};
    p.op[2] = {3, 0.1f, 0.02f, 0.2f, 0, 0.2f};
    p.op[3] = {1, 0.08f, 0.02f, 0.3f, 0, 0.25f};
    sys_->apu.setPatch(ch, p);
    sys_->apu.keyOn(ch, hz, vol);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(6, 8, 10));
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    you_ = 0;
    them_ = 0;
    t_ = 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    you_ = 0;
    them_ = 0;
    px_ = BX[0] + BW * 0.5f;
    walk_ = 0;
    pourBed_ = -1;
    rivalFill_ = 0;
    rivalWait_ = 0.35f;
    hold_ = 0;
    drops_.clear();
    for (int i = 0; i < BEDS; i++) {
        fill_[i] = 0;
        bloom_[i] = 0;
    }
    tone(0, 523.f, 0.16f);
}

void Game::scoreBed(int i) {
    fill_[i] = 0;
    bloom_[i] = 0.45f;
    you_++;
    tone(1, 392.f + float(you_) * 28.f, 0.2f);
    if (sys_) sys_->rumble(0.12f, 0.3f, 40);
}

void Game::finish() {
    if (you_ >= GOAL) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        tone(2, 523.f, 0.22f);
        tone(3, 659.f, 0.18f);
        tone(4, 784.f, 0.16f);
        if (sys_) sys_->setLight(40, 140, 60);
        return;
    }
    if (them_ >= GOAL) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        tone(2, 110.f, 0.24f);
        if (sys_) sys_->setLight(160, 40, 20);
    }
}

void Game::botAim(float& ax, bool& pour) {
    int pick = nearest(px_);
    float best = 0;
    for (int i = 0; i < BEDS; i++) {
        if (fill_[i] > best) {
            best = fill_[i];
            pick = i;
        }
    }
    float c = BX[pick] + BW * 0.5f;
    float d = c - px_;
    if (std::fabs(d) > 5.f) {
        ax = d > 0 ? 1.f : -1.f;
        pour = false;
    } else {
        ax = 0;
        pour = true;
    }
}

void Game::play(float dt) {
    float ax = 0;
    bool pour = false;
    if (bot_) {
        botAim(ax, pour);
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) ax -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) ax += 1.f;
        if (std::fabs(pad.axisX) > 0.2f) ax = pad.axisX;
        pour = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.accel > 0.2f;
        if (ax > 1.f) ax = 1.f;
        if (ax < -1.f) ax = -1.f;
    }
    if (ax < -0.15f) face_ = -1;
    else if (ax > 0.15f) face_ = 1;
    if (ax != 0.f) walk_ += dt;
    else walk_ = 0;
    px_ = std::clamp(px_ + ax * SPEED * dt, 24.f, 250.f);

    int here = nearest(px_);
    float c = BX[here] + BW * 0.5f;
    bool on = pour && std::fabs(px_ - c) < 26.f;
    pourBed_ = on ? here : -1;
    if (on) {
        fill_[here] += POUR * dt;
        if ((int(t_ * 18.f) & 1) == 0) {
            Drop d;
            d.x = c + ((int(t_ * 40.f) & 7) - 3) * 3.f;
            d.y = BY - 6.f;
            d.vy = 50.f;
            d.life = 0.35f;
            drops_.push_back(d);
        }
        if (fill_[here] >= 1.f) scoreBed(here);
    }

    rivalWait_ -= dt;
    if (rivalWait_ <= 0.f) {
        rivalFill_ += RIVAL * dt;
        if (rivalFill_ >= 1.f) {
            rivalFill_ = 0;
            them_++;
            rivalWait_ = 0.55f;
            tone(5, 220.f, 0.08f);
        }
    }

    for (int i = 0; i < BEDS; i++)
        if (bloom_[i] > 0) bloom_[i] -= dt;
    for (auto& d : drops_) {
        d.y += d.vy * dt;
        d.life -= dt;
    }
    drops_.erase(std::remove_if(drops_.begin(), drops_.end(), [](const Drop& d) { return d.life <= 0; }), drops_.end());

    finish();
}

void Game::put(const gs::Mipped& m, float x, float y, float w, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || m.h < 1 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < 88) {
            float k = y / 88.f;
            int r = int(4 + k * 6);
            int g = int(8 + k * 3);
            int b = int(14 - k * 4);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else {
            v.lineBackdrop[y] = gs::rgb4(3, 6, 2);
        }
    }

    auto banner = [&](const gs::Mipped& m, float y, int pal) {
        put(m, 160.f - m.w * 0.5f, y, float(m.w), float(m.h), pal);
    };
    if (mode_ == Mode::Title) {
        banner(art_.title, 28, PAL_GOLD);
        banner(art_.sub, 78, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        banner(art_.win, 28, PAL_GOOD);
    } else if (mode_ == Mode::Lose) {
        banner(art_.lose, 28, PAL_WARN);
    }

    for (const Drop& d : drops_) put(art_.drop, d.x, d.y, 6, 8, PAL_WATER);

    bool pouring = pourBed_ >= 0;
    int pose = pouring ? 1 : (walk_ > 0.05f && (int(walk_ * 8.f) & 1) ? 1 : 0);
    if (!pouring && walk_ > 0.05f) pose = (int(walk_ * 8.f) & 1);
    float manX = (mode_ == Mode::Title) ? 150.f + std::sin(t_ * 1.4f) * 18.f : px_;
    float manY = 168.f;
    put(art_.shade, manX - 10, manY - 2, 20, 6, PAL_MAN, false, true);
    put(art_.man[pose ? 1 : 0], manX - 14, manY - 36, 28, 36, PAL_MAN, face_ < 0);
    if (pouring) put(art_.can, manX + (face_ < 0 ? -20.f : 8.f), manY - 24, 16, 14, PAL_CAN, face_ < 0);

    for (int i = BEDS - 1; i >= 0; i--) {
        float x = BX[i];
        int stage = fill_[i] > 0.66f ? 2 : fill_[i] > 0.28f ? 1 : 0;
        if (bloom_[i] > 0) stage = 2;
        float ph = 18.f + stage * 6.f;
        put(art_.sprout[stage], x + 16, BY - ph + 18, 40, ph, PAL_LEAF);
        int soilPal = fill_[i] > 0.45f ? PAL_WATER : PAL_SOIL;
        put(art_.soil, x + 10, BY + 16, 52, 14, soilPal);
        put(art_.bed, x, BY, BW, 40, PAL_WOOD);
        float bar = std::clamp(fill_[i], 0.f, 1.f);
        if (bar > 0.02f) put(art_.pipOn, x + 8, BY + 42, std::max(8.f, 56.f * bar), 6, PAL_WATER);
    }

    put(art_.rival, 276, 150, 24, 32, PAL_RIVAL);
    put(art_.bed, 262, 118, 52, 28, PAL_WOOD);
    if (rivalFill_ > 0.02f) put(art_.pipOn, 270, 148, std::max(6.f, 36.f * rivalFill_), 5, PAL_WARN);

    if (mode_ != Mode::Title) {
        char you[16];
        char them[16];
        std::snprintf(you, sizeof(you), "YOU %d", you_);
        std::snprintf(them, sizeof(them), "THEM %d", them_);
        hud(1, 1, you, PAL_GOOD);
        hud(30, 1, them, PAL_WARN);
        hudC(26, "FIRST TO 7", PAL_HUD);
        for (int i = 0; i < GOAL; i++) {
            const gs::Mipped& pip = i < you_ ? art_.pipOn : art_.pipOff;
            put(pip, 96.f + i * 12.f, 12.f, 8, 8, PAL_PIP);
        }
    } else {
        hudC(22, "ARROWS MOVE", PAL_DIM);
        hudC(24, "C WATERS A BED", PAL_DIM);
        hudC(26, "START", PAL_GOLD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (mode_ == Mode::Title) {
        bool go = bot_ && t_ > 0.35f;
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C))) go = true;
        if (go) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
            hold_ = hold_ > 0 ? 0 : 1;
        }
        if (hold_ <= 0.f) play(DT);
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C))) {
        t_ = 0;
        mode_ = Mode::Title;
        over_ = false;
        won_ = false;
    }
    draw();
}

}  // namespace bedsseven
