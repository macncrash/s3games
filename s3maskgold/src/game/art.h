// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace maskgold {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_CLAY = 3,
    PAL_WOOD = 4,
    PAL_BLADE = 5,
    PAL_RIBBON = 6,
    PAL_FACE = 7,
    PAL_TITLE = 8,
    PAL_BAD = 9,
    PAL_DIM = 10,
    PAL_LEAF = 11,
    PAL_POT = 12,
    PAL_BENCH = 13,
    PAL_SHADE = 14,
    PAL_WALL = 15
};

struct Art {
    int font[96] = {};
    gs::Image mask;
    gs::Image blade;
    gs::Image stroke;
    gs::Image leaf;
    gs::Image pot;
    gs::Image ribbon;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace maskgold
