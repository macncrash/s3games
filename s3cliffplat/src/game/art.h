// S3 CLIFF PLAT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cliffplat {

enum Pal {
    PAL_HUD = 0,
    PAL_CAGE = 1,
    PAL_RIVAL = 2,
    PAL_ROCK = 3,
    PAL_PLAT = 4,
    PAL_SEA = 5,
    PAL_GOOD = 6,
    PAL_BAD = 7
};

struct Art {
    gs::Mipped cage, rival, rock, plat, stripe, gull;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cliffplat
