#include "game/tilemark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tilemark {
namespace {

constexpr float DT = 1.f / 60.f;

float slotX(int i) { return 52.f + float(i) * 72.f; }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(3, 3, 4));
    sys.apu.setMaster(0.7f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = won_ = marked_ = finished_ = false;
    cursor_ = laid_ = pace_ = hold_ = 0;
    clock_ = 0;
    for (int i = 0; i < kN; i++) face_[i] = 0;
    note_[0] = 0;
    if (sys_) sys_->apu.silence();
}

void Game::begin() {
    toTitle();
    mode_ = Mode::Lay;
    std::snprintf(note_, sizeof note_, "LAY THE TILES");
    blip(440.f);
}

void Game::countLaid() {
    laid_ = 0;
    for (int i = 0; i < kN; i++)
        if (face_[i] == kMark[i]) laid_++;
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = std::max(beep_, 0.09f);
}

void Game::chord(float a, float b, float c) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.08f);
    sys_->apu.tone(1, b, 0.06f);
    sys_->apu.tone(2, c, 0.05f);
    beep_ = 0.28f;
}

void Game::decay() {
    if (beep_ <= 0 || !sys_) return;
    beep_ -= DT;
    if (beep_ <= 0) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::turn() {
    if (cursor_ < 0 || cursor_ >= kN) return;
    face_[cursor_] = (face_[cursor_] + 1) % 4;
    countLaid();
    blip(380.f + float(face_[cursor_]) * 50.f);
    if (laid_ >= kN) {
        marked_ = true;
        mode_ = Mode::Stamp;
        std::snprintf(note_, sizeof note_, "PRESS THE MARK");
        chord(659.25f, 783.99f, 987.77f);
        if (sys_) sys_->setLight(255, 190, 40);
    }
}

void Game::finish() {
    if (!marked_ || laid_ < kN) return;
    finished_ = true;
    won_ = true;
    mode_ = Mode::Win;
    hold_ = 0;
    std::snprintf(note_, sizeof note_, "FINISHED MARK");
    chord(523.25f, 659.25f, 1046.5f);
    if (sys_) {
        sys_->rumble(0.35f, 0.7f, 140);
        sys_->setLight(255, 200, 60);
    }
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z);
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    if (pad.pressed(gs::BTN_LEFT)) in.dx -= 1;
    if (pad.pressed(gs::BTN_RIGHT)) in.dx += 1;
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && clock_ > 0.35f) {
        in.start = true;
        return in;
    }
    if (mode_ == Mode::Lay && (pace_ % 8) == 7) {
        int want = 0;
        while (want < kN && face_[want] == kMark[want]) want++;
        if (want < kN) {
            if (cursor_ < want) in.dx = 1;
            else if (cursor_ > want) in.dx = -1;
            else in.action = true;
        }
        return in;
    }
    if (mode_ == Mode::Stamp && (pace_ % 8) == 4) in.action = true;
    return in;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += DT;
    pace_++;
    decay();

    Input in = bot_ ? botInput() : readPad(sys.pad);

    if (mode_ == Mode::Title) {
        if (in.back && !bot_) sys.quit();
        else if (in.start || (in.action && !bot_)) begin();
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (in.start || in.action) mode_ = held_;
        else if (in.back) sys.quit();
        draw();
        return;
    }
    if (mode_ == Mode::Win) {
        hold_++;
        if (hold_ > 70) {
            over_ = true;
            if (!sys.headless) sys.quit();
        }
        draw();
        return;
    }

    if (!bot_ && in.start) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        return;
    }
    if (!bot_ && in.back) {
        toTitle();
        draw();
        return;
    }

    bool opened = false;
    if (mode_ == Mode::Lay) {
        if (in.dx) cursor_ = std::max(0, std::min(kN - 1, cursor_ + in.dx));
        if (in.action) {
            turn();
            opened = mode_ == Mode::Stamp;
        }
    }
    if (mode_ == Mode::Stamp && in.action && !opened) finish();

    draw();
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(2, 2, 4);
        if (y > 28) c = gs::rgb4(3, 3, 5);
        if (y > 88) c = gs::rgb4(5, 4, 3);
        if (y > 168) c = gs::rgb4(4, 3, 2);
        v.lineBackdrop[y] = c;
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    bool showPlay = mode_ != Mode::Title;
    if (showPlay && mode_ != Mode::Win)
        spr(art_.cursor, slotX(cursor_), mode_ == Mode::Stamp ? 48.f : 168.f, 14.f, 8.f, PAL_CURSOR);
    if (marked_ || mode_ == Mode::Title || mode_ == Mode::Win)
        spr(art_.stamp, 160.f, mode_ == Mode::Title ? 70.f : 36.f, 26.f, 26.f, PAL_STAMP);

    if (showPlay) {
        for (int i = kN - 1; i >= 0; i--) {
            spr(art_.plate, slotX(i), 64.f, 36.f, 40.f, PAL_MARK);
            spr(art_.face[kMark[i]], slotX(i), 64.f, 22.f, 26.f, PAL_MARK);
            spr(art_.plate, slotX(i), 140.f, 36.f, 40.f, PAL_TILE);
            spr(art_.face[face_[i]], slotX(i), 140.f, 24.f, 28.f, PAL_TILE);
        }
    }

    if (mode_ == Mode::Title) {
        hudC(2, "S3 TILEMARK", PAL_GOLD);
        hudC(5, "A FINISHED MARK ENDS IT", PAL_INK);
        hudC(18, "MATCH THE ROW", PAL_INK);
        hudC(20, "THEN STAMP THE MARK", PAL_INK);
        hudC(24, "START", PAL_OK);
    } else if (mode_ == Mode::Pause) {
        hudC(2, "PAUSED", PAL_INK);
    } else if (mode_ == Mode::Win) {
        hudC(2, note_, PAL_GOLD);
        hudC(24, "THE MARK IS DONE", PAL_OK);
    } else {
        hudC(2, note_, marked_ ? PAL_GOLD : PAL_INK);
        char prog[20];
        std::snprintf(prog, sizeof prog, "%d OF %d", laid_, kN);
        hudC(24, mode_ == Mode::Stamp ? "A STAMPS THE MARK" : prog, PAL_INK);
    }
}

}  // namespace tilemark
