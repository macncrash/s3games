#include "game/fishseven.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace fishseven {

float Game::wrap(float x) {
    while (x < 0) x += 320.f;
    while (x >= 320.f) x -= 320.f;
    return x;
}

float Game::wrapDelta(float to, float from) {
    float d = to - from;
    while (d > 160.f) d -= 320.f;
    while (d < -160.f) d += 320.f;
    return d;
}

void Game::blip(float hz, float vol) {
    if (!sys_) return;
    sys_->apu.tone(0, hz, vol);
}

void Game::seed() {
    const float x[5] = {40, 110, 180, 250, 300};
    const float y[5] = {100, 128, 156, 184, 140};
    const float vx[5] = {1.15f, -1.05f, 1.25f, -0.9f, 1.4f};
    for (int i = 0; i < 5; i++) {
        fish_[i].x = x[i];
        fish_[i].y = y[i];
        fish_[i].vx = vx[i];
    }
    you_ = them_ = 0;
    lureOn_ = false;
    hooked_ = target_ = -1;
    boatX_ = 48;
    rivalX_ = 240;
    rivalT_ = 0;
    call_[0] = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.clear();
    seed();
    phase_ = bot_ ? Phase::Cast : Phase::Title;
    over_ = won_ = false;
    t_ = 0;
}

int Game::marker() const {
    if (phase_ == Phase::Title) return 0;
    if (phase_ == Phase::Win) return 3;
    if (phase_ == Phase::Lose) return 4;
    if (phase_ == Phase::Call) return 2;
    return 1;
}

void Game::stamp(const gs::Image& img, float x, float y, int pal, bool flip) {
    if (!sys_) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int x, int y, const char* s, int pal) {
    int cx = x;
    for (const char* p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c < 32 || c > 127) c = ' ';
        int gi = c - 32;
        stamp(art_.glyph[gi], float(cx), float(y), pal);
        cx += art_.glyph[gi].w + 1;
    }
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < 52) v.lineBackdrop[y] = gs::rgb4(6, 10, 14);
        else if (y < 72) v.lineBackdrop[y] = gs::rgb4(4, 9, 13);
        else if (y < 118) v.lineBackdrop[y] = gs::rgb4(2, 7, 12);
        else if (y < 168) v.lineBackdrop[y] = gs::rgb4(1, 5, 10);
        else v.lineBackdrop[y] = gs::rgb4(1, 3, 8);
    }
}

void Game::moveFish() {
    for (int i = 0; i < 5; i++) {
        if (i == hooked_) continue;
        fish_[i].x = wrap(fish_[i].x + fish_[i].vx);
    }
}

void Game::botSteer() {
    if (lureOn_ || phase_ != Phase::Cast) return;
    target_ = 0;
    float best = 1e9f;
    for (int i = 0; i < 5; i++) {
        float d = std::fabs(wrapDelta(fish_[i].x, boatX_));
        if (d < best) {
            best = d;
            target_ = i;
        }
    }
    Fish& g = fish_[target_];
    int steps = 0;
    float y = 58.f;
    while (y + 5.f < g.y && steps < 48) {
        y += 5.f;
        steps++;
    }
    if (steps < 1) steps = 1;
    float pred = wrap(g.x + g.vx * float(steps));
    float d = wrapDelta(pred, boatX_);
    if (std::fabs(d) <= 2.4f) {
        boatX_ = pred;
        lureOn_ = true;
        lureX_ = pred;
        lureY_ = 58.f;
        blip(360.f, 0.05f);
    } else {
        float step = d;
        if (step > 4.2f) step = 4.2f;
        if (step < -4.2f) step = -4.2f;
        boatX_ = wrap(boatX_ + step);
    }
}

void Game::humanSteer() {
    if (!sys_) return;
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT)) boatX_ = wrap(boatX_ - 2.6f);
    if (p.down(gs::BTN_RIGHT)) boatX_ = wrap(boatX_ + 2.6f);
    if (!lureOn_ && phase_ == Phase::Cast && (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_DOWN))) {
        lureOn_ = true;
        lureX_ = boatX_ + 14.f;
        lureY_ = 58.f;
        target_ = -1;
        blip(360.f, 0.05f);
    }
    if (lureOn_ && p.down(gs::BTN_LEFT)) lureX_ = wrap(lureX_ - 1.4f);
    if (lureOn_ && p.down(gs::BTN_RIGHT)) lureX_ = wrap(lureX_ + 1.4f);
    if (lureOn_ && p.pressed(gs::BTN_B)) lureOn_ = false;
}

void Game::moveLure() {
    if (!lureOn_) return;
    if (bot_ && target_ >= 0) lureX_ = fish_[target_].x + 6.f;
    lureY_ += 5.f;
    for (int i = 0; i < 5; i++) {
        float dx = wrapDelta(fish_[i].x + 8.f, lureX_);
        float dy = fish_[i].y - lureY_;
        if (std::fabs(dx) < 12.f && std::fabs(dy) < 9.f) {
            land(i);
            return;
        }
    }
    if (lureY_ > 208.f) {
        lureOn_ = false;
        std::snprintf(call_, sizeof call_, "MISSED");
        phase_ = Phase::Call;
        callT_ = 0;
        blip(130.f, 0.04f);
    }
}

void Game::land(int i) {
    lureOn_ = false;
    hooked_ = -1;
    you_++;
    fish_[i].x = wrap(boatX_ + 90.f + float(i) * 17.f);
    fish_[i].y = 96.f + float((i * 17) % 90);
    fish_[i].vx = (i & 1) ? -1.1f : 1.2f;
    std::snprintf(call_, sizeof call_, "KEEPER  %d", you_);
    blip(680.f, 0.06f);
    if (you_ >= 7 && them_ < 7) {
        finish(true);
        return;
    }
    phase_ = Phase::Call;
    callT_ = 0;
}

void Game::rivalTick() {
    if (phase_ != Phase::Cast && phase_ != Phase::Call) return;
    if (you_ >= 7 || them_ >= 7) return;
    rivalX_ = wrap(rivalX_ + 0.55f);
    rivalT_++;
    if (rivalT_ < 110) return;
    rivalT_ = 0;
    them_++;
    blip(220.f, 0.04f);
    if (them_ >= 7 && you_ < 7) finish(false);
}

void Game::finish(bool yours) {
    if (yours) {
        won_ = true;
        phase_ = Phase::Win;
        std::snprintf(call_, sizeof call_, "FIRST TO SEVEN");
        blip(880.f, 0.07f);
    } else {
        won_ = false;
        phase_ = Phase::Lose;
        std::snprintf(call_, sizeof call_, "THEY GOT SEVEN");
        blip(110.f, 0.06f);
    }
    callT_ = 0;
    lureOn_ = false;
}

void Game::playTick() {
    moveFish();
    if (bot_) botSteer();
    else humanSteer();
    moveLure();
    rivalTick();
}

void Game::draw() {
    sys_->vdp.clearSprites();
    backdrop();
    stamp(art_.sun, 270, 8, PAL_SUN);
    stamp(art_.reed, 6, 48, PAL_REED);
    stamp(art_.reed, 300, 52, PAL_REED);
    stamp(art_.boat, boatX_ - 10, 46, PAL_BOAT);
    stamp(art_.angler, boatX_ + 8, 30, PAL_BOAT);
    stamp(art_.boat, rivalX_ - 10, 46, PAL_RIVAL);
    stamp(art_.angler, rivalX_ + 8, 30, PAL_RIVAL);
    for (int i = 0; i < 5; i++) {
        bool flip = fish_[i].vx < 0;
        float x = fish_[i].x;
        if (flip) x -= 8;
        stamp(art_.fish, x, fish_[i].y, (i & 1) ? PAL_RIVAL : PAL_FISH, flip);
    }
    if (lureOn_) {
        stamp(art_.lure, lureX_, lureY_, PAL_LURE);
    }
    char line[48];
    std::snprintf(line, sizeof line, "YOU %d   THEM %d   FIRST 7", you_, them_);
    hud(8, 6, line, PAL_HUD);
    if (phase_ == Phase::Title) {
        hud(86, 96, "S3 FISH SEVEN", PAL_INK);
        hud(70, 112, "FIRST TO SEVEN", PAL_HUD);
        hud(58, 132, "ARROWS  A CAST", PAL_HUD);
    } else if (phase_ == Phase::Call || phase_ == Phase::Win || phase_ == Phase::Lose) {
        hud(78, 100, call_, phase_ == Phase::Lose ? PAL_LURE : PAL_INK);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (phase_ == Phase::Title) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || bot_) {
            phase_ = Phase::Cast;
            blip(440.f, 0.05f);
        }
    } else if (phase_ == Phase::Cast) {
        playTick();
    } else if (phase_ == Phase::Call) {
        callT_++;
        moveFish();
        rivalTick();
        if (callT_ > 28) phase_ = Phase::Cast;
    } else if (phase_ == Phase::Win || phase_ == Phase::Lose) {
        callT_++;
        if (callT_ > 70) {
            over_ = true;
            if (phase_ == Phase::Win) won_ = true;
            sys.quit();
        }
    }
    if (phase_ != Phase::Cast) sys.apu.tone(0, 0, 0);
    draw();
}

}  // namespace fishseven
