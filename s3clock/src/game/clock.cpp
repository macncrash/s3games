#include "game/clock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace clk {
namespace {

constexpr int kSecFrames = 15;
constexpr int kDwell = 18;
constexpr int kStrikeGap = 20;

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.16f;
    p.op[0] = {1.414f, 0.8f, 0.002f, 0.32f, 0.0f, 0.28f};
    p.op[1] = {1.0f, 1.0f, 0.001f, 0.85f, 0.0f, 0.6f};
    p.op[2] = {2.76f, 0.26f, 0.001f, 0.38f, 0.0f, 0.32f};
    p.op[3] = {5.2f, 0.1f, 0.001f, 0.2f, 0.0f, 0.18f};
    p.vol = 0.28f;
    p.tone = 2600;
    p.echo = 0.42f;
    return p;
}

gs::FMPatch clunkPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.45f;
    p.op[0] = {1, 1, 0.001f, 0.08f, 0.0f, 0.12f};
    p.op[1] = {2.2f, 0.55f, 0.001f, 0.1f, 0.0f, 0.1f};
    p.op[2] = {0.5f, 0.8f, 0.001f, 0.14f, 0.0f, 0.16f};
    p.op[3] = {3.5f, 0.25f, 0.001f, 0.06f, 0.0f, 0.08f};
    p.vol = 0.32f;
    p.tone = 380;
    p.drive = 0.45f;
    p.echo = 0.08f;
    return p;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

int wrapHour(int h) {
    if (h > 12) return 1;
    if (h < 1) return 12;
    return h;
}

}  // namespace

uint32_t Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return rng_;
}

int Game::hourStep() const {
    float deg = float(hour_ % 12) * 30.f + float(minute_) * 0.5f;
    int s = int(std::lround(deg / 6.f)) % 60;
    if (s < 0) s += 60;
    return s;
}

bool Game::aligned() const { return hour_ == target_ && minute_ == 0 && second_ == 0; }

void Game::blip() {
    float f = 480.f + float(grip_) * 160.f;
    sys_->apu.tone(1, f, 0.03f);
    blipLeft_ = 2;
}

void Game::tock(bool hour) {
    sys_->apu.tone(0, hour ? 1480.f : 1046.f, hour ? 0.055f : 0.035f);
    tockLeft_ = 3;
}

void Game::silenceTicks() {
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    tockLeft_ = blipLeft_ = 0;
}

void Game::layFixed() {
    target_ = 7;
    hour_ = 2;
    minute_ = 41;
    second_ = 17;
}

void Game::layRandom() {
    target_ = 1 + int(rnd() % 12);
    hour_ = 1 + int(rnd() % 12);
    if (hour_ == target_) hour_ = wrapHour(hour_ + 1);
    minute_ = 1 + int(rnd() % 59);
    second_ = int(rnd() % 60);
}

void Game::begin() {
    if (!opened_) {
        layFixed();
        opened_ = true;
    } else {
        layRandom();
    }
    grip_ = 0;
    ropes_ = 3;
    dwell_ = 0;
    secAcc_ = 0;
    foul_ = 0;
    playFrame_ = 0;
    rep_ = 0;
    pause_ = false;
    strikesLeft_ = 0;
    strikeWait_ = 0;
    chimeHold_ = 0;
    bellSwing_ = 0;
    ropeY_ = 0;
    over_ = false;
    won_ = false;
    mode_ = Mode::Play;
    silenceTicks();
}

void Game::turn(int dir) {
    if (dir == 0) return;
    if (grip_ == 0) hour_ = wrapHour(hour_ + dir);
    else if (grip_ == 1) minute_ = (minute_ + dir + 60) % 60;
    else {
        second_ = (second_ + dir + 60) % 60;
        dwell_ = second_ == 0 ? kDwell : 0;
    }
    blip();
}

void Game::haul() {
    if (mode_ != Mode::Play || pause_ || foul_ > 0) return;
    ropeY_ = 8;
    if (!aligned()) {
        ropes_--;
        foul_ = 40;
        bellSwing_ = 5;
        second_ = (second_ + 23) % 60;
        if (second_ == 0) second_ = 7;
        dwell_ = 0;
        secAcc_ = 0;
        sys_->apu.keyOn(1, 78.f, 0.4f);
        sys_->apu.noiseBurst(0.28f, 180.f, 0.12f);
        if (!bot_) sys_->rumble(0.45f, 0.15f, 80);
        if (ropes_ <= 0) {
            mode_ = Mode::Dead;
            won_ = false;
            over_ = true;
            sys_->apu.noiseBurst(0.4f, 90.f, 0.35f);
        }
        return;
    }
    mode_ = Mode::Chime;
    strikesLeft_ = target_;
    strikeWait_ = 0;
    chimeHold_ = 0;
    if (!bot_) sys_->rumble(0.15f, 0.35f, 60);
}

void Game::advanceSecond() {
    if (mode_ != Mode::Play || pause_ || grip_ == 2) return;
    if (++secAcc_ < kSecFrames) return;
    secAcc_ = 0;
    if (dwell_ > 0) {
        if (--dwell_ > 0) {
            tock(true);
            return;
        }
    }
    second_ = (second_ + 1) % 60;
    if (second_ == 0) dwell_ = kDwell;
    tock(second_ == 0);
}

void Game::botAct() {
    auto stepToward = [](int cur, int want, int mod) {
        int cw = (want - cur + mod) % mod;
        int ccw = (cur - want + mod) % mod;
        if (cw == 0) return 0;
        return cw <= ccw ? 1 : -1;
    };
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
    } else if (++rep_ > 10 && (rep_ % 3) == 0) {
        turn(dir);
    }
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) haul();
}

void Game::updateChime() {
    if (ropeY_ > 0) ropeY_--;
    if (bellSwing_ > 0) bellSwing_--;
    if (strikeWait_ > 0) {
        strikeWait_--;
        return;
    }
    if (strikesLeft_ > 0) {
        strikesLeft_--;
        strikeWait_ = kStrikeGap;
        bellSwing_ = 14;
        ropeY_ = 5;
        sys_->apu.keyOn(0, 523.25f, 0.34f);
        if (!bot_) sys_->rumble(0.12f, 0.28f, 40);
        return;
    }
    if (++chimeHold_ > 36) {
        mode_ = Mode::Won;
        won_ = true;
        over_ = true;
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

void Game::image(const gs::Image& img, float cx, float cy, int pal) {
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

void Game::axle(const gs::Image& img, int pal) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(kCx - kPivot);
    s.y = int16_t(kCy - kPivot);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::banner(const std::string& s, float y, int pal) {
    const float adv = 18.f;
    float x = 160.f - float(s.size()) * adv * 0.5f + adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        image(art_.glyph[c - 32], x + float(i) * adv, y, pal);
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 1, 6);
    uint16_t mid = gs::rgb4(11, 5, 4);
    uint16_t bot = gs::rgb4(2, 1, 3);
    if (mode_ == Mode::Chime || mode_ == Mode::Won) mid = gs::rgb4(13, 8, 3);
    if (mode_ == Mode::Dead) {
        top = gs::rgb4(2, 0, 1);
        mid = gs::rgb4(5, 1, 1);
        bot = gs::rgb4(1, 0, 1);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        v.lineBackdrop[y] = u < 0.58f ? lerpC(top, mid, u / 0.58f) : lerpC(mid, bot, (u - 0.58f) / 0.42f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.setFogColor(gs::rgb4(2, 1, 3));
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    int f = int(sys_->frame);
    int hs, ms, ss;
    if (mode_ == Mode::Title) {
        hs = (f / 20) % 60;
        ms = (f / 5) % 60;
        ss = (f * 2) % 60;
    } else {
        hs = hourStep();
        ms = minute_;
        ss = second_;
    }
    bool live = mode_ == Mode::Play;
    bool celebrate = mode_ == Mode::Chime || mode_ == Mode::Won;
    bool hot = live && aligned();
    int hp = (hot || celebrate || (live && grip_ == 0)) ? PAL_HLIT : PAL_HOUR;
    int mp = (hot || celebrate || (live && grip_ == 1)) ? PAL_MLIT : PAL_MIN;
    int sp = (hot || celebrate || (live && grip_ == 2)) ? PAL_SLIT : PAL_SEC;
    // Earlier sprites paint over later ones.
    axle(art_.cap, PAL_HOUR);
    axle(art_.hand[2][ss], sp);
    axle(art_.hand[1][ms], mp);
    axle(art_.hand[0][hs], hp);
    if (hot) image(art_.ring, float(kCx), float(kCy), PAL_RING);
    float ox = std::sin(f * 0.85f) * float(bellSwing_) * 0.35f;
    image(art_.bell, float(kCx) + ox, float(kBellY), PAL_BELL);
    image(art_.rope, float(kRopeX), 128.f + float(ropeY_), PAL_BELL);
    float swing = mode_ == Mode::Dead ? 6.f : std::sin(f * 0.08f) * 6.f;
    image(art_.bob, 160.f + swing, 188, PAL_BELL);
    image(art_.rod, 160.f + swing * 0.35f, 178, PAL_BELL);
    image(art_.moon, 36, 34, PAL_MOON);
    static const int kStar[][2] = {{14, 10}, {70, 8}, {96, 16}, {248, 8}, {286, 14}, {304, 6}, {262, 22}, {18, 22}};
    for (int i = 0; i < 8; i++) {
        if (((f + i * 5) % 23) == 0) continue;
        image(art_.star, float(kStar[i][0]), float(kStar[i][1]), PAL_HUD);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        banner("S3 CLOCK", 12, PAL_GOLD);
        hudC(25, "THREE HANDS", PAL_GOLD);
        hudC(26, "THE HOUR HAS TO CHIME", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Dead) {
        banner("THE WORKS JAM", 12, PAL_BAD);
        hudC(25, "THREE ROPES", PAL_BAD);
        hudC(26, "THE HOUR DID NOT CHIME", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else if (mode_ == Mode::Won || mode_ == Mode::Chime) {
        banner("THE HOUR CHIMES", 12, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:00:00", target_);
        hudC(25, buf, PAL_GOLD);
        int rung = target_ - strikesLeft_;
        if (rung < 0) rung = 0;
        if (rung > 12) rung = 12;
        std::string marks(size_t(rung), '*');
        if (!marks.empty()) hudC(26, marks, PAL_GOLD);
        if (mode_ == Mode::Won && (f & 16) == 0) hudC(27, "START", PAL_HUD);
    } else {
        std::snprintf(buf, sizeof buf, "CHIME %d", target_);
        hud(1, 25, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour_, minute_, second_);
        hudC(25, buf, aligned() ? PAL_GOLD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "ROPES %d", ropes_);
        hud(32, 25, buf, ropes_ < 3 ? PAL_BAD : PAL_HUD);
        if (foul_ > 0) hudC(26, "NOT THE HOUR", PAL_BAD);
        else if (pause_) hudC(26, "PAUSED", PAL_GOLD);
        else if (aligned()) hudC(26, "HAUL THE ROPE", PAL_GOLD);
        else if (playFrame_ < 240) {
            std::snprintf(buf, sizeof buf, "SET %d:00:00 THEN HAUL", target_);
            hudC(26, buf, PAL_GOLD);
        } else {
            hud(4, 26, "HOUR", grip_ == 0 ? PAL_GOLD : PAL_DIM);
            hud(14, 26, "MINUTE", grip_ == 1 ? PAL_GOLD : PAL_DIM);
            hud(26, 26, "SECOND", grip_ == 2 ? PAL_GOLD : PAL_DIM);
        }
        hudC(27, "UP DN HAND   LR TURN   C ROPE", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.32f, 0.38f, 0.24f);
    sys.apu.setPatch(0, bellPatch());
    sys.apu.setPatch(1, clunkPatch());
    sys.apu.setPan(0, 0.05f);
    sys.apu.setPan(1, -0.15f);
    mode_ = Mode::Title;
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
        playFrame_++;
        if (!bot_ && pad.pressed(gs::BTN_START)) pause_ = !pause_;
        if (!pause_) {
            if (bot_) botAct();
            else human(pad);
            if (mode_ == Mode::Play) advanceSecond();
        }
        if (mode_ == Mode::Chime) updateChime();
        if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            pause_ = false;
            silenceTicks();
            sys.apu.silence();
        }
    } else if (mode_ == Mode::Chime) {
        updateChime();
        if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            silenceTicks();
            sys.apu.silence();
        }
    } else if (mode_ == Mode::Won || mode_ == Mode::Dead) {
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            silenceTicks();
            sys.apu.silence();
        }
    }

    if (ropeY_ > 0 && mode_ == Mode::Play) ropeY_--;
    if (bellSwing_ > 0 && mode_ != Mode::Chime) bellSwing_--;
    if (foul_ > 0 && mode_ == Mode::Play) foul_--;
    if (tockLeft_ > 0 && --tockLeft_ == 0) sys.apu.tone(0, 0, 0);
    if (blipLeft_ > 0 && --blipLeft_ == 0) sys.apu.tone(1, 0, 0);
    if (!bot_) {
        int r = mode_ == Mode::Dead ? 90 : mode_ == Mode::Won || mode_ == Mode::Chime ? 180 : 40;
        int g = mode_ == Mode::Dead ? 16 : mode_ == Mode::Won ? 90 : 28;
        sys.setLight(r, g, 24);
    }
    draw();
}

}  // namespace clk
