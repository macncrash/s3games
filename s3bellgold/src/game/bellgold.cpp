#include "game/bellgold.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace bellgold {
namespace {
constexpr int kWindowLo = 16;
constexpr int kWindowHi = 22;
constexpr int kMissAt = 30;
static_assert(kGolds * kGoldFace * 2 + kCreams * kCreamFace >= kLine, "doubled gold clears the line");
static_assert(kGolds * kGoldFace + kCreams * kCreamFace < kLine, "bare faces stay short");
static_assert(kGolds * kGoldFace * 2 < kLine, "gold alone does not buy the line");
static_assert((kGolds - 1) * kGoldFace * 2 + kCreams * kCreamFace < kLine, "leave on the last gold");
static_assert(kRings == kGolds + kCreams, "the short bell is six rings");
}  // namespace

bool Game::open() const {
    const int face = golds_ * kGoldFace * 2 + cream_ * kCreamFace;
    return !spoiled_ && finisherGold_ && rings_ == kRings && golds_ == kGolds && cream_ == kCreams && score_ == face &&
           bare_ < kLine && score_ >= kLine;
}

void Game::begin() {
    score_ = bare_ = golds_ = cream_ = rings_ = 0;
    wind_ = anim_ = 0;
    over_ = won_ = finisherGold_ = spoiled_ = false;
    why_ = "";
    mode_ = Mode::Swing;
    sys_->apu.silence();
}

void Game::pull(bool hit) {
    const bool gold = ringGold(rings_);
    if (hit) {
        if (gold) {
            bare_ += kGoldFace;
            score_ += kGoldFace * 2;
            golds_++;
            finisherGold_ = true;
            sys_->apu.keyOn(0, 294.f, 0.32f);
            sys_->apu.keyOn(1, 440.f, 0.2f);
            sys_->apu.noiseBurst(0.08f, 700.f, 0.05f);
        } else {
            bare_ += kCreamFace;
            score_ += kCreamFace;
            cream_++;
            finisherGold_ = false;
            sys_->apu.keyOn(0, 220.f, 0.22f);
            sys_->apu.noiseBurst(0.06f, 280.f, 0.05f);
        }
    } else {
        spoiled_ = true;
        finisherGold_ = false;
        why_ = "the pull missed the swing";
        sys_->apu.noiseBurst(0.22f, 90.f, 0.16f);
    }
    rings_++;
    anim_ = 0;
    mode_ = Mode::Toll;
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
    uint16_t top = gs::rgb4(1, 1, 3);
    uint16_t mid = gs::rgb4(3, 3, 6);
    uint16_t bot = gs::rgb4(2, 2, 2);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(10, 7, 2);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(4, 1, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.6f ? mix(top, mid, u / 0.6f) : mix(mid, bot, (u - 0.6f) / 0.4f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    int show = rings_ < kRings ? rings_ : kRings - 1;
    if (mode_ == Mode::Title) show = int(sys_->frame / 24) % kRings;
    const bool gold = ringGold(show);
    const int bellPal = gold ? PAL_GOLD : PAL_CREAM;

    float ang = 0.f;
    if (mode_ == Mode::Swing) {
        float t = wind_ / float(kMissAt);
        ang = std::sin(t * 3.1415926f) * (wind_ >= kWindowLo && wind_ <= kWindowHi ? 1.f : 0.72f);
        if (wind_ < kWindowLo) ang *= wind_ / float(kWindowLo);
    } else if (mode_ == Mode::Toll) {
        ang = 0.35f * std::sin(anim_ * 0.7f);
    } else if (mode_ == Mode::Title) {
        ang = 0.25f * std::sin(sys_->frame * 0.05f);
    }

    const float pivotX = 176.f;
    const float pivotY = 36.f;
    const float bx = pivotX + ang * 36.f;
    const float by = pivotY + 36.f + std::fabs(ang) * 4.f;

    spr(art_.tower, 176.f, 96.f, PAL_STONE);
    spr(art_.ringer, 52.f, 148.f, PAL_RINGER);
    spr(art_.rope, 118.f, 150.f - ang * 6.f, PAL_ROPE);
    spr(art_.bell, bx, by, bellPal, ang < 0);
    spr(art_.clapper, bx + ang * 8.f, by + 18.f, PAL_BRONZE);
    if (mode_ == Mode::Toll && anim_ < 8) spr(art_.lip, bx + 18.f, by + 22.f, gold ? PAL_GOLD : PAL_CREAM);

    char buf[72];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 BELL GOLD", PAL_GOLD);
        hudC(22, "A SHORT BELL", PAL_HUD);
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
        hudC(25, "THE SHORT BELL IS DONE", PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(1, "STILL AT THE ROPE", PAL_BAD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        hudC(23, buf, PAL_BAD);
        hudC(24, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "BARE %d", bare_);
        hud(12, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "LINE %d", kLine);
        hud(28, 1, buf, PAL_DIM);
        std::snprintf(buf, sizeof buf, "RING %d/%d", show + 1, kRings);
        hudC(22, buf, gold ? PAL_GOLD : PAL_CREAM);
        std::snprintf(buf, sizeof buf, "GOLDS %d  CREAM %d", golds_, cream_);
        hudC(23, buf, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(25, "PAUSED", PAL_GOLD);
        else if (open() || (rings_ == kRings && !spoiled_)) hudC(25, "LEAVE  THE GOLD IS DOUBLE", PAL_GOLD);
        else if (gold) hudC(25, "PULL THE GOLD", PAL_GOLD);
        else hudC(25, "CREAM IS NOT A DOUBLE", PAL_CREAM);
        hudC(27, "C PULL  A LEAVE", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 2));
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
        if (rings_ >= kRings) {
            // The short bell is rung. Wait for the leave.
        } else if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Swing;
            mode_ = Mode::Pause;
        } else {
            const bool tap = bot_ ? (wind_ == (kWindowLo + kWindowHi) / 2) : pad.pressed(gs::BTN_C);
            if (tap) pull(wind_ >= kWindowLo && wind_ <= kWindowHi);
            else if (++wind_ >= kMissAt) pull(false);
        }
    } else if (mode_ == Mode::Toll) {
        anim_++;
        if (anim_ >= 14) {
            wind_ = 0;
            anim_ = 0;
            mode_ = Mode::Gap;
        }
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (anim_ >= 16) {
            anim_ = 0;
            if (rings_ >= kRings) {
                if (bot_) leave();
                else mode_ = Mode::Swing;
            } else {
                mode_ = Mode::Swing;
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    if (mode_ == Mode::Swing && rings_ >= kRings) {
        if (bot_ || pad.pressed(gs::BTN_A)) leave();
    } else if (!bot_ && (mode_ == Mode::Swing || mode_ == Mode::Gap || mode_ == Mode::Toll) &&
               pad.pressed(gs::BTN_A)) {
        leave();
    }

    draw();
}

}  // namespace bellgold
