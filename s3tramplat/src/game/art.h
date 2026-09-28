// S3 TRAM PLAT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tramplat {

enum Pal {
    PAL_HUD = 0,
    PAL_CREAM = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_TRAM = 4,
    PAL_STOP = 5,
    PAL_WIRE = 6,
    PAL_TOWN = 7,
    PAL_CLOCK = 8
};

struct Art {
    gs::Mipped tram, bogie, pan, stop, stripe, shelter, clock, pole, rival;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tramplat
