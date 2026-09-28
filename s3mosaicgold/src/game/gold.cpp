#include "game/gold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace mosaicgold {
namespace {

constexpr int UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3;

int stepCell(int cell, int dir) {
    int c = cell % COLS, r = cell / COLS;
    if (dir == UP) r--;
    else if (dir == RIGHT) c++;
    else if (dir == DOWN) r++;
    else c--;
    if (c < 0 || r < 0 || c >= COLS || r >= ROWS) return -1;
    return r * COLS + c;
}

gs::Button dirButton(int dir) {
    static const gs::Button k[4] = {gs::BTN_UP, gs::BTN_RIGHT, gs::BTN_DOWN, gs::BTN_LEFT};
    return k[dir & 3];
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.14f, 0.28f, 0.16f);
    phase_ = Phase::Title;
    t_ = 0;
    won_ = false;
    over_ = false;
    moves_ = 0;
    tally();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    pumpAudio();
    if (paused_) {
        if (sys.pad.pressed(gs::BTN_START)) paused_ = false;
        else {
            draw();
            return;
        }
    } else if (!bot_ && phase_ == Phase::Play && sys.pad.pressed(gs::BTN_START)) {
        paused_ = true;
        draw();
        return;
    }
    update();
    draw();
}

int Game::marker() const {
    if (phase_ == Phase::Victory) return 3;
    if (phase_ == Phase::Seal) return 2;
    if (phase_ == Phase::Play || phase_ == Phase::Scramble) return 1;
    return 0;
}

uint32_t Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return rng_ >> 16;
}

bool Game::confirm() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
}

bool Game::arranged() const {
    for (int i = 0; i < CELLS; i++)
        if (grid_[i] != i) return false;
    return true;
}

void Game::tally() {
    gold_ = 0;
    cream_ = 0;
    for (int i = 0; i < CELLS; i++) {
        int id = grid_[i];
        if (id == BLANK) continue;
        if (art_.gold[id]) gold_++;
        else cream_++;
    }
    bare_ = gold_ + cream_;
    score_ = gold_ * 2 + cream_;
}

void Game::begin() {
    for (int i = 0; i < CELLS; i++) grid_[i] = i;
    blank_ = BLANK;
    rng_ = 0xC0FFEE01u;
    buildPath();
    anim_.on = false;
    step_ = 0;
    moves_ = 0;
    t_ = 0;
    paused_ = false;
    hist_.clear();
    tally();
    phase_ = Phase::Study;
}

void Game::buildPath() {
    int g[CELLS];
    for (int i = 0; i < CELLS; i++) g[i] = i;
    int blank = BLANK;
    path_.clear();
    solve_.clear();
    int last = -1;
    const int need = 12;
    int guard = 0;
    auto solved = [&]() {
        for (int i = 0; i < CELLS; i++)
            if (g[i] != i) return false;
        return true;
    };
    while ((int)path_.size() < need || solved()) {
        if (++guard > 4000) break;
        int opts[4], n = 0, any = -1;
        for (int d = 0; d < 4; d++) {
            if (stepCell(blank, d) < 0) continue;
            any = d;
            if (last < 0 || d != (last ^ 2)) opts[n++] = d;
        }
        int dir = n ? opts[int(rnd() % uint32_t(n))] : any;
        if (dir < 0) break;
        int nxt = stepCell(blank, dir);
        std::swap(g[blank], g[nxt]);
        blank = nxt;
        path_.push_back(dir);
        last = dir;
        if ((int)path_.size() >= need && !solved()) break;
    }
    for (int i = (int)path_.size() - 1; i >= 0; --i) solve_.push_back(path_[i] ^ 2);
}

bool Game::slide(int dir, bool count) {
    int nxt = stepCell(blank_, dir);
    if (nxt < 0) return false;
    int id = grid_[nxt];
    std::swap(grid_[nxt], grid_[blank_]);
    anim_.on = true;
    anim_.id = id;
    anim_.from = nxt;
    anim_.to = blank_;
    anim_.t = 0;
    anim_.n = slideFrames();
    blank_ = nxt;
    if (count) {
        moves_++;
        hist_.push_back(dir);
    }
    click();
    tally();
    return true;
}

void Game::undo() {
    if (hist_.empty() || anim_.on) {
        bonk();
        return;
    }
    int dir = hist_.back();
    hist_.pop_back();
    if (moves_ > 0) moves_--;
    if (!slide(dir ^ 2, false)) {
        hist_.push_back(dir);
        moves_++;
        bonk();
    }
}

void Game::update() {
    if (anim_.on) {
        if (++anim_.t >= anim_.n) anim_.on = false;
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (phase_ == Phase::Title) {
        if (bot_) {
            if (++t_ > 10) begin();
        } else if (confirm()) {
            begin();
        }
        return;
    }
    if (phase_ == Phase::Study) {
        int lim = bot_ ? 8 : 90;
        if (++t_ >= lim || (!bot_ && confirm())) {
            t_ = 0;
            step_ = 0;
            phase_ = Phase::Scramble;
        }
        return;
    }
    if (phase_ == Phase::Scramble) {
        if (step_ >= (int)path_.size()) {
            phase_ = Phase::Play;
            step_ = 0;
            t_ = 0;
            return;
        }
        slide(path_[step_++], false);
        return;
    }
    if (phase_ == Phase::Play) {
        if (bot_) {
            if (step_ >= (int)solve_.size()) {
                if (arranged()) {
                    phase_ = Phase::Seal;
                    t_ = 0;
                    fan(0);
                }
                return;
            }
            slide(solve_[step_++], true);
            return;
        }
        if (p.pressed(gs::BTN_X) || p.pressed(gs::BTN_B)) undo();
        for (int d = 0; d < 4; d++) {
            if (p.pressed(dirButton(d))) {
                if (!slide(d, true)) bonk();
                break;
            }
        }
        if (arranged()) {
            phase_ = Phase::Seal;
            t_ = 0;
            fan(0);
        }
        return;
    }
    if (phase_ == Phase::Seal) {
        if (++t_ >= (bot_ ? 10 : 28)) {
            phase_ = Phase::Victory;
            t_ = 0;
            const bool math = score_ == gold_ * 2 + cream_ && bare_ == gold_ + cream_;
            if (arranged() && gold_ > 0 && bare_ < LINE && score_ >= LINE && math) {
                won_ = true;
                fan(1);
            }
        }
        return;
    }
    if (phase_ == Phase::Victory) {
        if (++t_ > 24 && bot_) over_ = true;
    }
}

void Game::pumpAudio() {
    if (clickLeft_ > 0 && --clickLeft_ == 0) sys_->apu.tone(1, 0, 0);
    if (bonkLeft_ > 0 && --bonkLeft_ == 0) sys_->apu.tone(2, 0, 0);
    if (fanLeft_ > 0 && --fanLeft_ == 0) sys_->apu.tone(0, 0, 0);
}

void Game::click() {
    sys_->apu.tone(1, 740.f, 0.05f);
    clickLeft_ = 4;
}

void Game::bonk() {
    sys_->apu.tone(2, 90.f, 0.04f);
    bonkLeft_ = 5;
}

void Game::fan(int step) {
    static const float n[4] = {523.f, 659.f, 784.f, 1046.f};
    sys_->apu.tone(0, n[step & 3], 0.08f);
    fanLeft_ = 10;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int x = col + i;
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::blit(const gs::Image& img, int x, int y, int pal, int w, int h) {
    if (w < 0) w = img.w;
    if (h < 0) h = img.h;
    if (w < 1 || h < 1 || img.w < 1 || img.h < 1) return;
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
    int r0 = 2, g0 = 2, b0 = 4, r1 = 5, g1 = 4, b1 = 2;
    if (phase_ == Phase::Victory) {
        r0 = 3;
        g0 = 2;
        b0 = 1;
        r1 = 6;
        g1 = 4;
        b1 = 1;
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r = r0 + (r1 - r0) * y / gs::SCREEN_H;
        int g = g0 + (g1 - g0) * y / gs::SCREEN_H;
        int b = b0 + (b1 - b0) * y / gs::SCREEN_H;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        sys_->vdp.road[y].on = false;
    }
}

void Game::drawTitle() {
    auto center = [&](const gs::Image& img, int y, int pal) { blit(img, (gs::SCREEN_W - img.w) / 2, y, pal); };
    center(art_.title, 28, PAL_GOLD);
    center(art_.sub, 62, PAL_CREAM);
    center(art_.tag, 80, PAL_CREAM);
    blit(art_.thumb, (gs::SCREEN_W - art_.thumb.w) / 2, 100, PAL_MOSAIC);
    if ((sys_->frame / 24) % 2 == 0) center(art_.prompt, 186, PAL_GOLD);
    hudC(26, "ARROWS SLIDE    CREAM IS NOT DOUBLE", PAL_DIM);
    const char* ver = S3_VERSION_STRING;
    hud(40 - int(std::strlen(ver)), 27, ver, PAL_DIM);
}

void Game::drawBoard() {
    if (anim_.on) {
        float u = anim_.n > 0 ? anim_.t / float(anim_.n) : 1.f;
        if (u < 0) u = 0;
        if (u > 1) u = 1;
        int x0 = cellX(anim_.from), y0 = cellY(anim_.from);
        int x1 = cellX(anim_.to), y1 = cellY(anim_.to);
        int x = x0 + int(std::lround((x1 - x0) * u));
        int y = y0 + int(std::lround((y1 - y0) * u));
        blit(art_.tile[anim_.id], x, y, PAL_MOSAIC);
    }
    for (int i = 0; i < CELLS; i++) {
        if (anim_.on && i == anim_.to) continue;
        int id = grid_[i];
        if (id == BLANK) continue;
        blit(art_.tile[id], cellX(i), cellY(i), PAL_MOSAIC);
    }
    if (phase_ == Phase::Play && !paused_ && !anim_.on) {
        for (int d = 0; d < 4; d++) {
            if (stepCell(blank_, d) < 0) continue;
            const gs::Image& a = art_.arrow[d];
            int x = cellX(blank_) + (TILE - a.w) / 2;
            int y = cellY(blank_) + (TILE - a.h) / 2;
            blit(a, x, y, PAL_GOLD);
        }
    }

    blit(art_.thumb, THUMB_X, OY + 24, PAL_MOSAIC);
    gs::Sprite sh;
    sh.img = art_.frame;
    sh.x = int16_t(OX - FRAME + 3);
    sh.y = int16_t(OY - FRAME + 4);
    sh.w = art_.frame.w;
    sh.h = art_.frame.h;
    sh.pal = PAL_WOOD;
    sh.shadow = true;
    sys_->vdp.sprite(sh);
    blit(art_.frame, OX - FRAME, OY - FRAME, PAL_WOOD);

    char buf[48];
    hud(1, 0, "S3 MOSAIC GOLD", PAL_DIM);
    if (phase_ == Phase::Victory && won_) hudC(1, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
    else if (phase_ == Phase::Study) hudC(1, "STUDY THE MEDALLION", PAL_CREAM);
    else if (phase_ == Phase::Scramble) hudC(1, "SHUFFLING", PAL_GOLD);
    else if (phase_ == Phase::Seal) hudC(1, "THE PICTURE IS WHOLE", PAL_GREEN);
    else hudC(1, "SLIDE UNTIL THE LINE CLEARS", PAL_CREAM);

    std::snprintf(buf, sizeof buf, "GOLD %d X2", gold_);
    hud(1, 25, buf, PAL_GOLD);
    std::snprintf(buf, sizeof buf, "CREAM %d", cream_);
    hud(14, 25, buf, PAL_CREAM);
    std::snprintf(buf, sizeof buf, "SCORE %d", score_);
    hud(26, 25, buf, won_ ? PAL_GREEN : PAL_CREAM);
    std::snprintf(buf, sizeof buf, "BARE %d  LINE %d  MOVES %d", bare_, LINE, moves_);
    hud(1, 26, buf, PAL_DIM);
    if (paused_) hudC(27, "PAUSED", PAL_GOLD);
    else if (phase_ == Phase::Victory && won_) hudC(27, "LEAVE", PAL_GOLD);
    else hudC(27, "CREAM DOES NOT BUY THE DOUBLE", PAL_DIM);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    if (phase_ == Phase::Title) drawTitle();
    else drawBoard();
}

}  // namespace mosaicgold
