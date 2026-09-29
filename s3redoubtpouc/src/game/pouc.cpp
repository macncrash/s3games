#include "game/pouc.h"

#include <cmath>

namespace pouc {
namespace {

constexpr float GROUND = 176;
constexpr float GRAV = 0.28f;
constexpr float JUMP = -5.0f;
constexpr float RUN = 1.55f;
constexpr float PW = 14;
constexpr float PH = 32;
constexpr float BAR_X = 92;
constexpr float BAR_W = 18;
constexpr float PIT_L = 214;
constexpr float PIT_R = 246;
constexpr float GATE_X = 300;
constexpr float GUARD_L = 146;
constexpr float GUARD_R = 158;
constexpr float GW = 14;
constexpr float GH = 22;

bool hit(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    return 1;
}

void Game::chime(bool win) {
    if (!sys_) return;
    if (win) {
        sys_->apu.tone(0, 523, 0.18f);
        sys_->apu.tone(1, 659, 0.14f);
        sys_->apu.tone(2, 784, 0.12f);
    } else {
        sys_->apu.tone(0, 110, 0.2f);
        sys_->apu.noiseBurst(0.25f, 1400, 0.2f);
    }
}

void Game::resetRun() {
    runFrame_ = 0;
    px_ = 22;
    py_ = GROUND - PH;
    vy_ = 0;
    ground_ = true;
    face_ = true;
    guard_ = GUARD_L;
    guardDir_ = 1;
    have_ = true;
    pouchX_ = px_ + 8;
    pouchY_ = py_ + 14;
    pouchVy_ = 0;
    won_ = false;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    resetRun();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys.vdp.road[y].on = false;
        int dusk = y < 120 ? y / 18 : 6;
        sys.vdp.lineBackdrop[y] = gs::rgb4(2 + dusk / 3, 2 + dusk / 4, 5 + (y > 150 ? 1 : 0));
        sys.vdp.lineFog[y] = 0;
    }
}

void Game::bot(bool& left, bool& right, bool& jump) const {
    left = false;
    right = true;
    jump = false;
    const bool closed = (runFrame_ % 170) >= 96;
    const float next = px_ + RUN;
    if (closed && next + PW > BAR_X - 2 && px_ < BAR_X + BAR_W) {
        if (px_ + PW < BAR_X + 4) right = false;
    }
    // One arc clears the pacing sentry. The next clears the ditch.
    if (ground_ && px_ >= 116 && px_ <= 128) jump = true;
    if (ground_ && px_ >= 182 && px_ <= 198) jump = true;
}

void Game::logic() {
    bool left = false, right = false, jump = false;
    if (bot_) {
        bot(left, right, jump);
    } else if (sys_) {
        const gs::Pad& p = sys_->pad;
        left = p.down(gs::BTN_LEFT);
        right = p.down(gs::BTN_RIGHT);
        jump = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_UP);
        if (p.axisX < -0.3f) left = true;
        if (p.axisX > 0.3f) right = true;
    }
    if (left && !right) {
        px_ -= RUN;
        face_ = false;
    } else if (right && !left) {
        px_ += RUN;
        face_ = true;
    }
    if (jump && ground_ && have_) {
        vy_ = JUMP;
        ground_ = false;
        if (sys_) sys_->apu.tone(0, 340, 0.08f);
    }

    const bool closed = (runFrame_ % 170) >= 96;
    if (closed && have_) {
        if (hit(px_, py_, PW, PH, BAR_X, GROUND - 78, BAR_W, 78)) {
            if (px_ + PW * 0.5f < BAR_X + 3) px_ = BAR_X - PW - 0.5f;
            else if (px_ + PW * 0.5f > BAR_X + BAR_W - 3) px_ = BAR_X + BAR_W + 0.5f;
            else {
                have_ = false;
                pouchVy_ = -2.2f;
            }
        }
    }

    vy_ += GRAV;
    py_ += vy_;
    const float cx = px_ + PW * 0.5f;
    const bool overPit = cx > PIT_L && cx < PIT_R;
    if (!overPit && py_ >= GROUND - PH) {
        py_ = GROUND - PH;
        vy_ = 0;
        ground_ = true;
    } else if (overPit && py_ >= GROUND - PH) {
        ground_ = false;
        if (py_ > GROUND + 8) have_ = false;
    } else {
        ground_ = false;
    }

    guard_ += guardDir_ * 0.28f;
    if (guard_ > GUARD_R) {
        guard_ = GUARD_R;
        guardDir_ = -1;
    } else if (guard_ < GUARD_L) {
        guard_ = GUARD_L;
        guardDir_ = 1;
    }
    if (have_ && hit(px_, py_, PW, PH, guard_, GROUND - GH, GW, GH)) {
        have_ = false;
        pouchVy_ = -1.6f;
    }

    if (px_ < 8) px_ = 8;
    if (px_ > 306) px_ = 306;

    if (have_) {
        pouchX_ = px_ + (face_ ? 8 : -2);
        pouchY_ = py_ + 14;
        if (px_ + PW >= GATE_X && ground_ && cx > PIT_R) {
            mode_ = Mode::Won;
            won_ = true;
            over_ = true;
            chime(true);
        }
    } else {
        pouchVy_ += GRAV;
        pouchY_ += pouchVy_;
        if (pouchY_ > GROUND - 6) {
            pouchY_ = GROUND - 6;
            pouchVy_ = 0;
            if (mode_ == Mode::Run) {
                mode_ = Mode::Lost;
                over_ = true;
                chime(false);
            }
        }
    }
    runFrame_++;
}

void Game::spr(const gs::Image& img, float x, float y, float w, float h, int pal, bool flip, int fog) {
    gs::Sprite s;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(fog);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    const int flicker = (runFrame_ / 8) & 1;

    if (mode_ == Mode::Title || mode_ == Mode::Won || mode_ == Mode::Lost) {
        spr(art_.title, 78, 16, float(art_.title.w), float(art_.title.h), PAL_HUD);
        spr(art_.hint, 70, 48, float(art_.hint.w), float(art_.hint.h), PAL_HUD);
    }
    if (mode_ == Mode::Won) spr(art_.win, 62, 78, float(art_.win.w), float(art_.win.h), PAL_HUD);
    if (mode_ == Mode::Lost) spr(art_.lose, 78, 78, float(art_.lose.w), float(art_.lose.h), PAL_HUD);
    if (mode_ == Mode::Title) spr(art_.mark, 128, 70, float(art_.mark.w), float(art_.mark.h), PAL_HUD);

    spr(art_.runner, px_, py_, 24, 36, PAL_RUNNER, !face_);
    spr(art_.pouch, pouchX_, pouchY_, 16, 14, PAL_POUCH);

    const bool closed = (runFrame_ % 170) >= 96;
    float barY = closed ? GROUND - 78 : GROUND - 126;
    spr(art_.bar, BAR_X, barY, BAR_W, 52, PAL_IRON);

    spr(art_.sentry, guard_, GROUND - GH - 2, 22, 34, PAL_SENTRY, guardDir_ < 0);
    spr(art_.gate, GATE_X - 6, GROUND - 62, 28, 64, PAL_IRON);

    for (float x = PIT_L - 4; x < PIT_R; x += 32) spr(art_.water, x, GROUND - 2 + flicker, 32, 20, PAL_WATER, false, 1);

    for (float x = 0; x < 320; x += 32) {
        if (x + 32 > PIT_L && x < PIT_R) continue;
        spr(art_.plank, x, GROUND, 32, 16, PAL_STONE);
    }
    for (int row = 0; row < 6; row++) {
        for (float x = 0; x < 320; x += 32) {
            spr(art_.block, x, 8 + row * 22, 32, 22, PAL_STONE, false, row < 2 ? 6 : 2);
        }
    }
    for (float x = 4; x < 320; x += 22) spr(art_.merlon, x, 0, 16, 14, PAL_STONE, false, 4);
    spr(art_.torch, 36, 78, 10, 16, PAL_TORCH);
    spr(art_.torch, 250, 78, 10, 16, PAL_TORCH);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || sys.pad.anyPressed()) {
            resetRun();
            mode_ = Mode::Run;
            over_ = false;
            won_ = false;
        }
    } else if (mode_ == Mode::Run) {
        logic();
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
        resetRun();
        mode_ = Mode::Run;
        over_ = false;
        won_ = false;
    }
    draw();
    if (mode_ != Mode::Run && sys.frame > 8 && (sys.frame % 40) == 0) sys.apu.tone(2, 0, 0);
}

}  // namespace pouc
