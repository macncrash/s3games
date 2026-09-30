#include "game/hornbell.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace hornbell {

void Game::begin() {
    dead_ = attempt_ = age_ = anim_ = 0;
    over_ = won_ = rung_ = false;
    why_ = "";
    mode_ = Mode::Breath;
    sys_->apu.silence();
}

void Game::blow(bool hit) {
    if (hit && attempt_ == kTries - 1 && dead_ == kTries - 1) {
        rung_ = true;
        why_ = "the bell rings before the third try dies";
        sys_->apu.keyOn(0, 440.f, 0.36f);
        sys_->apu.keyOn(1, 880.f, 0.22f);
        sys_->apu.keyOn(2, 1320.f, 0.1f);
        anim_ = 0;
        mode_ = Mode::Ring;
        return;
    }
    if (hit) {
        why_ = "the bell waits for the third try";
        sys_->apu.keyOn(0, 330.f, 0.12f);
    }
    killTry();
}

void Game::killTry() {
    if (why_[0] == 0) why_ = "that try died";
    if (!rung_) {
        if (dead_ + 1 >= kTries) why_ = "the third try died";
        else if (why_[0] == 0 || std::strcmp(why_, "the bell waits for the third try") != 0) why_ = "that try died";
    }
    dead_++;
    anim_ = 0;
    mode_ = Mode::Gap;
    sys_->apu.noiseBurst(0.18f, 90.f, 0.12f);
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
    uint16_t top = gs::rgb4(1, 2, 4);
    uint16_t mid = gs::rgb4(3, 4, 6);
    uint16_t bot = gs::rgb4(2, 3, 2);
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
        v.lineBackdrop[y] = u < 0.5f ? mix(top, mid, u / 0.5f) : mix(mid, bot, (u - 0.5f) / 0.5f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    float swing = 0.f;
    if (rung_) swing = std::sin(anim_ * 0.35f) * 10.f;
    spr(art_.stand, 248.f, 130.f, PAL_YARD);
    spr(art_.yoke, 248.f, 78.f, PAL_YARD);
    spr(art_.bell, 248.f + swing, 100.f, rung_ ? PAL_GOLD : PAL_BELL);

    float puff = 0.f;
    if (mode_ == Mode::Breath) puff = std::sin(age_ * 0.2f) * 2.f;
    if (mode_ == Mode::Ring) puff = 6.f - anim_ * 0.15f;
    spr(art_.player, 70.f, 148.f, PAL_COAT);
    spr(art_.horn, 128.f + puff, 138.f, PAL_BRASS);

    for (int i = 0; i < kTries; i++) {
        int pal = PAL_DIM;
        if (i < dead_) pal = PAL_BAD;
        else if (i == attempt_ && mode_ != Mode::Over) pal = PAL_GOLD;
        if (rung_ && i == attempt_) pal = PAL_GOLD;
        spr(art_.lamp, 120.f + i * 22.f, 40.f, pal);
    }

    if (mode_ == Mode::Breath) {
        int filled = age_ * 28 / kDieAt;
        if (filled < 1) filled = 1;
        if (filled > 28) filled = 28;
        gs::Sprite bar;
        bar.img = art_.stand;
        bar.w = 6;
        bar.h = int16_t(filled);
        bar.x = 20;
        bar.y = int16_t(160 - filled);
        const bool sweet = age_ >= kSweetLo && age_ <= kSweetHi;
        bar.pal = sweet ? PAL_GOLD : PAL_BRASS;
        v.sprite(bar);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(2, "S3 HORN BELL", PAL_GOLD);
        hudC(20, "BLOW THE HORN", PAL_HUD);
        hudC(21, "THE BELL RINGS", PAL_GOLD);
        hudC(22, "BEFORE THE THIRD TRY DIES", PAL_HUD);
        if ((f / 16) % 2 == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(2, "THE BELL RINGS", PAL_GOLD);
        hudC(21, "BEFORE THE THIRD TRY DIED", PAL_HUD);
        std::snprintf(buf, sizeof buf, "DEAD %d  TRY %d", dead_, attempt_ + 1);
        hudC(23, buf, PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(2, "STILL AT THE HORN", PAL_BAD);
        hudC(22, why_, PAL_HUD);
        if ((f / 16) % 2 == 0) hudC(26, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "TRY %d/%d", attempt_ + 1, kTries);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "DEAD %d", dead_);
        hud(30, 1, buf, dead_ ? PAL_BAD : PAL_DIM);
        if (mode_ == Mode::Pause) hudC(24, "PAUSED", PAL_GOLD);
        else if (mode_ == Mode::Breath) {
            int mark = age_ * 20 / kDieAt;
            if (mark > 20) mark = 20;
            char bar[24];
            for (int i = 0; i < 20; i++) {
                int at = i * kDieAt / 20;
                bool sweet = at >= kSweetLo && at <= kSweetHi;
                bar[i] = (i == mark) ? '|' : (sweet ? '=' : '-');
            }
            bar[20] = 0;
            hudC(23, bar, PAL_GOLD);
            if (attempt_ == kTries - 1 && dead_ == kTries - 1) hudC(25, "SOUND IT BEFORE IT DIES", PAL_GOLD);
            else hudC(25, "THIS TRY IS NOT THE BELL", PAL_DIM);
            hudC(26, "C BLOWS", PAL_DIM);
        } else if (mode_ == Mode::Ring) {
            hudC(24, "THE BELL RINGS", PAL_GOLD);
        } else {
            hudC(24, why_, PAL_BAD);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 2, 3));
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Breath) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Breath;
            mode_ = Mode::Pause;
        } else {
            bool tap = false;
            if (bot_) {
                if (dead_ < kTries - 1) tap = age_ == 6;
                else tap = age_ == (kSweetLo + kSweetHi) / 2;
            } else {
                tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
            }
            if (tap) {
                const bool hit = age_ >= kSweetLo && age_ <= kSweetHi;
                why_ = "";
                blow(hit);
            } else if (++age_ >= kDieAt) {
                why_ = "";
                blow(false);
            }
        }
    } else if (mode_ == Mode::Ring) {
        anim_++;
        if (anim_ >= 48) {
            won_ = rung_ && dead_ == kTries - 1 && attempt_ == kTries - 1;
            over_ = true;
            mode_ = Mode::Over;
            if (won_) why_ = "the bell rings before the third try dies";
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
                why_ = "";
                mode_ = Mode::Breath;
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        anim_++;
        if (!bot_ && !won_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) begin();
    }

    draw();
}

}  // namespace hornbell
