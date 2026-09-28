// S3 OVEN GOLD sprites. Painted at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ovengold {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_BRICK = 3,
    PAL_FIRE = 4,
    PAL_LOAF = 5,
    PAL_ALERT = 6,
    PAL_OK = 7,
    PAL_WOOD = 8,
    PAL_ASH = 9
};

struct Art {
    int font[96] = {};
    gs::Mipped loaf;
    gs::Mipped arch;
    gs::Mipped flame;
    gs::Mipped peel;
    gs::Image solid;
    gs::Image word;
    gs::Image doubled;
    gs::Image noDouble;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ovengold
