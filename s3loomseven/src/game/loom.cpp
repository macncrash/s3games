#include "game/loom.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace loomseven {
namespace {
constexpr int kSpan = 36;
constexpr int kOpenLo = 14;
constexpr int kOpenHi = 22;
constexpr int kBotAt = (kOpenLo + kOpenHi) / 2;
constexpr int kBeat = 10;
constexpr int kHouseEvery = 52;
}  // namespace

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.22f); }

void Game::begin() {
    you_ = house_ = 0;
    wind_ = beat_ = houseClock_ = 0;
    rtl_ = false;
    over_ = won_ = false;
    mode_ = Mode::Fly;
    sys_->apu.silence();
}

void Game::finish() {
    over_ = true;
    won_ = you_ >= kGoal && house_ < kGoal;
    mode_ = won_ ? Mode::Win : Mode::Lose;
    if (won_) {
        sys_->apu.keyOn(0, 392.f, 0.28f);
        sys_->apu.keyOn(1, 494.f, 0.22f);
        sys_->apu.keyOn(2, 587.f, 0.16f);
    } else {
        sys_->apu.noiseBurst(0.28f, 70.f, 0.2f);
    }
}

void Game::throwShuttle(bool inShed) {
    if (inShed && you_ < kGoal) {
        you_++;
        sys_->apu.keyOn(0, 220.f + you_ * 28.f, 0.28f);
    } else if (!inShed) {
        sys_->apu.noiseBurst(0.18f, 90.f, 0.12f);
    }
    rtl_ = !rtl_;
    beat_ = 0;
    mode_ = Mode::Beat;
    if (you_ >= kGoal && house_ < kGoal) finish();
}

void Game::houseTick() {
    if (over_) return;
    if (++houseClock_ < kHouseEvery) return;
    houseClock_ = 0;
    if (house_ < kGoal) {
        house_++;
        sys_->apu.keyOn(2, 146.f, 0.12f);
    }
    if (house_ >= kGoal && you_ < kGoal) finish();
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
    uint16_t mid = gs::rgb4(4, 3, 3);
    uint16_t bot = gs::rgb4(3, 2, 2);
    if (mode_ == Mode::Win) mid = gs::rgb4(3, 6, 10);
    if (mode_ == Mode::Lose) mid = gs::rgb4(6, 2, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.45f ? mix(top, mid, u / 0.45f) : mix(mid, bot, (u - 0.45f) / 0.55f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::bench(float cy, int rows, int pal, float shuttleX, bool showShuttle, bool rtl) {
    const float cx = 160.f;
    spr(art_.post, 86.f, cy, PAL_WOOD);
    spr(art_.post, 234.f, cy, PAL_WOOD);
    spr(art_.beam, cx, cy - 30.f, PAL_WOOD);
    spr(art_.beam, cx, cy + 30.f, PAL_WOOD);
    for (int i = 0; i < 8; i++) spr(art_.warp, 100.f + i * 16.f, cy, PAL_WARP);
    for (int i = 0; i < rows && i < kGoal; i++) spr(art_.weft, cx, cy + 22.f - float(i) * 7.f, pal);
    if (showShuttle) {
        float sy = cy + 22.f - float(rows) * 7.f - 8.f;
        spr(art_.shuttle, shuttleX, sy, PAL_SHUTTLE, rtl);
        spr(art_.reed, cx, sy + 8.f, PAL_WOOD);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    sky();

    float travel = wind_ / float(kSpan);
    if (travel > 1.f) travel = 1.f;
    if (mode_ != Mode::Fly) travel = 0.5f;
    float sx = rtl_ ? 220.f - travel * 120.f : 100.f + travel * 120.f;
    int hphase = int(sys_->frame / 6) % kSpan;
    float ht = hphase / float(kSpan);
    bool hrtl = ((sys_->frame / (6 * kSpan)) & 1) != 0;
    float hx = hrtl ? 220.f - ht * 120.f : 100.f + ht * 120.f;

    bool live = mode_ == Mode::Fly || mode_ == Mode::Beat;
    bench(62.f, house_, PAL_HOUSE, hx, mode_ != Mode::Title, hrtl);
    bench(164.f, you_, PAL_YOU, sx, live || mode_ == Mode::Title, rtl_);

    char buf[64];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 LOOM SEVEN", PAL_YOU);
        hudC(12, "TWO BENCHES", PAL_HUD);
        hudC(13, "FIRST TO SEVEN WEFTS", PAL_YOU);
        hudC(14, "THROW IN THE OPEN SHED", PAL_HOUSE);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_YOU);
    } else if (mode_ == Mode::Win) {
        hudC(1, "FIRST TO SEVEN", PAL_YOU);
        std::snprintf(buf, sizeof buf, "YOU %d   HOUSE %d", you_, house_);
        hudC(13, buf, PAL_YOU);
        hudC(14, "THE NEAR BENCH LEAVES", PAL_HUD);
        if (!bot_ && (f & 16) == 0) hudC(26, "START", PAL_YOU);
    } else if (mode_ == Mode::Lose) {
        hudC(1, "HOUSE FINISHED FIRST", PAL_BAD);
        std::snprintf(buf, sizeof buf, "YOU %d   HOUSE %d", you_, house_);
        hudC(13, buf, PAL_BAD);
        if ((f & 16) == 0) hudC(26, "START", PAL_HOUSE);
    } else {
        std::snprintf(buf, sizeof buf, "YOU %d", you_);
        hud(1, 1, buf, PAL_YOU);
        std::snprintf(buf, sizeof buf, "HOUSE %d", house_);
        hud(28, 1, buf, PAL_HOUSE);
        hudC(0, "FIRST TO SEVEN", PAL_HUD);
        bool open = mode_ == Mode::Fly && wind_ >= kOpenLo && wind_ <= kOpenHi;
        if (open) hudC(26, "SHED OPEN", PAL_YOU);
        else hudC(26, "C THROW", PAL_DIM);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 27, ver, PAL_DIM);
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
    } else if (mode_ == Mode::Fly) {
        const bool tap = bot_ ? (wind_ == kBotAt) : (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A));
        if (tap) throwShuttle(wind_ >= kOpenLo && wind_ <= kOpenHi);
        else if (++wind_ >= kSpan) throwShuttle(false);
        if (!over_) houseTick();
    } else if (mode_ == Mode::Beat) {
        beat_++;
        if (!over_) houseTick();
        if (!over_ && beat_ >= kBeat) {
            wind_ = 0;
            mode_ = Mode::Fly;
        }
    } else if (!bot_ && (mode_ == Mode::Win || mode_ == Mode::Lose)) {
        if (pad.pressed(gs::BTN_START)) begin();
    }
    draw();
}

}  // namespace loomseven
