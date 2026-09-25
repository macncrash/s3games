#include "game/mosaic.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace mosaic {
namespace {

constexpr int UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3;

// Blank steps in `dir`. Returns the cell it enters, or -1 at the edge.
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
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.16f, 0.32f, 0.18f);
    phase_ = Phase::Title;
    t_ = 0;
    won_ = false;
    over_ = false;
    paused_ = false;
    moves_ = 0;
    picture_ = 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    pumpAudio();
    lamp();
    // Start toggles pause. The press that resumes must not latch another pause.
    if (paused_) {
        if (sys.pad.pressed(gs::BTN_START)) {
            paused_ = false;
            pauseLatch_ = false;
        } else {
            draw();
            return;
        }
    } else if (!bot_ && phase_ == Phase::Play && sys.pad.pressed(gs::BTN_START)) {
        pauseLatch_ = true;
    }
    update();
    if (pauseLatch_ && !anim_.on && phase_ == Phase::Play) {
        pauseLatch_ = false;
        paused_ = true;
    }
    draw();
}

int Game::marker() const {
    if (phase_ == Phase::Victory) return 3;
    if (phase_ == Phase::Title) return 0;
    if (phase_ == Phase::Banner) return 2;
    if (phase_ == Phase::Play || phase_ == Phase::Scramble) return 1;
    return 4;
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

void Game::beginPicture(int index) {
    picture_ = index;
    for (int i = 0; i < CELLS; i++) grid_[i] = i;
    blank_ = BLANK;
    rng_ = 0xA341316Cu ^ (uint32_t(index + 1) * 0x85EBCA6Bu);
    buildPath();
    anim_.on = false;
    step_ = 0;
    picMoves_ = 0;
    t_ = 0;
    held_ = -1;
    heldT_ = 0;
    want_ = -1;
    paused_ = false;
    pauseLatch_ = false;
    undoLatch_ = false;
    hist_.clear();
    phase_ = Phase::Study;
}

void Game::buildPath() {
    int g[CELLS];
    for (int i = 0; i < CELLS; i++) g[i] = i;
    int blank = BLANK;
    path_.clear();
    solve_.clear();
    int last = -1;
    int need = art_.depth[picture_];
    int guard = 0;
    auto solved = [&]() {
        for (int i = 0; i < CELLS; i++)
            if (g[i] != i) return false;
        return true;
    };
    while ((int)path_.size() < need || solved()) {
        if (++guard > 8000) break;
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

    for (int i = 0; i < CELLS; i++) g[i] = i;
    blank = BLANK;
    for (int dir : path_) {
        int nxt = stepCell(blank, dir);
        std::swap(g[blank], g[nxt]);
        blank = nxt;
    }
    for (int dir : solve_) {
        int nxt = stepCell(blank, dir);
        std::swap(g[blank], g[nxt]);
        blank = nxt;
    }
    if (!solved() || blank != BLANK)
        std::fprintf(stderr, "s3mosaic: picture %d scramble does not invert\n", picture_ + 1);
}

bool Game::slide(int dir, bool count) {
    int nxt = stepCell(blank_, dir);
    if (nxt < 0) return false;
    int from = nxt;
    int to = blank_;
    int id = grid_[from];
    std::swap(grid_[from], grid_[to]);
    blank_ = from;
    anim_.on = true;
    anim_.id = id;
    anim_.from = from;
    anim_.to = to;
    anim_.t = 0;
    anim_.n = slideFrames();
    if (count) {
        moves_++;
        picMoves_++;
        hist_.push_back(dir);
    }
    click(id);
    return true;
}

void Game::undo() {
    if (hist_.empty()) {
        bonk();
        return;
    }
    int dir = hist_.back();
    hist_.pop_back();
    if (moves_ > 0) moves_--;
    if (picMoves_ > 0) picMoves_--;
    if (!slide(dir ^ 2, false)) {
        hist_.push_back(dir);
        moves_++;
        picMoves_++;
        bonk();
    }
}

void Game::snapScramble() {
    anim_.on = false;
    while (step_ < (int)path_.size()) {
        int dir = path_[step_++];
        int nxt = stepCell(blank_, dir);
        if (nxt < 0) break;
        std::swap(grid_[blank_], grid_[nxt]);
        blank_ = nxt;
    }
    phase_ = Phase::Play;
    step_ = 0;
    t_ = 0;
    picMoves_ = 0;
    want_ = -1;
    held_ = -1;
    hist_.clear();
}

void Game::senseDir() {
    const gs::Pad& p = sys_->pad;
    for (int d = 0; d < 4; d++) {
        if (!p.pressed(dirButton(d))) continue;
        held_ = d;
        heldT_ = 0;
        want_ = d;
        return;
    }
    if (held_ >= 0 && p.down(dirButton(held_))) {
        heldT_++;
        if (heldT_ == 16 || (heldT_ > 16 && ((heldT_ - 16) % 8) == 0)) want_ = held_;
    } else {
        held_ = -1;
    }
}

void Game::update() {
    if (!bot_ && phase_ == Phase::Scramble && sys_->pad.pressed(gs::BTN_START)) {
        snapScramble();
        return;
    }
    if (!bot_ && phase_ == Phase::Play) {
        if (sys_->pad.pressed(gs::BTN_B) || sys_->pad.pressed(gs::BTN_X)) undoLatch_ = true;
        senseDir();
    }

    if (anim_.on) {
        if (++anim_.t >= anim_.n) anim_.on = false;
        if (anim_.on) return;
    }

    switch (phase_) {
    case Phase::Title:
        if (++t_ > 8 && (bot_ ? t_ > 40 : confirm())) {
            moves_ = 0;
            sys_->apu.tone(0, 660.f, 0.05f);
            fanLeft_ = 5;
            beginPicture(0);
        }
        break;
    case Phase::Study:
        if (++t_ > studyLimit() || (!bot_ && confirm())) {
            phase_ = Phase::Pop;
            t_ = 0;
        }
        break;
    case Phase::Pop:
        if (++t_ > popLimit()) {
            phase_ = Phase::Scramble;
            step_ = 0;
            t_ = 0;
        }
        break;
    case Phase::Scramble:
        if (step_ < (int)path_.size()) slide(path_[step_++], false);
        else {
            phase_ = Phase::Play;
            step_ = 0;
            t_ = 0;
            picMoves_ = 0;
            want_ = -1;
            held_ = -1;
            hist_.clear();
        }
        break;
    case Phase::Play:
        if (arranged()) {
            phase_ = Phase::Seal;
            t_ = 0;
            sys_->apu.tone(0, 988.f, 0.07f);
            fanLeft_ = 8;
            sys_->apu.noiseBurst(0.1f, 1400.f, 0.04f);
            sys_->rumble(0.25f, 0.4f, 120);
            break;
        }
        if (bot_) {
            if (step_ < (int)solve_.size()) slide(solve_[step_++], true);
            else {
                std::fprintf(stderr, "s3mosaic: solver stuck on picture %d\n", picture_ + 1);
                won_ = false;
                over_ = true;
            }
            break;
        }
        if (undoLatch_) {
            undoLatch_ = false;
            want_ = -1;
            undo();
            break;
        }
        if (want_ >= 0) {
            int dir = want_;
            want_ = -1;
            if (!slide(dir, true)) bonk();
        }
        break;
    case Phase::Seal:
        if (++t_ > sealLimit()) {
            phase_ = Phase::Banner;
            t_ = 0;
        }
        break;
    case Phase::Banner:
        if (t_ < 32 && t_ % 8 == 0) fan(t_ / 8);
        if (++t_ > bannerLimit() || (!bot_ && confirm())) {
            if (picture_ + 1 >= PICTURES) {
                phase_ = Phase::Victory;
                t_ = 0;
                won_ = true;
                sys_->rumble(0.45f, 0.75f, 280);
            } else {
                beginPicture(picture_ + 1);
            }
        }
        break;
    case Phase::Victory:
        if (t_ < 48 && t_ % 8 == 0) fan(t_ / 8);
        if (++t_ > 56 && bot_) over_ = true;
        else if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
            phase_ = Phase::Title;
            t_ = 0;
            won_ = false;
            moves_ = 0;
            paused_ = false;
        }
        break;
    }
}

void Game::pumpAudio() {
    if (clickLeft_ > 0 && --clickLeft_ == 0) sys_->apu.tone(1, 0, 0);
    if (bonkLeft_ > 0 && --bonkLeft_ == 0) sys_->apu.tone(2, 0, 0);
    if (fanLeft_ > 0 && --fanLeft_ == 0) sys_->apu.tone(0, 0, 0);
}

void Game::click(int id) {
    float freq = 300.f + float((id * 37) % 200);
    float vol = phase_ == Phase::Scramble ? 0.025f : 0.05f;
    sys_->apu.tone(1, freq, vol);
    clickLeft_ = 2;
    if (phase_ != Phase::Scramble) sys_->apu.noiseBurst(0.045f, 1900.f, 0.02f);
}

void Game::bonk() {
    sys_->apu.tone(2, 90.f, 0.04f);
    bonkLeft_ = 3;
}

void Game::fan(int step) {
    static const float n[] = {523.25f, 659.25f, 783.99f, 1046.5f, 783.99f, 1174.66f};
    if (step < 0 || step > 5) return;
    sys_->apu.tone(0, n[step], 0.075f);
    fanLeft_ = 8;
}

void Game::lamp() {
    if (phase_ == Phase::Title) sys_->setLight(8, 14, 32);
    else if (phase_ == Phase::Victory) sys_->setLight(48, 36, 10);
    else if (picture_ == 0) sys_->setLight(32, 26, 8);
    else if (picture_ == 1) sys_->setLight(12, 36, 10);
    else sys_->setLight(14, 18, 42);
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
    int r0, g0, b0, r1, g1, b1;
    if (phase_ == Phase::Title) {
        r0 = 1;
        g0 = 2;
        b0 = 6;
        r1 = 3;
        g1 = 2;
        b1 = 5;
    } else if (phase_ == Phase::Victory) {
        r0 = 2;
        g0 = 2;
        b0 = 5;
        r1 = 4;
        g1 = 3;
        b1 = 2;
    } else if (picture_ == 0) {
        r0 = 1;
        g0 = 3;
        b0 = 7;
        r1 = 2;
        g1 = 4;
        b1 = 6;
    } else if (picture_ == 1) {
        r0 = 2;
        g0 = 4;
        b0 = 3;
        r1 = 4;
        g1 = 4;
        b1 = 2;
    } else {
        r0 = 1;
        g0 = 1;
        b0 = 4;
        r1 = 2;
        g1 = 1;
        b1 = 3;
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r = r0 + (r1 - r0) * y / gs::SCREEN_H;
        int g = g0 + (g1 - g0) * y / gs::SCREEN_H;
        int b = b0 + (b1 - b0) * y / gs::SCREEN_H;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
    }
}

void Game::drawTitle() {
    auto center = [&](const gs::Image& img, int y, int pal) { blit(img, (gs::SCREEN_W - img.w) / 2, y, pal); };
    center(art_.title, 26, PAL_GOLD);
    center(art_.sub, 56, PAL_CREAM);
    center(art_.tag, 78, PAL_CREAM);
    const int tw = 64, gap = 16;
    int x0 = (gs::SCREEN_W - (tw * PICTURES + gap * (PICTURES - 1))) / 2;
    for (int i = 0; i < PICTURES; i++) blit(art_.thumb[i], x0 + i * (tw + gap), 96, art_.pal[i], tw, tw);
    if ((sys_->frame / 24) % 2 == 0) center(art_.prompt, 176, PAL_GOLD);
    hudC(26, "ARROWS MOVE THE GAP    X UNDOES", PAL_DIM);
    const char* ver = S3_VERSION_STRING;
    hud(40 - int(std::strlen(ver)), 27, ver, PAL_DIM);
}

void Game::drawTiles() {
    int pal = art_.pal[picture_];
    if (anim_.on) {
        float u = anim_.n > 0 ? anim_.t / float(anim_.n) : 1.f;
        if (u < 0) u = 0;
        if (u > 1) u = 1;
        u = u * u * (3.f - 2.f * u);
        int x0 = cellX(anim_.from), y0 = cellY(anim_.from);
        int x1 = cellX(anim_.to), y1 = cellY(anim_.to);
        int x = x0 + int(std::lround((x1 - x0) * u));
        int y = y0 + int(std::lround((y1 - y0) * u));
        blit(art_.tile[picture_][anim_.id], x, y, pal);
    }

    float ghost = -1.f;
    if (phase_ == Phase::Study || phase_ == Phase::Banner || phase_ == Phase::Victory) ghost = 1.f;
    else if (phase_ == Phase::Pop) ghost = 1.f - t_ / float(popLimit());
    else if (phase_ == Phase::Seal) {
        float u = t_ / float(sealLimit());
        if (u < 0) u = 0;
        if (u > 1) u = 1;
        ghost = 1.f - (1.f - u) * (1.f - u);
    }
    if (ghost > 0.04f) {
        int side = std::max(1, int(std::lround(TILE * ghost)));
        int x = cellX(BLANK) + (TILE - side) / 2;
        int y = cellY(BLANK) + (TILE - side) / 2 - int((1.f - ghost) * 8.f);
        blit(art_.tile[picture_][BLANK], x, y, pal, side, side);
    }

    for (int i = 0; i < CELLS; i++) {
        if (anim_.on && i == anim_.to) continue;
        int id = grid_[i];
        if (id == BLANK) continue;
        blit(art_.tile[picture_][id], cellX(i), cellY(i), pal);
    }
}

void Game::drawChevrons() {
    if (phase_ != Phase::Play || paused_ || anim_.on) return;
    for (int d = 0; d < 4; d++) {
        if (stepCell(blank_, d) < 0) continue;
        const gs::Image& a = art_.arrow[d];
        int x = cellX(blank_) + (TILE - a.w) / 2;
        int y = cellY(blank_) + (TILE - a.h) / 2;
        if (d == UP) y = cellY(blank_) + 2;
        else if (d == DOWN) y = cellY(blank_) + TILE - a.h - 2;
        else if (d == LEFT) x = cellX(blank_) + 2;
        else x = cellX(blank_) + TILE - a.w - 2;
        blit(a, x, y, PAL_GOLD);
    }
}

void Game::drawBoard() {
    drawTiles();
    drawChevrons();

    int pal = art_.pal[picture_];
    int ty = OY + (BOARD - art_.thumb[picture_].h) / 2;
    blit(art_.goal, THUMB_X + (art_.thumb[picture_].w - art_.goal.w) / 2, ty - art_.goal.h - 3, PAL_GOLD);
    blit(art_.thumb[picture_], THUMB_X, ty, pal);

    gs::Sprite sh;
    sh.img = art_.frame;
    sh.x = int16_t(OX - FRAME + 4);
    sh.y = int16_t(OY - FRAME + 5);
    sh.w = art_.frame.w;
    sh.h = art_.frame.h;
    sh.pal = PAL_WOOD;
    sh.shadow = true;
    sys_->vdp.sprite(sh);
    blit(art_.frame, OX - FRAME, OY - FRAME, PAL_WOOD);
    drawHud();
}

void Game::drawHud() {
    char buf[40];
    hud(1, 0, "S3 MOSAIC", PAL_DIM);
    if (phase_ == Phase::Victory) {
        hudC(1, "PICTURE COMPLETE", PAL_GREEN);
        hudC(2, "THE MOSAIC IS WHOLE", PAL_GOLD);
    } else {
        hudC(1, art_.name[picture_], PAL_CREAM);
        if (phase_ == Phase::Study || phase_ == Phase::Pop) hudC(2, "STUDY THE PICTURE", PAL_GOLD);
        else if (phase_ == Phase::Scramble) hudC(2, "SHUFFLING", PAL_GOLD);
        else if (phase_ == Phase::Play) hudC(2, "SLIDE THE TILES", PAL_GOLD);
        else if (phase_ == Phase::Banner) hudC(2, "COMPLETE", PAL_GREEN);
    }
    if (paused_) {
        hudC(12, "PAUSED", PAL_GOLD);
        hudC(14, "START RESUMES", PAL_CREAM);
    }

    int shown = phase_ == Phase::Victory ? moves_ : picMoves_;
    std::snprintf(buf, sizeof buf, "MOVES %d", shown);
    hud(1, 26, buf, PAL_CREAM);
    if (phase_ != Phase::Victory) {
        std::snprintf(buf, sizeof buf, "PAR %d", (int)path_.size());
        hud(14, 26, buf, PAL_DIM);
        std::snprintf(buf, sizeof buf, "%d/%d", picture_ + 1, PICTURES);
        hud(37, 26, buf, PAL_GOLD);
    }
    if (phase_ == Phase::Victory) {
        if ((sys_->frame / 30) % 2 == 0) hudC(27, "PRESS START", PAL_GOLD);
    } else if (paused_) {
        hudC(27, "START RESUMES", PAL_DIM);
    } else if (phase_ == Phase::Study || phase_ == Phase::Pop || phase_ == Phase::Scramble) {
        hudC(27, "START SKIPS", PAL_DIM);
    } else {
        hudC(27, "ARROWS MOVE THE GAP    X UNDOES", PAL_DIM);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    if (phase_ == Phase::Title) drawTitle();
    else drawBoard();
}

}  // namespace mosaic
