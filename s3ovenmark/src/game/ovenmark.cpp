#include "game/ovenmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ovenmark {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float LOAF_X = 160.f;
constexpr float LOAF_Y = 118.f;
constexpr float COIN_SEAT = 100.f;
constexpr float COIN_HOVER = 72.f;
constexpr float GOLD_LO = 62.f;
constexpr float GOLD_HI = 82.f;
constexpr float CHAR_AT = 94.f;
constexpr float HEAT_RATE = 16.f;
constexpr float LIFT_R = 18.f;
constexpr float HAND_V = 150.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(4, 2, 1));
    sys.apu.setMaster(0.7f);
    toTitle();
    if (bot_) begin();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = won_ = rules_ = false;
    marked_ = onGold_ = lifted_ = finished_ = false;
    heat_ = 0;
    lock_ = 0;
    coinY_ = COIN_HOVER;
    handX_ = 46.f;
    handY_ = 188.f;
    note_[0] = 0;
    if (sys_) sys_->apu.silence();
}

void Game::begin() {
    toTitle();
    rules_ = true;
    over_ = won_ = false;
    mode_ = Mode::Bake;
    std::snprintf(note_, sizeof note_, "PRESS THE MARK");
    blip(392.f);
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
    std::snprintf(note_, sizeof note_, "THE CRUST DIED");
    blip(96.f);
    if (sys_) sys_->setLight(120, 30, 20);
}

void Game::finish() {
    lifted_ = true;
    finished_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    std::snprintf(note_, sizeof note_, "FINISHED MARK");
    chord(523.25f, 659.25f, 1046.5f);
    if (sys_) {
        sys_->rumble(0.35f, 0.75f, 140);
        sys_->setLight(255, 190, 50);
    }
}

void Game::stamp() {
    if (lock_ > 0 || marked_) return;
    if (heat_ >= GOLD_LO && heat_ <= GOLD_HI) {
        marked_ = true;
        onGold_ = true;
        coinY_ = COIN_SEAT;
        mode_ = Mode::Lift;
        std::snprintf(note_, sizeof note_, "LIFT THE MARK");
        chord(659.25f, 783.99f, 987.77f);
        if (sys_) {
            sys_->rumble(0.25f, 0.55f, 50);
            sys_->setLight(255, 180, 40);
        }
        return;
    }
    lock_ = 0.35f;
    coinY_ = COIN_SEAT + 6.f;
    if (heat_ < GOLD_LO) {
        std::snprintf(note_, sizeof note_, "TOO SOON");
        blip(140.f);
    } else {
        std::snprintf(note_, sizeof note_, "PAST THE GOLD");
        blip(110.f);
    }
}

void Game::lift() {
    float dx = LOAF_X - handX_;
    float dy = coinY_ - handY_;
    if (std::hypot(dx, dy) > LIFT_R) {
        std::snprintf(note_, sizeof note_, "CLOSER");
        blip(160.f);
        return;
    }
    coinY_ = COIN_HOVER - 8.f;
    finish();
}

int Game::loafPal() const {
    if (heat_ >= CHAR_AT) return PAL_CHAR;
    if (onGold_ || heat_ >= GOLD_LO) return PAL_CRUST;
    if (heat_ >= 28.f) return PAL_PALE;
    return PAL_LOAF;
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z);
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    if (pad.down(gs::BTN_LEFT)) in.x -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) in.x += 1.f;
    if (pad.down(gs::BTN_UP)) in.y -= 1.f;
    if (pad.down(gs::BTN_DOWN)) in.y += 1.f;
    if (std::fabs(pad.axisX) > 0.2f) in.x = pad.axisX;
    if (std::fabs(pad.axisY) > 0.2f) in.y = -pad.axisY;
    float len = std::hypot(in.x, in.y);
    if (len > 1.f) {
        in.x /= len;
        in.y /= len;
    }
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && clock_ > 0.3f) {
        in.start = true;
        return in;
    }
    if (mode_ == Mode::Bake && lock_ <= 0 && heat_ >= 70.f && heat_ <= 78.f) {
        in.action = true;
        return in;
    }
    if (mode_ == Mode::Lift) {
        float dx = LOAF_X - handX_;
        float dy = coinY_ - handY_;
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
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }
    if (lock_ > 0) {
        lock_ -= DT;
        if (lock_ <= 0 && mode_ == Mode::Bake && !marked_) coinY_ = COIN_HOVER;
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
    } else if (mode_ == Mode::Bake) {
        if (in.start && !bot_) {
            held_ = mode_;
            mode_ = Mode::Pause;
        } else if (in.back && !bot_) toTitle();
        else {
            heat_ = std::min(100.f, heat_ + HEAT_RATE * DT);
            if (in.action) stamp();
            if (mode_ == Mode::Bake && heat_ >= CHAR_AT) fail();
        }
    } else if (mode_ == Mode::Lift) {
        if (in.start && !bot_) {
            held_ = mode_;
            mode_ = Mode::Pause;
        } else if (in.back && !bot_) toTitle();
        else {
            handX_ = clampf(handX_ + in.x * HAND_V * DT, 16.f, 304.f);
            handY_ = clampf(handY_ + in.y * HAND_V * DT, 16.f, 210.f);
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
        uint16_t c = gs::rgb4(3, 2, 4);
        if (y > 40) c = gs::rgb4(5, 3, 3);
        if (y > 150) c = gs::rgb4(4, 2, 2);
        if (y > 190) c = gs::rgb4(2, 1, 1);
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

    float bob = std::sin(clock_ * 3.f) * 1.5f;
    spr(art_.arch, 160.f, 108.f, float(art_.arch.w), float(art_.arch.h), PAL_OVEN);
    for (int i = 0; i < 5; i++) {
        float fx = 92.f + float(i) * 28.f;
        float fh = 18.f + 6.f * std::sin(clock_ * 8.f + float(i));
        spr(art_.flame, fx, 156.f - fh * 0.2f, 14.f, fh, PAL_FIRE);
    }
    spr(art_.peel, 168.f, 168.f, float(art_.peel.w), float(art_.peel.h), PAL_PEEL);
    spr(art_.loaf, LOAF_X, LOAF_Y, float(art_.loaf.w), float(art_.loaf.h), loafPal());

    float cy = coinY_ + (mode_ == Mode::Bake && lock_ <= 0 ? bob : 0.f);
    spr(art_.coin, LOAF_X, cy, float(art_.coin.w), float(art_.coin.h), PAL_COIN);
    if (mode_ == Mode::Lift || mode_ == Mode::Over || mode_ == Mode::Pause)
        spr(art_.hand, handX_, handY_, float(art_.hand.w), float(art_.hand.h), PAL_HAND);

    int filled = int(clampf(heat_, 0.f, 100.f) / 100.f * 20.f);
    for (int i = 0; i < 20; i++) {
        int pal = PAL_LOAF;
        if (i >= 12 && i < 17) pal = PAL_CRUST;
        if (i >= 19) pal = PAL_CHAR;
        if (i >= filled) pal = PAL_OVEN;
        spr(art_.bar, 80.f + float(i) * 8.f, 186.f, 6.f, 6.f, pal);
    }

    if (mode_ == Mode::Title) {
        hudC(3, "S3 OVENMARK", PAL_GOLD);
        hudC(6, "A FINISHED MARK ENDS IT", PAL_INK);
        hudC(22, "START", PAL_OK);
    } else if (mode_ == Mode::Pause) {
        hudC(3, "PAUSED", PAL_INK);
    } else if (mode_ == Mode::Over) {
        hudC(3, note_, won_ ? PAL_GOLD : PAL_ALERT);
        if (!bot_) hudC(22, won_ ? "THE MARK IS DONE" : "START", won_ ? PAL_OK : PAL_ALERT);
    } else {
        hudC(2, note_, onGold_ ? PAL_GOLD : PAL_INK);
        hudC(24, mode_ == Mode::Lift ? "WALK IN AND LIFT" : "STAMP THE GOLD BAND", PAL_INK);
    }
}

}  // namespace ovenmark
