#include "game/lensgold.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace lensgold {
namespace {
constexpr int kWindowLo = 16;
constexpr int kWindowHi = 22;
constexpr int kMissAt = 30;
static_assert(kGolds * kGoldFace * 2 + kCreams * kCreamFace >= kLine, "doubled gold clears the order");
static_assert(kGolds * kGoldFace + kCreams * kCreamFace < kLine, "bare faces stay short");
static_assert(kGolds * kGoldFace * 2 < kLine, "gold alone does not buy the order");
static_assert((kGolds - 1) * kGoldFace * 2 + kCreams * kCreamFace < kLine, "leave on the last gold");
static_assert(kPlates == kGolds + kCreams, "the short lens is six plates");
}  // namespace

bool Game::open() const {
    const int face = golds_ * kGoldFace * 2 + cream_ * kCreamFace;
    return !spoiled_ && finisherGold_ && plates_ == kPlates && golds_ == kGolds && cream_ == kCreams && score_ == face &&
           bare_ < kLine && score_ >= kLine;
}

void Game::begin() {
    score_ = bare_ = golds_ = cream_ = plates_ = 0;
    wind_ = anim_ = 0;
    over_ = won_ = finisherGold_ = spoiled_ = false;
    why_ = "";
    mode_ = Mode::Rack;
    sys_->apu.silence();
}

void Game::seat(bool hit) {
    const bool gold = plateGold(plates_);
    if (hit) {
        if (gold) {
            bare_ += kGoldFace;
            score_ += kGoldFace * 2;
            golds_++;
            finisherGold_ = true;
            sys_->apu.keyOn(0, 220.f, 0.32f);
            sys_->apu.keyOn(1, 440.f, 0.2f);
        } else {
            bare_ += kCreamFace;
            score_ += kCreamFace;
            cream_++;
            finisherGold_ = false;
            sys_->apu.keyOn(0, 164.f, 0.22f);
        }
    } else {
        spoiled_ = true;
        finisherGold_ = false;
        why_ = "the lens missed the plate";
        sys_->apu.noiseBurst(0.26f, 70.f, 0.18f);
    }
    plates_++;
    anim_ = 0;
    mode_ = Mode::Seat;
}

void Game::leave() {
    won_ = open();
    over_ = true;
    mode_ = Mode::Over;
    if (!won_ && why_[0] == 0) why_ = "the double was not the leave";
    if (won_) {
        why_ = "only the gold counts double";
        sys_->apu.keyOn(0, 440.f, 0.3f);
        sys_->apu.keyOn(1, 554.f, 0.24f);
        sys_->apu.keyOn(2, 659.f, 0.18f);
    } else {
        sys_->apu.noiseBurst(0.3f, 80.f, 0.22f);
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
    uint16_t top = gs::rgb4(1, 1, 3);
    uint16_t mid = gs::rgb4(2, 4, 7);
    uint16_t bot = gs::rgb4(1, 1, 2);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(8, 6, 2);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 1, 2);
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

    const float plateX = 248.f;
    const float railY = 150.f;
    int show = plates_ < kPlates ? plates_ : kPlates - 1;
    if (mode_ == Mode::Title) show = int(sys_->frame / 24) % kPlates;
    const bool gold = plateGold(show);

    float travel = 0.f;
    if (mode_ == Mode::Rack) {
        if (wind_ < kWindowLo) travel = wind_ / float(kWindowLo);
        else if (wind_ <= kWindowHi) travel = 1.f;
        else travel = 1.f - 0.4f * ((wind_ - kWindowHi) / float(kMissAt - kWindowHi));
    } else if (mode_ == Mode::Seat) {
        travel = 1.f;
    } else if (mode_ == Mode::Title) {
        travel = 0.5f + 0.12f * std::sin(sys_->frame * 0.07f);
    }

    spr(art_.bench, 160.f, railY + 18.f, PAL_BENCH);
    spr(gold ? art_.plateGold : art_.plateCream, plateX, railY - 36.f, gold ? PAL_GOLD : PAL_CREAM);

    const float lx = 48.f + travel * 150.f;
    spr(art_.lens, lx, railY - 22.f, PAL_BRASS);
    const bool sharp = (mode_ == Mode::Rack && wind_ >= kWindowLo && wind_ <= kWindowHi) || mode_ == Mode::Seat ||
                       mode_ == Mode::Title;
    if (sharp) spr(art_.spark, plateX, railY - 36.f, PAL_SPARK);

    char buf[72];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 LENS GOLD", PAL_GOLD);
        hudC(22, "A SHORT LENS", PAL_HUD);
        hudC(23, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(24, "CREAM KEEPS ITS FACE", PAL_CREAM);
        hudC(25, "LEAVE WHEN THAT IS TRUE", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(1, "LEFT ON THE DOUBLE", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        hudC(23, buf, PAL_GOLD);
        hudC(24, "ONLY THE GOLD COUNTED DOUBLE", PAL_HUD);
        hudC(25, "THE SHORT LENS IS DONE", PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(1, "STILL AT THE LENS", PAL_BAD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        hudC(23, buf, PAL_BAD);
        hudC(24, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "BARE %d", bare_);
        hud(12, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "ORDER %d", kLine);
        hud(28, 1, buf, PAL_DIM);
        std::snprintf(buf, sizeof buf, "PLATE %d/%d", show + 1, kPlates);
        hudC(22, buf, gold ? PAL_GOLD : PAL_CREAM);
        std::snprintf(buf, sizeof buf, "GOLDS %d  CREAM %d", golds_, cream_);
        hudC(23, buf, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(25, "PAUSED", PAL_GOLD);
        else if (plates_ == kPlates && !spoiled_) hudC(25, "LEAVE  THE GOLD IS DOUBLE", PAL_GOLD);
        else if (gold) hudC(25, "SEAT THE GOLD", PAL_GOLD);
        else hudC(25, "CREAM IS NOT A DOUBLE", PAL_CREAM);
        hudC(27, "C SEAT  A LEAVE", PAL_DIM);
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
        if (plates_ >= kPlates) {
            // The short lens is done. Wait for the leave.
        } else if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Rack;
            mode_ = Mode::Pause;
        } else {
            const bool tap = bot_ ? (wind_ == (kWindowLo + kWindowHi) / 2) : pad.pressed(gs::BTN_C);
            if (tap) seat(wind_ >= kWindowLo && wind_ <= kWindowHi);
            else if (++wind_ >= kMissAt) seat(false);
        }
    } else if (mode_ == Mode::Seat) {
        anim_++;
        if (anim_ >= 14) {
            wind_ = 0;
            anim_ = 0;
            mode_ = Mode::Gap;
        }
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (anim_ >= 12) {
            anim_ = 0;
            mode_ = Mode::Rack;
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    if (mode_ == Mode::Rack && plates_ >= kPlates) {
        if (bot_ || pad.pressed(gs::BTN_A)) leave();
    } else if (!bot_ && (mode_ == Mode::Rack || mode_ == Mode::Gap || mode_ == Mode::Seat) && pad.pressed(gs::BTN_A)) {
        leave();
    }

    draw();
}

}  // namespace lensgold
