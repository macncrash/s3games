#include "game/maskmark.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace maskmark {

namespace {
constexpr int kPeriod = 48;
}

const char* Game::markName() const { return finished_ ? "finished" : "open"; }

void Game::begin() {
    mode_ = Mode::Title;
    locked_ = 0;
    smearN_ = 0;
    flash_ = 0;
    titleWait_ = 0;
    stampT_ = 0;
    doneWait_ = 0;
    clock_ = 0;
    over_ = false;
    won_ = false;
    finished_ = false;
    sealed_ = false;
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
    beep_ = 0.12f;
}

void Game::lockCut() {
    if (locked_ >= kCuts) return;
    locked_++;
    toneAt(220.f + locked_ * 70.f, 0.35f);
    if (locked_ >= kCuts) {
        mode_ = Mode::Stamp;
        stampT_ = 0;
    }
}

void Game::smear() {
    smearN_++;
    flash_ = 14;
    toneAt(90.f, 0.28f);
}

void Game::seat() {
    if (sealed_) return;
    sealed_ = true;
    stampT_ = 1;
    toneAt(180.f, 0.3f);
}

void Game::finish() {
    if (finished_) return;
    finished_ = true;
    won_ = true;
    mode_ = Mode::Done;
    toneAt(440.f, 0.4f);
    if (sys_) {
        sys_->apu.tone(1, 660.f, 0.22f);
        sys_->apu.tone(2, 880.f, 0.16f);
    }
}

float Game::needle() const {
    int t = clock_ % kPeriod;
    float u = t / float(kPeriod);
    return u < 0.5f ? u * 2.f : (1.f - u) * 2.f;
}

bool Game::inGroove() const {
    float n = needle();
    return n > 0.42f && n < 0.58f;
}

void Game::cutLine(int i, float& x0, float& y0, float& x1, float& y1) const {
    if (i == 0) {
        x0 = 128;
        y0 = 86;
        x1 = 192;
        y1 = 86;
    } else if (i == 1) {
        x0 = 122;
        y0 = 112;
        x1 = 150;
        y1 = 112;
    } else if (i == 2) {
        x0 = 170;
        y0 = 112;
        x1 = 198;
        y1 = 112;
    } else {
        x0 = 140;
        y0 = 152;
        x1 = 180;
        y1 = 152;
    }
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.cut = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
    in.start = pad.pressed(gs::BTN_START) || in.cut;
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title) {
        if (titleWait_ > 10) in.start = true;
        return in;
    }
    if (mode_ == Mode::Stamp) {
        if (released_ && !sealed_) in.cut = true;
        return in;
    }
    if (mode_ == Mode::Cut && released_) {
        float n = needle();
        if (n > 0.46f && n < 0.54f) in.cut = true;
    }
    return in;
}

void Game::stepPlay(const Input& in) {
    if (mode_ == Mode::Title) {
        titleWait_++;
        if (in.start && titleWait_ > 6) {
            mode_ = Mode::Cut;
            clock_ = 0;
            released_ = false;
            toneAt(330.f, 0.2f);
        }
        return;
    }
    if (mode_ == Mode::Cut) {
        if (in.cut && released_) {
            released_ = false;
            if (inGroove()) lockCut();
            else smear();
        }
        if (!in.cut) released_ = true;
        return;
    }
    if (mode_ == Mode::Stamp) {
        if (!sealed_) {
            if (in.cut && released_) {
                released_ = false;
                seat();
            }
            if (!in.cut) released_ = true;
        } else {
            stampT_++;
            if (stampT_ > 28) finish();
        }
        return;
    }
    if (mode_ == Mode::Done) {
        doneWait_++;
        if (doneWait_ == 36 && sys_) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
        if (doneWait_ > 50) over_ = true;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    Input in = bot_ ? botInput() : readPad(sys.pad);
    stepPlay(in);
    if (mode_ == Mode::Cut || mode_ == Mode::Title) clock_++;
    if (beep_ > 0) {
        beep_ -= 1.f / 60.f;
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            if (mode_ != Mode::Done) {
                sys.apu.tone(1, 0, 0);
                sys.apu.tone(2, 0, 0);
            }
        }
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
        if (y < 150) {
            int g = 3 + y / 40;
            v.lineBackdrop[y] = gs::rgb4(5, g, 3);
        } else if (y < 168) {
            v.lineBackdrop[y] = gs::rgb4(7, 5, 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(4 + (y & 8 ? 1 : 0), 3, 1);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.lamp, 46, 48, 36, 40, PAL_LAMP);
    spr(art_.pot, 274, 150, 36, 42, PAL_WOOD);
    spr(art_.hands, 78, 176, 40, 36, PAL_FACE);
    spr(art_.hands, 242, 176, 40, 36, PAL_FACE);

    int maskPal = flash_ > 0 ? PAL_SMEAR : PAL_CLAY;
    spr(art_.mask, 160, 118, 96, 124, maskPal);

    for (int i = 0; i < locked_ && i < kCuts; i++) {
        float x0, y0, x1, y1;
        cutLine(i, x0, y0, x1, y1);
        float mx = (x0 + x1) * 0.5f;
        float my = (y0 + y1) * 0.5f;
        float len = std::fabs(x1 - x0);
        spr(art_.stroke, mx, my, len, 8, PAL_CUT);
    }

    if (mode_ == Mode::Cut && locked_ < kCuts) {
        float x0, y0, x1, y1;
        cutLine(locked_, x0, y0, x1, y1);
        float n = needle();
        float x = x0 + (x1 - x0) * n;
        float y = y0 + (y1 - y0) * n;
        spr(art_.chisel, x, y - 14, 16, 24, inGroove() ? PAL_CUT : PAL_TOOL);
    }

    if (sealed_ || mode_ == Mode::Done) {
        float drop = sealed_ ? 1.f : 0.f;
        if (stampT_ < 12) drop = stampT_ / 12.f;
        float y = 40 + drop * 48.f;
        spr(art_.stamp, 160, y, 40, 40, PAL_SEAL);
    } else if (mode_ == Mode::Stamp) {
        spr(art_.stamp, 160, 42, 34, 34, PAL_SEAL);
    }

    if (mode_ == Mode::Title) {
        hudC(3, "S3 MASKMARK", PAL_TITLE);
        hudC(6, "A SHORT MASK", PAL_INK);
        hudC(18, "CUT IN THE GROOVE", PAL_GOLD);
        hudC(21, "A TO CUT   START", PAL_DIM);
    } else if (mode_ == Mode::Cut) {
        const char* name = "BROW";
        if (locked_ == 1) name = "LEFT EYE";
        else if (locked_ == 2) name = "RIGHT EYE";
        else if (locked_ == 3) name = "MOUTH";
        hudC(2, name, flash_ ? PAL_BAD : PAL_GOLD);
        hud(2, 25, "OPEN", PAL_DIM);
        char buf[16];
        std::snprintf(buf, sizeof(buf), "CUTS %d", locked_);
        hud(30, 25, buf, PAL_INK);
    } else if (mode_ == Mode::Stamp && !sealed_) {
        hudC(2, "SEAT THE SEAL", PAL_GOLD);
        hudC(25, "A TO MARK", PAL_INK);
    } else if (mode_ == Mode::Stamp || mode_ == Mode::Done) {
        hudC(2, "FINISHED MARK", PAL_GOLD);
        hudC(25, "THE MASK IS CLOSED", PAL_INK);
    }
}

}  // namespace maskmark
