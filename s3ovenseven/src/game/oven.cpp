#include "game/oven.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ovenseven {
namespace {
constexpr float kDt = 1.f / 60.f;
constexpr float kRise = 0.52f;
constexpr int kRace = 7;
constexpr float kGold0 = 0.66f;
constexpr float kGold1 = 0.80f;
constexpr float kCream0 = 0.48f;
}  // namespace

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    you_ = 0;
    them_ = 0;
    heat_ = 0;
    say_[0] = 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    sawSix_ = false;
    you_ = 0;
    them_ = 0;
    lastPts_ = 0;
    heat_ = 0;
    rival_ = 0;
    pop_ = 0;
    sayT_ = 0;
    std::snprintf(say_, sizeof say_, "HEAT THE OVEN");
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    clock_ = 0;
    if (bot_) begin();
    else toTitle();
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.22f);
    sys_->apu.tone(1, freq * 0.5f, 0.08f);
}

void Game::settle() {
    if (you_ == 6) sawSix_ = true;
    if (you_ >= kRace && them_ < kRace) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        std::snprintf(say_, sizeof say_, "FIRST TO SEVEN");
        blip(880.f);
        return;
    }
    if (them_ >= kRace && you_ < kRace) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        std::snprintf(say_, sizeof say_, "THEY LEFT FIRST");
        blip(140.f);
    }
}

void Game::pull(int pts) {
    you_ += pts;
    lastPts_ = pts;
    heat_ = 0;
    pop_ = 0.55f;
    sayT_ = 0.7f;
    if (pts >= 2) std::snprintf(say_, sizeof say_, "GOLD CRUST  +2");
    else std::snprintf(say_, sizeof say_, "CREAM LOAF  +1");
    blip(pts >= 2 ? 660.f : 440.f);
    settle();
}

void Game::burn() {
    heat_ = 0;
    sayT_ = 0.7f;
    std::snprintf(say_, sizeof say_, "BURNED");
    blip(110.f);
    scoreThem(1);
}

void Game::scoreThem(int pts) {
    them_ += pts;
    settle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    const gs::Pad& pad = sys.pad;
    const bool start = pad.pressed(gs::BTN_START);
    const bool action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z);

    if (mode_ == Mode::Title) {
        heat_ = 0.5f + 0.5f * std::sin(clock_ * 1.6f);
        if (bot_ || start || action) begin();
    } else if (mode_ == Mode::Play) {
        heat_ += kRise * kDt;
        rival_ += kDt;
        if (rival_ >= 2.35f) {
            rival_ = 0;
            sayT_ = std::max(sayT_, 0.4f);
            scoreThem(1);
            if (mode_ == Mode::Play) std::snprintf(say_, sizeof say_, "THEIR LOAF  +1");
        }
        if (mode_ == Mode::Play && heat_ >= 1.f) burn();
        bool take = false;
        if (mode_ == Mode::Play && bot_) take = heat_ >= 0.70f && heat_ < kGold1;
        if (mode_ == Mode::Play && !bot_ && action) take = true;
        if (take && mode_ == Mode::Play) {
            if (heat_ >= kGold0 && heat_ < kGold1) pull(2);
            else if (heat_ >= kCream0 && heat_ < kGold0) pull(1);
            else if (heat_ >= kGold1) burn();
            else {
                heat_ = 0;
                sayT_ = 0.5f;
                std::snprintf(say_, sizeof say_, "TOO SOON");
                blip(200.f);
            }
        }
        if (pop_ > 0) pop_ -= kDt;
        if (sayT_ > 0) sayT_ -= kDt;
        if (!bot_ && start) {
            mode_ = Mode::Title;
        }
    } else if (!bot_ && (start || action)) {
        toTitle();
    }

    if (mode_ != Mode::Play) sys.apu.tone(0, 0, 0);
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    const float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
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
    const int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 90) {
            float u = y / 90.f;
            v.lineBackdrop[y] = gs::rgb4(4 + int(u * 4), 2 + int(u * 2), 1);
        } else if (y < 160) {
            v.lineBackdrop[y] = gs::rgb4(7, 4, 2);
        } else {
            int g = 3 + ((y / 6) & 1);
            v.lineBackdrop[y] = gs::rgb4(g + 2, g, 1);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    backdrop();

    auto word = [&](const gs::Image& img, float cx, float cy, int pal) {
        if (img.w < 1) return;
        gs::Sprite s;
        s.w = img.w;
        s.h = img.h;
        s.x = int16_t(std::lround(cx - s.w * 0.5f));
        s.y = int16_t(std::lround(cy - s.h * 0.5f));
        s.img = img;
        s.pal = uint8_t(pal);
        sys_->vdp.sprite(s);
    };

    if (mode_ == Mode::Title) word(art_.title, 160.f, 36.f, PAL_TITLE);
    if (mode_ == Mode::Win) {
        word(art_.seven, 160.f, 78.f, PAL_GOOD);
        word(art_.leave, 160.f, 108.f, PAL_TITLE);
    }
    if (mode_ == Mode::Lose) word(art_.seven, 160.f, 86.f, PAL_BAD);

    const float mouth = 78.f + (1.f - std::min(heat_, 1.f)) * 22.f;
    float fh = 10.f + heat_ * 28.f;
    if (mode_ == Mode::Title) fh = 16.f + 8.f * std::sin(clock_ * 6.f);
    spr(art_.flame, 78.f, mouth, std::max(8.f, fh), PAL_FLAME);
    spr(art_.flame, 242.f, 128.f, 14.f + 4.f * std::sin(clock_ * 5.f + 1.f), PAL_FLAME);

    if (pop_ > 0.f || mode_ == Mode::Win) {
        float y = 96.f - (mode_ == Mode::Win ? 8.f : (0.55f - pop_) * 40.f);
        int pal = lastPts_ >= 2 || mode_ == Mode::Win ? PAL_GOLD : PAL_LOAF;
        spr(art_.loaf, 78.f, y, mode_ == Mode::Win ? 28.f : 20.f, pal);
    }
    if (them_ > 0) spr(art_.loaf, 242.f, 96.f, 16.f, PAL_CREAM);

    spr(art_.head, 28.f, 150.f + std::sin(clock_ * 3.f) * 1.5f, 40.f, PAL_BAKER, false);
    spr(art_.head, 292.f, 156.f, 34.f, PAL_RIVAL, true);

    for (int i = 0; i < kRace; i++) {
        int yp = i < you_ ? (i == 6 ? PAL_GOOD : PAL_GOLD) : PAL_INK;
        int tp = i < them_ ? PAL_BAD : PAL_INK;
        float s = (i == 6) ? 12.f : 8.f;
        spr(art_.pip, 40.f + float(i) * 16.f, 188.f, s, yp);
        spr(art_.pip, 168.f + float(i) * 16.f, 188.f, i == 6 ? 12.f : 8.f, tp);
    }

    spr(art_.rack, 78.f, 132.f, 10.f, PAL_OVEN);
    spr(art_.rack, 242.f, 140.f, 8.f, PAL_RIVAL);
    spr(art_.oven, 78.f, 128.f, 120.f, PAL_OVEN);
    spr(art_.oven, 242.f, 138.f, 92.f, PAL_RIVAL);

    char buf[48];
    hud(1, 0, "V" S3_VERSION, PAL_INK);
    if (mode_ == Mode::Title) {
        hudC(18, "FIRST TO SEVEN", PAL_TITLE);
        hudC(20, "CREAM 1    GOLD 2", PAL_GOLD);
        hudC(21, "A SIX IS STILL SHORT", PAL_BAD);
        hudC(23, "A OR C PULLS THE LOAF", PAL_INK);
        if ((int(clock_ * 2.f) & 1) == 0) hudC(25, "PRESS START", PAL_TITLE);
        return;
    }
    std::snprintf(buf, sizeof buf, "YOU %d", you_);
    hud(1, 1, buf, you_ >= kRace ? PAL_GOOD : (you_ == 6 ? PAL_BAD : PAL_GOLD));
    std::snprintf(buf, sizeof buf, "THEM %d", them_);
    hud(40 - int(std::strlen(buf)), 1, buf, them_ >= kRace ? PAL_BAD : PAL_INK);
    if (mode_ == Mode::Win) {
        hudC(22, "FIRST TO SEVEN", PAL_GOOD);
        hudC(23, "LEAVE THE OVEN", PAL_TITLE);
        std::snprintf(buf, sizeof buf, "YOU %d   THEM %d", you_, them_);
        hudC(24, buf, PAL_INK);
        if (!bot_) hudC(26, "START BAKES AGAIN", PAL_INK);
    } else if (mode_ == Mode::Lose) {
        hudC(22, "THEY HIT SEVEN", PAL_BAD);
        hudC(23, "UNDER SEVEN IS NOT THE GAME", PAL_BAD);
        if (!bot_) hudC(26, "START TRIES AGAIN", PAL_INK);
    } else {
        if (you_ == 6) hudC(22, "A SIX IS STILL SHORT", PAL_BAD);
        else if (heat_ >= kGold0 && heat_ < kGold1) hudC(22, "GOLD  PULL", PAL_GOLD);
        else if (heat_ >= kCream0 && heat_ < kGold0) hudC(22, "CREAM  PULL", PAL_CREAM);
        else if (heat_ >= kGold1) hudC(22, "TOO HOT", PAL_BAD);
        else hudC(22, "HEATING", PAL_INK);
        if (say_[0] && sayT_ > 0.f) hudC(24, say_, lastPts_ >= 2 ? PAL_GOLD : PAL_INK);
        if (!bot_ && you_ == 0) hudC(26, "A OR C PULLS", PAL_INK);
    }
}

}  // namespace ovenseven
