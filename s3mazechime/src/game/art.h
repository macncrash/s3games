#pragma once

#include "console/vdp.h"

namespace mazechime {

// Hedge court. The gate is one open cell. It stays shut until the hour.
constexpr int MW = 13;
constexpr int MH = 9;
constexpr int CELL = 16;
constexpr int OX = (gs::SCREEN_W - MW * CELL) / 2;
constexpr int OY = 40;

static_assert(OX % 8 == 0 && OY % 8 == 0, "maze origin sits on a tile");
static_assert((MW * CELL) % 8 == 0 && (MH * CELL) % 8 == 0, "maze covers whole tiles");

enum Pal {
    PAL_MAZE = 0,
    PAL_YOU = 1,
    PAL_GATE = 2,
    PAL_SHADE = 3,
    PAL_CLOCK = 4,
    PAL_TITLE = 5,
    PAL_INK = 6,
    PAL_GOLD = 7,
    PAL_RED = 8,
    PAL_DIM = 9
};

struct Art {
    gs::Image body[3][2];
    gs::Image shadow;
    gs::Image gate;
    gs::Image clock;
    gs::Image poster;
    gs::Image wordTitle;
    gs::Image wordRule;
    gs::Image wordMove;
    gs::Image wordGo;
    int fontBase = 1;

    void bake(gs::VDP& vdp, const uint8_t* hedge, int ex, int ey);
};

}  // namespace mazechime
