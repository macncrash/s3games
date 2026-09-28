// S3 TRAM BOX sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace trambox {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_CAB = 4,
    PAL_CITY = 5,
    PAL_POST = 6,
    PAL_LEVER = 7,
    PAL_SIGN = 8,
    PAL_ROAD = 12,
    PAL_BAY = 13
};

struct Art {
    gs::Mipped dash, lever[4], pole, block, wire, shelter, board;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace trambox
