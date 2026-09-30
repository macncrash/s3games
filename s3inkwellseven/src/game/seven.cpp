#include "game/seven.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace inkwellseven {
namespace {
constexpr int kWindowLo = 12;
constexpr int kWindowHi = 20;
constexpr int kMissAt = 30;
constexpr int kMid = (kWindowLo + kWindowHi) / 2;
}  // namespace

void Game::begin() {
    you_ = them_ = wind_ = anim_ = dips_ = 0;
    sink_ = 0;
    over_ = won_ = left_ = false;
    yours_ = true;
    why_ = "";
    mode_ = Mode::Dip;
    sys_->apu.silence();
}

void Game::dip(bool hit) {
    sink_ = hit ? 1.f : 0.25f;
    if (hit) {
        if (yours_) {
            you_ += kGoldFace;
            sys_->apu.tone(0, 523.f, 0.08f);
            sys_->apu.tone(1, 784.f, 0.05f);
        } else {
            them_ += kCreamFace;
            sys_->apu.tone(0, 330.f, 0.06f);
        }
    } else {
        sys_->apu.tone(0, 140.f, 0.08f);
        sys_->apu.noiseBurst(0.1f, 500.f, 0.05f);
    }
    dips_++;
    anim_ = 0;
    mode_ = Mode::Splash;
}

void Game::afterSplash() {
    wind_ = 0;
    anim_ = 0;
    sink_ = 0;
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
        sys_->apu.tone(0, 110.f, 0.14f);
        return;
    }
    yours_ = !yours_;
    mode_ = Mode::Dip;
}

void Game::leave() {
    left_ = true;
    over_ = true;
    won_ = you_ >= kSeven && them_ < kSeven;
    mode_ = Mode::Over;
    if (won_) {
        why_ = "first to seven";
        sys_->apu.tone(0, 523.f, 0.1f);
        sys_->apu.tone(1, 659.f, 0.1f);
        sys_->apu.tone(2, 784.f, 0.14f);
    } else {
        why_ = "left before first to seven";
        sys_->apu.tone(0, 160.f, 0.12f);
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip) {
    if (img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 2, 4);
    uint16_t mid = gs::rgb4(5, 4, 6);
    uint16_t bot = gs::rgb4(3, 2, 2);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(10, 8, 3);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(6, 2, 2);
    if (mode_ == Mode::Leave) mid = gs::rgb4(9, 7, 3);
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

    const float goldX = 108.f;
    const float creamX = 214.f;
    const float wellY = 156.f;
    const bool gold = yours_;
    float lift = 46.f;
    if (mode_ == Mode::Dip) {
        if (wind_ < kWindowLo) lift = 46.f - 40.f * (wind_ / float(kWindowLo));
        else if (wind_ <= kWindowHi) lift = 6.f;
        else lift = 6.f + 28.f * ((wind_ - kWindowHi) / float(kMissAt - kWindowHi));
    } else if (mode_ == Mode::Splash) {
        lift = anim_ < 6 ? 6.f + sink_ * 14.f : 18.f;
    } else if (mode_ == Mode::Title) {
        lift = 18.f + float((sys_->frame / 10) % 5);
    } else if (mode_ == Mode::Leave) {
        lift = 10.f;
    }

    spr(art_.desk, 160.f, 188.f, float(art_.desk.w), float(art_.desk.h), PAL_DESK);
    spr(art_.page, 48.f, 132.f, float(art_.page.w), float(art_.page.h), PAL_PAGE);
    spr(art_.well, goldX, wellY, 56.f, 42.f, PAL_YOU);
    spr(art_.well, creamX, wellY, 56.f, 42.f, PAL_RIVAL);
    spr(art_.pool, goldX, wellY - 12.f, 24.f, 10.f, PAL_YOU);
    spr(art_.pool, creamX, wellY - 12.f, 24.f, 10.f, PAL_RIVAL);

    const float qx = (mode_ == Mode::Title) ? 160.f : (gold ? goldX : creamX);
    const float qy = wellY - 52.f - lift;
    spr(art_.quill, qx, qy, 16.f, 64.f, PAL_QUILL);
    if (sink_ > 0.2f && (mode_ == Mode::Splash || mode_ == Mode::Leave)) {
        const int pal = gold ? PAL_YOU : PAL_RIVAL;
        spr(art_.bead, qx - 6.f, qy + 36.f, 8.f, 8.f, pal);
        spr(art_.bead, qx + 7.f, qy + 30.f, 6.f, 6.f, pal);
    }

    const int marks = yours_ ? you_ : them_;
    for (int i = 0; i < marks && i < 12; i++) {
        float bx = 22.f + (i % 6) * 9.f;
        float by = 112.f + (i / 6) * 12.f;
        spr(art_.bead, bx, by, 6.f, 6.f, yours_ ? PAL_GOLD : PAL_CREAM);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 INKWELL SEVEN", PAL_GOLD);
        hudC(20, "A SHORT INKWELL", PAL_HUD);
        hudC(21, "YOUR GOLD DIP IS TWO", PAL_GOLD);
        hudC(22, "THEIR CREAM DIP IS ONE", PAL_CREAM);
        hudC(23, "FIRST TO SEVEN", PAL_HUD);
        hudC(24, "LEAVE WHEN THAT IS TRUE", PAL_GOLD);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(1, "LEFT ON SEVEN", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "YOU %d  THEM %d", you_, them_);
        hudC(21, buf, PAL_GOLD);
        hudC(22, "FIRST TO SEVEN", PAL_HUD);
        hudC(23, "A COUNT PAST SEVEN STANDS", PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(1, "STILL AT THE WELL", PAL_BAD);
        std::snprintf(buf, sizeof buf, "YOU %d  THEM %d", you_, them_);
        hudC(21, buf, PAL_BAD);
        hudC(22, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
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
            hudC(21, "FIRST TO SEVEN", PAL_GOLD);
            hudC(22, "THE COUNT STANDS", PAL_HUD);
            hudC(24, "LEAVE", PAL_GOLD);
        } else if (yours_) {
            hudC(22, "YOUR GOLD  TWO", PAL_GOLD);
            hudC(24, "C DIP", PAL_DIM);
        } else {
            hudC(22, "RIVAL CREAM  ONE", PAL_CREAM);
            hudC(24, "THEIR DIP", PAL_DIM);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.55f);
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (sink_ > 0.f && mode_ != Mode::Splash) sink_ *= 0.8f;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Dip) {
        if (!bot_ && yours_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Dip;
            mode_ = Mode::Pause;
        } else if (!bot_ && yours_ && pad.pressed(gs::BTN_A)) {
            leave();
        } else {
            const bool autoTap = bot_ || !yours_;
            const bool tap = autoTap ? (wind_ == kMid) : pad.pressed(gs::BTN_C);
            if (tap) dip(wind_ >= kWindowLo && wind_ <= kWindowHi);
            else if (++wind_ >= kMissAt) dip(false);
        }
    } else if (mode_ == Mode::Splash) {
        anim_++;
        if (anim_ >= 14) afterSplash();
    } else if (mode_ == Mode::Leave) {
        if (bot_ || pad.pressed(gs::BTN_A)) leave();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    draw();
}

}  // namespace inkwellseven
