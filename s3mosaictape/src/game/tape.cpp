#include "game/tape.h"

#include <cstdio>
#include <cstring>

namespace tape {
namespace {

constexpr int PITCH = 24;
constexpr int OX = (gs::SCREEN_W - N * PITCH) / 2;
constexpr int TAPE_Y = 62;
constexpr int DRAWER_Y = 128;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    phase_ = Phase::Title;
    t_ = 0;
    cx_ = 0;
    swaps_ = 0;
    won_ = false;
    over_ = false;
    paused_ = false;
    shuffle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (paused_) {
        if (sys.pad.pressed(gs::BTN_START)) paused_ = false;
        draw();
        return;
    }
    update();
    draw();
}

int Game::marker() const {
    if (phase_ == Phase::Title) return 0;
    if (phase_ == Phase::Slide) return 1;
    if (phase_ == Phase::Seal) return 2;
    return 3;
}

bool Game::matched() const {
    for (int i = 0; i < N; i++)
        if (drawer_[i] != art_.order[i]) return false;
    return true;
}

uint32_t Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return rng_;
}

void Game::shuffle() {
    for (int i = 0; i < N; i++) drawer_[i] = i;
    for (int i = N - 1; i > 0; i--) {
        int j = int(rnd() % uint32_t(i + 1));
        int tmp = drawer_[i];
        drawer_[i] = drawer_[j];
        drawer_[j] = tmp;
    }
    if (matched()) {
        int tmp = drawer_[0];
        drawer_[0] = drawer_[1];
        drawer_[1] = tmp;
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.18f);
    beep_ = 5;
}

void Game::swapRight() {
    if (cx_ >= N - 1) return;
    int tmp = drawer_[cx_];
    drawer_[cx_] = drawer_[cx_ + 1];
    drawer_[cx_ + 1] = tmp;
    swaps_++;
    blip(220.f + float(cx_) * 30.f);
}

void Game::update() {
    const gs::Pad& p = sys_->pad;
    t_++;
    if (phase_ == Phase::Title) {
        if (bot_ ? t_ > 12 : (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C))) {
            phase_ = Phase::Slide;
            t_ = 0;
        }
        return;
    }
    if (phase_ == Phase::Seal) {
        if (t_ > (bot_ ? 14 : 48)) {
            phase_ = Phase::Victory;
            t_ = 0;
            won_ = true;
            blip(523.f);
        }
        return;
    }
    if (phase_ == Phase::Victory) {
        if (t_ > (bot_ ? 10 : 90)) over_ = true;
        return;
    }

    if (!bot_ && p.pressed(gs::BTN_START)) {
        paused_ = true;
        return;
    }

    if (bot_) {
        if (matched()) {
            phase_ = Phase::Seal;
            t_ = 0;
            return;
        }
        int goal = -1;
        for (int i = 0; i < N; i++) {
            if (drawer_[i] != art_.order[i]) {
                goal = i;
                break;
            }
        }
        int src = goal;
        for (int i = goal; i < N; i++) {
            if (drawer_[i] == art_.order[goal]) {
                src = i;
                break;
            }
        }
        if (cx_ < src - 1) cx_++;
        else if (cx_ > src - 1) cx_--;
        else swapRight();
        if (matched()) {
            phase_ = Phase::Seal;
            t_ = 0;
        }
        return;
    }

    if (p.pressed(gs::BTN_LEFT) && cx_ > 0) cx_--;
    if (p.pressed(gs::BTN_RIGHT) && cx_ < N - 1) cx_++;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) swapRight();
    if (p.pressed(gs::BTN_B) && cx_ > 0) {
        cx_--;
        swapRight();
        cx_++;
    }
    if (matched()) {
        phase_ = Phase::Seal;
        t_ = 0;
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Image& img, int x, int y, int w, int h, int pal) {
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys_->vdp.lineBackdrop[y] = gs::rgb4(3 + y / 70, 2, 2 + (y > 150 ? 1 : 0));
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    backdrop();

    if (phase_ == Phase::Title) {
        hudC(6, "S3 MOSAIC TAPE", PAL_GOLD);
        hudC(9, "THE DRAWER HAS TO", PAL_CREAM);
        hudC(11, "MATCH THE TAPE", PAL_CREAM);
        hudC(16, bot_ ? "SOLVING" : "PRESS START", PAL_LEAF);
        hudC(22, "ARROWS MOVE   A SWAPS RIGHT", PAL_DIM);
        vdp.render(sys_->fb);
        return;
    }

    hudC(1, "TAPE", PAL_GOLD);
    spr(art_.tape, OX - 4, TAPE_Y - 4, N * PITCH + 8, 28, PAL_TILE);
    for (int i = 0; i < N; i++) {
        int id = art_.order[i];
        spr(art_.piece[id], OX + i * PITCH + 2, TAPE_Y, 20, 20, PAL_TILE);
    }

    hudC(12, "DRAWER", PAL_CREAM);
    spr(art_.drawer, OX - 6, DRAWER_Y - 6, N * PITCH + 12, 36, PAL_TILE);
    for (int i = 0; i < N; i++) {
        int id = drawer_[i];
        spr(art_.piece[id], OX + i * PITCH + 2, DRAWER_Y + 2, 20, 20, PAL_TILE);
    }
    if (phase_ == Phase::Slide && !paused_) spr(art_.cursor, OX + cx_ * PITCH, DRAWER_Y + 24, 24, 6, PAL_TILE);

    char line[40];
    int same = 0;
    for (int i = 0; i < N; i++)
        if (drawer_[i] == art_.order[i]) same++;
    std::snprintf(line, sizeof line, "MATCH %d/%d   SWAPS %d", same, N, swaps_);
    hudC(24, line, PAL_DIM);

    if (paused_) hudC(20, "PAUSED", PAL_GOLD);
    else if (phase_ == Phase::Seal) hudC(20, "THE DRAWER MATCHES", PAL_LEAF);
    else if (phase_ == Phase::Victory) hudC(20, "TAPE SEALED", PAL_GOLD);
    else hudC(26, "A SWAP RIGHT    B SWAP LEFT", PAL_DIM);

    vdp.render(sys_->fb);
}

}  // namespace tape
