// S3 ALLEY COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace alley {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_CAR = 4,
    PAL_VAN = 5,
    PAL_TRUCK = 6,
    PAL_BRICK = 7,
    PAL_CRATE = 8,
    PAL_TREE = 9,
    PAL_FX = 10,
    PAL_RED = 11,
    PAL_ROAD = 12,
    PAL_LAMP = 13,
    PAL_FLAG = 14
};

struct Art {
    gs::Mipped car, van, truck;
    gs::Mipped brick, crate, barrow, lamp;
    gs::Mipped tree, dust, shadow, cloud;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace alley
