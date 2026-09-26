// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace mazegold {

constexpr int MW = 15;
constexpr int MH = 11;
constexpr int CELL = 16;
constexpr int OX = (gs::SCREEN_W - MW * CELL) / 2;
constexpr int OY = 24;
constexpr int kLine = 6;
constexpr int kCoins = 5;

static_assert(OX % 8 == 0 && OY % 8 == 0, "origin on a tile");
static_assert((MW * CELL) % 8 == 0 && (MH * CELL) % 8 == 0, "maze covers whole tiles");
static_assert(MW * CELL <= gs::SCREEN_W, "maze fits the screen");
static_assert(OY + MH * CELL <= gs::SCREEN_H - 16, "room under the hedge for the line");

struct Art {
    gs::Image body[3][2];
    gs::Image shadow;
    gs::Image goldCoin;
    gs::Image creamCoin;
    gs::Image bars;
    gs::Image lamp;
    gs::Image moon;
    gs::Image star;
    gs::Image titlePic;
    gs::Image titleName;
    gs::Image titleRule;
    gs::Image titleLeave;
    gs::Image titleMove;
    int fontBase = 1;

    void bake(gs::VDP& vdp, const uint8_t* hedge, int sx, int sy, int ex, int ey, const int* coinX, const int* coinY,
              int nCoins);
};

}  // namespace mazegold
