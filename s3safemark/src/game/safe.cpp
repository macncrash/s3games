#include "game/safe.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace safemark {

bool Game::solved() const {
    return dial_[0] == code_[0] && dial_[1] == code_[1] && dial_[2] == code_[2];
}

void Game::deal() {
    std::uniform_int_distribution<int> dist(0, 9);
    int a, b, c;
    do {
        a = dist(rng_);
        b = dist(rng_);
        c = dist(rng_);
    } while (a == b || b == c || a == c);
    code_[0] = a;
    code_[1] = b;
    code_[2] = c;
    gold_ = std::uniform_int_distribution<int>(0, 2)(rng_);
    dial_[0] = dial_[1] = dial_[2] = 0;
    sel_ = 0;
    slide_ = 0;
    shake_ = 0;
    wrong_ = 0;
    stampT_ = 0;
    won_ = false;
    finished_ = false;
    marked_ = false;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    beep_ = 5;
}

void Game::nudgeSel(int d) {
    sel_ = (sel_ + d + 3) % 3;
    blip(640);
}

void Game::nudgeDial(int d) {
    dial_[sel_] = (dial_[sel_] + d + 10) % 10;
    blip(160.0f + dial_[sel_] * 26.0f);
}

void Game::pull() {
    if (mode_ != Mode::Play) return;
    if (!solved() || sel_ != gold_) {
        shake_ = 12;
        wrong_ = solved() ? 50 : 40;
        sys_->apu.noiseBurst(0.4f, 360, 14);
        sys_->apu.tone(2, 70, 0.07f);
        sys_->rumble(0.4f, 0.1f, 90);
        return;
    }
    mode_ = Mode::Stamp;
    stampT_ = 0;
    marked_ = true;
    sys_->apu.tone(0, 0, 0);
    sys_->rumble(0.15f, 0.45f, 200);
    sys_->setLight(255, 190, 40);
}

void Game::botAct() {
    if (t_ < 18) return;
    for (int i = 0; i < 3; i++) {
        if (dial_[i] == code_[i]) continue;
        int up = (code_[i] - dial_[i] + 10) % 10;
        int dn = (dial_[i] - code_[i] + 10) % 10;
        sel_ = i;
        dial_[i] = (up <= dn) ? (dial_[i] + 1) % 10 : (dial_[i] + 9) % 10;
        blip(160.0f + dial_[i] * 26.0f);
        return;
    }
    if (sel_ != gold_) {
        int dir = (gold_ > sel_) ? 1 : -1;
        nudgeSel(dir);
        return;
    }
    pull();
}

void Game::human() {
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
    if (sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_C) || sys_->pad.pressed(gs::BTN_START)) pull();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    try {
        std::random_device rd;
        rng_.seed(rd());
    } catch (...) {
        rng_.seed(0x5AFE);
    }
    if (bot_) rng_.seed(0x5A11);
    buildArt(sys.vdp, art_);
    deal();
    mode_ = bot_ ? Mode::Play : Mode::Title;
    over_ = false;
    sys.apu.setMaster(0.85f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (shake_ > 0) shake_--;
    if (wrong_ > 0) wrong_--;

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            mode_ = Mode::Play;
            blip(500);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (bot_) botAct();
        else human();
        if (!bot_ && pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else if (mode_ == Mode::Stamp) {
        stampT_++;
        float u = std::min(1.0f, stampT_ / 28.0f);
        slide_ = u * art_.slideOpen;
        if (stampT_ == 1) sys.apu.tone(1, 523, 0.06f);
        else if (stampT_ == 8) sys.apu.tone(1, 659, 0.06f);
        else if (stampT_ == 16) sys.apu.tone(1, 784, 0.07f);
        else if (stampT_ == 24) sys.apu.tone(1, 1046, 0.08f);
        if (stampT_ >= 32) {
            slide_ = art_.slideOpen;
            sys.apu.tone(1, 0, 0);
            mode_ = Mode::Over;
            won_ = true;
            finished_ = true;
            if (bot_) over_ = true;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    }

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
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

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

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float jx = (shake_ > 0) ? ((shake_ & 1) ? 3.0f : -3.0f) : 0.0f;
    float doorX = art_.doorX + slide_ + jx;

    if (mode_ != Mode::Title) {
        if (mode_ == Mode::Play) {
            int pal = (wrong_ > 0 && (wrong_ & 4)) ? PAL_RED : (sel_ == gold_ ? PAL_GOLD : PAL_AMBER);
            spr(art_.caret, art_.dialX[sel_] + slide_ + jx, art_.dialY - 18, 10, pal);
        }
        for (int i = 0; i < 3; i++) {
            float cx = art_.dialX[i] + slide_ + jx;
            spr(art_.wheel, cx, art_.dialY, 22, PAL_WHEEL);
            spr(art_.digit[dial_[i]], cx, art_.dialY + 1, 12, PAL_INK);
        }
    }

    spr(art_.door, doorX + art_.doorW * 0.5f, art_.doorY + art_.doorH * 0.5f, art_.doorH, PAL_DOOR);

    for (int i = 0; i < 3; i++) {
        int pal = (i == gold_) ? PAL_GOLD : PAL_INK;
        if (i == gold_) spr(art_.ring, art_.clueX[i], art_.clueY[i], 34, PAL_GOLD);
        spr(art_.digit[code_[i]], art_.clueX[i], art_.clueY[i], 16, pal);
    }

    if (mode_ == Mode::Stamp || mode_ == Mode::Over) {
        float drop = (mode_ == Mode::Over) ? 1.0f : std::min(1.0f, stampT_ / 24.0f);
        spr(art_.stamp, 160, 150 - (1.0f - drop) * 40.0f, 28 + drop * 6, PAL_GOLD);
    }
    if (mode_ == Mode::Title) spr(art_.solid, 160, 162, 120, PAL_SHADE);

    if (mode_ == Mode::Title) {
        hudC(13, "S3 SAFEMARK", PAL_AMBER);
        hudC(15, "THREE DIALS", PAL_TEXT);
        hudC(16, "THE ROOM HAS THE NUMBERS", PAL_TEXT);
        hudC(18, "THE GOLD RING IS THE MARK", PAL_GOLD);
        hudC(20, "MATCH THEM  THEN PULL GOLD", PAL_TEXT);
        hudC(22, "ARROWS PICK AND TURN", PAL_TEXT);
        hudC(23, "A TRIES THE HANDLE", PAL_TEXT);
        if ((t_ / 30) % 2 == 0) hudC(25, "PRESS START", PAL_AMBER);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 12, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Over) {
        hudC(25, "FINISHED MARK", PAL_GREEN);
        hudC(26, "THE MARK ENDS IT", PAL_AMBER);
    } else if (wrong_ > 0 && solved()) {
        hudC(25, "NOT THE GOLD MARK", PAL_RED);
        hudC(26, "PULL WITH THE GOLD DIAL", PAL_AMBER);
    } else if (wrong_ > 0) {
        hudC(25, "STUCK", PAL_RED);
        hudC(26, "READ THE ROOM", PAL_AMBER);
    } else {
        hudC(26, "GOLD RING IS THE MARK", PAL_GOLD);
        hudC(27, "ARROWS TURN   A HANDLE", PAL_AMBER);
    }
}

}  // namespace safemark
