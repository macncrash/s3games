#include "game/pressbell.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace pressbell {

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.22f); }

void Game::begin() {
    dead_ = attempt_ = age_ = anim_ = 0;
    over_ = won_ = rung_ = false;
    why_ = "";
    mode_ = Mode::Try;
    sys_->apu.silence();
}

void Game::pull(bool hit) {
    if (hit) {
        rung_ = true;
        why_ = "the bell rings before the third try dies";
        sys_->apu.keyOn(0, 494.f, 0.42f);
        sys_->apu.keyOn(1, 740.f, 0.26f);
        sys_->apu.noiseBurst(0.12f, 220.f, 0.07f);
        anim_ = 0;
        mode_ = Mode::Ring;
    } else {
        killTry();
    }
}

void Game::killTry() {
    dead_++;
    anim_ = 0;
    mode_ = Mode::Gap;
    sys_->apu.noiseBurst(0.2f, 60.f, 0.18f);
    why_ = dead_ >= kTries ? "the third try died" : "that try died";
    if (dead_ >= kTries) rung_ = false;
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::shop() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 1, 1);
    uint16_t mid = gs::rgb4(5, 3, 2);
    uint16_t bot = gs::rgb4(2, 2, 2);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(8, 6, 2);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 1, 1);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.55f ? mix(top, mid, u / 0.55f) : mix(mid, bot, (u - 0.55f) / 0.45f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    shop();

    const float px = 188.f;
    const float bed = 168.f;
    float swing = 0.f;
    if (rung_ && (mode_ == Mode::Ring || mode_ == Mode::Over)) swing = std::sin(anim_ * 0.42f) * 7.f;

    spr(art_.post, 48.f, 128.f, PAL_WOOD);
    spr(art_.bell, 48.f + swing, 78.f, PAL_BELL);
    spr(art_.frame, px, 118.f, PAL_WOOD);
    spr(art_.sheet, px, bed - 18.f, PAL_PAPER);
    spr(art_.roller, px - 52.f, bed - 6.f, PAL_INK);
    spr(art_.roller, px + 52.f, bed - 4.f, PAL_INK);

    float drop = 0.f;
    if (mode_ == Mode::Try) drop = 62.f * (age_ / float(kDieAt));
    else if (mode_ == Mode::Ring) drop = 62.f;
    else if (mode_ == Mode::Gap) drop = 8.f;
    spr(art_.screw, px, 78.f + drop * 0.35f, PAL_IRON);
    spr(art_.platen, px, 92.f + drop, PAL_IRON);

    for (int i = 0; i < kTries; i++) {
        int pal = i < dead_ ? PAL_BAD : (i == attempt_ && mode_ != Mode::Over ? PAL_GOLD : PAL_DIM);
        if (rung_ && i == attempt_) pal = PAL_GOLD;
        spr(art_.lamp, 250.f + i * 16.f, 28.f, pal);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(2, "S3 PRESS BELL", PAL_GOLD);
        hudC(20, "PULL THE PLATEN", PAL_HUD);
        hudC(21, "THE BELL RINGS", PAL_GOLD);
        hudC(22, "BEFORE THE THIRD TRY DIES", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(2, "THE BELL RINGS", PAL_GOLD);
        hudC(21, "BEFORE THE THIRD TRY DIED", PAL_HUD);
        std::snprintf(buf, sizeof buf, "DEAD %d  TRY %d", dead_, attempt_ + 1);
        hudC(23, buf, PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(2, "THE THIRD TRY DIED", PAL_BAD);
        hudC(22, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "TRY %d/%d", attempt_ + 1, kTries);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "DEAD %d", dead_);
        hud(28, 1, buf, dead_ ? PAL_BAD : PAL_DIM);
        if (mode_ == Mode::Try) {
            int mark = age_ * 20 / kDieAt;
            if (mark > 20) mark = 20;
            char bar[24];
            for (int i = 0; i < 20; i++) {
                bool sweet = (i * kDieAt / 20) >= kSweetLo && (i * kDieAt / 20) <= kSweetHi;
                bar[i] = (i == mark) ? '|' : (sweet ? '=' : '-');
            }
            bar[20] = 0;
            hudC(24, bar, PAL_GOLD);
            hudC(26, "C PULLS THE PRESS", PAL_DIM);
        } else if (mode_ == Mode::Ring) {
            hudC(24, "THE BELL RINGS", PAL_GOLD);
        } else if (dead_ < kTries) {
            hudC(24, "THAT TRY DIED", PAL_BAD);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Try) {
        bool tap = false;
        if (bot_) {
            if (dead_ < kTries - 1) tap = age_ == 10;
            else tap = age_ == (kSweetLo + kSweetHi) / 2;
        } else {
            tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
        }
        if (tap) {
            pull(age_ >= kSweetLo && age_ <= kSweetHi);
        } else if (++age_ >= kDieAt) {
            pull(false);
        }
    } else if (mode_ == Mode::Ring) {
        anim_++;
        if (anim_ >= 40) {
            won_ = rung_ && dead_ < kTries && attempt_ == dead_;
            over_ = true;
            mode_ = Mode::Over;
            if (won_) {
                why_ = "the bell rings before the third try dies";
                blip(659.f);
                sys_->apu.keyOn(1, 880.f, 0.22f);
            }
        }
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (anim_ >= 24) {
            if (dead_ >= kTries) {
                won_ = false;
                over_ = true;
                mode_ = Mode::Over;
                why_ = "the third try died";
            } else {
                attempt_++;
                age_ = 0;
                anim_ = 0;
                mode_ = Mode::Try;
            }
        }
    } else if (mode_ == Mode::Over) {
        anim_++;
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    draw();
}

}  // namespace pressbell
