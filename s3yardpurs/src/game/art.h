// S3 YARD PURSE sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ypurs {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_MULE = 2,
    PAL_WELD = 3,
    PAL_CRUSH = 4,
    PAL_BALE = 5,
    PAL_PURSE = 6,
    PAL_PROP = 7,
    PAL_FX = 8,
    PAL_LAMP = 9,
    PAL_SHOCK = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped loader;
    gs::Mipped mule, welder, crusher, baler;
    gs::Mipped purse;
    gs::Mipped pile, drum, shack, fence, crane, post, bulb, lamp;
    gs::Mipped chev, puff, spark, shadow, moon, star;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ypurs
