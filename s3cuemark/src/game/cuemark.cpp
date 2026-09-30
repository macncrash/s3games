#include "game/cuemark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace cuemark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kNeed = 4;
constexpr int kSmudge = 3;
constexpr float kPeriod = 0.92f;
constexpr float kWinLo = 0.80f;

bool strokeDown(const gs::Pad& p) {
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_RIGHT);
}

}  // namespace

void Game::tick() {
    sys_->apu.tone(1, 660.f, 0.06f);
    toneT_ = 0.06f;
}

void Game::clack() {
    sys_->apu.tone(0, 392.f, 0.10f);
    sys_->apu.noiseBurst(0.05f, 1400.f, 0.04f);
    toneT_ = 0.12f;
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
    smudges_ = 0;
    phase_ = 0.2f;
    flash_ = 0;
    roll_ = 0;
    ballX_ = 148.f;
    won_ = false;
    over_ = false;
    finished_ = false;
    seated_ = false;
    botHeld_ = false;
    rules_ = true;
    mode_ = Mode::Play;
    modeT_ = 0;
    sys_->apu.silence();
    if (!bot_) sys_->setLight(20, 80, 40);
}

void Game::finishMark() {
    finished_ = true;
    seated_ = true;
    won_ = true;
    mode_ = Mode::Show;
    modeT_ = 0;
    flash_ = 1.f;
    roll_ = 0;
    ballX_ = 236.f;
    sys_->apu.tone(0, 349.f, 0.12f);
    sys_->apu.tone(1, 523.f, 0.10f);
    toneT_ = 0.5f;
    if (!bot_) sys_->setLight(40, 160, 80);
}

void Game::fail() {
    won_ = false;
    finished_ = false;
    seated_ = false;
    mode_ = Mode::Lose;
    modeT_ = 0;
    sys_->apu.silence();
    sys_->apu.tone(0, 110.f, 0.16f);
    toneT_ = 0.35f;
    if (!bot_) sys_->setLight(20, 20, 40);
}

bool Game::inWindow() const {
    float m = 0.5f + 0.5f * std::sin(phase_ * 6.2831853f / kPeriod);
    return m >= kWinLo;
}

float Game::cueX() const {
    float m = 0.5f + 0.5f * std::sin(phase_ * 6.2831853f / kPeriod);
    return 36.f + m * 78.f;
}

void Game::stroke() {
    if (mode_ != Mode::Play) return;
    pulls_++;
    if (inWindow()) {
        good_++;
        flash_ = 0.28f;
        roll_ = 0.35f;
        clack();
        if (!bot_) sys_->rumble(0.2f, 0.45f, 30);
        if (good_ >= kNeed) finishMark();
    } else {
        smudges_++;
        tick();
        if (!bot_) sys_->rumble(0.4f, 0.05f, 50);
        if (smudges_ >= kSmudge) fail();
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
        stroke();
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
    sys.vdp.setFogColor(gs::rgb4(0, 1, 1));
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
        if (strokeDown(pad)) stroke();
        if (roll_ > 0) {
            roll_ = std::max(0.f, roll_ - kDt);
            float t = 1.f - roll_ / 0.35f;
            ballX_ = 148.f + t * 88.f;
            if (roll_ <= 0 && mode_ == Mode::Play) ballX_ = 148.f;
        }
    } else if (mode_ == Mode::Show) {
        modeT_ += kDt;
        ballX_ = 236.f;
        if (modeT_ > 0.55f) over_ = true;
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
        int g = 1;
        if (y > 70 && y < 190) g = 2 + (y % 6 == 0 ? 1 : 0);
        v.lineBackdrop[y] = gs::rgb4(0, g, 1);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.table, 176, 132, 92, PAL_CLOTH);
    spr(art_.player, 42, 118, 72, PAL_PLAYER);

    float tip = cueX();
    spr(art_.cue, tip, 132, 12, PAL_CUE);

    for (int i = 0; i < kNeed; i++) {
        int pal = i < good_ ? PAL_GOLD : PAL_WOOD;
        spr(art_.spot, 188.f + float(i) * 16.f, 108, 12, pal);
    }
    spr(art_.ball, ballX_, 132, mode_ == Mode::Show ? 20 : 16, PAL_BALL);
    spr(art_.chalk, 70, 168, 14, PAL_CHALK);

    if (flash_ > 0.04f) {
        spr(art_.spot, ballX_ + 8, 120, 16, PAL_GOLD);
        spr(art_.chalk, 92, 124, 12, PAL_CHALK);
    }

    if (art_.title.w > 0 && (mode_ == Mode::Title || mode_ == Mode::Show)) {
        gs::Sprite s;
        s.img = art_.title;
        s.w = int16_t(art_.title.w);
        s.h = int16_t(art_.title.h);
        s.x = int16_t(160 - art_.title.w / 2);
        s.y = 16;
        s.pal = PAL_GOLD;
        v.sprite(s);
    }

    if (mode_ == Mode::Title) {
        hudC(8, "A FINISHED MARK ENDS IT", PAL_HUD);
        hudC(10, "STROKE WHEN THE CUE", PAL_HUD);
        hudC(11, "MEETS THE CHALK SPOT", PAL_HUD);
        hudC(24, "A TO BEGIN", PAL_HUD);
    } else if (mode_ == Mode::Play) {
        char line[32];
        std::snprintf(line, sizeof(line), "MARK %d OF %d", good_, kNeed);
        hudC(2, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "SMUDGE %d OF %d", smudges_, kSmudge);
        hudC(3, line, PAL_HUD);
        hudC(25, inWindow() ? "CHALK" : "HOLD", inWindow() ? PAL_GOLD : PAL_HUD);
    } else if (mode_ == Mode::Show) {
        hudC(8, "FINISHED MARK", PAL_GOLD);
        hudC(10, "THE SPOT IS CHALKED", PAL_HUD);
    } else {
        hudC(8, "THE CHALK SMUDGED", PAL_HUD);
        hudC(10, "NO FINISHED MARK", PAL_HUD);
    }
}

}  // namespace cuemark
