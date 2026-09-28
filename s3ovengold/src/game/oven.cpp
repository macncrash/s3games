#include "game/oven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ovengold {
namespace {

constexpr int kReady = 90;
constexpr int kGoldLo = 150;
constexpr int kGoldHi = 190;
constexpr int kBurn = 230;
constexpr int kBotGold = 164;
constexpr int kBotCream = 112;

const char* kName[kPans] = {"ROUND", "TIN", "COB"};

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
        case Mode::Title: return "title";
        case Mode::Bake: return "bake";
        case Mode::Show: return "show";
        case Mode::Win: return "win";
        case Mode::Lose: return "lose";
        case Mode::Pause: return "pause";
    }
    return "?";
}

bool Game::paid() const {
    const int bare = gold_ + cream_;
    return finisherGold_ && gold_ >= 1 && score_ >= kLine && bare < kLine && score_ == gold_ * 2 + cream_;
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.06f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(3, 2, 2));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.2f, 0.12f);
    if (bot_) begin();
    else toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    finisherGold_ = false;
    gold_ = cream_ = score_ = pan_ = heat_ = 0;
    say_[0] = 0;
    for (int i = 0; i < kPans; i++) mark_[i] = -2;
    if (sys_) sys_->apu.silence();
}

void Game::begin() {
    toTitle();
    mode_ = Mode::Bake;
    heat_ = 0;
    lock_ = 8;
    if (sys_) sys_->setLight(190, 80, 24);
}

void Game::nextPan() {
    pan_++;
    heat_ = 0;
    lock_ = 12;
    mode_ = Mode::Bake;
    say_[0] = 0;
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    std::snprintf(say_, sizeof say_, "DOUBLE");
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.1f);
        sys_->apu.tone(1, 659.f, 0.08f);
        sys_->setLight(255, 190, 40);
    }
}

void Game::lose() {
    won_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    if (say_[0] == 0) std::snprintf(say_, sizeof say_, "NO DOUBLE");
    if (sys_) sys_->setLight(180, 30, 20);
}

void Game::burn() {
    if (pan_ >= 0 && pan_ < kPans) mark_[pan_] = -1;
    std::snprintf(say_, sizeof say_, "BURNED");
    blip(70.f);
    if (sys_) sys_->apu.noiseBurst(0.5f, 90.f, 0.3f);
    lose();
}

void Game::pull() {
    if (pan_ < 0 || pan_ >= kPans || heat_ < kReady || heat_ >= kBurn) {
        if (heat_ < kReady) std::snprintf(say_, sizeof say_, "TOO SOON");
        return;
    }
    const bool gold = goldKind_[pan_];
    const bool band = heat_ >= kGoldLo && heat_ < kGoldHi;
    int mark = 0;
    if (gold && band) {
        gold_++;
        score_ += 2;
        mark = 2;
        if (score_ >= kLine) finisherGold_ = true;
        std::snprintf(say_, sizeof say_, "DOUBLE");
        blip(523.f);
    } else if (!gold && score_ + 1 >= kLine) {
        mark = 0;
        std::snprintf(say_, sizeof say_, "NOT DOUBLE");
        blip(98.f);
    } else {
        cream_++;
        score_ += 1;
        mark = 1;
        std::snprintf(say_, sizeof say_, "CREAM");
        blip(330.f);
    }
    mark_[pan_] = mark;
    show_ = 28;
    mode_ = Mode::Show;
}

void Game::afterShow() {
    if (paid()) win();
    else if (pan_ + 1 >= kPans) lose();
    else nextPan();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) begin();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Bake) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            heat_++;
            if (heat_ >= kBurn) burn();
            else if (lock_ > 0) lock_--;
            else if (bot_) {
                const bool gold = goldKind_[pan_];
                if ((gold && heat_ >= kBotGold) || (!gold && heat_ >= kBotCream)) pull();
            } else if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_B)) {
                pull();
            }
        }
    } else if (mode_ == Mode::Show) {
        if (--show_ <= 0) afterShow();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Bake;
        else if (pad.pressed(gs::BTN_MODE)) toTitle();
    } else if (!bot_) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE)) toTitle();
    }
    anim_ += 1.f / 60.f;
    if (mode_ == Mode::Bake && (heat_ % 40) == 0) sys.apu.noise(0.02f, 700.f, false);
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 220));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 400));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::solid(float x, float y, float w, float h, int pal) {
    gs::Sprite s;
    s.img = art_.solid;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s || !s[0]) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float k = float(y) / 223.f;
        int r = int(2 + (1.f - k) * 8);
        int g = int(1 + (1.f - k) * 3);
        int b = 2;
        if (mode_ == Mode::Win) g = std::min(15, g + 3);
        if (mode_ == Mode::Lose) r = std::min(15, r + 3);
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        sys_->vdp.lineFog[y] = 0;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    backdrop();
    if (mode_ == Mode::Title) {
        gs::Sprite s;
        s.img = art_.word;
        s.w = art_.word.w;
        s.h = art_.word.h;
        s.x = int16_t(160 - s.w / 2);
        s.y = 28;
        s.pal = PAL_GOLD;
        vdp.sprite(s);
        hudC(8, "A SHORT OVEN", PAL_INK);
        hudC(10, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(14, "A DRAW", PAL_CREAM);
        hudC(16, "LINE 4", PAL_INK);
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        gs::Sprite s;
        s.img = mode_ == Mode::Win ? art_.doubled : art_.noDouble;
        s.w = s.img.w;
        s.h = s.img.h;
        s.x = int16_t(160 - s.w / 2);
        s.y = 18;
        s.pal = mode_ == Mode::Win ? PAL_GOLD : PAL_ALERT;
        vdp.sprite(s);
    }

    for (int i = 0; i < kPans; i++) {
        float x = 70.f + float(i) * 90.f;
        spr(art_.arch, x, 118, 70, PAL_BRICK);
        int pal = goldKind_[i] ? PAL_GOLD : PAL_CREAM;
        if (mark_[i] == 1) pal = PAL_CREAM;
        if (mark_[i] == 2) pal = PAL_GOLD;
        if (mark_[i] == -1) pal = PAL_ASH;
        float ly = (i == pan_ && mode_ == Mode::Bake) ? 108.f : 124.f;
        if (mark_[i] != -2 || i == pan_) spr(art_.loaf, x, ly, 28, pal);
        if (i == pan_ && (mode_ == Mode::Bake || mode_ == Mode::Show)) {
            spr(art_.flame, x, 148, 18 + 2.f * std::sin(anim_ * 10.f), PAL_FIRE);
            spr(art_.peel, x, 176, 14, PAL_WOOD);
            float w = 64.f * std::min(heat_, kBurn) / float(kBurn);
            solid(x - 32.f, 160, w, 4, heat_ >= kGoldLo ? PAL_GOLD : PAL_OK);
            solid(x - 32.f + 64.f * kGoldLo / float(kBurn), 158, 64.f * (kGoldHi - kGoldLo) / float(kBurn), 8, PAL_GOLD);
        }
    }
    char buf[48];
    std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
    hudC(24, buf, PAL_INK);
    if (mode_ == Mode::Bake && pan_ >= 0 && pan_ < kPans) {
        std::snprintf(buf, sizeof buf, "%s  %s", goldKind_[pan_] ? "GOLD" : "CREAM", kName[pan_]);
        hudC(22, buf, goldKind_[pan_] ? PAL_GOLD : PAL_CREAM);
    }
    if (say_[0]) hudC(20, say_, PAL_ALERT);
    hud(1, 26, "A DRAW", PAL_INK);
    hud(28, 26, "LINE 4", PAL_GOLD);
}

}  // namespace ovengold
