#include "game/cuebell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cuebell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPeriod = 1.05f;
constexpr float kWin = 0.84f;
constexpr float kBallHome = 118.f;
constexpr float kBellX = 268.f;
constexpr int kDeadLimit = 3;

bool strokeDown(const gs::Pad& p) {
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C);
}

}  // namespace

float Game::meter() const {
    float t = std::fmod(phase_, kPeriod) / kPeriod;
    return t < 0.5f ? t * 2.f : (1.f - t) * 2.f;
}

bool Game::inWindow() const { return meter() >= kWin; }

float Game::cueTip() const { return 22.f + meter() * 86.f; }

void Game::hush() {
    if (toneT_ <= 0) return;
    toneT_ = std::max(0.f, toneT_ - kDt);
    if (toneT_ <= 0) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
    }
}

void Game::begin() {
    dead_ = 0;
    tryNo_ = 0;
    phase_ = 0.15f;
    ballX_ = kBallHome;
    roll_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    botHeld_ = false;
    why_ = "";
    bellAmp_ = 0.1f;
    mode_ = Mode::Aim;
    modeT_ = 0;
    sys_->apu.silence();
    if (!bot_) sys_->setLight(40, 80, 20);
}

void Game::ring() {
    rung_ = true;
    won_ = true;
    why_ = "rung";
    mode_ = Mode::Ring;
    modeT_ = 0;
    ballX_ = kBellX - 18.f;
    roll_ = 0;
    bellAmp_ = 1.f;
    sys_->apu.tone(0, 523.f, 0.16f);
    sys_->apu.tone(1, 784.f, 0.10f);
    toneT_ = 0.7f;
    if (!bot_) {
        sys_->setLight(180, 140, 30);
        sys_->rumble(0.15f, 0.55f, 80);
    }
}

void Game::dieTry(const char* why) {
    dead_++;
    why_ = why;
    mode_ = Mode::Dead;
    modeT_ = 0;
    roll_ = 0.28f;
    bellAmp_ = 0.08f;
    sys_->apu.tone(0, 98.f, 0.12f);
    toneT_ = 0.28f;
    if (!bot_) {
        sys_->setLight(80, 16, 16);
        sys_->rumble(0.45f, 0.05f, 40);
    }
}

void Game::stroke() {
    if (mode_ != Mode::Aim) return;
    tryNo_++;
    if (inWindow()) {
        mode_ = Mode::Roll;
        modeT_ = 0;
        roll_ = 0.42f;
        sys_->apu.tone(0, 330.f, 0.08f);
        sys_->apu.noiseBurst(0.05f, 900.f, 0.04f);
        toneT_ = 0.12f;
        if (!bot_) sys_->rumble(0.2f, 0.35f, 24);
    } else {
        dieTry("thin");
    }
}

void Game::botPlay() {
    if (mode_ == Mode::Title) {
        if (modeT_ > 0.2f) begin();
        return;
    }
    if (mode_ != Mode::Aim) return;
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
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Aim) {
        phase_ += kDt;
        if (strokeDown(pad)) stroke();
    } else if (mode_ == Mode::Roll) {
        roll_ = std::max(0.f, roll_ - kDt);
        float t = 1.f - roll_ / 0.42f;
        ballX_ = kBallHome + t * (kBellX - 18.f - kBallHome);
        if (roll_ <= 0) ring();
    } else if (mode_ == Mode::Dead) {
        modeT_ += kDt;
        if (roll_ > 0) {
            roll_ = std::max(0.f, roll_ - kDt);
            ballX_ = kBallHome + (1.f - roll_ / 0.28f) * 36.f;
        }
        if (modeT_ > 0.45f) {
            ballX_ = kBallHome;
            if (dead_ >= kDeadLimit) {
                won_ = false;
                rung_ = false;
                why_ = "third";
                mode_ = Mode::Lose;
                modeT_ = 0;
            } else {
                mode_ = Mode::Aim;
                modeT_ = 0;
                phase_ = 0.1f;
                botHeld_ = false;
            }
        }
    } else if (mode_ == Mode::Ring) {
        modeT_ += kDt;
        ballX_ = kBellX - 18.f;
        if (modeT_ > 0.5f) over_ = true;
    } else if (mode_ == Mode::Lose) {
        modeT_ += kDt;
        if (modeT_ > 0.55f) over_ = true;
    }
    if (bot_) botPlay();
    if (mode_ == Mode::Title || mode_ == Mode::Aim) modeT_ += kDt;
    bellPh_ += kDt * (rung_ ? 11.f : 2.4f);
    bellAmp_ *= rung_ ? 0.992f : 0.985f;
    if (bellAmp_ < 0.08f) bellAmp_ = rung_ ? 0.4f : 0.08f;
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
        int g = y < 48 ? 1 : (y < 180 ? 2 : 1);
        v.lineBackdrop[y] = gs::rgb4(0, g, y < 40 ? 2 : 1);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.table, 176, 148, 78, PAL_CLOTH);
    spr(art_.player, 36, 112, 64, PAL_PLAYER);

    float swing = std::sin(bellPh_) * bellAmp_ * 8.f;
    spr(art_.bell, kBellX + swing, 96, 36, PAL_BELL);

    if (mode_ == Mode::Aim || mode_ == Mode::Title || mode_ == Mode::Dead || mode_ == Mode::Lose) {
        spr(art_.cue, cueTip(), 148, 10, PAL_CUE);
    }
    spr(art_.ball, ballX_, 150, mode_ == Mode::Ring ? 18 : 14, PAL_BALL);

    for (int i = 0; i < kDeadLimit; i++) {
        int pal = i < dead_ ? PAL_DEAD : PAL_WOOD;
        spr(art_.mark, 132.f + float(i) * 16.f, 118, 10, pal);
    }

    if (art_.title.w > 0 && (mode_ == Mode::Title || mode_ == Mode::Ring)) {
        gs::Sprite s;
        s.img = art_.title;
        s.w = int16_t(art_.title.w);
        s.h = int16_t(art_.title.h);
        s.x = int16_t(160 - art_.title.w / 2);
        s.y = 8;
        s.pal = PAL_BELL;
        v.sprite(s);
    }

    if (mode_ == Mode::Title) {
        hudC(8, "THE BELL RINGS BEFORE", PAL_HUD);
        hudC(9, "THE THIRD TRY DIES", PAL_HUD);
        hudC(12, "STROKE WHEN THE TIP", PAL_HUD);
        hudC(13, "MEETS THE BALL", PAL_HUD);
        hudC(25, "A TO BEGIN", PAL_HUD);
    } else if (mode_ == Mode::Aim) {
        char line[40];
        std::snprintf(line, sizeof(line), "TRY %d", tryNo_ + 1);
        hudC(2, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "DEAD %d OF %d", dead_, kDeadLimit);
        hudC(3, line, PAL_HUD);
        hudC(25, inWindow() ? "STROKE" : "HOLD", inWindow() ? PAL_BELL : PAL_HUD);
    } else if (mode_ == Mode::Roll) {
        hudC(2, "THE CUE IS AWAY", PAL_HUD);
    } else if (mode_ == Mode::Dead) {
        char line[40];
        std::snprintf(line, sizeof(line), "TRY %d DIED", tryNo_);
        hudC(2, line, PAL_DEAD);
        std::snprintf(line, sizeof(line), "DEAD %d OF %d", dead_, kDeadLimit);
        hudC(3, line, PAL_HUD);
    } else if (mode_ == Mode::Ring) {
        hudC(8, "THE BELL RINGS", PAL_BELL);
        char line[40];
        std::snprintf(line, sizeof(line), "BEFORE TRY %d DIED", dead_ + 1);
        hudC(10, line, PAL_HUD);
    } else {
        hudC(8, "THE THIRD TRY DIED", PAL_DEAD);
        hudC(10, "THE BELL STAYS QUIET", PAL_HUD);
    }
}

}  // namespace cuebell
