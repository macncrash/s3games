// S3 TRENCH COLUMN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace trench {

enum Pal {
    PAL_HUD = 0,
    PAL_BAG = 1,
    PAL_YOU = 2,
    PAL_TRUCK = 3,
    PAL_CABLE = 4,
    PAL_FLARE = 5,
    PAL_GOOD = 6,
    PAL_ALERT = 7,
    PAL_STAKE = 8,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped truck, you, bag, stake, cable, flare, wire;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace trench
