// S3 MILL RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mill {

enum Pal {
    PAL_HUD = 0,
    PAL_MILL = 1,
    PAL_SAIL = 2,
    PAL_YOU = 3,
    PAL_FOE = 4,
    PAL_WAIN = 5,
    PAL_BELL = 6,
    PAL_FX = 7,
    PAL_OK = 8,
    PAL_ALERT = 9,
    PAL_WATER = 10
};

struct Art {
    gs::Mipped mill, sailPlus, sailCross, cap;
    gs::Mipped miller, sack;
    gs::Mipped reaper[2], runner[2], wain[2];
    gs::Mipped bell, rope, sluice, sweep;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mill
