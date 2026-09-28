#include "game/lot.h"

#include <string>

namespace lot {

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
}

void Game::spr(const gs::Image& img, int x, int y, int pal) {
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::paintSky() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r, g, b;
        if (y < 128) {
            r = 2 + y / 28;
            g = 2 + y / 40;
            b = 7 - y / 32;
        } else if (y < 150) {
            r = 9;
            g = 5;
            b = 3;
        } else {
            r = 3;
            g = 3;
            b = 3;
        }
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::begin() {
    mode_ = Mode::Walk;
    pace_ = 0;
    walk_ = 0;
    since_ = 0;
    dust_ = 0;
    flash_ = 0;
    over_ = false;
    won_ = false;
    why_ = "";
    sys_->apu.tone(0, 0, 0);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.clearSprites();
    sys.vdp.HUD.clear();
    paintSky();

    bool fireEdge = sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_C);
    if (bot_ && mode_ == Mode::Walk && pace_ == 3 && since_ == 4) fireEdge = true;

    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || bot_) begin();
    } else if (mode_ == Mode::Walk) {
        walk_++;
        if (pace_ < 3 && walk_ >= kPaceAt[pace_]) {
            pace_++;
            since_ = 0;
            dust_ = 14;
            sys.apu.noiseBurst(0.45f, 8000.f, 0.08f);
            sys.apu.tone(0, 90.f + pace_ * 30.f, 0.2f);
        } else if (pace_ > 0) {
            since_++;
            if (since_ > 8) sys.apu.tone(0, 0, 0);
        }
        if (dust_ > 0) dust_--;

        if (fireEdge) {
            if (pace_ == 3 && since_ <= kWindow) {
                mode_ = Mode::Win;
                won_ = true;
                over_ = true;
                flash_ = 18;
                why_ = "THIRD PACE";
                sys.apu.tone(0, 440.f, 0.35f);
                sys.apu.tone(1, 660.f, 0.25f);
            } else {
                mode_ = Mode::Lose;
                won_ = false;
                over_ = true;
                why_ = pace_ < 3 ? "TOO SOON" : "TOO LATE";
                sys.apu.tone(0, 70.f, 0.4f);
                sys.apu.tone(1, 0, 0);
            }
        } else if (pace_ == 3 && since_ > kWindow) {
            mode_ = Mode::Lose;
            won_ = false;
            over_ = true;
            why_ = "TOO LATE";
            sys.apu.tone(0, 70.f, 0.4f);
        }
    } else if (sys.pad.pressed(gs::BTN_START) && !bot_) {
        mode_ = Mode::Title;
        over_ = false;
        won_ = false;
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }

    if (flash_ > 0) flash_--;

    spr(art_.fence, 0, 112, PAL_LOT);
    spr(art_.lot, 0, 140, PAL_LOT);

    int step = (walk_ / 8) & 1;
    int apart = pace_ * 28 + (pace_ < 3 && pace_ > 0 ? (since_ * 28) / (kPaceAt[pace_] - kPaceAt[pace_ - 1]) : 0);
    if (pace_ == 0) apart = 0;
    int youX = 148 - apart;
    int rivX = 168 + apart;
    if (youX < 16) youX = 16;
    if (rivX > 276) rivX = 276;
    spr(art_.you, youX, 96, PAL_MAN);
    spr(art_.rival[step], rivX, 96, PAL_MAN);

    for (int i = 0; i < 3; i++) {
        int mx = 150 - (i + 1) * 28;
        spr(art_.mark, mx, 148, i < pace_ ? PAL_GOLD : PAL_INK);
        int rx = 176 + (i + 1) * 28;
        spr(art_.mark, rx, 148, i < pace_ ? PAL_GOLD : PAL_INK);
    }
    if (dust_ > 0) {
        spr(art_.dust, youX + 4, 146, PAL_DUST);
        spr(art_.dust, rivX + 4, 146, PAL_DUST);
    }
    if (flash_ > 0) {
        gs::Sprite s;
        s.img = art_.dust;
        s.x = int16_t(youX + 28);
        s.y = 112;
        s.w = 22;
        s.h = 14;
        s.pal = PAL_FLASH;
        sys.vdp.sprite(s);
    }

    if (mode_ == Mode::Title) {
        hudC(3, "S3 LOTPACE", PAL_GOLD);
        hudC(6, "YOU HAVE THE LOT", PAL_INK);
        hudC(9, "WAIT FOR THE THIRD PACE", PAL_INK);
        hudC(12, "THEN FIRE", PAL_GOLD);
        hudC(18, "A FIRES   START WALKS", PAL_INK);
        if ((sys.frame / 30) & 1) hudC(22, "PRESS START", PAL_GREEN);
    } else if (mode_ == Mode::Walk) {
        hudC(2, "THE LOT", PAL_GOLD);
        const char* n = pace_ == 0 ? "HOLD" : pace_ == 1 ? "PACE 1" : pace_ == 2 ? "PACE 2" : "PACE 3";
        hudC(4, n, pace_ == 3 ? PAL_GREEN : PAL_INK);
        if (pace_ == 3 && since_ <= kWindow) hudC(20, "FIRE", PAL_GREEN);
        else hudC(20, "DO NOT FIRE", PAL_RED);
    } else if (mode_ == Mode::Win) {
        hudC(3, "THE LOT IS YOURS", PAL_GREEN);
        hudC(6, why_, PAL_GOLD);
        hudC(20, "THIRD PACE HELD", PAL_INK);
    } else {
        hudC(3, "THE LOT IS LOST", PAL_RED);
        hudC(6, why_, PAL_GOLD);
        hudC(20, "ANYTHING ELSE IS A LOSS", PAL_INK);
    }
}

}  // namespace lot
