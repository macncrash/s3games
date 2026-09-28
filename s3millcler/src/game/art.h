// S3 MILL CLER sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mill {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_FIGURE = 5,
    PAL_SACK = 6,
    PAL_REED = 7,
    PAL_FX = 8,
    PAL_SKY = 9,
    PAL_WOOD = 10,
    PAL_MILL = 11,
    PAL_ROAD = 12,
    PAL_CLOCK = 13,
    PAL_WATER = 14
};

struct Art {
    gs::Mipped hand[2];
    gs::Mipped rake;
    gs::Mipped mill;
    gs::Mipped sail[2];
    gs::Mipped door;
    gs::Mipped sack;
    gs::Mipped stone;
    gs::Mipped reed;
    gs::Mipped chaff;
    gs::Mipped clock;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mill
