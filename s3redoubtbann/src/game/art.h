// S3 REDOUBT BANN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace redoubtbann {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_HERO = 4,
    PAL_FOE = 5,
    PAL_BANNER = 6,
    PAL_EARTH = 7,
    PAL_WOOD = 8,
    PAL_BURST = 9
};

struct Art {
    gs::Mipped stand, hop, foe, banner, staff, bag, works, puff;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace redoubtbann
