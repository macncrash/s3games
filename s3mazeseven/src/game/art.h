#pragma once

#include "console/vdp.h"

namespace mazeseven {

// One hedge on the 320x224 screen. Side margins hold the seven pips.
constexpr int MW = 11;
constexpr int MH = 9;
constexpr int CELL = 16;
constexpr int OX = (gs::SCREEN_W - MW * CELL) / 2;
constexpr int OY = 32;

static_assert(OX % 8 == 0 && OY % 8 == 0, "maze origin sits on a tile");
static_assert((MW * CELL) % 8 == 0 && (MH * CELL) % 8 == 0, "maze covers whole tiles");
static_assert(OX == 72 && OY == 32, "margins for the pips and the top line");
static_assert(OY + MH * CELL == 176, "room under the hedge for the line");

enum Pal {
    PAL_MAZE = 0,
    PAL_YOU = 1,
    PAL_THEM = 2,
    PAL_LAMP = 3,
    PAL_SHADE = 4,
    PAL_MOON = 5,
    PAL_STAR = 6,
    PAL_TITLE = 7,
    PAL_INK = 8,
    PAL_GOLD = 9,
    PAL_RED = 10,
    PAL_GREEN = 11,
    PAL_DIM = 12
};

struct Art {
    gs::Image body[3][2];
    gs::Image shadow;
    gs::Image lamp[3];
    gs::Image pip;
    gs::Image moon;
    gs::Image star;
    gs::Image poster;
    gs::Image wordTitle;
    gs::Image wordFirst;
    gs::Image wordOne;
    gs::Image wordSix;
    gs::Image wordMove;
    gs::Image wordTake;
    gs::Image wordEnter;
    gs::Image wordYou;
    gs::Image wordThem;
    gs::Image sayPiece;
    gs::Image sayShort;
    gs::Image sayWin;
    gs::Image sayLose;
    int fontBase = 1;

    void bake(gs::VDP& vdp, const uint8_t* hedge, const uint8_t* lamps, int yx, int yy, int tx, int ty);
};

}  // namespace mazeseven
