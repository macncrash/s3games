#include "game/pawn.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace pawnchime {

int Game::clockSec() const { return kStartSec + playFrames_ / 60; }

int Game::hour() const {
    int h = (clockSec() / 3600) % 12;
    return h == 0 ? 12 : h;
}

int Game::minute() const { return (clockSec() / 60) % 60; }

int Game::second() const { return clockSec() % 60; }

bool Game::onHour() const {
    const int s = clockSec();
    return s >= kHourSec && s < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

bool Game::ready() const { return misses_ == 0 && steps_ == kSteps; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.4f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
}

void Game::begin() {
    steps_ = 0;
    square_ = 1;
    misses_ = 0;
    playFrames_ = 0;
    over_ = won_ = false;
    why_ = "";
    mode_ = Mode::File;
    sys_->apu.silence();
    sys_->apu.tone(0, 392.f, 0.05f);
}

void Game::fail(const char* why) {
    won_ = false;
    over_ = true;
    why_ = why;
    mode_ = Mode::Over;
    misses_++;
    sys_->apu.noiseBurst(0.28f, 80.f, 0.18f);
}

void Game::push() {
    // The square after the fourth step is occupied. Do not take it.
    const int next = square_ + 1;
    if (steps_ >= kSteps || next >= 8 || next > 1 + kSteps) {
        fail("the pawn stepped off the file");
        return;
    }
    square_ = next;
    steps_++;
    sys_->apu.tone(0, 440.f + steps_ * 40.f, 0.06f);
}

void Game::leave() {
    over_ = true;
    mode_ = Mode::Over;
    if (ready() && onHour()) {
        won_ = true;
        why_ = "CHIME";
        sys_->apu.tone(0, 523.f, 0.12f);
        sys_->apu.tone(1, 659.f, 0.08f);
        sys_->apu.tone(2, 784.f, 0.06f);
    } else {
        won_ = false;
        if (misses_ > 0) why_ = "the pawn stepped off the file";
        else if (steps_ < kSteps) why_ = "the file is not full";
        else if (pastHour()) why_ = "the chime went unheard";
        else why_ = "the hour has not chimed";
        sys_->apu.noiseBurst(0.28f, 70.f, 0.18f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::File) {
        playFrames_++;
        if (pastHour()) {
            fail("the chime went unheard");
        } else if (bot_) {
            if (steps_ < kSteps && playFrames_ % 40 == 18) push();
            else if (steps_ == kSteps && onHour()) leave();
        } else {
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_X)) push();
            else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_B)) leave();
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (!won_) {
                mode_ = Mode::Title;
                over_ = false;
            }
        }
        if (bot_ || won_) {
            // One painted frame of the result, then the sim (or the player) is done.
            if (won_ && !sys.headless) sys.quit();
        }
    }
    paint();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
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

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int band = y < 90 ? 0 : (y < 160 ? 1 : 2);
        uint16_t c = gs::rgb4(1, 2, 4);
        if (band == 1) c = gs::rgb4(1, 4, 3);
        if (band == 2) c = gs::rgb4(2, 3, 2);
        if (mode_ != Mode::Title && onHour()) c = gs::rgb4(6, 5, 2);
        if (mode_ == Mode::Over && won_) c = gs::rgb4(4, 5, 2);
        v.lineBackdrop[y] = c;
    }
}

void Game::paint() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    spr(art_.board, 160.f, 128.f, float(art_.board.w), float(art_.board.h), PAL_BOARD);

    const float origin = 160.f - 3.5f * 30.f;
    const int showSq = (mode_ == Mode::Title) ? 1 : square_;
    spr(art_.pawn, origin + showSq * 30.f, 118.f, float(art_.pawn.w), float(art_.pawn.h), PAL_PAWN);
    spr(art_.foe, origin + (2 + kSteps) * 30.f, 118.f, float(art_.foe.w), float(art_.foe.h), PAL_FOE);

    float bell = onHour() && mode_ != Mode::Title ? 36.f : 28.f;
    spr(art_.bell, 160.f, 46.f, bell, bell, PAL_BELL);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(2, "S3 PAWNCHIME", PAL_TITLE);
        hudC(18, "PUSH THE FILE", PAL_INK);
        hudC(20, "LEAVE WHEN THE HOUR CHIMES", PAL_HINT);
        hudC(24, "A STEP   START LEAVE", PAL_INK);
    } else {
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hudC(2, buf, onHour() ? PAL_WIN : PAL_TITLE);
        std::snprintf(buf, sizeof buf, "STEPS %d/%d", steps_, kSteps);
        hudC(18, buf, PAL_INK);
        if (mode_ == Mode::Over) {
            hudC(21, won_ ? "THE HOUR CHIMES" : why_, won_ ? PAL_WIN : PAL_BAD);
        } else if (onHour() && ready()) {
            hudC(21, "CHIME  LEAVE", PAL_WIN);
        } else if (steps_ < kSteps) {
            hudC(21, "STEP THE PAWN", PAL_HINT);
        } else {
            hudC(21, "WAIT FOR THE HOUR", PAL_HINT);
        }
    }
}

}  // namespace pawnchime
