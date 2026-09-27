#pragma once

#include "console/vdp.h"

namespace mazebell {

// A short hedge. The bell hangs in one open cell. Pits are the tries that die.
constexpr int MW = 11;
constexpr int MH = 9;
constexpr int CELL = 16;
constexpr int OX = (gs::SCREEN_W - MW * CELL) / 2;
constexpr int OY = 32;

static_assert(OX % 8 == 0 && OY % 8 == 0, "maze origin sits on a tile");
static_assert((MW * CELL) % 8 == 0 && (MH * CELL) % 8 == 0, "maze covers whole tiles");
static_assert(OX == 72 && OY == 32, "margins for the lines");

enum Pal {
    PAL_MAZE = 0,
    PAL_YOU = 1,
    PAL_BELL = 2,
    PAL_FAKE = 3,
    PAL_SHADE = 4,
    PAL_SKY = 5,
    PAL_TITLE = 6,
    PAL_INK = 7,
    PAL_GOLD = 8,
    PAL_RED = 9,
    PAL_DIM = 10
};

struct Art {
    gs::Image body[3][2];
    gs::Image shadow;
    gs::Image bell;
    gs::Image fake;
    gs::Image poster;
    gs::Image wordTitle;
    gs::Image wordRule;
    gs::Image wordMove;
    gs::Image wordGo;
    int fontBase = 1;

    void bake(gs::VDP& vdp, const uint8_t* hedge, int bx, int by, int fx, int fy, int px, int py);
};

}  // namespace mazebell
