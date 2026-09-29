#include "game/anvilmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace anvilmark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kNeed = 4;
constexpr int kMisses = 3;
constexpr float kPeriod = 0.85f;
constexpr float kWinLo = 0.78f;

bool strikeDown(const gs::Pad& p) {
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_DOWN);
}

}  // namespace

void Game::blip(bool high) {
    sys_->apu.tone(1, high ? 880.f : 220.f, 0.08f);
    toneT_ = 0.08f;
}

void Game::clang() {
    sys_->apu.tone(0, 196.f, 0.18f);
    sys_->apu.noiseBurst(0.12f, 900.f, 0.06f);
    toneT_ = 0.18f;
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
    swings_ = 0;
    misses_ = 0;
    phase_ = 0.15f;
    flash_ = 0;
    won_ = false;
    over_ = false;
    finished_ = false;
    seated_ = false;
    botHeld_ = false;
    rules_ = true;
    mode_ = Mode::Play;
    modeT_ = 0;
    sys_->apu.silence();
    if (!bot_) sys_->setLight(80, 40, 10);
}

void Game::finishMark() {
    finished_ = true;
    seated_ = true;
    won_ = true;
    mode_ = Mode::Show;
    modeT_ = 0;
    flash_ = 1.f;
    sys_->apu.tone(0, 330.f, 0.12f);
    sys_->apu.tone(1, 494.f, 0.10f);
    toneT_ = 0.5f;
    if (!bot_) sys_->setLight(180, 140, 30);
}

void Game::fail() {
    won_ = false;
    finished_ = false;
    seated_ = false;
    mode_ = Mode::Lose;
    modeT_ = 0;
    sys_->apu.silence();
    sys_->apu.tone(0, 90.f, 0.16f);
    toneT_ = 0.35f;
    if (!bot_) sys_->setLight(40, 10, 8);
}

bool Game::inWindow() const {
    float m = 0.5f + 0.5f * std::sin(phase_ * 6.2831853f / kPeriod);
    return m >= kWinLo;
}

float Game::hammerY() const {
    float m = 0.5f + 0.5f * std::sin(phase_ * 6.2831853f / kPeriod);
    return 46.f + (1.f - m) * 52.f;
}

void Game::strike() {
    if (mode_ != Mode::Play) return;
    swings_++;
    if (inWindow()) {
        good_++;
        flash_ = 0.22f;
        clang();
        if (!bot_) sys_->rumble(0.35f, 0.7f, 40);
        if (good_ >= kNeed) finishMark();
    } else {
        misses_++;
        blip(false);
        if (!bot_) sys_->rumble(0.5f, 0.1f, 60);
        if (misses_ >= kMisses) fail();
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
        strike();
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
        if (strikeDown(pad)) strike();
    } else if (mode_ == Mode::Show) {
        modeT_ += kDt;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.f || m.h < 1) return;
    float sc = h / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.w = int16_t(std::clamp(int(std::lround(m.w * sc)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int heat = 0;
        if (y > 140) heat = (y - 140) / 18;
        v.lineBackdrop[y] = gs::rgb4(1 + heat, 1, 2 + (40 - y / 8) / 8);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    if (flash_ > 0.05f) {
        spr(art_.spark, 168, 132, 22, PAL_SPARK);
        spr(art_.spark, 196, 124, 14, PAL_SPARK);
        spr(art_.spark, 150, 126, 12, PAL_GOLD);
    }

    spr(art_.hammer, 188, hammerY(), 70, PAL_IRON);
    spr(art_.bar, 176, 148, 20, mode_ == Mode::Lose ? PAL_IRON : PAL_FIRE);
    for (int i = 0; i < kNeed; i++) {
        int pal = i < good_ ? PAL_GOLD : PAL_IRON;
        spr(art_.notch, 148.f + float(i) * 18.f, 148, 14, pal);
    }
    spr(art_.anvil, 176, 176, 58, PAL_IRON);
    spr(art_.smith, 78, 150, 86, PAL_SMITH);

    if (art_.title.w > 0 && (mode_ == Mode::Title || mode_ == Mode::Show)) {
        gs::Sprite s;
        s.img = art_.title;
        s.w = int16_t(art_.title.w);
        s.h = int16_t(art_.title.h);
        s.x = int16_t(160 - art_.title.w / 2);
        s.y = 18;
        s.pal = PAL_GOLD;
        v.sprite(s);
    }

    if (mode_ == Mode::Title) {
        hudC(8, "A FINISHED MARK ENDS IT", PAL_HUD);
        hudC(10, "STRIKE WHEN THE HAMMER", PAL_HUD);
        hudC(11, "SITS IN THE GOLD", PAL_HUD);
        hudC(24, "A TO BEGIN", PAL_HUD);
    } else if (mode_ == Mode::Play) {
        char line[32];
        std::snprintf(line, sizeof(line), "MARK %d OF %d", good_, kNeed);
        hudC(2, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "COOL %d OF %d", misses_, kMisses);
        hudC(3, line, PAL_HUD);
        hudC(25, inWindow() ? "GOLD" : "WAIT", inWindow() ? PAL_GOLD : PAL_HUD);
    } else if (mode_ == Mode::Show) {
        hudC(8, "FINISHED MARK", PAL_GOLD);
        hudC(10, "THE STAMP IS SEATED", PAL_HUD);
    } else {
        hudC(8, "THE IRON COOLED", PAL_HUD);
        hudC(10, "NO FINISHED MARK", PAL_HUD);
    }
}

}  // namespace anvilmark
