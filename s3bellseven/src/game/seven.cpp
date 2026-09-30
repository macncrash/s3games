#include "game/seven.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace bellseven {
namespace {
constexpr int kWindowLo = 13;
constexpr int kWindowHi = 21;
constexpr int kMissAt = 36;
constexpr int kSweet = 17;
}  // namespace

void Game::begin() {
    you_ = them_ = wind_ = anim_ = pulls_ = 0;
    over_ = won_ = left_ = false;
    yours_ = true;
    lastHit_ = false;
    why_ = "";
    mode_ = Mode::Swing;
    sys_->apu.silence();
}

void Game::pull(bool hit) {
    lastHit_ = hit;
    if (hit) {
        if (yours_) {
            you_ += kYourFace;
            sys_->apu.keyOn(0, 262.f, 0.34f);
            sys_->apu.keyOn(1, 392.f, 0.16f);
            sys_->apu.noiseBurst(0.1f, 540.f, 0.05f);
        } else {
            them_ += kTheirFace;
            sys_->apu.keyOn(0, 196.f, 0.22f);
            sys_->apu.noiseBurst(0.06f, 240.f, 0.05f);
        }
    } else {
        sys_->apu.noiseBurst(0.2f, 70.f, 0.14f);
    }
    pulls_++;
    anim_ = 0;
    mode_ = Mode::Toll;
}

void Game::afterToll() {
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
        why_ = "the other rope was first to seven";
        mode_ = Mode::Over;
        sys_->apu.noiseBurst(0.26f, 60.f, 0.2f);
        return;
    }
    yours_ = !yours_;
    mode_ = Mode::Swing;
}

void Game::leave() {
    left_ = true;
    over_ = true;
    won_ = you_ >= kSeven && them_ < kSeven;
    mode_ = Mode::Over;
    if (won_) {
        why_ = "first to seven";
        sys_->apu.keyOn(0, 330.f, 0.28f);
        sys_->apu.keyOn(1, 415.f, 0.2f);
        sys_->apu.keyOn(2, 523.f, 0.16f);
    } else {
        why_ = "left before first to seven";
        sys_->apu.noiseBurst(0.24f, 80.f, 0.18f);
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal, bool hflip) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    s.hflip = hflip;
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
    uint16_t top = gs::rgb4(1, 1, 4);
    uint16_t mid = gs::rgb4(4, 4, 7);
    uint16_t bot = gs::rgb4(2, 2, 2);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(11, 8, 2);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 1, 2);
    if (mode_ == Mode::Leave) mid = gs::rgb4(9, 6, 2);
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
    sky();

    float ang = 0.f;
    if (mode_ == Mode::Swing) {
        float t = wind_ / float(kMissAt);
        ang = std::sin(t * 3.1415926f);
        if (wind_ < kWindowLo) ang *= 0.55f + 0.45f * (wind_ / float(kWindowLo));
        if (wind_ > kWindowHi) ang *= 0.7f;
        if (!yours_) ang = -ang;
    } else if (mode_ == Mode::Toll) {
        float damp = std::exp(-anim_ * 0.18f);
        ang = (yours_ ? 1.f : -1.f) * 0.55f * damp * std::sin(anim_ * 0.9f);
    } else if (mode_ == Mode::Title || mode_ == Mode::Leave) {
        ang = 0.22f * std::sin(sys_->frame * 0.06f);
    }

    const float pivotX = 160.f;
    const float pivotY = 28.f;
    const float bx = pivotX + ang * 28.f;
    const float by = pivotY + 34.f + std::fabs(ang) * 3.f;
    const bool yourRope = yours_ || mode_ == Mode::Title;

    spr(art_.belfry, 160.f, 86.f, PAL_STONE);
    spr(art_.you, 46.f, 168.f, PAL_YOU);
    spr(art_.them, 274.f, 168.f, PAL_THEM, true);
    spr(art_.rope, 92.f, 150.f + (yourRope ? -ang * 8.f : 0.f), PAL_ROPE);
    spr(art_.rope, 228.f, 150.f + (!yourRope ? ang * 8.f : 0.f), PAL_ROPE);
    spr(art_.bell, bx, by, yours_ ? PAL_GOLD : PAL_CREAM, ang < 0);
    spr(art_.clapper, bx + ang * 7.f, by + 16.f, PAL_BRONZE);

    if (mode_ == Mode::Toll && lastHit_ && anim_ < 10) {
        spr(art_.wave, bx + (yours_ ? 22.f : -22.f), by + 8.f, yours_ ? PAL_GOLD : PAL_CREAM);
        if (yours_) spr(art_.wave, bx + 34.f, by + 2.f, PAL_GOLD);
    }

    const int marks = you_ > kSeven ? kSeven : you_;
    for (int i = 0; i < kSeven; i++) {
        int pal = i < marks ? PAL_PEG : PAL_DIM;
        // Peg palette index 1 is the empty post, 2 is a filled notch when PAL_PEG.
        if (i >= marks) pal = PAL_STONE;
        spr(art_.peg, 112.f + i * 16.f, 206.f, pal);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 BELL SEVEN", PAL_GOLD);
        hudC(21, "PLAY THE BELL", PAL_HUD);
        hudC(22, "YOUR TOLL IS TWO", PAL_GOLD);
        hudC(23, "THEIR CHIME IS ONE", PAL_CREAM);
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
        hudC(24, "THE BELL STANDS", PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(1, "STILL ON THE ROPE", PAL_BAD);
        std::snprintf(buf, sizeof buf, "YOU %d  THEM %d", you_, them_);
        hudC(22, buf, PAL_BAD);
        hudC(23, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "YOU %d", you_);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "THEM %d", them_);
        hud(12, 1, buf, PAL_CREAM);
        std::snprintf(buf, sizeof buf, "SEVEN %d", kSeven);
        hud(26, 1, buf, PAL_DIM);
        if (mode_ == Mode::Pause) {
            hudC(24, "PAUSED", PAL_GOLD);
        } else if (mode_ == Mode::Leave) {
            hudC(22, "FIRST TO SEVEN", PAL_GOLD);
            hudC(23, "THE COUNT STANDS", PAL_HUD);
            hudC(25, "LEAVE", PAL_GOLD);
        } else if (yours_) {
            hudC(24, "YOUR TOLL  TWO", PAL_GOLD);
            hudC(26, "C PULL", PAL_DIM);
        } else {
            hudC(24, "THEIR CHIME  ONE", PAL_CREAM);
            hudC(26, "THE OTHER ROPE", PAL_DIM);
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
    } else if (mode_ == Mode::Swing) {
        if (!bot_ && yours_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Swing;
            mode_ = Mode::Pause;
        } else if (!bot_ && yours_ && pad.pressed(gs::BTN_A)) {
            leave();
        } else {
            const bool handsOff = bot_ || !yours_;
            const bool tap = handsOff ? (wind_ == kSweet) : pad.pressed(gs::BTN_C);
            if (tap) pull(wind_ >= kWindowLo && wind_ <= kWindowHi);
            else if (++wind_ >= kMissAt) pull(false);
        }
    } else if (mode_ == Mode::Toll) {
        anim_++;
        if (anim_ >= 18) afterToll();
    } else if (mode_ == Mode::Leave) {
        if (bot_ || pad.pressed(gs::BTN_A)) leave();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    draw();
}

}  // namespace bellseven
