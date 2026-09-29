#include "game/fishgold.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace fishgold {

static_assert(2 * 20 >= kLine && 2 * 10 < kLine && 20 < kLine, "gold double is the leave");

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
    const int kind[6] = {KIND_GOLD, KIND_CREAM, KIND_SILVER, KIND_GOLD, KIND_MINNOW, KIND_SILVER};
    const float x[6] = {48, 180, 20, 250, 140, 300};
    const float y[6] = {108, 132, 96, 168, 150, 186};
    const float vx[6] = {1.15f, -1.35f, 0.8f, -0.95f, 1.5f, 0.7f};
    for (int i = 0; i < 6; i++) {
        fish_[i].kind = kind[i];
        fish_[i].x = x[i];
        fish_[i].y = y[i];
        fish_[i].vx = vx[i];
        fish_[i].gone = false;
    }
    score_ = bare_ = golds_ = cream_ = caught_ = casts_ = 0;
    leave_ = refused_ = finisherGold_ = false;
    lureOn_ = false;
    hooked_ = target_ = -1;
    boatX_ = 40;
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
    if (phase_ == Phase::Call || phase_ == Phase::Fight) return 2;
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
        if (y < 58) v.lineBackdrop[y] = gs::rgb4(6, 10, 14);
        else if (y < 78) v.lineBackdrop[y] = gs::rgb4(4, 9, 13);
        else if (y < 120) v.lineBackdrop[y] = gs::rgb4(2, 7, 12);
        else if (y < 170) v.lineBackdrop[y] = gs::rgb4(1, 5, 10);
        else v.lineBackdrop[y] = gs::rgb4(1, 3, 8);
    }
}

void Game::moveFish() {
    for (int i = 0; i < 6; i++) {
        if (fish_[i].gone) continue;
        if (i == hooked_) continue;
        fish_[i].x = wrap(fish_[i].x + fish_[i].vx);
    }
}

void Game::botSteer() {
    if (lureOn_ || phase_ != Phase::Cast) return;
    target_ = -1;
    for (int i = 0; i < 6; i++) {
        if (!fish_[i].gone && fish_[i].kind == KIND_GOLD) {
            target_ = i;
            break;
        }
    }
    if (target_ < 0) return;
    Fish& g = fish_[target_];
    int steps = 0;
    float y = 52.f;
    while (y + 6.f < g.y && steps < 40) {
        y += 6.f;
        steps++;
    }
    if (steps < 1) steps = 1;
    float pred = wrap(g.x + g.vx * float(steps));
    float d = wrapDelta(pred, boatX_);
    if (std::fabs(d) <= 2.2f) {
        boatX_ = pred;
        lureOn_ = true;
        lureX_ = pred;
        lureY_ = 52.f;
        casts_++;
        blip(340.f, 0.05f);
    } else {
        float step = d;
        if (step > 3.2f) step = 3.2f;
        if (step < -3.2f) step = -3.2f;
        boatX_ = wrap(boatX_ + step);
    }
}

void Game::humanSteer() {
    if (!sys_) return;
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT)) boatX_ = wrap(boatX_ - 2.4f);
    if (p.down(gs::BTN_RIGHT)) boatX_ = wrap(boatX_ + 2.4f);
    if (!lureOn_ && phase_ == Phase::Cast && (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_DOWN))) {
        lureOn_ = true;
        lureX_ = boatX_ + 16.f;
        lureY_ = 52.f;
        casts_++;
        target_ = -1;
        blip(340.f, 0.05f);
    }
    if (lureOn_ && p.pressed(gs::BTN_B)) lureOn_ = false;
}

void Game::moveLure() {
    if (!lureOn_) return;
    if (bot_ && target_ >= 0 && !fish_[target_].gone) {
        lureX_ = fish_[target_].x;
    }
    lureY_ += 6.f;
    if (lureY_ > 206.f) {
        lureOn_ = false;
        std::snprintf(call_, sizeof call_, "MISSED");
        phase_ = Phase::Call;
        callT_ = 0;
        refused_ = false;
        blip(140.f, 0.05f);
    }
}

void Game::land(int i) {
    fish_[i].gone = true;
    hooked_ = -1;
    lureOn_ = false;
    caught_++;
    refused_ = false;
    Kind k = kindOf(fish_[i].kind);
    if (k.gold) {
        bare_ += k.face;
        score_ += k.points;
        golds_++;
        std::snprintf(call_, sizeof call_, "GOLD DOUBLE %d", k.points);
        if (score_ >= kLine && bare_ < kLine) {
            leave_ = true;
            finisherGold_ = true;
        }
        blip(720.f, 0.06f);
    } else if (k.points > 0 && score_ + k.points >= kLine) {
        refused_ = true;
        if (k.cream) cream_++;
        std::snprintf(call_, sizeof call_, "%s DOES NOT DOUBLE", k.name);
        blip(180.f, 0.05f);
    } else if (k.points > 0) {
        score_ += k.points;
        bare_ += k.points;
        if (k.cream) cream_++;
        std::snprintf(call_, sizeof call_, "%s %d", k.name, k.points);
        blip(420.f, 0.05f);
    }
    phase_ = Phase::Call;
    callT_ = 0;
}

void Game::tryHook() {
    if (!lureOn_) return;
    for (int i = 0; i < 6; i++) {
        if (fish_[i].gone) continue;
        if (bot_ && fish_[i].kind != KIND_GOLD) continue;
        float dx = wrapDelta(fish_[i].x + 8.f, lureX_);
        float dy = fish_[i].y - lureY_;
        if (std::fabs(dx) < 14.f && std::fabs(dy) < 10.f) {
            hooked_ = i;
            phase_ = Phase::Fight;
            fightT_ = 0;
            blip(520.f, 0.06f);
            if (sys_) sys_->rumble(0.15f, 0.3f, 40);
            return;
        }
    }
}

void Game::finish() {
    won_ = leave_ && finisherGold_ && score_ >= kLine && bare_ < kLine && golds_ > 0 && cream_ == 0;
    over_ = true;
    phase_ = won_ ? Phase::Win : Phase::Lose;
    if (sys_) {
        if (won_) sys_->setLight(255, 200, 40);
        else sys_->setLight(40, 70, 90);
    }
}

void Game::playTick() {
    if (phase_ == Phase::Fight) {
        if (hooked_ >= 0) {
            fish_[hooked_].y -= 4.f;
            lureY_ = fish_[hooked_].y;
            lureX_ = fish_[hooked_].x + 8.f;
        }
        if (++fightT_ > 10) land(hooked_);
        return;
    }
    if (phase_ == Phase::Call) {
        if (++callT_ > 28) {
            if (leave_ || casts_ >= kCasts) finish();
            else phase_ = Phase::Cast;
        }
        return;
    }
    if (phase_ != Phase::Cast) return;
    if (bot_) botSteer();
    else humanSteer();
    moveLure();
    tryHook();
}

void Game::draw() {
    sys_->vdp.clearSprites();
    backdrop();
    stamp(art_.sun, 250, 10, PAL_SUN);
    stamp(art_.reed, 8, 58, PAL_REED);
    stamp(art_.reed, 28, 64, PAL_REED);
    stamp(art_.reed, 292, 60, PAL_REED);
    bool face = false;
    for (int i = 0; i < 6; i++) {
        if (fish_[i].gone) continue;
        int pal = fish_[i].kind;
        bool flip = fish_[i].vx < 0;
        float x = fish_[i].x;
        if (flip) x -= art_.fish[pal].w * 0.3f;
        stamp(art_.fish[pal], x, fish_[i].y - 8, pal, flip);
    }
    if (lureOn_ || phase_ == Phase::Fight) {
        float x0 = boatX_ + 18.f;
        float y0 = 62.f;
        int n = 6;
        for (int i = 1; i <= n; i++) {
            float u = float(i) / float(n);
            stamp(art_.lure, x0 + (lureX_ - x0) * u - 3, y0 + (lureY_ - y0) * u - 4, PAL_LURE);
        }
        if (phase_ == Phase::Fight) stamp(art_.splash, lureX_ - 8, lureY_ - 4, PAL_LURE);
    }
    stamp(art_.boat, boatX_ - 10, 56, PAL_BOAT, face);
    stamp(art_.angler, boatX_ + 6, 38, PAL_BOAT);

    if (phase_ == Phase::Title) {
        hud(78, 78, "S3 FISH GOLD", PAL_INK);
        hud(36, 96, "ONLY THE GOLD COUNTS DOUBLE", PAL_HUD);
        hud(78, 114, "LINE 40", PAL_INK);
        hud(48, 140, "ARROWS MOVE   A CASTS", PAL_HUD);
        hud(70, 156, "START TO FISH", PAL_ALERT);
    } else {
        char line[64];
        std::snprintf(line, sizeof line, "SCORE %d  BARE %d  LINE %d", score_, bare_, kLine);
        hud(8, 4, line, PAL_HUD);
        std::snprintf(line, sizeof line, "GOLDS %d  CREAM %d  CAST %d/%d", golds_, cream_, casts_, kCasts);
        hud(8, 16, line, PAL_HUD);
        if (phase_ == Phase::Call) hud(8, 32, call_, refused_ ? PAL_ALERT : PAL_INK);
        if (phase_ == Phase::Win) {
            hud(48, 88, "DOUBLE", PAL_INK);
            hud(20, 106, "ONLY THE GOLD COUNTS DOUBLE", PAL_HUD);
        }
        if (phase_ == Phase::Lose) {
            hud(96, 88, "SHORT", PAL_ALERT);
            hud(28, 106, "THE BARE FACES STAY SHORT", PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (phase_ == Phase::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
            phase_ = Phase::Cast;
            blip(480.f, 0.04f);
        }
    } else if (phase_ == Phase::Win || phase_ == Phase::Lose) {
        over_ = true;
    } else {
        moveFish();
        playTick();
    }
    if (t_ % 8 == 0) sys.apu.tone(0, 0, 0);
    draw();
}

}  // namespace fishgold
