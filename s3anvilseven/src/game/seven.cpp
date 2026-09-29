#include "game/seven.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace anvilseven {
namespace {
constexpr int kWindowLo = 14;
constexpr int kWindowHi = 22;
constexpr int kMissAt = 32;
constexpr int kMid = (kWindowLo + kWindowHi) / 2;
}  // namespace

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.22f); }

void Game::begin() {
    you_ = them_ = wind_ = anim_ = heats_ = sparkN_ = 0;
    over_ = won_ = left_ = false;
    yours_ = true;
    why_ = "";
    mode_ = Mode::Heat;
    sys_->apu.silence();
}

void Game::strike(bool hit) {
    if (hit) {
        if (yours_) {
            you_ += kGoldFace;
            sys_->apu.keyOn(0, 220.f, 0.32f);
            sys_->apu.keyOn(1, 440.f, 0.18f);
            sys_->apu.noiseBurst(0.18f, 220.f, 0.07f);
        } else {
            them_ += kCreamFace;
            sys_->apu.keyOn(0, 164.f, 0.22f);
            sys_->apu.noiseBurst(0.1f, 120.f, 0.07f);
        }
        sparkN_ = 6;
    } else {
        sparkN_ = 0;
        sys_->apu.noiseBurst(0.22f, 60.f, 0.16f);
    }
    heats_++;
    anim_ = 0;
    mode_ = Mode::Spark;
}

void Game::afterSpark() {
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
        sys_->apu.noiseBurst(0.28f, 70.f, 0.22f);
        return;
    }
    yours_ = !yours_;
    mode_ = Mode::Heat;
}

void Game::leave() {
    left_ = true;
    over_ = true;
    won_ = you_ >= kSeven && them_ < kSeven;
    mode_ = Mode::Over;
    if (won_) {
        why_ = "first to seven";
        sys_->apu.keyOn(0, 392.f, 0.28f);
        sys_->apu.keyOn(1, 494.f, 0.22f);
        sys_->apu.keyOn(2, 587.f, 0.16f);
    } else {
        why_ = "left before first to seven";
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

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 1, 3);
    uint16_t mid = gs::rgb4(7, 3, 1);
    uint16_t bot = gs::rgb4(2, 2, 2);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(12, 8, 2);
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
    sky();

    const float ax = 168.f;
    const float ay = 158.f;
    const bool gold = yours_;
    float lift = 62.f;
    if (mode_ == Mode::Heat) {
        if (wind_ < kWindowLo) lift = 62.f - 62.f * (wind_ / float(kWindowLo));
        else if (wind_ <= kWindowHi) lift = 0.f;
        else lift = 36.f * ((wind_ - kWindowHi) / float(kMissAt - kWindowHi));
    } else if (mode_ == Mode::Spark) {
        lift = anim_ < 5 ? float(anim_ * 4) : 16.f;
    } else if (mode_ == Mode::Title) {
        lift = 8.f + float((sys_->frame / 8) % 6);
    }

    const bool showRival = !yours_ && mode_ != Mode::Title;
    spr(art_.smith, showRival ? 250.f : 64.f, 128.f, showRival ? PAL_RIVAL : PAL_SMITH, showRival);
    spr(art_.anvil, ax, ay, PAL_IRON);
    if (mode_ != Mode::Over || anim_ < 40) {
        const gs::Image& bar = gold ? art_.barGold : art_.barCream;
        float by = ay - 26.f;
        if (mode_ == Mode::Spark && anim_ < 3) by += 2.f;
        spr(bar, ax - 2.f, by, gold ? PAL_GOLD : PAL_CREAM);
    }
    spr(art_.hammer, ax + 8.f, ay - 64.f - lift, PAL_IRON);
    if (sparkN_ > 0 && (mode_ == Mode::Spark || mode_ == Mode::Leave)) {
        for (int i = 0; i < sparkN_; i++) {
            float sx = ax - 24.f + i * 10.f;
            float sy = ay - 40.f - (anim_ % 6) * 2.f - (i & 1) * 5.f;
            spr(art_.spark, sx, sy, PAL_FIRE);
        }
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 ANVIL SEVEN", PAL_GOLD);
        hudC(21, "PLAY THE ANVIL", PAL_HUD);
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
        hudC(1, "STILL AT THE ANVIL", PAL_BAD);
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
            hudC(25, "C STRIKE", PAL_DIM);
        } else {
            hudC(23, "RIVAL CREAM  ONE", PAL_CREAM);
            hudC(25, "THEIR BLOW", PAL_DIM);
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
    } else if (mode_ == Mode::Heat) {
        if (!bot_ && yours_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Heat;
            mode_ = Mode::Pause;
        } else if (!bot_ && yours_ && pad.pressed(gs::BTN_A)) {
            leave();
        } else {
            const bool autoTap = bot_ || !yours_;
            const bool tap = autoTap ? (wind_ == kMid) : pad.pressed(gs::BTN_C);
            if (tap) strike(wind_ >= kWindowLo && wind_ <= kWindowHi);
            else if (++wind_ >= kMissAt) strike(false);
        }
    } else if (mode_ == Mode::Spark) {
        anim_++;
        if (anim_ >= 16) afterSpark();
    } else if (mode_ == Mode::Leave) {
        if (bot_ || pad.pressed(gs::BTN_A)) leave();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    draw();
}

}  // namespace anvilseven
