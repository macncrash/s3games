// S3 CAB BOX sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cabbox {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_CAB = 4,
    PAL_CITY = 5,
    PAL_POST = 6,
    PAL_WHEEL = 7,
    PAL_ROAD = 12,
    PAL_BAY = 13
};

struct Art {
    gs::Mipped dash, wheel[4], post, block, lamp, fare;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cabbox
