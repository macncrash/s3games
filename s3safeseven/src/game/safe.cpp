#include "game/safe.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace safeseven {

bool Game::matched() const {
    return dial_[0] == code_[0] && dial_[1] == code_[1] && dial_[2] == code_[2];
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    you_ = 0;
    them_ = 0;
    won_ = false;
    over_ = false;
    t_ = 0;
}

void Game::newMatch() {
    you_ = 0;
    them_ = 0;
    won_ = false;
    over_ = false;
    desk_ = 0;
    slide_ = 0;
    shake_ = 0;
    wrong_ = 0;
    deal();
    mode_ = Mode::Play;
}

void Game::deal() {
    std::uniform_int_distribution<int> dist(0, 9);
    do {
        code_[0] = dist(rng_);
        code_[1] = dist(rng_);
        code_[2] = dist(rng_);
    } while (code_[0] == 0 && code_[1] == 0 && code_[2] == 0);
    dial_[0] = dial_[1] = dial_[2] = 0;
    sel_ = 0;
    slide_ = 0;
    shake_ = 0;
    wrong_ = 0;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    beep_ = 4;
}

void Game::nudgeSel(int d) {
    sel_ = (sel_ + d + 3) % 3;
    blip(520);
}

void Game::nudgeDial(int d) {
    dial_[sel_] = (dial_[sel_] + d + 10) % 10;
    blip(140.0f + dial_[sel_] * 22.0f);
}

void Game::pull() {
    if (mode_ != Mode::Play) return;
    if (!matched()) {
        shake_ = 10;
        wrong_ = 36;
        sys_->apu.noiseBurst(0.35f, 240, 12);
        sys_->apu.tone(1, 70, 0.08f);
        sys_->rumble(0.4f, 0.1f, 80);
        return;
    }
    you_++;
    blip(880);
    sys_->rumble(0.15f, 0.35f, 120);
    if (you_ >= 7) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        slide_ = -70;
        return;
    }
    mode_ = Mode::Flash;
    flash_ = 26;
    slide_ = -70;
}

void Game::deskTick() {
    if (++desk_ < 110) return;
    desk_ = 0;
    them_++;
    sys_->apu.tone(2, 180, 0.06f);
    if (them_ >= 7 && you_ < 7) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
    }
}

void Game::botAct() {
    if (mode_ == Mode::Title) {
        if (t_ > 10) newMatch();
        return;
    }
    if (mode_ != Mode::Play) return;
    for (int i = 0; i < 3; i++) {
        if (dial_[i] == code_[i]) continue;
        int up = (code_[i] - dial_[i] + 10) % 10;
        int dn = (dial_[i] - code_[i] + 10) % 10;
        sel_ = i;
        dial_[i] = (up <= dn) ? (dial_[i] + 1) % 10 : (dial_[i] + 9) % 10;
        blip(140.0f + dial_[i] * 22.0f);
        return;
    }
    pull();
}

void Game::human() {
    if (mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A)) newMatch();
        return;
    }
    if (mode_ != Mode::Play) return;
    auto tap = [&](gs::Button b, int slot, auto&& fn) {
        int& h = hold_[slot];
        if (sys_->pad.pressed(b)) {
            h = 0;
            fn();
        } else if (sys_->pad.down(b)) {
            if (++h >= 14 && (h % 5) == 0) fn();
        } else {
            h = 0;
        }
    };
    tap(gs::BTN_LEFT, 0, [&] { nudgeSel(-1); });
    tap(gs::BTN_RIGHT, 1, [&] { nudgeSel(1); });
    tap(gs::BTN_UP, 2, [&] { nudgeDial(1); });
    tap(gs::BTN_DOWN, 3, [&] { nudgeDial(-1); });
    if (sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_C) || sys_->pad.pressed(gs::BTN_B)) pull();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (shake_ > 0) shake_--;
    if (wrong_ > 0) wrong_--;

    if (mode_ == Mode::Flash) {
        if (--flash_ <= 0) {
            deal();
            mode_ = Mode::Play;
        }
    } else if (mode_ == Mode::Play && !over_) {
        deskTick();
    }

    if (bot_) botAct();
    else human();

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

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s{};
    s.w = int16_t(std::clamp(std::lround(w), 1L, 2000L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::box(float x, float y, float w, float h, int pal) {
    if (w < 1 || h < 1) return;
    gs::Sprite s{};
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.img = art_.solid.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float jx = (shake_ > 0) ? ((shake_ & 1) ? 2.0f : -2.0f) : 0.0f;
    float doorCy = art_.doorY + art_.doorH * 0.5f + slide_;

    if (mode_ == Mode::Title) box(28, 64, 264, 110, PAL_SHADE);

    spr(art_.door, art_.doorX + art_.doorW * 0.5f + jx, doorCy, art_.doorH, PAL_DOOR);

    if (mode_ != Mode::Title) {
        int pal = (wrong_ > 0 && (wrong_ & 4)) ? PAL_RED : PAL_GOLD;
        spr(art_.caret, art_.dialX[sel_] + jx, art_.dialY - 16, 8, pal);
        for (int i = 0; i < 3; i++)
            spr(art_.wheel[dial_[i]], art_.dialX[i] + jx, art_.dialY, 16, PAL_WHEEL);
        for (int i = 0; i < 3; i++) spr(art_.digit[code_[i]], art_.clueX[i], art_.clueY[i], 18, PAL_INK);
    }

    for (int i = 0; i < 7; i++) {
        box(8.0f + i * 10, 8, 8, 6, i < you_ ? PAL_GREEN : PAL_SHADE);
        box(242.0f + i * 10, 8, 8, 6, i < them_ ? PAL_RED : PAL_SHADE);
    }

    if (mode_ == Mode::Title) {
        hudC(10, "S3 SAFE SEVEN", PAL_AMBER);
        hudC(12, "A SHORT SAFE", PAL_TEXT);
        hudC(14, "FIRST TO SEVEN", PAL_AMBER);
        hudC(16, "SLIP  TAG  BIN", PAL_TEXT);
        hudC(18, "ARROWS TURN   A PULLS", PAL_TEXT);
        if ((t_ / 30) % 2 == 0) hudC(20, "PRESS START", PAL_AMBER);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 9, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Win) {
        hudC(26, "FIRST TO SEVEN", PAL_GREEN);
    } else if (mode_ == Mode::Lose) {
        hudC(26, "THE DESK GOT THERE", PAL_RED);
    } else if (wrong_ > 0) {
        hudC(26, "STUCK", PAL_RED);
    } else if (mode_ == Mode::Flash) {
        hudC(26, "OPEN", PAL_GREEN);
    } else {
        char line[40];
        std::snprintf(line, sizeof(line), "YOU %d   DESK %d", you_, them_);
        hudC(26, line, PAL_AMBER);
        hudC(27, "UNDER SEVEN IS NOT DONE", PAL_TEXT);
    }
}

}  // namespace safeseven
