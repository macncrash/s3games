#include "game/loomgold.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace loomgold {
namespace {
constexpr int kWindowLo = 16;
constexpr int kWindowHi = 22;
constexpr int kMissAt = 30;
static_assert(kGolds * kGoldFace * 2 + kCreams * kCreamFace >= kLine, "doubled gold clears the order");
static_assert(kGolds * kGoldFace + kCreams * kCreamFace < kLine, "bare faces stay short");
static_assert(kGolds * kGoldFace * 2 < kLine, "gold alone does not buy the order");
static_assert((kGolds - 1) * kGoldFace * 2 + kCreams * kCreamFace < kLine, "leave on the last gold");
static_assert(kPicks == kGolds + kCreams, "the short loom is six picks");
}  // namespace

bool Game::open() const {
    const int face = golds_ * kGoldFace * 2 + cream_ * kCreamFace;
    return !spoiled_ && finisherGold_ && picks_ == kPicks && golds_ == kGolds && cream_ == kCreams && score_ == face &&
           bare_ < kLine && score_ >= kLine;
}

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.22f); }

void Game::begin() {
    score_ = bare_ = golds_ = cream_ = picks_ = 0;
    wind_ = anim_ = 0;
    over_ = won_ = finisherGold_ = spoiled_ = false;
    why_ = "";
    mode_ = Mode::Shed;
    sys_->apu.silence();
}

void Game::catchPick(bool hit) {
    const bool gold = pickGold(picks_);
    if (hit) {
        if (gold) {
            bare_ += kGoldFace;
            score_ += kGoldFace * 2;
            golds_++;
            finisherGold_ = true;
            sys_->apu.keyOn(0, 220.f, 0.32f);
            sys_->apu.keyOn(1, 440.f, 0.18f);
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
        why_ = "the shuttle missed the shed";
        sys_->apu.noiseBurst(0.24f, 90.f, 0.16f);
    }
    picks_++;
    anim_ = 0;
    mode_ = Mode::Beat;
}

void Game::leave() {
    won_ = open();
    over_ = true;
    mode_ = Mode::Over;
    if (!won_ && why_[0] == 0) why_ = "the double was not the leave";
    if (won_) {
        why_ = "only the gold counts double";
        sys_->apu.keyOn(0, 392.f, 0.28f);
        sys_->apu.keyOn(1, 494.f, 0.22f);
        sys_->apu.keyOn(2, 587.f, 0.16f);
    } else {
        sys_->apu.noiseBurst(0.28f, 70.f, 0.2f);
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

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 2, 4);
    uint16_t mid = gs::rgb4(5, 3, 2);
    uint16_t bot = gs::rgb4(3, 2, 1);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(10, 7, 2);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 1, 1);
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
    v.A.enabled = false;
    v.B.enabled = false;
    sky();

    const float cx = 160.f;
    const float clothTop = 168.f;
    spr(art_.post, 78.f, 120.f, PAL_WOOD);
    spr(art_.post, 242.f, 120.f, PAL_WOOD);
    spr(art_.beam, cx, 48.f, PAL_WOOD);
    spr(art_.beam, cx, 188.f, PAL_WOOD);
    for (int i = 0; i < 9; i++) spr(art_.warp, 96.f + i * 16.f, 118.f, PAL_WARP);

    const int woven = picks_;
    for (int i = 0; i < woven && i < kPicks; i++) {
        const bool g = pickGold(i);
        float y = clothTop - float(i) * 10.f;
        spr(g ? art_.weftGold : art_.weftCream, cx, y, g ? PAL_GOLD : PAL_CREAM);
    }

    int show = picks_ < kPicks ? picks_ : kPicks - 1;
    if (mode_ == Mode::Title) show = int(sys_->frame / 24) % kPicks;
    const bool gold = pickGold(show);

    float travel = 0.f;
    if (mode_ == Mode::Shed) travel = wind_ / float(kMissAt);
    else if (mode_ == Mode::Beat) travel = 0.55f;
    else travel = 0.15f;
    if (travel > 1.f) travel = 1.f;
    const bool rtl = (show & 1) != 0;
    float sx = 96.f + travel * 128.f;
    if (rtl) sx = 224.f - travel * 128.f;
    float sy = 78.f;
    if (picks_ < kPicks && mode_ != Mode::Title) sy = clothTop - float(picks_) * 10.f - 14.f;
    if (mode_ != Mode::Over || !won_) spr(art_.shuttle, sx, sy, PAL_SHUTTLE, rtl);
    float reedY = sy + 12.f;
    if (mode_ == Mode::Beat) reedY = sy + 4.f + float(anim_);
    spr(art_.reed, cx, reedY, PAL_WOOD);
    spr(art_.bobbin, 40.f, 70.f, gold ? PAL_SHUTTLE : PAL_CREAM);
    spr(art_.bobbin, 40.f, 100.f, gold ? PAL_CREAM : PAL_SHUTTLE);

    char buf[72];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 LOOM GOLD", PAL_GOLD);
        hudC(22, "A SHORT LOOM", PAL_HUD);
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
        hudC(25, "THE SHORT LOOM IS DONE", PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(1, "STILL AT THE LOOM", PAL_BAD);
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
        std::snprintf(buf, sizeof buf, "PICK %d/%d", show + 1, kPicks);
        hudC(22, buf, gold ? PAL_GOLD : PAL_CREAM);
        std::snprintf(buf, sizeof buf, "GOLDS %d  CREAM %d", golds_, cream_);
        hudC(23, buf, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(25, "PAUSED", PAL_GOLD);
        else if (open() || (picks_ == kPicks && !spoiled_)) hudC(25, "LEAVE  THE GOLD IS DOUBLE", PAL_GOLD);
        else if (gold) hudC(25, "CATCH THE GOLD WEFT", PAL_GOLD);
        else hudC(25, "CREAM IS NOT A DOUBLE", PAL_CREAM);
        hudC(27, "C CATCH  A LEAVE", PAL_DIM);
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
    } else if (mode_ == Mode::Shed) {
        if (picks_ >= kPicks) {
            if (bot_) leave();
            else if (pad.pressed(gs::BTN_A)) leave();
        } else if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Shed;
            mode_ = Mode::Pause;
        } else if (!bot_ && pad.pressed(gs::BTN_A)) {
            leave();
        } else {
            const bool tap = bot_ ? (wind_ == (kWindowLo + kWindowHi) / 2) : pad.pressed(gs::BTN_C);
            if (tap) catchPick(wind_ >= kWindowLo && wind_ <= kWindowHi);
            else if (++wind_ >= kMissAt) catchPick(false);
        }
    } else if (mode_ == Mode::Beat) {
        anim_++;
        if (anim_ >= 12) {
            wind_ = 0;
            anim_ = 0;
            mode_ = Mode::Gap;
        }
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (anim_ >= 14) {
            anim_ = 0;
            wind_ = 0;
            mode_ = Mode::Shed;
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
    }
    draw();
}

}  // namespace loomgold
