#include "game/granary.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace granary {

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(6, 5, 3));
    sys.apu.silence();
    mode_ = Mode::Title;
    anim_ = 0;
}

void Game::begin() {
    pace_ = step_ = hold_ = early_ = shots_ = anim_ = 0;
    fired_ = over_ = won_ = false;
    reason_ = "";
    mode_ = Mode::Pace;
    sys_->apu.silence();
}

void Game::fail(const char* why) {
    if (!won_) reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    anim_ = 0;
    if (sys_) sys_->apu.tone(0, 90.f, 0.08f);
}

void Game::shoot() {
    if (fired_ || mode_ != Mode::Pace) return;
    fired_ = true;
    shots_++;
    sys_->apu.noiseBurst(0.55f, 90.f, 0.18f);
    sys_->rumble(0.4f, 0.7f, 80);
    if (pace_ < kPaces) {
        early_++;
        fail("EARLY");
        return;
    }
    won_ = true;
    reason_ = "DONE";
    mode_ = Mode::Done;
    anim_ = 0;
    sys_->apu.keyOn(1, 392.f, 0.28f);
    sys_->apu.keyOn(2, 523.f, 0.22f);
}

void Game::tickPace() {
    if (pace_ >= kPaces) {
        if (!fired_) {
            hold_++;
            if (hold_ > kWindow) fail("LATE");
        }
        return;
    }
    step_++;
    if (step_ >= kStep) {
        step_ = 0;
        pace_++;
        sys_->apu.tone(0, 110.f, 0.05f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_++;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_) {
            if (anim_ > 8) begin();
        } else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            begin();
        }
    } else if (mode_ == Mode::Pace) {
        tickPace();
        bool fire = false;
        if (bot_) fire = pace_ >= kPaces && hold_ >= 12 && !fired_;
        else fire = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
        if (fire) shoot();
    } else if (mode_ == Mode::Done) {
        if (anim_ > 24) over_ = true;
    } else if (mode_ == Mode::Over) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            anim_ = 0;
            over_ = false;
        }
    }
    draw();
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
    uint16_t top = gs::rgb4(3, 5, 8);
    uint16_t mid = gs::rgb4(10, 8, 5);
    uint16_t bot = gs::rgb4(6, 5, 2);
    if (mode_ == Mode::Done) mid = gs::rgb4(12, 9, 4);
    if (mode_ == Mode::Over) mid = gs::rgb4(7, 3, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        uint16_t c = u < 0.62f ? mix(top, mid, u / 0.62f) : mix(mid, bot, (u - 0.62f) / 0.38f);
        if (y > 168) c = gs::rgb4(5, 4, 1);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    const float barnX = 214.f;
    const float barnY = 118.f;
    spr(art_.barn, barnX, barnY, PAL_BARN);
    spr(art_.sack, barnX - 36.f, 168.f, PAL_BARN);
    spr(art_.sack, barnX + 40.f, 170.f, PAL_BARN);

    for (int i = 0; i < 9; i++) {
        float x = 18.f + i * 34.f;
        float y = 188.f + ((i & 1) ? 4.f : 0.f);
        if (std::fabs(x - barnX) < 36.f) continue;
        spr(art_.stalk, x, y, PAL_WHEAT);
    }

    float along = float(pace_);
    if (pace_ < kPaces) along += step_ / float(kStep);
    if (along > float(kPaces)) along = float(kPaces);
    float manX = 78.f + along * kStride;
    float manY = 156.f;
    bool stepA = pace_ >= kPaces || ((step_ / 7) % 2 == 0);
    spr(stepA ? art_.manA : art_.manB, manX, manY, PAL_MAN, true);

    for (int p = 1; p <= pace_ && p <= kPaces; p++) spr(art_.boot, 78.f + p * kStride, 178.f, PAL_DUST);

    spr(art_.gun, 46.f, 150.f, PAL_GUN);
    if (fired_ && anim_ < 8) spr(art_.flash, 78.f, 146.f, PAL_GUN);

    char line[40];
    if (mode_ == Mode::Title) {
        hudC(4, "S3 GRANARYPACE", PAL_GOLD);
        hudC(8, "ONE GRANARY", PAL_INK);
        hudC(11, "WAIT FOR THE THIRD PACE", PAL_INK);
        hudC(13, "THEN FIRE", PAL_INK);
        hudC(22, "ENTER", PAL_DIM);
    } else if (mode_ == Mode::Pace) {
        std::snprintf(line, sizeof(line), "PACE %d / %d", pace_, kPaces);
        hudC(2, line, pace_ >= kPaces ? PAL_GOLD : PAL_INK);
        if (pace_ < kPaces) hudC(4, "HOLD YOUR FIRE", PAL_DIM);
        else hudC(4, "NOW", PAL_GOLD);
        hudC(25, "A FIRE", PAL_DIM);
    } else if (mode_ == Mode::Done) {
        hudC(3, "DONE", PAL_GOLD);
        hudC(6, "THE THIRD PACE", PAL_INK);
    } else {
        hudC(3, reason_ && reason_[0] ? reason_ : "FAIL", PAL_BAD);
        if (early_) hudC(6, "BEFORE THE THIRD PACE", PAL_INK);
        else hudC(6, "THE PACE PASSED", PAL_INK);
        hudC(22, "ENTER", PAL_DIM);
    }
}

}  // namespace granary
