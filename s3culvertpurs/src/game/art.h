// S3 CULVERT PURSE sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cpurs {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_PUMP = 2,
    PAL_ROLL = 3,
    PAL_DRAIN = 4,
    PAL_STONE = 5,
    PAL_SPARK = 6,
    PAL_WATER = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped you;
    gs::Mipped pump;
    gs::Mipped roller;
    gs::Mipped drainer;
    gs::Mipped rib;
    gs::Mipped grate;
    gs::Mipped puff;
    gs::Mipped spark;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cpurs
