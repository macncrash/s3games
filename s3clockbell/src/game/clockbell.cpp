#include "game/clockbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace clockbell {
namespace {

int wrapHour(int h) {
    if (h > 12) return 1;
    if (h < 1) return 12;
    return h;
}

int stepToward(int cur, int want, int mod) {
    int cw = (want - cur + mod) % mod;
    int ccw = (cur - want + mod) % mod;
    if (cw == 0) return 0;
    return cw <= ccw ? 1 : -1;
}

}  // namespace

int Game::hourStep() const {
    int deg = (hour_ % 12) * 30 + minute_ / 2;
    int s = deg / 6;
    if (s < 0) s += kSteps;
    return s % kSteps;
}

void Game::blip(float freq) { sys_->apu.tone(1, freq, 0.04f); }

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w == 0 || img.h == 0) return;
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
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::resetFace(int which) {
    if (which <= 0) {
        hour_ = 11;
        minute_ = 37;
    } else if (which == 1) {
        hour_ = 2;
        minute_ = 48;
    } else {
        hour_ = 9;
        minute_ = 15;
    }
    grip_ = 0;
    rep_ = 0;
    foul_ = 0;
    ropeY_ = 0;
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
    resetFace(0);
    mode_ = Mode::Play;
    blip(392.f);
}

void Game::turn(int dir) {
    if (dir == 0 || mode_ != Mode::Play) return;
    if (grip_ == 0) hour_ = wrapHour(hour_ + dir);
    else minute_ = (minute_ + dir + 60) % 60;
    blip(grip_ == 0 ? 520.f : 740.f);
}

void Game::dieTry() {
    if (mode_ != Mode::Play || rung_) return;
    dead_++;
    swing_ = 4;
    ropeY_ = 6;
    foul_ = 36;
    sys_->apu.tone(0, 146.f, 0.08f);
    sys_->apu.noiseBurst(0.18f, 420.f, 0.08f);
    if (!bot_) sys_->rumble(0.4f, 0.12f, 70);
    if (dead_ >= kMaxDead) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        hold_ = 0;
        if (!bot_) sys_->setLight(140, 24, 24);
        return;
    }
    tryNo_ = dead_ + 1;
    resetFace(dead_);
    if (!bot_) sys_->setLight(120, 48, 28);
}

void Game::ring() {
    if (rung_ || dead_ >= kMaxDead || !trueHour()) return;
    rung_ = true;
    won_ = true;
    tryNo_ = dead_ + 1;
    mode_ = Mode::Ring;
    hold_ = 0;
    swing_ = 16;
    ropeY_ = 8;
    sys_->apu.tone(0, 523.f, 0.1f);
    sys_->apu.tone(2, 784.f, 0.07f);
    if (!bot_) {
        sys_->rumble(0.2f, 0.55f, 140);
        sys_->setLight(255, 196, 64);
    }
}

void Game::haul() {
    if (mode_ != Mode::Play || pause_ || rung_) return;
    ropeY_ = 7;
    if (!trueHour()) {
        dieTry();
        return;
    }
    ring();
}

void Game::botAct() {
    if (hour_ != kTarget) {
        grip_ = 0;
        int cur = hour_ == 12 ? 0 : hour_;
        int want = kTarget == 12 ? 0 : kTarget;
        turn(stepToward(cur, want, 12));
        return;
    }
    if (minute_ != 0) {
        grip_ = 1;
        turn(stepToward(minute_, 0, 60));
        return;
    }
    haul();
}

void Game::human(const gs::Pad& pad) {
    if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_DOWN)) {
        grip_ = grip_ ^ 1;
        blip(360.f);
    }
    int dir = 0;
    if (pad.down(gs::BTN_RIGHT)) dir += 1;
    if (pad.down(gs::BTN_LEFT)) dir -= 1;
    if (dir == 0) rep_ = 0;
    else if (pad.pressed(gs::BTN_RIGHT) || pad.pressed(gs::BTN_LEFT)) {
        rep_ = 0;
        turn(dir);
    } else if (++rep_ > 8 && (rep_ % 3) == 0) {
        turn(dir);
    }
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) haul();
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 1, 5);
    uint16_t mid = gs::rgb4(8, 4, 3);
    uint16_t bot = gs::rgb4(2, 1, 2);
    if (mode_ == Mode::Ring) mid = gs::rgb4(13, 9, 3);
    if (mode_ == Mode::Lose) {
        top = gs::rgb4(2, 0, 1);
        mid = gs::rgb4(5, 1, 1);
        bot = gs::rgb4(1, 0, 1);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
        auto mix = [&](uint16_t a, uint16_t b, float t) {
            t = std::clamp(t, 0.f, 1.f);
            auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
            return gs::rgb4(L(8), L(4), L(0));
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

    int f = int(sys_->frame);
    int hs = mode_ == Mode::Title ? (f / 8) % kSteps : hourStep();
    int ms = mode_ == Mode::Title ? (f / 3) % kSteps : minute_;
    bool hot = mode_ == Mode::Play && trueHour();
    int hp = (hot || mode_ == Mode::Ring || (mode_ == Mode::Play && grip_ == 0)) ? PAL_LIT : PAL_HOUR;
    int mp = (hot || mode_ == Mode::Ring || (mode_ == Mode::Play && grip_ == 1)) ? PAL_LIT : PAL_MIN;
    if (mode_ == Mode::Play && grip_ == 0 && !hot) hp = PAL_HOUR;
    if (mode_ == Mode::Play && grip_ == 1 && !hot) mp = PAL_MIN;
    if (mode_ == Mode::Play && grip_ == 0) hp = hot ? PAL_LIT : PAL_GOLD;
    if (mode_ == Mode::Play && grip_ == 1) mp = hot ? PAL_LIT : PAL_GOLD;

    spr(art_.dial, float(kCx), float(kCy), PAL_DIAL);
    spr(art_.minute[ms], float(kCx), float(kCy), mp);
    spr(art_.hour[hs], float(kCx), float(kCy), hp);
    spr(art_.cap, float(kCx), float(kCy), PAL_HOUR);
    float ox = std::sin(f * 0.7f) * float(swing_) * 0.45f;
    spr(art_.bell, float(kCx) + ox, 46.f, mode_ == Mode::Ring ? PAL_LIT : PAL_BELL);
    spr(art_.rope, 236.f, 120.f + float(ropeY_), PAL_ROPE);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(1, "S3 CLOCKBELL", PAL_GOLD);
        hudC(24, "A SHORT CLOCK", PAL_GOLD);
        hudC(25, "BELL BEFORE THE THIRD TRY DIES", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Lose) {
        hudC(1, "THIRD TRY DIED", PAL_BAD);
        hudC(25, "THE BELL STAYED QUIET", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else if (mode_ == Mode::Ring) {
        hudC(1, "THE BELL RINGS", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "TRY %d OF %d", tryNo_, kMaxDead);
        hudC(25, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:00", kTarget);
        hudC(26, buf, PAL_HUD);
    } else {
        std::snprintf(buf, sizeof buf, "POST %d:00", kTarget);
        hud(1, 24, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "TRY %d", tryNo_);
        hud(32, 24, buf, dead_ ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d:%02d", hour_, minute_);
        hudC(25, buf, hot ? PAL_GOLD : PAL_HUD);
        if (foul_ > 0) hudC(26, "NOT THE HOUR", PAL_BAD);
        else if (pause_) hudC(26, "PAUSED", PAL_GOLD);
        else if (hot) hudC(26, "HAUL THE ROPE", PAL_GOLD);
        else hudC(26, grip_ == 0 ? "HOUR HAND" : "MINUTE HAND", PAL_GOLD);
        hudC(27, "UD HAND  LR TURN  C ROPE", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.7f);
    rules_ = kTarget >= 1 && kTarget <= 12 && kMaxDead == 3;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    rung_ = false;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) pause_ = !pause_;
        if (!pause_) {
            if (bot_) botAct();
            else human(pad);
        }
        if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            pause_ = false;
            sys.apu.silence();
        }
    } else if (mode_ == Mode::Ring) {
        if (++hold_ == 8 || hold_ == 22) {
            swing_ = 14;
            sys_->apu.tone(0, hold_ == 8 ? 659.f : 784.f, 0.08f);
        }
        if (hold_ > 48) {
            over_ = true;
            won_ = true;
        }
    } else if (mode_ == Mode::Lose) {
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            sys.apu.silence();
        }
    }

    if (ropeY_ > 0) ropeY_--;
    if (swing_ > 0 && mode_ != Mode::Ring) swing_--;
    if (mode_ == Mode::Ring && swing_ > 0 && (hold_ % 2) == 0) swing_--;
    if (foul_ > 0 && mode_ == Mode::Play) foul_--;
    draw();
}

}  // namespace clockbell
