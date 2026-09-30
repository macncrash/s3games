#include "game/sally.h"

#include <cmath>
#include <cstdio>

namespace sally {

static void putSpr(gs::VDP& vdp, gs::Image img, int x, int y, int pal, bool flip) {
    if (img.w < 1 || img.h < 1) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    vdp.sprite(s);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    art_.build(sys.vdp);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.HUD.enabled = true;
    sys.vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
    reason_.clear();
    youX_ = 132.f;
    foeX_ = 188.f;
}

void Game::begin() {
    mode_ = Mode::Duel;
    age_ = 0;
    pace_ = 0;
    call_ = 70;
    walk_ = 0;
    window_ = 0;
    resolved_ = false;
    youShot_ = false;
    foeShot_ = false;
    over_ = false;
    won_ = false;
    reason_.clear();
    youX_ = youFrom_ = youTo_ = 132.f;
    foeX_ = foeFrom_ = foeTo_ = 188.f;
}

bool Game::wantFire() const {
    if (bot_) return window_ > 0;
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_Z);
}

void Game::blip(int ch, float freq, float vol) {
    sys_->apu.tone(ch, freq, vol);
    tone_ = 10;
}

void Game::shot() { sys_->apu.noiseBurst(0.45f, 14000.f, 0.86f); }

void Game::update() {
    age_++;
    if (tone_ > 0 && --tone_ == 0) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
    }
    if (resolved_) return;

    if (walk_ > 0) {
        float u = 1.f - float(walk_ - 1) / 18.f;
        youX_ = youFrom_ + (youTo_ - youFrom_) * u;
        foeX_ = foeFrom_ + (foeTo_ - foeFrom_) * u;
        walk_--;
    }

    if (call_ > 0) call_--;
    if (pace_ < 3 && call_ == 0 && window_ == 0 && walk_ == 0) {
        pace_++;
        youFrom_ = youX_;
        foeFrom_ = foeX_;
        youTo_ = 132.f - float(pace_) * 36.f;
        foeTo_ = 188.f + float(pace_) * 36.f;
        walk_ = 18;
        blip(0, 90.f + float(pace_) * 30.f, 0.18f);
        if (pace_ < 3) call_ = 64;
        else window_ = 40;
    }

    if (wantFire()) {
        resolved_ = true;
        if (window_ > 0 && pace_ == 3) {
            won_ = true;
            youShot_ = true;
            reason_ = "FIRED ON THE THIRD PACE";
            blip(1, 520.f, 0.12f);
        } else {
            won_ = false;
            youShot_ = true;
            reason_ = pace_ < 3 ? "EARLY. THE SALLY WAS NOT YOURS YET" : "MISSED THE THIRD PACE";
            blip(1, 70.f, 0.16f);
        }
        shot();
        return;
    }

    if (window_ > 0) {
        window_--;
        if (window_ == 0) {
            resolved_ = true;
            won_ = false;
            foeShot_ = true;
            reason_ = "TOO LATE. HE FIRED";
            shot();
            blip(1, 70.f, 0.16f);
        }
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = float(y) / float(gs::SCREEN_H - 1);
        int r = int(2 + (12 - 2) * (1.f - t) * 0.55f + 6.f * (1.f - t));
        int g = int(2 + 5.f * (1.f - t));
        int b = int(6 + 6.f * t);
        if (r > 15) r = 15;
        if (g > 15) g = 15;
        if (b > 15) b = 15;
        if (y > 150) {
            r = 3;
            g = 5;
            b = 2;
        }
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::ground() {
    gs::VDP& v = sys_->vdp;
    v.B.clear();
    for (int y = 19; y < 28; y++) {
        int tile = y == 19 ? 201 : 200;
        for (int x = 0; x < 40; x++) v.B.set(x, y, gs::entry(tile, PAL_GND));
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.fontBase + (c - 32), pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::figure(float cx, int pal, bool flip, bool raised, bool smoked) {
    gs::VDP& v = sys_->vdp;
    int bob = (walk_ > 0 && ((age_ / 4) & 1)) ? -2 : 0;
    int x = int(std::lround(cx)) - art_.body.w / 2;
    int y = 108 + bob;
    putSpr(v, art_.body, x, y, pal, flip);
    int gx = flip ? x + art_.body.w - 8 : x - 10;
    int gy = y + (raised || smoked ? 30 : 44);
    putSpr(v, art_.gun, gx, gy, pal, flip);
    if (smoked) {
        int sx = flip ? gx + 16 : gx - 16;
        putSpr(v, art_.smoke, sx, gy - 6, PAL_FX, flip);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    ground();

    putSpr(v, art_.sun, 250, 18, PAL_SUN, false);
    putSpr(v, art_.tree, 8, 112, PAL_GND, false);
    putSpr(v, art_.tree, 292, 116, PAL_GND, false);
    putSpr(v, art_.tree, 36, 120, PAL_GND, false);

    bool third = pace_ == 3 && window_ > 0 && !resolved_;
    figure(youX_, PAL_YOU, false, third || youShot_, youShot_);
    figure(foeX_, PAL_FOE, true, foeShot_, foeShot_);

    for (int i = 0; i < 3; i++) {
        int lit = i < pace_ ? PAL_GOLD : PAL_INK;
        putSpr(v, art_.pip, 128 + i * 28, 96, lit, false);
    }

    hudC(1, "S3 SALLY PACE", PAL_GOLD);
    if (mode_ == Mode::Title) {
        hudC(4, "YOU HAVE THE SALLY", PAL_INK);
        hudC(6, "WAIT FOR THE THIRD PACE", PAL_INK);
        hudC(8, "THEN FIRE", PAL_GOLD);
        hudC(24, "A START", PAL_INK);
        hudC(26, "ANYTHING ELSE IS A LOSS", PAL_BAD);
    } else if (mode_ == Mode::Duel && !resolved_) {
        hudC(4, "THE SALLY IS IN YOUR HAND", PAL_INK);
        if (pace_ == 0) hudC(23, "HOLD", PAL_INK);
        else if (pace_ < 3) {
            char buf[16];
            std::snprintf(buf, sizeof buf, "PACE %d", pace_);
            hudC(23, buf, PAL_GOLD);
        } else if (window_ > 0) {
            hudC(23, "THIRD PACE  FIRE", PAL_GOOD);
        }
        hudC(26, "A FIRES THE SALLY", PAL_INK);
    } else {
        hudC(4, won_ ? "THE FIELD IS YOURS" : "LOSS", won_ ? PAL_GOOD : PAL_BAD);
        hudC(7, reason_, won_ ? PAL_GOLD : PAL_BAD);
        if (!bot_) hudC(26, "A AGAIN", PAL_INK);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        age_++;
        bool go = sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C);
        if (bot_ && age_ > 24) go = true;
        draw();
        if (go) begin();
        return;
    }
    if (mode_ == Mode::Duel) {
        update();
        draw();
        if (resolved_) {
            mode_ = Mode::Over;
            over_ = true;
        }
        return;
    }
    draw();
    if (!bot_ && (sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_START))) begin();
}

}  // namespace sally
