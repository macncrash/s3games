#include "game/seven.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace lensseven {
namespace {
constexpr int kWindowLo = 15;
constexpr int kWindowHi = 23;
constexpr int kMissAt = 34;
constexpr int kMid = (kWindowLo + kWindowHi) / 2;
}  // namespace

void Game::begin() {
    you_ = them_ = wind_ = anim_ = seated_ = flareN_ = 0;
    over_ = won_ = left_ = false;
    yours_ = true;
    why_ = "";
    mode_ = Mode::Rack;
    sys_->apu.silence();
}

void Game::seat(bool hit) {
    if (hit) {
        if (yours_) {
            you_ += kGoldFace;
            sys_->apu.keyOn(0, 247.f, 0.3f);
            sys_->apu.keyOn(1, 494.f, 0.16f);
        } else {
            them_ += kCreamFace;
            sys_->apu.keyOn(0, 175.f, 0.2f);
        }
        flareN_ = 5;
    } else {
        flareN_ = 0;
        sys_->apu.noiseBurst(0.2f, 55.f, 0.16f);
    }
    seated_++;
    anim_ = 0;
    mode_ = Mode::Seat;
}

void Game::afterSeat() {
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
        why_ = "the rival was first to seven";
        mode_ = Mode::Over;
        sys_->apu.noiseBurst(0.26f, 68.f, 0.2f);
        return;
    }
    yours_ = !yours_;
    mode_ = Mode::Rack;
}

void Game::leave() {
    left_ = true;
    over_ = true;
    won_ = you_ >= kSeven && them_ < kSeven;
    mode_ = Mode::Over;
    if (won_) {
        why_ = "first to seven";
        sys_->apu.keyOn(0, 392.f, 0.26f);
        sys_->apu.keyOn(1, 523.f, 0.2f);
        sys_->apu.keyOn(2, 659.f, 0.14f);
    } else {
        why_ = "left before first to seven";
        sys_->apu.noiseBurst(0.24f, 78.f, 0.18f);
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
    uint16_t top = gs::rgb4(1, 1, 4);
    uint16_t mid = gs::rgb4(2, 5, 8);
    uint16_t bot = gs::rgb4(1, 1, 2);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(10, 8, 2);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 1, 2);
    if (mode_ == Mode::Leave) mid = gs::rgb4(6, 8, 10);
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
    sky();

    const float plateX = 236.f;
    const float railY = 152.f;
    const bool gold = yours_ || mode_ == Mode::Title;

    float travel = 0.15f;
    if (mode_ == Mode::Rack) {
        if (wind_ < kWindowLo) travel = wind_ / float(kWindowLo);
        else if (wind_ <= kWindowHi) travel = 1.f;
        else travel = 1.f - 0.35f * ((wind_ - kWindowHi) / float(kMissAt - kWindowHi));
    } else if (mode_ == Mode::Seat || mode_ == Mode::Leave) {
        travel = 1.f;
    } else if (mode_ == Mode::Title) {
        travel = 0.45f + 0.1f * std::sin(sys_->frame * 0.08f);
    }

    spr(art_.rail, 160.f, railY + 16.f, PAL_RAIL);
    spr(gold ? art_.plateGold : art_.plateCream, plateX, railY - 28.f, gold ? PAL_GOLD : PAL_CREAM);
    const float lx = 42.f + travel * 150.f;
    spr(art_.lens, lx, railY - 18.f, PAL_BRASS);

    const bool sharp = (mode_ == Mode::Rack && wind_ >= kWindowLo && wind_ <= kWindowHi) || mode_ == Mode::Seat ||
                       mode_ == Mode::Leave || mode_ == Mode::Title;
    if (sharp && flareN_ >= 0) {
        int n = (mode_ == Mode::Seat || mode_ == Mode::Leave) ? flareN_ : 1;
        for (int i = 0; i < n; i++) {
            float sx = plateX - 16.f + i * 8.f;
            float sy = railY - 48.f - (anim_ % 5) * 2.f;
            spr(art_.flare, sx, sy, PAL_FLARE);
        }
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 LENS SEVEN", PAL_GOLD);
        hudC(21, "PLAY THE LENS", PAL_HUD);
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
        hudC(1, "STILL AT THE LENS", PAL_BAD);
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
            hudC(23, "PAUSED", PAL_GOLD);
        } else if (mode_ == Mode::Leave) {
            hudC(22, "FIRST TO SEVEN", PAL_GOLD);
            hudC(23, "THE COUNT STANDS", PAL_HUD);
            hudC(25, "LEAVE", PAL_GOLD);
        } else if (yours_) {
            hudC(23, "YOUR GOLD  TWO", PAL_GOLD);
            hudC(25, "C SEAT", PAL_DIM);
        } else {
            hudC(23, "RIVAL CREAM  ONE", PAL_CREAM);
            hudC(25, "THEIR PLATE", PAL_DIM);
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
    } else if (mode_ == Mode::Rack) {
        if (!bot_ && yours_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Rack;
            mode_ = Mode::Pause;
        } else if (!bot_ && yours_ && pad.pressed(gs::BTN_A)) {
            leave();
        } else {
            const bool autoTap = bot_ || !yours_;
            const bool tap = autoTap ? (wind_ == kMid) : pad.pressed(gs::BTN_C);
            if (tap) seat(wind_ >= kWindowLo && wind_ <= kWindowHi);
            else if (++wind_ >= kMissAt) seat(false);
        }
    } else if (mode_ == Mode::Seat) {
        anim_++;
        if (anim_ >= 14) afterSeat();
    } else if (mode_ == Mode::Leave) {
        if (bot_ || pad.pressed(gs::BTN_A)) leave();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    draw();
}

}  // namespace lensseven
