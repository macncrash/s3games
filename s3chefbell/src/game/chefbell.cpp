#include "game/chefbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace chefbell {

void Game::blip(float freq) { sys_->apu.tone(1, freq, 0.045f); }

void Game::spr(const gs::Image& img, float cx, float cy, float h, int pal, bool flip) {
    if (img.w == 0 || img.h == 0 || h < 1.f) return;
    float w = h * float(img.w) / float(img.h);
    gs::Sprite s;
    s.img = img;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 400));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::solid(float x, float y, float w, float h, int pal) {
    if (w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = art_.solid;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::resetPan(int which) {
    heat_ = which == 1 ? 0.12f : (which == 2 ? 0.04f : 0.f);
    reach_ = 0;
    foul_ = 0;
}

void Game::begin() {
    rung_ = false;
    won_ = false;
    over_ = false;
    pause_ = false;
    dead_ = 0;
    tryNo_ = 1;
    swing_ = 0;
    hold_ = 0;
    resetPan(0);
    mode_ = Mode::Play;
    blip(392.f);
}

void Game::dieTry() {
    if (mode_ != Mode::Play || rung_) return;
    dead_++;
    foul_ = 28;
    swing_ = 0;
    sys_->apu.tone(0, 110.f, 0.08f);
    sys_->apu.noiseBurst(0.2f, 380.f, 0.1f);
    if (!bot_) sys_->rumble(0.45f, 0.15f, 80);
    if (dead_ >= kMaxDead) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        hold_ = 0;
        if (!bot_) sys_->setLight(140, 24, 24);
        return;
    }
    tryNo_ = dead_ + 1;
    resetPan(dead_);
    if (!bot_) sys_->setLight(160, 60, 20);
}

void Game::ring() {
    if (rung_ || dead_ >= kMaxDead || !inGold()) return;
    rung_ = true;
    won_ = true;
    tryNo_ = dead_ + 1;
    mode_ = Mode::Ring;
    hold_ = 0;
    swing_ = 18;
    reach_ = 0.2f;
    over_ = true;
    sys_->apu.tone(0, 523.f, 0.11f);
    sys_->apu.tone(2, 784.f, 0.08f);
    if (!bot_) {
        sys_->rumble(0.2f, 0.55f, 140);
        sys_->setLight(255, 196, 64);
    }
}

void Game::plate() {
    if (mode_ != Mode::Play || pause_ || rung_) return;
    reach_ = 0.16f;
    if (!inGold()) {
        dieTry();
        return;
    }
    ring();
}

void Game::advance(float dt) {
    if (mode_ != Mode::Play || pause_) return;
    heat_ += dt / kCook;
    if (heat_ >= 1.f) {
        heat_ = 1.f;
        dieTry();
    }
}

void Game::botAct() {
    if (mode_ != Mode::Play || pause_ || rung_) return;
    if (heat_ >= kSweet && heat_ < kGoldHi) plate();
}

void Game::human(const gs::Pad& pad) {
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_TURBO)) plate();
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t wall = gs::rgb4(6, 3, 2);
    uint16_t tile = gs::rgb4(12, 9, 7);
    uint16_t steel = gs::rgb4(5, 6, 7);
    uint16_t floor = gs::rgb4(3, 2, 2);
    if (mode_ == Mode::Ring) tile = gs::rgb4(14, 11, 6);
    if (mode_ == Mode::Lose) {
        wall = gs::rgb4(3, 1, 1);
        tile = gs::rgb4(6, 2, 2);
        steel = gs::rgb4(3, 2, 2);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c = wall;
        if (y >= 28 && y < 118) c = ((y / 8) & 1) ? tile : gs::rgb4((tile >> 8) - 1, ((tile >> 4) & 15) - 1, (tile & 15));
        else if (y >= 118 && y < 168) c = steel;
        else if (y >= 168) c = floor;
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.setFogColor(gs::rgb4(2, 1, 1));
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    float bob = std::sin(anim_ * 6.f) * 1.2f;
    int bellPal = (mode_ == Mode::Ring) ? PAL_GOLD : PAL_BELL;
    float swing = 0;
    if (swing_ > 0) swing = std::sin(anim_ * 22.f) * float(swing_);
    spr(art_.bell, 268 + swing * 0.35f, 36, 28, bellPal);
    spr(art_.clapper, 268 + swing, 50, 10, PAL_BELL);

    const gs::Image& pose = reach_ > 0 ? art_.chefReach : art_.chef;
    spr(pose, 108, 168 + bob, 72, PAL_CHEF);

    int foodPal = PAL_FOOD;
    if (mode_ == Mode::Lose || heat_ >= 0.92f) foodPal = PAL_RED;
    else if (inGold() || mode_ == Mode::Ring) foodPal = PAL_GOLD;
    float lift = (inGold() ? std::sin(anim_ * 8.f) * 1.5f : 0.f);
    if (mode_ != Mode::Title || true) spr(art_.steak, 196, 128 + lift, 30, foodPal);
    spr(art_.pan, 196, 146, 26, PAL_STEEL);
    spr(art_.flame[int(anim_ * 10.f) & 1], 188, 164, 16, PAL_FIRE);

    float barX = 120.f;
    float barW = 140.f;
    solid(barX, 78, barW, 8, PAL_BAR);
    float fw = barW * std::clamp(heat_, 0.f, 1.f);
    int fill = heat_ >= kGoldHi ? PAL_RED : (inGold() ? PAL_GOLD : PAL_BAR);
    if (fw >= 1.f) solid(barX, 78, fw, 8, fill == PAL_BAR ? PAL_RED : fill);
    solid(barX + barW * kGoldLo, 76, barW * (kGoldHi - kGoldLo), 2, PAL_OK);

    for (int i = 0; i < kMaxDead; i++) {
        int pal = i < dead_ ? PAL_RED : (i == dead_ && mode_ == Mode::Play ? PAL_GOLD : PAL_HUD);
        if (mode_ == Mode::Ring && i == dead_) pal = PAL_OK;
        solid(16.f + float(i) * 14.f, 16, 10, 8, pal);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 CHEFBELL", PAL_GOLD);
        hudC(5, "A SHORT CHEF", PAL_HUD);
        hudC(22, "PLATE THE CHOP IN THE GOLD", PAL_HUD);
        hudC(23, "BEFORE THE THIRD TRY DIES", PAL_HUD);
        hudC(25, "Z PLATES", PAL_GOLD);
        if ((int(anim_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_GOLD);
    } else {
        hud(1, 0, "S3 CHEFBELL", PAL_HUD);
        std::snprintf(buf, sizeof buf, "TRY %d", tryNo_);
        hud(40 - int(std::strlen(buf)), 0, buf, mode_ == Mode::Lose ? PAL_RED : PAL_GOLD);
        hudC(8, "CHOPS", PAL_HUD);
        if (mode_ == Mode::Play) {
            if (foul_ > 0) hudC(20, "TRY DIED", PAL_RED);
            else if (inGold()) hudC(20, "GOLD  PLATE", PAL_OK);
            else if (heat_ < kGoldLo) hudC(20, "STILL RAW", PAL_HUD);
            else hudC(20, "TOO HOT", PAL_RED);
            if (pause_) hudC(14, "PAUSED", PAL_GOLD);
        } else if (mode_ == Mode::Ring) {
            hudC(16, "BELL", PAL_GOLD);
            hudC(18, "BEFORE THE THIRD TRY DIED", PAL_OK);
        } else if (mode_ == Mode::Lose) {
            hudC(16, "THIRD TRY DIED", PAL_RED);
            hudC(18, "BELL SILENT", PAL_HUD);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.85f);
    rules_ = kMaxDead == 3 && kGoldLo > 0.2f && kGoldHi < 1.f && kSweet >= kGoldLo && kSweet < kGoldHi;
    heat_ = 0.4f;
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += 1.f / 60.f;
    if (reach_ > 0) reach_ -= 1.f / 60.f;
    if (swing_ > 0) swing_--;
    if (foul_ > 0) foul_--;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        heat_ = 0.62f + 0.12f * std::sin(anim_ * 1.4f);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) pause_ = !pause_;
        else if (!bot_ && pause_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            pause_ = false;
        } else if (!pause_) {
            if (bot_) {
                advance(1.f / 60.f);
                botAct();
            } else {
                human(pad);
                if (mode_ == Mode::Play) advance(1.f / 60.f);
            }
        }
    } else if ((mode_ == Mode::Ring || mode_ == Mode::Lose) && !bot_) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    }

    if (mode_ == Mode::Ring && !bot_) sys.setLight(255, 190, 50);
    else if (mode_ == Mode::Play && inGold() && !bot_) sys.setLight(220, 160, 40);
    draw();
}

}  // namespace chefbell
