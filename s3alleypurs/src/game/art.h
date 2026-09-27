// S3 ALLEY PURSE sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace apurs {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_CART = 2,
    PAL_FLOAT = 3,
    PAL_VAN = 4,
    PAL_DOOR = 5,
    PAL_BRICK = 6,
    PAL_FX = 7,
    PAL_LAMP = 8,
    PAL_SHOCK = 9,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped barrow;
    gs::Mipped cart, milk, van;
    gs::Mipped door, brick, lamp, bin, pipe;
    gs::Mipped chev, puff, spark, shadow, drip;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace apurs
