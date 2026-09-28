// S3 LOTPACE pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace lot {

enum Pal {
    PAL_LOT = 0,
    PAL_MAN = 1,
    PAL_INK = 2,
    PAL_GOLD = 3,
    PAL_RED = 4,
    PAL_GREEN = 5,
    PAL_FLASH = 6,
    PAL_DUST = 7
};

struct Art {
    gs::Image lot;
    gs::Image fence;
    gs::Image you;
    gs::Image rival[2];
    gs::Image mark;
    gs::Image dust;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lot
