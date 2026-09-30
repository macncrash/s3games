#include "game/bellmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace bellmark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kNeed = 4;
constexpr int kMiss = 3;
constexpr float kPeriod = 1.05f;
constexpr float kWinLo = 0.78f;

bool pullDown(const gs::Pad& p) {
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_DOWN);
}

}  // namespace

void Game::ding() {
    sys_->apu.tone(0, 294.f, 0.12f);
    sys_->apu.tone(1, 440.f, 0.08f);
    sys_->apu.noiseBurst(0.03f, 900.f, 0.04f);
    toneT_ = 0.45f;
}

void Game::thud() {
    sys_->apu.tone(1, 140.f, 0.07f);
    sys_->apu.noiseBurst(0.06f, 400.f, 0.05f);
    toneT_ = 0.16f;
}

void Game::hush() {
    if (toneT_ <= 0) return;
    toneT_ = std::max(0.f, toneT_ - kDt);
    if (toneT_ <= 0) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
    }
}

void Game::begin() {
    good_ = 0;
    pulls_ = 0;
    misses_ = 0;
    phase_ = 0.15f;
    flash_ = 0;
    sway_ = 0;
    won_ = false;
    over_ = false;
    finished_ = false;
    struck_ = false;
    botHeld_ = false;
    rules_ = true;
    mode_ = Mode::Play;
    modeT_ = 0;
    sys_->apu.silence();
    if (!bot_) sys_->setLight(80, 50, 10);
}

void Game::finishMark() {
    finished_ = true;
    struck_ = true;
    won_ = true;
    mode_ = Mode::Show;
    modeT_ = 0;
    flash_ = 1.f;
    sway_ = 1.f;
    sys_->apu.tone(0, 330.f, 0.14f);
    sys_->apu.tone(1, 494.f, 0.12f);
    toneT_ = 0.7f;
    if (!bot_) sys_->setLight(180, 140, 40);
}

void Game::fail() {
    won_ = false;
    finished_ = false;
    struck_ = false;
    mode_ = Mode::Lose;
    modeT_ = 0;
    sys_->apu.silence();
    sys_->apu.tone(0, 90.f, 0.16f);
    toneT_ = 0.35f;
    if (!bot_) sys_->setLight(20, 10, 30);
}

float Game::swing() const {
    return std::sin(phase_ * 6.2831853f / kPeriod);
}

bool Game::inWindow() const {
    return swing() >= kWinLo;
}

void Game::pull() {
    if (mode_ != Mode::Play) return;
    pulls_++;
    if (inWindow()) {
        good_++;
        flash_ = 0.32f;
        sway_ = 1.f;
        ding();
        if (!bot_) sys_->rumble(0.15f, 0.55f, 40);
        if (good_ >= kNeed) finishMark();
    } else {
        misses_++;
        thud();
        if (!bot_) sys_->rumble(0.45f, 0.05f, 50);
        if (misses_ >= kMiss) fail();
    }
}

void Game::botPlay() {
    if (mode_ == Mode::Title) {
        if (modeT_ > 0.25f) begin();
        return;
    }
    if (mode_ != Mode::Play) return;
    bool in = inWindow();
    if (in && !botHeld_) {
        pull();
        botHeld_ = true;
    }
    if (!in) botHeld_ = false;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 2));
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    mode_ = Mode::Title;
    modeT_ = 0;
    rules_ = false;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Play) {
        phase_ += kDt;
        if (sway_ > 0) sway_ = std::max(0.f, sway_ - kDt * 1.6f);
        if (pullDown(pad)) pull();
    } else if (mode_ == Mode::Show) {
        modeT_ += kDt;
        phase_ += kDt * 0.35f;
        if (modeT_ > 0.6f) over_ = true;
    } else if (mode_ == Mode::Lose) {
        modeT_ += kDt;
        if (modeT_ > 0.7f) over_ = true;
    }
    if (bot_) botPlay();
    if (mode_ == Mode::Title || mode_ == Mode::Play) modeT_ += kDt;
    if (flash_ > 0) flash_ = std::max(0.f, flash_ - kDt);
    hush();
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool hflip) {
    if (h < 1.f || m.h < 1) return;
    float sc = h / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.w = int16_t(std::clamp(int(std::lround(m.w * sc)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = hflip;
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int b = 2 + (y < 90 ? (90 - y) / 40 : 0);
        int r = y > 160 ? 2 : 1;
        v.lineBackdrop[y] = gs::rgb4(r, 1, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float ang = (mode_ == Mode::Title) ? 0.15f : swing();
    float bx = 168.f + ang * 28.f;
    float by = 78.f + std::fabs(ang) * 6.f;
    float ropeY = 150.f + (1.f - ang) * 10.f;

    spr(art_.tower, 168, 118, 150, PAL_STONE);
    spr(art_.ringer, 48, 150, 78, PAL_RINGER);
    spr(art_.bell, bx, by, mode_ == Mode::Show ? 78 : 70, mode_ == Mode::Show ? PAL_GOLD : PAL_BELL);
    spr(art_.clapper, bx + ang * 6.f, by + 22.f + flash_ * 4.f, 22, PAL_CLAP);
    spr(art_.rope, 118, ropeY, 70 + sway_ * 8.f, PAL_ROPE);

    for (int i = 0; i < kNeed; i++) {
        int pal = i < good_ ? PAL_GOLD : PAL_YOKE;
        spr(art_.notch, 230.f, 70.f + float(i) * 18.f, 14, pal);
    }

    if (flash_ > 0.04f) spr(art_.notch, bx + 18.f, by + 16.f, 16, PAL_GOLD);

    if (art_.title.w > 0 && (mode_ == Mode::Title || mode_ == Mode::Show)) {
        gs::Sprite s;
        s.img = art_.title;
        s.w = int16_t(art_.title.w);
        s.h = int16_t(art_.title.h);
        s.x = int16_t(160 - art_.title.w / 2);
        s.y = 12;
        s.pal = PAL_GOLD;
        v.sprite(s);
    }

    if (mode_ == Mode::Title) {
        hudC(22, "PULL AT THE TOP", PAL_HUD);
        hudC(24, "A FINISHED MARK ENDS IT", PAL_HUD);
        hudC(26, "START", PAL_HUD);
    } else if (mode_ == Mode::Play) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "MARK %d/%d", good_, kNeed);
        hudC(26, buf, PAL_HUD);
    } else if (mode_ == Mode::Show) {
        hudC(24, "FINISHED MARK", PAL_GOLD);
    } else {
        hudC(24, "THE MARK IS OPEN", PAL_HUD);
    }
}

}  // namespace bellmark
