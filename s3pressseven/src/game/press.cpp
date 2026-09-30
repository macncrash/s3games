#include "game/press.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace pressseven {
namespace {
constexpr int kWindowLo = 14;
constexpr int kWindowHi = 22;
constexpr int kMissAt = 32;
constexpr int kMid = (kWindowLo + kWindowHi) / 2;
}  // namespace

void Game::begin() {
    you_ = them_ = wind_ = anim_ = pulls_ = 0;
    over_ = won_ = left_ = false;
    yours_ = true;
    mode_ = Mode::Pull;
    sys_->apu.silence();
}

void Game::impress(bool hit) {
    if (hit) {
        if (yours_) {
            you_ += kGoldFace;
            sys_->apu.keyOn(0, 196.f, 0.3f);
            sys_->apu.keyOn(1, 392.f, 0.16f);
            sys_->apu.noiseBurst(0.16f, 200.f, 0.07f);
        } else {
            them_ += kCreamFace;
            sys_->apu.keyOn(0, 146.f, 0.2f);
            sys_->apu.noiseBurst(0.1f, 110.f, 0.06f);
        }
    } else {
        sys_->apu.noiseBurst(0.22f, 60.f, 0.16f);
    }
    pulls_++;
    anim_ = 0;
    mode_ = Mode::Slam;
}

void Game::afterSlam() {
    wind_ = 0;
    anim_ = 0;
    if (you_ >= kSeven && them_ < kSeven) {
        mode_ = Mode::Leave;
        return;
    }
    if (them_ >= kSeven) {
        won_ = false;
        left_ = false;
        over_ = true;
        mode_ = Mode::Over;
        sys_->apu.noiseBurst(0.26f, 70.f, 0.2f);
        return;
    }
    yours_ = !yours_;
    mode_ = Mode::Pull;
}

void Game::leave() {
    left_ = true;
    over_ = true;
    won_ = you_ >= kSeven && them_ < kSeven;
    mode_ = Mode::Over;
    if (won_) {
        sys_->apu.keyOn(0, 392.f, 0.28f);
        sys_->apu.keyOn(1, 494.f, 0.2f);
        sys_->apu.keyOn(2, 587.f, 0.14f);
    } else {
        sys_->apu.noiseBurst(0.26f, 80.f, 0.2f);
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal, bool flip) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    s.hflip = flip;
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

void Game::room() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 2, 4);
    uint16_t mid = gs::rgb4(6, 4, 2);
    uint16_t bot = gs::rgb4(3, 2, 2);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(11, 8, 2);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 1, 1);
    if (mode_ == Mode::Leave) mid = gs::rgb4(10, 6, 1);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.5f ? mix(top, mid, u / 0.5f) : mix(mid, bot, (u - 0.5f) / 0.5f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    room();

    const float px = 176.f;
    const float bedY = 164.f;
    const bool gold = yours_ || mode_ == Mode::Title;
    float drop = 58.f;
    if (mode_ == Mode::Pull) {
        if (wind_ < kWindowLo) drop = 58.f - 58.f * (wind_ / float(kWindowLo));
        else if (wind_ <= kWindowHi) drop = 0.f;
        else drop = 28.f * ((wind_ - kWindowHi) / float(kMissAt - kWindowHi));
    } else if (mode_ == Mode::Slam) {
        drop = anim_ < 4 ? 0.f : float(anim_ - 3) * 2.f;
    } else if (mode_ == Mode::Title) {
        drop = 10.f + float((sys_->frame / 8) % 5);
    }

    const bool rivalTurn = !yours_ && mode_ != Mode::Title;
    spr(art_.printer, rivalTurn ? 286.f : 48.f, 136.f, rivalTurn ? PAL_RIVAL : PAL_YOU, rivalTurn);
    spr(art_.frame, px, 112.f, PAL_WOOD);
    spr(art_.bed, px, bedY, PAL_IRON);
    const gs::Image& sheet = gold ? art_.sheetGold : art_.sheetCream;
    spr(sheet, px, bedY - 10.f, PAL_PAPER);
    spr(art_.screw, px, bedY - 78.f - drop * 0.15f, PAL_INK);
    spr(art_.platen, px, bedY - 42.f - drop, PAL_IRON);

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 PRESS SEVEN", PAL_GOLD);
        hudC(21, "PLAY THE PRESS", PAL_HUD);
        hudC(22, "YOUR GOLD IS TWO", PAL_GOLD);
        hudC(23, "THEIR CREAM IS ONE", PAL_CREAM);
        hudC(24, "FIRST TO SEVEN", PAL_HUD);
        hudC(25, "LEAVE WHEN THAT IS TRUE", PAL_GOLD);
        if ((f & 16) == 0) hudC(27, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(1, "LEFT ON SEVEN", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "YOU %d  THEM %d", you_, them_);
        hudC(22, buf, PAL_GOLD);
        hudC(23, "FIRST TO SEVEN", PAL_HUD);
        hudC(24, "A COUNT PAST SEVEN STANDS", PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(1, "STILL AT THE PRESS", PAL_BAD);
        std::snprintf(buf, sizeof buf, "YOU %d  THEM %d", you_, them_);
        hudC(22, buf, PAL_BAD);
        hudC(23, left_ ? "LEFT BEFORE FIRST TO SEVEN" : "THE RIVAL WAS FIRST TO SEVEN", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "YOU %d", you_);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "THEM %d", them_);
        hud(12, 1, buf, PAL_CREAM);
        std::snprintf(buf, sizeof buf, "SEVEN %d", kSeven);
        hud(26, 1, buf, PAL_DIM);
        if (mode_ == Mode::Pause) {
            hudC(23, "PAUSED", PAL_GOLD);
        } else if (mode_ == Mode::Leave) {
            hudC(22, "FIRST TO SEVEN", PAL_GOLD);
            hudC(23, "THE COUNT STANDS", PAL_HUD);
            hudC(25, "LEAVE", PAL_GOLD);
        } else if (yours_) {
            hudC(23, "YOUR GOLD  TWO", PAL_GOLD);
            hudC(25, "C PULL", PAL_DIM);
        } else {
            hudC(23, "RIVAL CREAM  ONE", PAL_CREAM);
            hudC(25, "THEIR PULL", PAL_DIM);
        }
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
    } else if (mode_ == Mode::Pull) {
        if (!bot_ && yours_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Pull;
            mode_ = Mode::Pause;
        } else if (!bot_ && yours_ && pad.pressed(gs::BTN_A)) {
            leave();
        } else {
            const bool autoTap = bot_ || !yours_;
            const bool tap = autoTap ? (wind_ == kMid) : pad.pressed(gs::BTN_C);
            if (tap) impress(wind_ >= kWindowLo && wind_ <= kWindowHi);
            else if (++wind_ >= kMissAt) impress(false);
        }
    } else if (mode_ == Mode::Slam) {
        anim_++;
        if (anim_ >= 16) afterSlam();
    } else if (mode_ == Mode::Leave) {
        if (bot_ || pad.pressed(gs::BTN_A)) leave();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    draw();
}

}  // namespace pressseven
