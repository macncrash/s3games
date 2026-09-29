#include "game/clockseven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace c7 {
namespace {

constexpr int kSecFrames = 18;
constexpr int kChime = 16;

int wrapHour(int h) {
    h %= 12;
    if (h <= 0) h += 12;
    return h;
}

int stepToward(int cur, int want, int mod) {
    int cw = (want - cur + mod) % mod;
    int ccw = (cur - want + mod) % mod;
    if (cw == 0) return 0;
    return cw <= ccw ? 1 : -1;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::hourStep() const {
    int s = ((hour_ % 12) * 5 + minute_ / 12) % 60;
    if (s < 0) s += 60;
    return s;
}

bool Game::aligned() const { return hour_ == target_ && minute_ == 0 && second_ == 0; }

void Game::blip() { sys_->apu.tone(1, 520.f + float(grip_) * 140.f, 0.03f); }

void Game::silence() {
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

void Game::deal() {
    hour_ = wrapHour(target_ - 1);
    minute_ = 2;
    second_ = 4;
    grip_ = 0;
    foul_ = 0;
    rep_ = 0;
    secAcc_ = 0;
    ropeY_ = 0;
    swing_ = 0;
}

void Game::begin() {
    you_ = 0;
    them_ = 0;
    target_ = 7;
    yours_ = true;
    pause_ = false;
    over_ = false;
    won_ = false;
    scored_ = 0;
    chime_ = 0;
    deal();
    mode_ = Mode::Play;
    silence();
}

void Game::turn(int dir) {
    if (dir == 0 || mode_ != Mode::Play) return;
    if (grip_ == 0) hour_ = wrapHour(hour_ + dir);
    else if (grip_ == 1) minute_ = (minute_ + dir + 60) % 60;
    else second_ = (second_ + dir + 60) % 60;
    blip();
}

void Game::haul() {
    if (mode_ != Mode::Play || pause_) return;
    ropeY_ = 8;
    swing_ = 10;
    if (!aligned()) {
        foul_ = 28;
        second_ = (second_ + 11) % 60;
        if (second_ == 0) second_ = 5;
        sys_->apu.noiseBurst(0.22f, 160.f, 0.1f);
        if (!bot_) sys_->rumble(0.4f, 0.1f, 70);
        yours_ = !yours_;
        deal();
        return;
    }
    if (yours_) you_++;
    else them_++;
    scored_ = yours_ ? 1 : 2;
    mode_ = Mode::Chime;
    chime_ = 0;
    sys_->apu.tone(0, 784.f, 0.08f);
    if (!bot_) sys_->rumble(0.1f, 0.3f, 50);
}

void Game::act() {
    if (hour_ != target_) {
        grip_ = 0;
        turn(stepToward(hour_, target_, 12));
        return;
    }
    if (minute_ != 0) {
        grip_ = 1;
        turn(stepToward(minute_, 0, 60));
        return;
    }
    if (second_ != 0) {
        grip_ = 2;
        turn(stepToward(second_, 0, 60));
        return;
    }
    haul();
}

void Game::human(const gs::Pad& pad) {
    if (pad.pressed(gs::BTN_UP)) {
        grip_ = (grip_ + 2) % 3;
        blip();
    }
    if (pad.pressed(gs::BTN_DOWN)) {
        grip_ = (grip_ + 1) % 3;
        blip();
    }
    int dir = 0;
    if (pad.down(gs::BTN_RIGHT)) dir += 1;
    if (pad.down(gs::BTN_LEFT)) dir -= 1;
    if (dir == 0) rep_ = 0;
    else if (pad.pressed(gs::BTN_RIGHT) || pad.pressed(gs::BTN_LEFT)) {
        rep_ = 0;
        turn(dir);
    } else if (++rep_ > 8 && (rep_ % 2) == 0) {
        turn(dir);
    }
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) haul();
}

void Game::tickSecond() {
    if (mode_ != Mode::Play || pause_ || grip_ == 2) return;
    if (++secAcc_ < kSecFrames) return;
    secAcc_ = 0;
    second_ = (second_ + 1) % 60;
    sys_->apu.tone(2, second_ == 0 ? 1320.f : 880.f, 0.025f);
}

void Game::finishChime() {
    if (you_ >= 7 && them_ < 7) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        return;
    }
    if (them_ >= 7 && you_ < 7) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        return;
    }
    target_ = wrapHour(target_ + 1);
    yours_ = !yours_;
    deal();
    mode_ = Mode::Play;
    silence();
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 1, 5);
    uint16_t mid = gs::rgb4(3, 2, 6);
    uint16_t bot = gs::rgb4(2, 2, 3);
    if (mode_ == Mode::Chime || mode_ == Mode::Win) mid = gs::rgb4(10, 7, 2);
    if (mode_ == Mode::Lose) {
        top = gs::rgb4(3, 0, 1);
        mid = gs::rgb4(5, 1, 1);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        v.lineBackdrop[y] = u < 0.62f ? lerpC(top, mid, u / 0.62f) : lerpC(mid, bot, (u - 0.62f) / 0.38f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
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

void Game::image(const gs::Image& img, float cx, float cy, int pal, int w, int h) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(w > 0 ? w : img.w);
    s.h = int16_t(h > 0 ? h : img.h);
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    int f = int(sys_->frame);
    int hs, ms, ss;
    if (mode_ == Mode::Title) {
        hs = (f / 18) % 60;
        ms = (f / 6) % 60;
        ss = (f * 2) % 60;
    } else {
        hs = hourStep();
        ms = minute_;
        ss = second_;
    }

    static const int kStar[][2] = {{18, 14}, {48, 28}, {78, 10}, {250, 16}, {286, 8}, {300, 30}, {228, 22}};
    for (int i = 0; i < 7; i++) {
        if (((f / 4 + i) % 9) == 0) continue;
        image(art_.star, float(kStar[i][0]), float(kStar[i][1]), PAL_GOLD, 7, 7);
    }

    image(art_.face, float(kCx), float(kCy), PAL_FACE);
    int hp = PAL_HOUR, mp = PAL_MIN, sp = PAL_SEC;
    if (mode_ == Mode::Play && grip_ == 0) hp = PAL_LIT;
    if (mode_ == Mode::Play && grip_ == 1) mp = PAL_LIT;
    if (mode_ == Mode::Play && grip_ == 2) sp = PAL_LIT;
    image(art_.hand[2][ss], float(kCx), float(kCy), sp);
    image(art_.hand[1][ms], float(kCx), float(kCy), mp);
    image(art_.hand[0][hs], float(kCx), float(kCy), hp);
    image(art_.cap, float(kCx), float(kCy), PAL_BELL, 10, 10);

    float ox = std::sin(f * 0.7f) * float(swing_) * 0.4f;
    image(art_.bell, float(kCx) + ox, 52.f, PAL_BELL);
    image(art_.rope, 214.f, 132.f + float(ropeY_), PAL_BELL);

    for (int i = 0; i < 7; i++) {
        image(i < you_ ? art_.lampOn : art_.lampOff, 28.f, 64.f + float(i) * 14.f, i < you_ ? PAL_YOU : PAL_DIM);
        image(i < them_ ? art_.lampOn : art_.lampOff, 292.f, 64.f + float(i) * 14.f, i < them_ ? PAL_THEM : PAL_DIM);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(1, "S3 CLOCK SEVEN", PAL_GOLD);
        hudC(3, "FIRST TO SEVEN", PAL_HUD);
        if ((f & 16) == 0) hudC(25, "PRESS START", PAL_GOLD);
        hudC(26, "SET THE HOUR  HAUL THE ROPE", PAL_DIM);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Win) {
        hudC(1, "FIRST TO SEVEN", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "YOU %d  THEM %d", you_, them_);
        hudC(25, buf, PAL_GOLD);
        if ((f & 16) == 0) hudC(26, "START", PAL_HUD);
    } else if (mode_ == Mode::Lose) {
        hudC(1, "THE HOUSE STRUCK SEVEN", PAL_BAD);
        std::snprintf(buf, sizeof buf, "YOU %d  THEM %d", you_, them_);
        hudC(25, buf, PAL_BAD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "HOUR %d", target_);
        hud(1, 1, buf, PAL_GOLD);
        hud(30, 1, yours_ ? "YOU" : "THEM", yours_ ? PAL_YOU : PAL_THEM);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour_, minute_, second_);
        hudC(24, buf, aligned() ? PAL_GOLD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "YOU %d", you_);
        hud(1, 25, buf, PAL_YOU);
        std::snprintf(buf, sizeof buf, "THEM %d", them_);
        hud(31, 25, buf, PAL_THEM);
        if (foul_ > 0) hudC(26, "NOT THE HOUR", PAL_BAD);
        else if (mode_ == Mode::Chime) hudC(26, "THE HOUR CHIMES", PAL_GOLD);
        else if (!yours_) hudC(26, "HOUSE SETS THE HOUR", PAL_THEM);
        else if (aligned()) hudC(26, "HAUL THE ROPE", PAL_GOLD);
        else hudC(26, "UP DN HAND  LR TURN  C ROPE", PAL_DIM);
    }
    if (ropeY_ > 0) ropeY_--;
    if (swing_ > 0) swing_--;
    if (foul_ > 0) foul_--;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) pause_ = !pause_;
        if (!pause_) {
            if (bot_ || !yours_) act();
            else human(pad);
            if (mode_ == Mode::Play) tickSecond();
        }
        if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            pause_ = false;
            silence();
        }
    } else if (mode_ == Mode::Chime) {
        sys_->apu.tone(0, 0, 0);
        if (++chime_ >= kChime) finishChime();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
        begin();
    }

    draw();
    if (mode_ == Mode::Play) sys_->apu.tone(2, 0, 0);
}

}  // namespace c7
