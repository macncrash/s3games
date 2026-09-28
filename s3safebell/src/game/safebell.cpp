#include "game/safebell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "version.h"

namespace safebell {

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
    dial_[0] = dial_[1] = dial_[2] = 0;
    sel_ = 0;
    slide_ = 0;
    shake_ = 0;
    wrong_ = 0;
    ringT_ = 0;
    tryNo_ = 1;
    dead_ = 0;
    won_ = false;
    rung_ = false;
    over_ = false;
    bellAmp_ = 0.12f;
    bellPh_ = 0;
    reason_ = "bell silent";
}

void Game::checkRules() {
    bool distinct = code_[0] != code_[1] && code_[1] != code_[2] && code_[0] != code_[2];
    bool digits = true;
    for (int i = 0; i < 3; i++) digits = digits && code_[i] >= 0 && code_[i] <= 9;
    rules_ = distinct && digits && art_.bell.h > 0 && art_.door.h > 0 && tryNo_ == 1 && dead_ == 0 && !rung_;
    if (!rules_) {
        reason_ = "rules failed";
        mode_ = Mode::Dead;
        over_ = true;
    }
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

void Game::killTry() {
    shake_ = 14;
    wrong_ = 40;
    dead_++;
    sys_->apu.noiseBurst(0.4f, 360, 16);
    sys_->apu.tone(2, 58, 0.09f);
    sys_->rumble(0.5f, 0.1f, 90);
    if (dead_ >= 3) {
        reason_ = "third try died";
        mode_ = Mode::Dead;
        rung_ = false;
        won_ = false;
        if (bot_) over_ = true;
        return;
    }
    tryNo_ = dead_ + 1;
    reason_ = "try died";
}

void Game::ring() {
    mode_ = Mode::Ring;
    ringT_ = 0;
    rung_ = true;
    bellAmp_ = 1.0f;
    bellTick_ = 1;
    reason_ = "bell";
    sys_->apu.tone(0, 0, 0);
    sys_->rumble(0.12f, 0.45f, 200);
    sys_->setLight(255, 200, 60);
}

void Game::pull() {
    if (mode_ != Mode::Play) return;
    if (!solved() || dead_ >= 3) {
        killTry();
        return;
    }
    ring();
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
    if (sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_C) || sys_->pad.pressed(gs::BTN_TURBO) ||
        sys_->pad.pressed(gs::BTN_START))
        pull();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    try {
        std::random_device rd;
        rng_.seed(rd());
    } catch (...) {
        rng_.seed(0xB311u);
    }
    buildArt(sys.vdp, art_);
    deal();
    mode_ = bot_ ? Mode::Play : Mode::Title;
    checkRules();
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.16f, 0.24f, 0.14f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (shake_ > 0) shake_--;
    if (wrong_ > 0) wrong_--;

    bellPh_ += rung_ ? 0.48f : 0.05f;
    if (rung_) bellAmp_ = std::max(0.18f, bellAmp_ - 0.01f);
    if (rung_ && --bellTick_ <= 0 && bellAmp_ > 0.35f) {
        sys.apu.tone(1, 784.0f + bellAmp_ * 80.0f, 0.04f + 0.08f * bellAmp_);
        bellTick_ = 12;
    }

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            mode_ = Mode::Play;
            blip(520);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (bot_) botAct();
        else human();
        if (pad.pressed(gs::BTN_MODE) && !bot_) mode_ = Mode::Title;
    } else if (mode_ == Mode::Ring) {
        ringT_++;
        float u = std::min(1.0f, ringT_ / 28.0f);
        float e = u * u * (3.0f - 2.0f * u);
        slide_ = e * art_.slideOpen;
        if (ringT_ >= 28) {
            slide_ = art_.slideOpen;
            sys.apu.tone(1, 0, 0);
            mode_ = Mode::Leave;
            won_ = true;
            reason_ = "left";
            if (bot_) over_ = true;
        }
    } else if (mode_ == Mode::Leave) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            deal();
            mode_ = Mode::Play;
            blip(520);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Dead) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            deal();
            mode_ = Mode::Play;
            blip(400);
        }
    }

    draw();
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

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
    float swing = std::sin(bellPh_) * bellAmp_ * 8.0f;
    spr(art_.bell, art_.bellX + swing, art_.bellY, float(art_.bell.h), PAL_BELL);

    if (mode_ == Mode::Title) {
        spr(art_.solid, 160, 158, 118, PAL_SHADE);
    } else {
        spr(art_.solid, 160, 216, 16, PAL_SHADE);
    }

    if (mode_ != Mode::Title && mode_ != Mode::Leave && mode_ != Mode::Dead) {
        int pal = (wrong_ > 0 && (wrong_ & 4)) ? PAL_RED : PAL_AMBER;
        spr(art_.caret, art_.dialX[sel_] + slide_ + jx, art_.dialY - 16, 10, pal);
    }
    if (mode_ != Mode::Title) {
        for (int i = 0; i < 3; i++)
            spr(art_.wheel[dial_[i]], art_.dialX[i] + slide_ + jx, art_.dialY, float(art_.wheel[dial_[i]].h),
                PAL_WHEEL);
    }

    spr(art_.door, art_.doorX + art_.doorW * 0.5f + slide_ + jx, art_.doorY + art_.doorH * 0.5f, art_.doorH, PAL_DOOR);

    for (int i = 0; i < 3; i++) spr(art_.digit[code_[i]], art_.clueX[i], art_.clueY[i], 18, PAL_INK);

    for (int i = 0; i < 3; i++) {
        int pal = (i < dead_) ? PAL_RED : PAL_LAMP;
        spr(art_.lamp, 132.0f + i * 16.0f, 198, 8, pal);
    }

    if (mode_ == Mode::Title) {
        hudC(14, "S3 SAFEBELL", PAL_AMBER);
        hudC(16, "PLAY SAFE UNTIL THE BELL", PAL_TEXT);
        hudC(17, "RINGS BEFORE THE THIRD TRY", PAL_TEXT);
        hudC(19, "BOOK  CLOCK  TICKET", PAL_AMBER);
        hudC(21, "ARROWS PICK AND TURN", PAL_TEXT);
        hudC(22, "A TRIES THE HANDLE", PAL_TEXT);
        if ((t_ / 30) % 2 == 0) hudC(24, "PRESS START", PAL_AMBER);
        std::string ver = S3_VERSION_STRING;
        hud(40 - int(ver.size()), 13, ver, PAL_TEXT);
    } else if (mode_ == Mode::Leave) {
        hudC(25, "BELL", PAL_GREEN);
        hudC(26, "LEFT BEFORE THE THIRD TRY DIED", PAL_AMBER);
    } else if (mode_ == Mode::Dead) {
        hudC(25, "BELL SILENT", PAL_RED);
        hudC(26, "THIRD TRY DIED", PAL_TEXT);
    } else if (mode_ == Mode::Ring) {
        hudC(26, "THE BELL RINGS", PAL_GREEN);
    } else {
        char line[40];
        std::snprintf(line, sizeof line, "TRY %d OF 3", tryNo_);
        hudC(25, line, wrong_ > 0 ? PAL_RED : PAL_AMBER);
        hudC(26, "MATCH THE ROOM THEN PULL", PAL_TEXT);
    }
}

}  // namespace safebell
