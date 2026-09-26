#pragma once

#include "console/vdp.h"

namespace mazemark {

// One short hedge. Cells are 16px so a walker stands in the path.
constexpr int MW = 15;
constexpr int MH = 9;
constexpr int CELL = 16;
constexpr int OX = (gs::SCREEN_W - MW * CELL) / 2;
constexpr int OY = 32;

static_assert(OX % 8 == 0 && OY % 8 == 0, "maze origin must sit on a tile");
static_assert((MW * CELL) % 8 == 0 && (MH * CELL) % 8 == 0, "maze must cover whole tiles");
static_assert(OX >= 0 && OX + MW * CELL <= gs::SCREEN_W, "maze wider than the screen");
static_assert(OY >= 16 && OY + MH * CELL <= gs::SCREEN_H - 32, "leave room for the lines");

struct Art {
    gs::Image body[3][2];
    gs::Image shadow;
    gs::Image coin;
    gs::Image dot;
    gs::Image tree;
    gs::Image moon;
    gs::Image star;
    gs::Image logo;
    gs::Image finished;
    gs::Image open;
    gs::Image card;
    int font[64] = {};

    void bake(gs::VDP& vdp, const uint8_t* hedge, int ex, int ey, int mx, int my, int sx, int sy);
};

}  // namespace mazemark
