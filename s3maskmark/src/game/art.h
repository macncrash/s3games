// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace maskmark {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_CLAY = 2,
    PAL_WOOD = 3,
    PAL_TOOL = 4,
    PAL_SEAL = 5,
    PAL_SMEAR = 6,
    PAL_FACE = 7,
    PAL_TITLE = 8,
    PAL_BAD = 9,
    PAL_DIM = 10,
    PAL_LAMP = 11,
    PAL_CUT = 12,
    PAL_BENCH = 13,
    PAL_SHADE = 14,
    PAL_WALL = 15
};

struct Art {
    int font[96] = {};
    gs::Image mask;
    gs::Image chisel;
    gs::Image stroke;
    gs::Image stamp;
    gs::Image lamp;
    gs::Image pot;
    gs::Image hands;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace maskmark
