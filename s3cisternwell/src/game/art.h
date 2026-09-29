// S3 CISTERNWELL sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace well {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_KEEPER = 2,
    PAL_RAIDER = 3,
    PAL_WATER = 4,
    PAL_EARTH = 5,
    PAL_FX = 6,
    PAL_BRUTE = 7
};

struct Art {
    gs::Mipped well;
    gs::Mipped mouth;
    gs::Mipped keeper;
    gs::Mipped raider;
    gs::Mipped brute;
    gs::Mipped drop;
    gs::Mipped splash;
    gs::Mipped reed;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace well
