#include "game/kilnbell.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace kilnbell {

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.22f); }

void Game::begin() {
    dead_ = attempt_ = age_ = anim_ = 0;
    over_ = won_ = rung_ = false;
    why_ = "";
    mode_ = Mode::Heat;
    sys_->apu.silence();
}

void Game::fire(bool hit) {
    if (hit) {
        rung_ = true;
        why_ = "the bell rings before the third try dies";
        sys_->apu.keyOn(0, 523.f, 0.4f);
        sys_->apu.keyOn(1, 784.f, 0.28f);
        sys_->apu.noiseBurst(0.12f, 220.f, 0.06f);
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
    sys_->apu.noiseBurst(0.22f, 70.f, 0.16f);
    blip(140.f);
    if (dead_ >= kTries) why_ = "the third try died";
    else why_ = "that try died";
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

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 1, 3);
    uint16_t mid = gs::rgb4(9, 3, 1);
    uint16_t bot = gs::rgb4(2, 2, 2);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(12, 8, 2);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 1, 1);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.45f ? mix(top, mid, u / 0.45f) : mix(mid, bot, (u - 0.45f) / 0.55f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    const float kx = 168.f;
    const float ky = 128.f;
    spr(art_.kiln, kx, ky, PAL_BRICK);

    float swing = 0.f;
    if (rung_ && (mode_ == Mode::Rise || mode_ == Mode::Over)) swing = std::sin(anim_ * 0.45f) * 7.f;
    spr(art_.bell, kx + swing, 28.f, PAL_BELL);

    const bool hot = mode_ != Mode::Gap || anim_ < 10;
    if (hot) {
        float fy = 168.f + 3.f * std::sin((sys_ ? sys_->frame : 0) * 0.25f);
        spr(art_.flame, kx, fy, PAL_FIRE);
    }

    const bool cracked = mode_ == Mode::Gap || (mode_ == Mode::Over && !won_);
    spr(cracked ? art_.crack : art_.pot, kx, 150.f, PAL_CLAY);

    spr(art_.bar, 160.f, 196.f, PAL_ASH);
    float tick = 94.f;
    if (mode_ == Mode::Heat) {
        float u = age_ / float(kDieAt);
        if (u > 1.f) u = 1.f;
        tick = 94.f + u * 132.f;
    } else if (rung_) {
        tick = 94.f + ((kSweetLo + kSweetHi) * 0.5f / kDieAt) * 132.f;
    }
    int conePal = PAL_GOLD;
    if (mode_ == Mode::Heat && age_ >= kSweetLo && age_ <= kSweetHi) conePal = PAL_ASH;
    spr(art_.cone, tick, 184.f, conePal);

    for (int i = 0; i < kTries; i++) {
        int pal = i < dead_ ? PAL_BAD : PAL_DIM;
        if (rung_ && i == attempt_) pal = PAL_GOLD;
        else if (!rung_ && i == attempt_ && mode_ != Mode::Over) pal = PAL_GOLD;
        spr(art_.lamp, 24.f + i * 16.f, 22.f, pal);
    }

    char buf[80];
    int f = sys_ ? int(sys_->frame) : 0;
    if (mode_ == Mode::Title) {
        hudC(2, "S3 KILN BELL", PAL_GOLD);
        hudC(20, "FIRE THE KILN", PAL_HUD);
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
        if (mode_ == Mode::Heat) {
            hudC(24, "A FIRES IN THE GREEN", PAL_HUD);
        } else if (mode_ == Mode::Rise) {
            hudC(24, "THE BELL CLIMBS", PAL_GOLD);
        } else if (dead_ < kTries) {
            hudC(24, "THAT TRY DIED", PAL_BAD);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
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
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Heat) {
        bool tap = false;
        if (bot_) {
            if (dead_ < kTries - 1) tap = age_ == 8;
            else tap = age_ == (kSweetLo + kSweetHi) / 2;
        } else {
            tap = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
        }
        if (tap) {
            const bool hit = age_ >= kSweetLo && age_ <= kSweetHi;
            fire(hit);
        } else if (++age_ >= kDieAt) {
            fire(false);
        }
    } else if (mode_ == Mode::Rise) {
        anim_++;
        if (anim_ >= 36) {
            won_ = rung_ && dead_ < kTries && attempt_ == dead_;
            over_ = true;
            mode_ = Mode::Over;
            if (won_) {
                why_ = "the bell rings before the third try dies";
                sys_->apu.keyOn(0, 659.f, 0.32f);
                sys_->apu.keyOn(1, 880.f, 0.22f);
            }
        }
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (anim_ >= 22) {
            if (dead_ >= kTries) {
                won_ = false;
                over_ = true;
                mode_ = Mode::Over;
                why_ = "the third try died";
            } else {
                attempt_++;
                age_ = 0;
                anim_ = 0;
                mode_ = Mode::Heat;
            }
        }
    } else if (mode_ == Mode::Over) {
        anim_++;
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    draw();
}

}  // namespace kilnbell
