#pragma once

#include "console/vdp.h"

namespace maze {

// One screen of hedge. Cells are 16px so the walker stands in the path.
constexpr int COLS = 9;
constexpr int ROWS = 5;
constexpr int MW = COLS * 2 + 1;
constexpr int MH = ROWS * 2 + 1;
constexpr int CELL = 16;
constexpr int OX = (gs::SCREEN_W - MW * CELL) / 2;
constexpr int OY = 24;

static_assert(OX % 8 == 0 && OY % 8 == 0, "maze origin must sit on a tile");
static_assert((MW * CELL) % 8 == 0 && (MH * CELL) % 8 == 0, "maze must cover whole tiles");
static_assert(MW * CELL <= gs::SCREEN_W, "maze wider than the screen");
static_assert(OY >= 16 && OY + MH * CELL <= gs::SCREEN_H - 16, "leave room for the lines");

struct Art {
    gs::Image body[3][2];
    gs::Image shadow;
    gs::Image bird[2];
    gs::Image titlePic;
    gs::Image titleName, titleSub, titleRule, titleMove, titleGo;
    gs::Image sayOnly, sayFind, sayShut, sayOut;
    int fontBase = 1;

    void bake(gs::VDP& vdp, const uint8_t* hedge, int ex, int ey, int sx, int sy, int dx, int dy);
};

}  // namespace maze
