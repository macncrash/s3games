// S3 RAIL TURN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace railturn {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_CAR = 4,
    PAL_STEEL = 5,
    PAL_CITY = 6,
    PAL_ROAD = 12
};

struct Art {
    gs::Image car[5];
    gs::Image shadow;
    gs::Image pylon;
    gs::Image block[3];
    gs::Image glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace railturn
