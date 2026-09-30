#include "game/inkwellmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace inkwellmark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kNeed = 4;
constexpr int kDry = 3;
constexpr float kPeriod = 0.92f;
constexpr float kWellHi = 0.22f;

bool dipDown(const gs::Pad& p) {
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_DOWN);
}

float dipAmount(float phase) {
    return 0.5f + 0.5f * std::sin(phase * 6.2831853f / kPeriod);
}

}  // namespace

void Game::blip(bool high) {
    sys_->apu.tone(1, high ? 740.f : 180.f, 0.07f);
    toneT_ = 0.08f;
}

void Game::wet() {
    sys_->apu.tone(0, 220.f, 0.10f);
    sys_->apu.noiseBurst(0.05f, 400.f, 0.04f);
    toneT_ = 0.14f;
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
    dips_ = 0;
    dry_ = 0;
    phase_ = 0.05f;
    flash_ = 0;
    won_ = false;
    over_ = false;
    finished_ = false;
    inked_ = false;
    botHeld_ = false;
    rules_ = true;
    mode_ = Mode::Play;
    modeT_ = 0;
    sys_->apu.silence();
    if (!bot_) sys_->setLight(20, 30, 80);
}

void Game::finishMark() {
    finished_ = true;
    inked_ = true;
    won_ = true;
    mode_ = Mode::Show;
    modeT_ = 0;
    flash_ = 1.f;
    sys_->apu.tone(0, 392.f, 0.12f);
    sys_->apu.tone(1, 523.f, 0.10f);
    toneT_ = 0.5f;
    if (!bot_) sys_->setLight(40, 60, 160);
}

void Game::fail() {
    won_ = false;
    finished_ = false;
    inked_ = false;
    mode_ = Mode::Lose;
    modeT_ = 0;
    sys_->apu.silence();
    sys_->apu.tone(0, 98.f, 0.16f);
    toneT_ = 0.35f;
    if (!bot_) sys_->setLight(30, 10, 10);
}

bool Game::inWell() const { return dipAmount(phase_) <= kWellHi; }

float Game::nibY() const {
    float m = dipAmount(phase_);
    return 78.f + (1.f - m) * 58.f;
}

void Game::dip() {
    if (mode_ != Mode::Play) return;
    dips_++;
    if (inWell()) {
        good_++;
        flash_ = 0.18f;
        wet();
        if (!bot_) sys_->rumble(0.2f, 0.45f, 30);
        if (good_ >= kNeed) finishMark();
    } else {
        dry_++;
        blip(false);
        if (!bot_) sys_->rumble(0.4f, 0.05f, 50);
        if (dry_ >= kDry) fail();
    }
}

void Game::botPlay() {
    if (mode_ == Mode::Title) {
        if (modeT_ > 0.25f) begin();
        return;
    }
    if (mode_ != Mode::Play) return;
    bool in = inWell();
    if (in && !botHeld_) {
        dip();
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
    sys.vdp.setFogColor(gs::rgb4(2, 2, 3));
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
        if (dipDown(pad)) dip();
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
        int shade = y < 120 ? 2 : 3;
        int blue = y < 90 ? 4 : 3;
        v.lineBackdrop[y] = gs::rgb4(shade, shade, blue);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.desk, 160, 196, 42, PAL_DESK);
    spr(art_.page, 118, 128, 86, PAL_PAGE);
    for (int i = 0; i < good_ && i < kNeed; i++) {
        float y = 108.f + float(i) * 14.f;
        spr(art_.stroke, 118, y, 12, PAL_INK);
    }
    spr(art_.well, 236, 156, 46, PAL_INK);
    if (flash_ > 0.04f) spr(art_.blot, 236, 148, 14, PAL_BLOT);

    float qy = (mode_ == Mode::Play || mode_ == Mode::Title) ? nibY() : 86.f;
    spr(art_.quill, 228, qy - 20.f, 78, PAL_QUILL);
    spr(art_.hand, 250, qy + 8.f, 28, PAL_HAND);

    if (art_.title.w > 0 && (mode_ == Mode::Title || mode_ == Mode::Show)) {
        gs::Sprite s;
        s.img = art_.title;
        s.w = int16_t(art_.title.w);
        s.h = int16_t(art_.title.h);
        s.x = int16_t(160 - art_.title.w / 2);
        s.y = 16;
        s.pal = PAL_HUD;
        v.sprite(s);
    }

    if (mode_ == Mode::Title) {
        hudC(8, "A FINISHED MARK ENDS IT", PAL_HUD);
        hudC(10, "DIP WHEN THE NIB", PAL_HUD);
        hudC(11, "SITS IN THE INK", PAL_HUD);
        hudC(24, "A TO BEGIN", PAL_HUD);
    } else if (mode_ == Mode::Play) {
        char line[32];
        std::snprintf(line, sizeof(line), "STROKE %d OF %d", good_, kNeed);
        hudC(2, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "DRY %d OF %d", dry_, kDry);
        hudC(3, line, PAL_HUD);
        hudC(25, inWell() ? "INK" : "WAIT", inWell() ? PAL_INK : PAL_HUD);
    } else if (mode_ == Mode::Show) {
        hudC(8, "FINISHED MARK", PAL_INK);
        hudC(10, "THE INK IS SEATED", PAL_HUD);
    } else {
        hudC(8, "THE NIB RAN DRY", PAL_HUD);
        hudC(10, "NO FINISHED MARK", PAL_HUD);
    }
}

}  // namespace inkwellmark
