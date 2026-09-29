// S3 REDOUBT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace redoubt {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_LORRY = 4,
    PAL_LEAD = 5,
    PAL_EARTH = 6,
    PAL_GUN = 7,
    PAL_TREE = 8,
    PAL_BURST = 9,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped lorry, lead, earth, gun, tree, burst, reticle;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace redoubt
