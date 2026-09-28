#include "game/ovenbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ovenbell {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float GOLD_LO = 56.f;
constexpr float GOLD_HI = 74.f;
constexpr float CHAR_AT = 92.f;
constexpr float HEAT_RATE = 20.f;
constexpr float LOAF_X = 158.f;
constexpr float LOAF_Y = 128.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Bake: return "bake";
    case Mode::Dead: return "dead";
    case Mode::Ring: return "ring";
    case Mode::Leave: return "leave";
    case Mode::Over: return won_ ? "bell" : "dead";
    }
    return "dead";
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(4, 2, 1));
    sys.apu.setMaster(0.7f);
    audit();
    toTitle();
    if (bot_) begin();
}

void Game::audit() {
    rules_ = kTries == 3 && GOLD_HI > GOLD_LO && CHAR_AT > GOLD_HI && HEAT_RATE > 0.f;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = won_ = rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    heat_ = 0.f;
    hold_ = 0;
    peelX_ = 36.f;
    bellAmp_ = 0.18f;
    why_ = "";
    note_[0] = 0;
    if (sys_) sys_->apu.silence();
}

void Game::begin() {
    over_ = won_ = rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    why_ = "";
    bellAmp_ = 0.18f;
    enterBake();
    blip(392.f);
}

void Game::enterBake() {
    if (tryNo_ >= kTries) {
        lose();
        return;
    }
    tryNo_++;
    heat_ = 0.f;
    peelX_ = 36.f;
    mode_ = Mode::Bake;
    std::snprintf(note_, sizeof note_, "TRY %d  PULL THE GOLD", tryNo_);
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = std::max(beep_, 0.09f);
}

void Game::chord(float a, float b, float c) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.09f);
    sys_->apu.tone(1, b, 0.07f);
    sys_->apu.tone(2, c, 0.05f);
    beep_ = 0.45f;
}

void Game::pull() {
    if (mode_ != Mode::Bake) return;
    peelX_ = 150.f;
    if (heat_ >= GOLD_LO && heat_ <= GOLD_HI) {
        ring();
        return;
    }
    if (heat_ < GOLD_LO) dieTry("TOO PALE");
    else dieTry("BURNT");
}

void Game::ring() {
    rung_ = true;
    bellAmp_ = 1.f;
    mode_ = Mode::Ring;
    hold_ = 70;
    std::snprintf(note_, sizeof note_, "THE BELL RINGS");
    chord(523.25f, 659.25f, 1046.5f);
    if (sys_) {
        sys_->rumble(0.4f, 0.85f, 180);
        sys_->setLight(255, 200, 60);
    }
}

void Game::dieTry(const char* why) {
    why_ = why;
    dead_++;
    mode_ = Mode::Dead;
    hold_ = 48;
    std::snprintf(note_, sizeof note_, "%s", why);
    blip(98.f);
    if (sys_) sys_->setLight(140, 30, 18);
}

void Game::lose() {
    won_ = false;
    rung_ = false;
    over_ = true;
    mode_ = Mode::Over;
    std::snprintf(note_, sizeof note_, "THIRD TRY DIED");
    blip(70.f);
}

int Game::loafPal() const {
    if (heat_ < GOLD_LO) return PAL_PALE;
    if (heat_ <= GOLD_HI) return PAL_CRUST;
    return PAL_CHAR;
}

void Game::frame(gs::System& sys) {
    clock_ += DT;
    bellPh_ += DT * (rung_ ? 16.f : 2.4f);
    if (rung_) bellAmp_ = std::max(0.22f, bellAmp_ - DT * 0.28f);
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }

    const gs::Pad& pad = sys.pad;
    const bool act = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_B);
    const bool start = pad.pressed(gs::BTN_START);

    if (mode_ == Mode::Title) {
        if (start || act) begin();
    } else if (mode_ == Mode::Bake) {
        heat_ += HEAT_RATE * DT;
        peelX_ = std::min(peelX_ + 28.f * DT, 90.f);
        bool go = act;
        if (bot_) go = heat_ >= (GOLD_LO + GOLD_HI) * 0.5f;
        if (heat_ >= CHAR_AT) dieTry("CHARRED");
        else if (go) pull();
    } else if (mode_ == Mode::Dead) {
        if (--hold_ <= 0) {
            if (dead_ >= kTries) lose();
            else enterBake();
        }
    } else if (mode_ == Mode::Ring) {
        peelX_ = std::min(220.f, peelX_ + 90.f * DT);
        if (--hold_ <= 0) {
            mode_ = Mode::Leave;
            hold_ = 36;
            std::snprintf(note_, sizeof note_, "LEAVE THE OVEN");
        }
    } else if (mode_ == Mode::Leave) {
        if (--hold_ <= 0) {
            won_ = true;
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && start) toTitle();
    }

    draw();
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(2, 1, 3);
        if (y > 28) c = gs::rgb4(4, 2, 3);
        if (y > 90) c = gs::rgb4(6, 3, 2);
        if (y > 168) c = gs::rgb4(3, 2, 1);
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

    spr(art_.arch, 160.f, 112.f, float(art_.arch.w), float(art_.arch.h), PAL_OVEN);
    for (int i = 0; i < 5; i++) {
        float fx = 96.f + float(i) * 26.f;
        float fh = 16.f + 7.f * std::sin(clock_ * 9.f + float(i) * 1.3f);
        spr(art_.flame, fx, 168.f - fh * 0.15f, 12.f, fh, PAL_FIRE);
    }
    spr(art_.peel, peelX_, 176.f, float(art_.peel.w) * 0.85f, float(art_.peel.h), PAL_PEEL);
    float lift = (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) ? -18.f : 0.f;
    spr(art_.loaf, LOAF_X + lift * 0.2f, LOAF_Y + lift, float(art_.loaf.w), float(art_.loaf.h), loafPal());

    float swing = std::sin(bellPh_) * (rung_ ? 12.f * bellAmp_ : 1.6f);
    float bellCx = 160.f + swing;
    float bellCy = 46.f;
    spr(art_.clapper, bellCx + swing * 0.25f, bellCy + 8.f, 6.f, 12.f, PAL_CLAP);
    spr(art_.bell, bellCx, bellCy, float(art_.bell.w), float(art_.bell.h), PAL_BELL);
    if (rung_ && bellAmp_ > 0.35f) {
        for (int i = 0; i < 5; i++) {
            float a = bellPh_ * 1.2f + float(i) * 1.256f;
            float rad = 16.f + (1.f - bellAmp_) * 18.f;
            spr(art_.pip, bellCx + std::cos(a) * rad, bellCy + std::sin(a) * rad * 0.45f, 5.f, 5.f, PAL_GOLD);
        }
    }

    int filled = int(clampf(heat_, 0.f, 100.f) / 100.f * 18.f);
    for (int i = 0; i < 18; i++) {
        int pal = PAL_PALE;
        if (i >= 10 && i < 14) pal = PAL_CRUST;
        if (i >= 16) pal = PAL_CHAR;
        if (i >= filled) pal = PAL_OVEN;
        spr(art_.bar, 88.f + float(i) * 8.f, 198.f, 6.f, 6.f, pal);
    }
    for (int i = 0; i < kTries; i++) {
        int pal = i < dead_ ? PAL_ALERT : (i + 1 == tryNo_ && !rung_ ? PAL_GOLD : PAL_OK);
        if (mode_ == Mode::Title) pal = PAL_INK;
        spr(art_.pip, 132.f + float(i) * 16.f, 18.f, 8.f, 8.f, pal);
    }

    if (mode_ == Mode::Title) {
        hudC(4, "S3 OVENBELL", PAL_GOLD);
        hudC(7, "THE BELL RINGS BEFORE", PAL_INK);
        hudC(8, "THE THIRD TRY DIES", PAL_INK);
        hudC(22, "START  PULL THE GOLD", PAL_OK);
    } else if (mode_ == Mode::Over) {
        hudC(3, note_, won_ ? PAL_GOLD : PAL_ALERT);
        if (!bot_) hudC(22, won_ ? "THE BELL IS DONE" : "START", won_ ? PAL_OK : PAL_ALERT);
    } else {
        hudC(2, note_, rung_ ? PAL_GOLD : (mode_ == Mode::Dead ? PAL_ALERT : PAL_INK));
        hudC(24, "A  PULL IN THE GOLD BAND", PAL_INK);
    }
}

}  // namespace ovenbell
