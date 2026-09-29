#include "game/memorymark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace memorymark {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float CARD_Y = 128.f;
constexpr float STAMP_X = 160.f;
constexpr float STAMP_SEAT = 72.f;
constexpr float LIFT_R = 16.f;
constexpr float HAND_V = 160.f;

float cardX(int i) { return 52.f + float(i) * 72.f; }

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 2, 4));
    sys.apu.setMaster(0.7f);
    toTitle();
    if (bot_) begin();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = won_ = rules_ = false;
    marked_ = lifted_ = finished_ = false;
    showI_ = showT_ = got_ = cursor_ = pace_ = 0;
    handX_ = 40.f;
    handY_ = 190.f;
    stampY_ = STAMP_SEAT;
    note_[0] = 0;
    if (sys_) sys_->apu.silence();
}

void Game::begin() {
    toTitle();
    rules_ = true;
    over_ = won_ = false;
    mode_ = Mode::Show;
    std::snprintf(note_, sizeof note_, "WATCH THE ORDER");
    blip(440.f);
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

void Game::fail() {
    won_ = false;
    finished_ = false;
    lifted_ = false;
    over_ = true;
    mode_ = Mode::Over;
    std::snprintf(note_, sizeof note_, "THE ORDER BROKE");
    blip(98.f);
    if (sys_) sys_->setLight(140, 30, 30);
}

void Game::finish() {
    lifted_ = true;
    finished_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    stampY_ = 36.f;
    std::snprintf(note_, sizeof note_, "FINISHED MARK");
    chord(523.25f, 659.25f, 1046.5f);
    if (sys_) {
        sys_->rumble(0.35f, 0.7f, 140);
        sys_->setLight(255, 200, 60);
    }
}

void Game::confirm() {
    if (cursor_ < 0 || cursor_ >= kN) return;
    if (cursor_ != seq_[got_]) {
        fail();
        return;
    }
    got_++;
    blip(520.f + float(got_) * 40.f);
    if (got_ >= kN) {
        marked_ = true;
        mode_ = Mode::Lift;
        stampY_ = STAMP_SEAT;
        std::snprintf(note_, sizeof note_, "LIFT THE MARK");
        chord(659.25f, 783.99f, 987.77f);
        if (sys_) sys_->setLight(255, 190, 40);
    }
}

void Game::lift() {
    float dx = STAMP_X - handX_;
    float dy = stampY_ - handY_;
    if (std::hypot(dx, dy) > LIFT_R) {
        std::snprintf(note_, sizeof note_, "CLOSER");
        blip(160.f);
        return;
    }
    finish();
}

bool Game::faceUp(int i) const {
    for (int k = 0; k < got_; k++)
        if (seq_[k] == i) return true;
    if (mode_ == Mode::Show && showI_ < kN && seq_[showI_] == i && showT_ < kShowOn) return true;
    return false;
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z);
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    if (pad.pressed(gs::BTN_LEFT)) in.dx -= 1;
    if (pad.pressed(gs::BTN_RIGHT)) in.dx += 1;
    if (pad.down(gs::BTN_LEFT)) in.x -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) in.x += 1.f;
    if (pad.down(gs::BTN_UP)) in.y -= 1.f;
    if (pad.down(gs::BTN_DOWN)) in.y += 1.f;
    if (std::fabs(pad.axisX) > 0.25f) in.x = pad.axisX;
    if (std::fabs(pad.axisY) > 0.25f) in.y = -pad.axisY;
    float len = std::hypot(in.x, in.y);
    if (len > 1.f) {
        in.x /= len;
        in.y /= len;
    }
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && clock_ > 0.35f) {
        in.start = true;
        return in;
    }
    if (mode_ == Mode::Recall && got_ < kN) {
        int target = seq_[got_];
        if ((pace_ % 8) == 7) {
            if (cursor_ < target) in.dx = 1;
            else if (cursor_ > target) in.dx = -1;
            else in.action = true;
        }
        return in;
    }
    if (mode_ == Mode::Lift) {
        float dx = STAMP_X - handX_;
        float dy = stampY_ - handY_;
        float d = std::hypot(dx, dy);
        if (d <= LIFT_R - 2.f) in.action = true;
        else if (d > 1e-3f) {
            in.x = dx / d;
            in.y = dy / d;
        }
    }
    return in;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += DT;
    pace_++;
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }

    if (mode_ == Mode::Show) {
        showT_++;
        int span = kShowOn + kShowGap;
        if (showT_ >= span) {
            showT_ = 0;
            showI_++;
            if (showI_ >= kN) {
                mode_ = Mode::Recall;
                cursor_ = 0;
                pace_ = 0;
                std::snprintf(note_, sizeof note_, "REPEAT THE ORDER");
                blip(330.f);
            }
        }
    }

    Input in = bot_ ? botInput() : readPad(sys.pad);

    if (mode_ == Mode::Title) {
        if (in.back && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (in.start || in.action) begin();
    } else if (mode_ == Mode::Pause) {
        if (in.start) mode_ = held_;
        else if (in.back) toTitle();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (in.start || in.action)) begin();
        else if (!bot_ && in.back) toTitle();
    } else if (mode_ == Mode::Show) {
        if (in.start && !bot_) {
            held_ = mode_;
            mode_ = Mode::Pause;
        } else if (in.back && !bot_) toTitle();
    } else if (mode_ == Mode::Recall) {
        if (in.start && !bot_) {
            held_ = mode_;
            mode_ = Mode::Pause;
        } else if (in.back && !bot_) toTitle();
        else {
            cursor_ = std::max(0, std::min(kN - 1, cursor_ + in.dx));
            if (in.action) confirm();
        }
    } else if (mode_ == Mode::Lift) {
        if (in.start && !bot_) {
            held_ = mode_;
            mode_ = Mode::Pause;
        } else if (in.back && !bot_) toTitle();
        else {
            handX_ = clampf(handX_ + in.x * HAND_V * DT, 12.f, 308.f);
            handY_ = clampf(handY_ + in.y * HAND_V * DT, 12.f, 210.f);
            if (in.action) lift();
        }
    }

    draw();
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(2, 2, 5);
        if (y > 36) c = gs::rgb4(3, 3, 6);
        if (y > 96) c = gs::rgb4(4, 5, 4);
        if (y > 168) c = gs::rgb4(3, 2, 2);
        v.lineBackdrop[y] = c;
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
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

    bool showHand = mode_ == Mode::Lift || (mode_ == Mode::Over && lifted_) ||
                    (mode_ == Mode::Pause && held_ == Mode::Lift);
    if (showHand) spr(art_.hand, handX_, handY_, float(art_.hand.w), float(art_.hand.h), PAL_HAND);

    if (mode_ == Mode::Recall) spr(art_.cursor, cardX(cursor_), CARD_Y - 32.f, 12.f, 8.f, PAL_CURSOR);

    if (marked_ || mode_ == Mode::Title)
        spr(art_.stamp, STAMP_X, mode_ == Mode::Title ? 78.f : stampY_, 28.f, 28.f, PAL_STAMP);

    for (int i = kN - 1; i >= 0; i--) {
        const gs::Image& img = faceUp(i) ? art_.face[i] : art_.back;
        int pal = faceUp(i) ? PAL_FACE : PAL_BACK;
        spr(img, cardX(i), CARD_Y, 36.f, 48.f, pal);
    }

    if (mode_ == Mode::Title) {
        hudC(2, "S3 MEMORYMARK", PAL_GOLD);
        hudC(5, "A FINISHED MARK ENDS IT", PAL_INK);
        hudC(20, "WATCH FOUR  THEN LIFT", PAL_INK);
        hudC(23, "START", PAL_OK);
    } else if (mode_ == Mode::Pause) {
        hudC(2, "PAUSED", PAL_INK);
    } else if (mode_ == Mode::Over) {
        hudC(2, note_, won_ ? PAL_GOLD : PAL_ALERT);
        if (!bot_) hudC(23, won_ ? "THE MARK IS DONE" : "START", won_ ? PAL_OK : PAL_ALERT);
    } else {
        hudC(2, note_, marked_ ? PAL_GOLD : PAL_INK);
        char prog[16];
        std::snprintf(prog, sizeof prog, "%d OF %d", got_, kN);
        hudC(24, mode_ == Mode::Lift ? "WALK IN AND LIFT" : prog, PAL_INK);
    }
}

}  // namespace memorymark
