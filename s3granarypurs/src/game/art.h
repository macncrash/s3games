// Granary yard sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gpurs {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_AUGER = 2,
    PAL_TIP = 3,
    PAL_THRESH = 4,
    PAL_MILL = 5,
    PAL_SILO = 6,
    PAL_FX = 7,
    PAL_SACK = 8,
    PAL_SHOCK = 9,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped miller;
    gs::Mipped auger, tipper, thresher;
    gs::Mipped door, silo, sack, chute, lamp;
    gs::Mipped chev, puff, spark, shadow, grain;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gpurs
