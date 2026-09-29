#include "game/loom.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace loombell {

void Game::begin() {
    dead_ = attempt_ = age_ = anim_ = 0;
    dir_ = 1;
    over_ = won_ = rung_ = false;
    why_ = "";
    mode_ = Mode::Try;
    sys_->apu.silence();
}

void Game::cast(bool hit) {
    if (hit) {
        rung_ = true;
        why_ = "the bell rings before the third try dies";
        sys_->apu.keyOn(0, 494.f, 0.38f);
        sys_->apu.keyOn(1, 740.f, 0.26f);
        sys_->apu.noiseBurst(0.12f, 160.f, 0.06f);
        anim_ = 0;
        mode_ = Mode::Rise;
    } else {
        killTry();
    }
}

void Game::killTry() {
    dead_++;
    anim_ = 0;
    mode_ = Mode::Gap;
    sys_->apu.noiseBurst(0.2f, 60.f, 0.14f);
    why_ = dead_ >= kTries ? "the third try died" : "that try died";
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal, bool hflip) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    s.hflip = hflip;
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

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 2, 4);
    uint16_t mid = gs::rgb4(5, 3, 4);
    uint16_t bot = gs::rgb4(2, 1, 2);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(8, 6, 2);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 1, 2);
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
    sky();

    const float loomX = 160.f;
    const float loomY = 128.f;
    float swing = 0.f;
    if (rung_ && (mode_ == Mode::Rise || mode_ == Mode::Over)) swing = std::sin(anim_ * 0.42f) * 7.f;

    spr(art_.frame, loomX, loomY, PAL_WOOD);
    spr(art_.beam, loomX, 52.f, PAL_WOOD);
    spr(art_.bell, loomX + swing, 28.f, PAL_BELL);
    spr(art_.clapper, loomX + swing * 1.4f, 36.f, PAL_BELL);

    const float warp0 = 118.f;
    const float gap = 14.f;
    float shed = 0.f;
    if (mode_ == Mode::Try) shed = (age_ & 16) ? 5.f : -5.f;
    for (int c = 0; c < kWarps; c++) {
        float x = warp0 + c * gap;
        spr(art_.heddle, x, 108.f + ((c & 1) ? shed : -shed), PAL_WOOD);
        spr(art_.warp, x, 118.f, PAL_WARP);
    }

    int shown = 0;
    if (rung_) shown = mode_ == Mode::Rise ? anim_ / 6 : kPicks;
    if (shown > kPicks) shown = kPicks;
    for (int r = 0; r < shown; r++) {
        float y = 168.f - r * 8.f;
        for (int c = 0; c < kWarps - 1; c++) spr(art_.pick, warp0 + gap * 0.5f + c * gap, y, PAL_CLOTH);
    }

    float reedY = 150.f;
    if (mode_ == Mode::Try) reedY = 142.f + (age_ / float(kDieAt)) * 22.f;
    if (mode_ == Mode::Rise) reedY = 164.f - (anim_ < 12 ? anim_ : 12);
    spr(art_.reed, loomX, reedY, PAL_WOOD);

    float t = 0.f;
    if (mode_ == Mode::Try) t = age_ / float(kDieAt);
    else if (mode_ == Mode::Rise) t = 1.f;
    if (dir_ < 0) t = 1.f - t;
    float sx = 100.f + t * 120.f;
    bool sweet = mode_ == Mode::Try && age_ >= kSweetLo && age_ <= kSweetHi;
    spr(art_.shuttle, sx, 132.f, sweet ? PAL_CLOTH : PAL_SHUTTLE, dir_ < 0);

    for (int i = 0; i < kTries; i++) {
        int pal = PAL_DIM;
        if (i < dead_) pal = PAL_BAD;
        else if (i == attempt_ && mode_ != Mode::Over) pal = PAL_LAMP;
        if (rung_ && i == attempt_) pal = PAL_GOLD;
        spr(art_.lamp, 24.f + i * 14.f, 24.f, pal);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(2, "S3 LOOMBELL", PAL_GOLD);
        hudC(19, "A SHORT LOOM", PAL_INK);
        hudC(21, "THE BELL RINGS", PAL_GOLD);
        hudC(22, "BEFORE THE THIRD TRY DIES", PAL_INK);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(2, "THE BELL RINGS", PAL_GOLD);
        hudC(21, "BEFORE THE THIRD TRY DIED", PAL_INK);
        std::snprintf(buf, sizeof buf, "DEAD %d  TRY %d", dead_, attempt_ + 1);
        hudC(23, buf, PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(2, "THE THIRD TRY DIED", PAL_BAD);
        hudC(22, why_, PAL_INK);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "TRY %d/%d", attempt_ + 1, kTries);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "DEAD %d", dead_);
        hud(30, 1, buf, dead_ ? PAL_BAD : PAL_DIM);
        if (mode_ == Mode::Try) {
            int mark = age_ * 20 / kDieAt;
            if (mark > 19) mark = 19;
            char bar[24];
            for (int i = 0; i < 20; i++) {
                bool band = (i * kDieAt / 20) >= kSweetLo && (i * kDieAt / 20) <= kSweetHi;
                bar[i] = (i == mark) ? '|' : (band ? '=' : '-');
            }
            bar[20] = 0;
            hudC(24, bar, PAL_GOLD);
            hudC(26, "C THROWS THE SHUTTLE", PAL_DIM);
        } else if (mode_ == Mode::Rise) {
            hudC(24, "THE CLOTH TAKES", PAL_GOLD);
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
            if (dead_ < kTries - 1) tap = age_ == 8;
            else tap = age_ == (kSweetLo + kSweetHi) / 2;
        } else {
            tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
        }
        if (tap) {
            cast(age_ >= kSweetLo && age_ <= kSweetHi);
        } else if (++age_ >= kDieAt) {
            cast(false);
        }
    } else if (mode_ == Mode::Rise) {
        anim_++;
        if (anim_ == 8 || anim_ == 16) sys_->apu.keyOn(1, 880.f, 0.16f);
        if (anim_ >= 40) {
            won_ = rung_ && dead_ < kTries && attempt_ == dead_;
            over_ = true;
            mode_ = Mode::Over;
            if (won_) {
                why_ = "the bell rings before the third try dies";
                sys_->apu.keyOn(0, 659.f, 0.3f);
                sys_->apu.keyOn(1, 988.f, 0.2f);
            }
        }
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (anim_ >= 20) {
            if (dead_ >= kTries) {
                won_ = false;
                over_ = true;
                mode_ = Mode::Over;
                why_ = "the third try died";
            } else {
                attempt_++;
                dir_ = -dir_;
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

}  // namespace loombell
