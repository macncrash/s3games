#include "game/gold.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace maskgold {

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Leave || mode_ == Mode::Done) return 2;
    return 1;
}

Game::Kind Game::kindAt(int i) const {
    static const Kind kKind[kSlots] = {Kind::Gold, Kind::Gold, Kind::Cream, Kind::Gold, Kind::Cream, Kind::Gold};
    if (i < 0 || i >= kSlots) return Kind::Cream;
    return kKind[i];
}

void Game::begin() {
    mode_ = Mode::Title;
    slot_ = 0;
    gold_ = 0;
    cream_ = 0;
    bare_ = 0;
    score_ = 0;
    for (int i = 0; i < kSlots; i++) laid_[i] = 0;
    flash_ = 0;
    titleWait_ = 0;
    leaveWait_ = 0;
    clock_ = 0;
    over_ = false;
    won_ = false;
    released_ = true;
    beep_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    begin();
}

void Game::toneAt(float freq, float vol) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    beep_ = 0.1f;
}

bool Game::clears() const {
    return gold_ > 0 && score_ == gold_ * 2 + cream_ && bare_ == gold_ + cream_ && bare_ < kLine && score_ >= kLine;
}

void Game::take() {
    if (slot_ < 0 || slot_ >= kSlots || laid_[slot_]) return;
    if (kindAt(slot_) == Kind::Gold) {
        gold_++;
        laid_[slot_] = 1;
        toneAt(392.f + gold_ * 40.f, 0.32f);
    } else {
        cream_++;
        laid_[slot_] = 2;
        toneAt(260.f, 0.22f);
    }
    bare_ = gold_ + cream_;
    score_ = gold_ * 2 + cream_;
    slot_++;
    clock_ = 0;
    if (clears()) leave();
    else if (slot_ >= kSlots) {
        mode_ = Mode::Fail;
        toneAt(90.f, 0.25f);
    }
}

void Game::pass() {
    if (slot_ >= kSlots) return;
    slot_++;
    clock_ = 0;
    toneAt(180.f, 0.12f);
    if (slot_ >= kSlots) {
        if (clears()) leave();
        else {
            mode_ = Mode::Fail;
            toneAt(90.f, 0.25f);
        }
    }
}

void Game::smear() {
    flash_ = 12;
    toneAt(78.f, 0.24f);
}

void Game::leave() {
    if (won_) return;
    won_ = true;
    mode_ = Mode::Leave;
    leaveWait_ = 0;
    toneAt(523.f, 0.36f);
    if (sys_) {
        sys_->apu.tone(1, 659.f, 0.2f);
        sys_->apu.tone(2, 784.f, 0.14f);
    }
}

float Game::needle() const {
    int t = clock_ % kPeriod;
    float u = t / float(kPeriod);
    return u < 0.5f ? u * 2.f : (1.f - u) * 2.f;
}

bool Game::inGroove() const {
    float n = needle();
    return n > 0.40f && n < 0.60f;
}

void Game::slotLine(int i, float& x0, float& y0, float& x1, float& y1) const {
    if (i == 0) {
        x0 = 128;
        y0 = 78;
        x1 = 192;
        y1 = 78;
    } else if (i == 1) {
        x0 = 124;
        y0 = 108;
        x1 = 150;
        y1 = 108;
    } else if (i == 2) {
        x0 = 172;
        y0 = 124;
        x1 = 198;
        y1 = 124;
    } else if (i == 3) {
        x0 = 136;
        y0 = 148;
        x1 = 184;
        y1 = 148;
    } else if (i == 4) {
        x0 = 116;
        y0 = 132;
        x1 = 140;
        y1 = 132;
    } else {
        x0 = 146;
        y0 = 168;
        x1 = 176;
        y1 = 168;
    }
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.cut = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
    in.skip = pad.pressed(gs::BTN_B);
    in.start = pad.pressed(gs::BTN_START);
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title) {
        if (titleWait_ > 8) in.start = true;
        return in;
    }
    if (mode_ == Mode::Leave) {
        if (leaveWait_ > 24) in.start = true;
        return in;
    }
    if (mode_ == Mode::Fail) {
        if (leaveWait_ > 20) in.start = true;
        return in;
    }
    if (mode_ == Mode::Sweep && released_ && slot_ < kSlots) {
        if (kindAt(slot_) == Kind::Cream) {
            if (clock_ > 4) in.skip = true;
        } else {
            float n = needle();
            if (n > 0.46f && n < 0.54f) in.cut = true;
        }
    }
    return in;
}

void Game::stepPlay(const Input& in) {
    if (mode_ == Mode::Title) {
        titleWait_++;
        if ((in.start || in.cut) && titleWait_ > 4) {
            mode_ = Mode::Sweep;
            clock_ = 0;
            released_ = false;
            toneAt(330.f, 0.18f);
        }
        return;
    }
    if (mode_ == Mode::Sweep) {
        if (released_) {
            if (in.skip) {
                released_ = false;
                pass();
            } else if (in.cut) {
                released_ = false;
                if (inGroove()) take();
                else smear();
            }
        }
        if (!in.cut && !in.skip) released_ = true;
        return;
    }
    if (mode_ == Mode::Leave) {
        leaveWait_++;
        if (leaveWait_ == 30 && sys_) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
        if ((in.start && leaveWait_ > 12) || leaveWait_ > 48) {
            mode_ = Mode::Done;
            over_ = true;
        }
        return;
    }
    if (mode_ == Mode::Fail) {
        leaveWait_++;
        if (in.start && leaveWait_ > 10) begin();
        return;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    Input in = bot_ ? botInput() : readPad(sys.pad);
    stepPlay(in);
    if (mode_ == Mode::Sweep || mode_ == Mode::Title) clock_++;
    if (beep_ > 0) {
        beep_ -= 1.f / 60.f;
        if (beep_ <= 0 && mode_ != Mode::Leave) sys.apu.tone(0, 0, 0);
    }
    if (flash_ > 0) flash_--;
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (!sys_ || img.w == 0 || w < 1 || h < 1) return;
    gs::Sprite s;
    s.img = img;
    s.x = int(std::lround(cx - w * 0.5f));
    s.y = int(std::lround(cy - h * 0.5f));
    s.w = int(std::lround(w));
    s.h = int(std::lround(h));
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
    if (!s) return;
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 36) v.lineBackdrop[y] = gs::rgb4(3, 2, 4);
        else if (y < 148) {
            int g = 2 + (y / 36);
            v.lineBackdrop[y] = gs::rgb4(4, g, 3);
        } else if (y < 170) {
            v.lineBackdrop[y] = gs::rgb4(8, 5, 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(5 + ((y / 4) & 1), 3, 1);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.ribbon, 160, 52, 72, 22, PAL_RIBBON);
    spr(art_.leaf, 42, 150, 36, 30, PAL_LEAF);
    spr(art_.pot, 278, 156, 32, 36, PAL_POT);

    int maskPal = flash_ > 0 ? PAL_BAD : PAL_CLAY;
    spr(art_.mask, 160, 118, 108, 126, maskPal);

    for (int i = 0; i < kSlots; i++) {
        if (!laid_[i]) continue;
        float x0, y0, x1, y1;
        slotLine(i, x0, y0, x1, y1);
        float mx = (x0 + x1) * 0.5f;
        float my = (y0 + y1) * 0.5f;
        float len = std::fabs(x1 - x0);
        int pal = laid_[i] == 1 ? PAL_GOLD : PAL_CREAM;
        float thick = laid_[i] == 1 ? 10.f : 6.f;
        spr(art_.stroke, mx, my, len, thick, pal);
    }

    if (mode_ == Mode::Sweep && slot_ < kSlots) {
        float x0, y0, x1, y1;
        slotLine(slot_, x0, y0, x1, y1);
        float n = needle();
        float x = x0 + (x1 - x0) * n;
        float y = y0 + (y1 - y0) * n;
        int pal = inGroove() ? (kindAt(slot_) == Kind::Gold ? PAL_GOLD : PAL_CREAM) : PAL_BLADE;
        spr(art_.blade, x, y - 16, 18, 30, pal);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(2, "S3 MASK GOLD", PAL_TITLE);
        hudC(5, "LAY THE LEAF", PAL_INK);
        hudC(18, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(21, "A CUT   B SKIP   START", PAL_DIM);
        hud(32, 26, S3_VERSION_STRING, PAL_DIM);
    } else if (mode_ == Mode::Sweep && slot_ < kSlots) {
        const char* name = kindAt(slot_) == Kind::Gold ? "GOLD LEAF" : "CREAM GLAZE";
        hudC(1, name, flash_ ? PAL_BAD : (kindAt(slot_) == Kind::Gold ? PAL_GOLD : PAL_CREAM));
        std::snprintf(buf, sizeof buf, "GOLD %d X2", gold_);
        hud(1, 25, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "CREAM %d", cream_);
        hud(14, 25, buf, PAL_CREAM);
        std::snprintf(buf, sizeof buf, "BARE %d LINE %d", bare_, kLine);
        hud(24, 25, buf, PAL_INK);
    } else if (mode_ == Mode::Leave || mode_ == Mode::Done) {
        hudC(1, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d  LINE %d", score_, bare_, kLine);
        hudC(24, buf, PAL_INK);
        hudC(26, "LEAVE", PAL_TITLE);
    } else if (mode_ == Mode::Fail) {
        hudC(1, "CREAM DOES NOT DOUBLE", PAL_BAD);
        hudC(24, "STILL SHORT", PAL_INK);
        hudC(26, "START TO TRY AGAIN", PAL_DIM);
    }
}

}  // namespace maskgold
