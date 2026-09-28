// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace paradegold {

enum Pal {
    PAL_INK = 0,
    PAL_ME = 1,
    PAL_WAGON = 2,
    PAL_CREAM = 3,
    PAL_GOLD = 4,
    PAL_BUNT = 5,
    PAL_CROWD = 6,
    PAL_SKY = 7
};

struct Art {
    int font[96] = {};
    gs::Mipped me, wagon, coin, cream, flag, shadow;
    gs::Image logo, done;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace paradegold
