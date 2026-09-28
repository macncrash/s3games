#include "game/safegold.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "version.h"

namespace safegold {

bool Game::solved() const {
    return dial_[0] == code_[0] && dial_[1] == code_[1] && dial_[2] == code_[2];
}

void Game::deal() {
    std::uniform_int_distribution<int> dig(1, 9);
    std::uniform_int_distribution<int> coin(0, 1);
    for (;;) {
        bool metal[3] = {false, false, true};
        int face[3];
        for (int i = 0; i < 3; i++) face[i] = dig(rng_);
        metal[0] = coin(rng_) == 1;
        metal[1] = coin(rng_) == 1;
        if (metal[0] && metal[1]) metal[coin(rng_)] = false;
        int goldSum = 0, creamSum = 0, goldN = 0, creamN = 0;
        for (int i = 0; i < 3; i++) {
            if (metal[i]) {
                goldSum += face[i];
                goldN++;
            } else {
                creamSum += face[i];
                creamN++;
            }
        }
        if (goldN < 1 || creamN < 1) continue;
        if (goldSum <= creamSum) continue;
        int run = 0;
        bool fin = false;
        const int line = goldSum * 2 + creamSum;
        for (int i = 0; i < 3; i++) {
            run += metal[i] ? face[i] * 2 : face[i];
            if (run >= line) {
                fin = metal[i];
                break;
            }
        }
        if (!fin) continue;
        if (goldSum + creamSum * 2 >= line) continue;
        for (int i = 0; i < 3; i++) {
            code_[i] = face[i];
            gold_[i] = metal[i];
        }
        goldN_ = goldN;
        creamN_ = creamN;
        goldFace_ = goldSum;
        creamFace_ = creamSum;
        line_ = line;
        finisherGold_ = true;
        break;
    }
    dial_[0] = dial_[1] = dial_[2] = 0;
    sel_ = 0;
    slide_ = 0;
    shake_ = 0;
    wrong_ = 0;
    openT_ = 0;
    won_ = false;
}

void Game::clickAt(float freq) {
    sys_->apu.tone(0, freq, 0.045f);
    beep_ = 5;
}

void Game::nudgeSel(int d) {
    sel_ = (sel_ + d + 3) % 3;
    clickAt(680);
}

void Game::nudgeDial(int d) {
    dial_[sel_] = (dial_[sel_] + d + 10) % 10;
    clickAt(170.0f + dial_[sel_] * 28.0f);
}

void Game::pull() {
    if (mode_ != Mode::Play) return;
    if (!solved()) {
        shake_ = 12;
        wrong_ = 46;
        sys_->apu.noiseBurst(0.42f, 380, 16);
        sys_->apu.tone(2, 62, 0.08f);
        thud_ = 10;
        sys_->rumble(0.55f, 0.12f, 100);
        return;
    }
    mode_ = Mode::Opening;
    openT_ = 0;
    beep_ = 0;
    sys_->apu.tone(0, 0, 0);
    sys_->rumble(0.16f, 0.5f, 240);
    sys_->setLight(255, 190, 48);
}

void Game::botAct() {
    if (t_ < 24) return;
    for (int i = 0; i < 3; i++) {
        if (dial_[i] == code_[i]) continue;
        int up = (code_[i] - dial_[i] + 10) % 10;
        int dn = (dial_[i] - code_[i] + 10) % 10;
        sel_ = i;
        dial_[i] = (up <= dn) ? (dial_[i] + 1) % 10 : (dial_[i] + 9) % 10;
        clickAt(170.0f + dial_[i] * 28.0f);
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
        rng_.seed(0x60DDu);
    }
    buildArt(sys.vdp, art_);
    deal();
    mode_ = bot_ ? Mode::Play : Mode::Title;
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.14f, 0.22f, 0.12f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (thud_ > 0 && --thud_ == 0) sys.apu.tone(2, 0, 0);
    if (shake_ > 0) shake_--;
    if (wrong_ > 0) wrong_--;

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            mode_ = Mode::Play;
            clickAt(520);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (bot_) botAct();
        else human();
        if (pad.pressed(gs::BTN_MODE) && !bot_) mode_ = Mode::Title;
    } else if (mode_ == Mode::Opening) {
        openT_++;
        float u = std::min(1.0f, openT_ / 36.0f);
        float e = u * u * (3.0f - 2.0f * u);
        slide_ = e * art_.slideOpen;
        if (openT_ == 1) sys.apu.tone(1, 523, 0.06f);
        else if (openT_ == 9) sys.apu.tone(1, 659, 0.06f);
        else if (openT_ == 17) sys.apu.tone(1, 784, 0.06f);
        else if (openT_ == 25) sys.apu.tone(1, 1046, 0.07f);
        if (openT_ >= 36) {
            slide_ = art_.slideOpen;
            sys.apu.tone(1, 0, 0);
            mode_ = Mode::Open;
            won_ = true;
            if (bot_) over_ = true;
        }
    } else if (mode_ == Mode::Open) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            deal();
            mode_ = Mode::Play;
            clickAt(520);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
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

void Game::box(const gs::Mipped& m, float x, float y, float w, float h, int pal) {
    if (w < 1 || h < 1) return;
    gs::Sprite s{};
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float jx = (shake_ > 0) ? ((shake_ & 1) ? 3.0f : -3.0f) : 0.0f;
    float doorX = art_.doorX + jx;
    float doorY = art_.doorY + slide_;

    if (mode_ == Mode::Title) {
        box(art_.solid, 16, 78, 288, 92, PAL_SHADE);
        box(art_.solid, 12, 74, 296, 100, PAL_GOLD);
    }

    if (mode_ != Mode::Title) {
        if (mode_ != Mode::Open) {
            int pal = (wrong_ > 0 && (wrong_ & 4)) ? PAL_RED : (gold_[sel_] ? PAL_GOLD : PAL_CREAM);
            spr(art_.caret, art_.dialX[sel_] + jx, art_.dialY + slide_ - 22, 10, pal);
        }
        for (int i = 0; i < 3; i++)
            spr(art_.wheel[dial_[i]], art_.dialX[i] + jx, art_.dialY + slide_, float(art_.wheel[dial_[i]].h),
                gold_[i] ? PAL_GOLD : PAL_CREAM);
    }

    spr(art_.door, doorX + art_.doorW * 0.5f, doorY + art_.doorH * 0.5f, art_.doorH, PAL_DOOR);

    for (int i = 0; i < 3; i++) {
        int pal = gold_[i] ? PAL_GOLD : PAL_CREAM;
        spr(art_.digit[code_[i]], art_.clueX[i], art_.clueY[i], 21, pal);
        box(art_.bar, art_.clueX[i] - 8, art_.clueY[i] + 14, 16, 4, pal);
    }

    if (mode_ == Mode::Title) {
        hudC(11, "S3 SAFE GOLD", PAL_AMBER);
        hudC(13, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(15, "CREAM COUNTS ONCE", PAL_CREAM);
        hudC(17, "CLOCK, NOTE, PLATE", PAL_TEXT);
        hudC(19, "ARROWS PICK AND TURN", PAL_TEXT);
        if ((t_ / 30) % 2 == 0) hudC(21, "PRESS START", PAL_AMBER);
        std::string ver = S3_VERSION_STRING;
        hud(40 - int(ver.size()), 10, ver, PAL_TEXT);
    } else if (mode_ == Mode::Open) {
        hudC(25, "OPEN  GOLD DOUBLED", PAL_GREEN);
        hud(1, 26, "LINE " + std::to_string(line_), PAL_GOLD);
        hud(16, 26, "TAKE " + std::to_string(score()), PAL_AMBER);
        hudC(27, "START FOR ANOTHER ROOM", PAL_TEXT);
    } else {
        if (wrong_ > 0) hudC(25, "STUCK", PAL_RED);
        hud(1, 26, "LINE " + std::to_string(line_), PAL_GOLD);
        hud(22, 26, "GOLD X2", PAL_GOLD);
        hudC(27, "ARROWS PICK AND TURN   A HANDLE", PAL_AMBER);
    }
}

}  // namespace safegold
