#include "game/drawergold.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace drawergold {
namespace {
static_assert(kGoldNeed * kGoldFace * 2 >= kLine, "doubled gold clears the line");
static_assert(kGoldNeed * kGoldFace < kLine, "bare gold faces stay short");
static_assert((kGoldNeed - 1) * kGoldFace * 2 < kLine, "leave on the last gold");
static_assert(kCreamFace < kGoldFace * 2, "cream is not a double");
}  // namespace

bool Game::open() const {
    return finisher_ && score_ >= kLine && bare_ < kLine && golds_ >= kGoldNeed && cream_ == 0;
}

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.18f); }

void Game::begin() {
    index_ = 0;
    score_ = 0;
    bare_ = 0;
    golds_ = 0;
    cream_ = 0;
    filed_ = 0;
    anim_ = 0;
    over_ = false;
    won_ = false;
    finisher_ = false;
    for (int i = 0; i < kSlips; i++) held_[i] = false;
    mode_ = Mode::Play;
    sys_->apu.silence();
}

void Game::file() {
    if (mode_ != Mode::Play || index_ >= kSlips) return;
    const bool gold = slipKind(index_) == 'G';
    filed_++;
    if (gold) {
        bare_ += kGoldFace;
        score_ += kGoldFace * 2;
        golds_++;
        if (score_ >= kLine && bare_ < kLine && cream_ == 0 && golds_ >= kGoldNeed) finisher_ = true;
        sys_->apu.keyOn(0, 392.f, 0.28f);
        sys_->apu.keyOn(1, 523.25f, 0.22f);
    } else {
        bare_ += kCreamFace;
        score_ += kCreamFace;
        cream_++;
        finisher_ = false;
        sys_->apu.keyOn(0, 196.f, 0.2f);
        sys_->apu.noiseBurst(0.12f, 140.f, 0.08f);
    }
    mode_ = Mode::Slide;
    anim_ = 0;
}

void Game::skip() {
    if (mode_ != Mode::Play || index_ >= kSlips) return;
    blip(slipKind(index_) == 'G' ? 240.f : 180.f);
    index_++;
}

void Game::leave() {
    if (mode_ != Mode::Play && mode_ != Mode::Slide) return;
    won_ = open();
    over_ = true;
    mode_ = Mode::Over;
    if (won_) {
        sys_->apu.keyOn(0, 440.f, 0.3f);
        sys_->apu.keyOn(1, 554.f, 0.24f);
        sys_->apu.keyOn(2, 659.f, 0.2f);
    } else {
        sys_->apu.noiseBurst(0.3f, 80.f, 0.24f);
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 2, 4);
    uint16_t mid = gs::rgb4(6, 4, 3);
    uint16_t bot = gs::rgb4(3, 2, 1);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(10, 7, 2);
    if (mode_ == Mode::Over && !won_) {
        top = gs::rgb4(3, 1, 1);
        mid = gs::rgb4(6, 2, 1);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.55f ? mix(top, mid, u / 0.55f) : mix(mid, bot, (u - 0.55f) / 0.45f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    sky();

    spr(art_.desk, 160.f, 118.f, PAL_DESK);
    spr(art_.drawer, 160.f, 168.f, PAL_DESK);
    spr(art_.knob, 160.f, 196.f, PAL_BRASS);

    int slot = 0;
    for (int i = 0; i < kSlips; i++) {
        if (!held_[i]) continue;
        float x = 92.f + slot * 44.f;
        bool gold = slipKind(i) == 'G';
        spr(gold ? art_.gold : art_.cream, x, 162.f, gold ? PAL_SLIP : PAL_PAPER);
        slot++;
    }

    if (index_ < kSlips && mode_ != Mode::Over) {
        bool gold = slipKind(index_) == 'G';
        float y = 78.f;
        if (mode_ == Mode::Slide) {
            float t = anim_ / 12.f;
            if (t > 1.f) t = 1.f;
            y = 78.f + (162.f - 78.f) * t;
        } else if (mode_ == Mode::Title) {
            y = 74.f + float((sys_->frame / 8) % 2);
        }
        float x = mode_ == Mode::Slide ? 92.f + slot * 44.f : 160.f;
        spr(gold ? art_.gold : art_.cream, x, y, gold ? PAL_SLIP : PAL_PAPER);
    }

    char buf[64];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 DRAWER GOLD", PAL_GOLD);
        hudC(22, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(23, "CREAM DOES NOT BUY THE LINE", PAL_HUD);
        hudC(24, "LEAVE WHEN THAT IS TRUE", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(1, "LEFT ON THE DOUBLE", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        hudC(23, buf, PAL_GOLD);
        hudC(24, "ONLY THE GOLD COUNTED DOUBLE", PAL_HUD);
        hudC(25, "THE DRAWER IS CLOSED", PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(1, "STILL OPEN", PAL_BAD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d  CREAM %d", score_, bare_, cream_);
        hudC(23, buf, PAL_BAD);
        hudC(24, "THE DOUBLE WAS NOT THE LEAVE", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "BARE %d", bare_);
        hud(12, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "LINE %d", kLine);
        hud(30, 1, buf, PAL_DIM);
        const bool gold = index_ < kSlips && slipKind(index_) == 'G';
        const bool cream = index_ < kSlips && slipKind(index_) == 'C';
        if (index_ >= kSlips) hudC(22, "NO SLIPS LEFT", PAL_DIM);
        else if (gold) hudC(22, "GOLD SLIP", PAL_GOLD);
        else hudC(22, "CREAM SLIP", PAL_CREAM);
        std::snprintf(buf, sizeof buf, "GOLDS %d  CREAM %d", golds_, cream_);
        hudC(23, buf, cream_ ? PAL_BAD : PAL_HUD);
        if (open()) hudC(24, "LEAVE  THE GOLD IS DOUBLE", PAL_GOLD);
        else if (gold) hudC(24, "FILE THE GOLD", PAL_GOLD);
        else if (cream) hudC(24, "CREAM IS NOT A DOUBLE", PAL_CREAM);
        else hudC(24, "THE DRAWER WAITS", PAL_HUD);
        hudC(26, "C FILE  B SKIP  A LEAVE", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            // pause is a held start on the next press back to title via MODE
        } else if (bot_) {
            if (open() || index_ >= kSlips) leave();
            else if (slipKind(index_) == 'G' && golds_ < kGoldNeed) file();
            else skip();
        } else {
            if (pad.pressed(gs::BTN_C)) file();
            if (pad.pressed(gs::BTN_B)) skip();
            if (pad.pressed(gs::BTN_A)) leave();
        }
    } else if (mode_ == Mode::Slide) {
        anim_++;
        if (anim_ >= 12) {
            held_[index_] = true;
            index_++;
            mode_ = Mode::Play;
            if (bot_ && open()) leave();
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    if (!bot_ && mode_ != Mode::Title && pad.pressed(gs::BTN_MODE)) {
        mode_ = Mode::Title;
        sys.apu.silence();
    }
    draw();
}

}  // namespace drawergold
