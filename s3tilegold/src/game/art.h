// S3 TILE GOLD — floor art drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tilegold {

enum Pal {
    PAL_FLOOR = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_GROUT = 3,
    PAL_INK = 4,
    PAL_TITLE = 5,
    PAL_WIN = 6,
    PAL_BAD = 7,
    PAL_HINT = 8
};

struct Art {
    gs::Image floor;
    gs::Image tile;
    gs::Image chip;
    gs::Image trowel;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tilegold
