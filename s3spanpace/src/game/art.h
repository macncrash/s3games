// S3 SPAN PACE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace spanpace {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STEEL = 4,
    PAL_COAT = 5,
    PAL_HOLD = 6,
    PAL_LIVE = 7,
    PAL_WATER = 8,
    PAL_FX = 9,
    PAL_SKY = 10,
    PAL_CABLE = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped coat[2];
    gs::Mipped fallen;
    gs::Mipped tower;
    gs::Mipped lamp;
    gs::Mipped link;
    gs::Mipped buoy;
    gs::Mipped gull[2];
    gs::Mipped bead;
    gs::Mipped pip;
    gs::Mipped flash;
    gs::Mipped dust;
    gs::Mipped moon;
    gs::Mipped cloud;
    gs::Mipped rail;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace spanpace
