// S3 MILL MAGA pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mmaga {

enum Pal {
    PAL_HUD = 0,
    PAL_MILL = 1,
    PAL_SAIL = 2,
    PAL_WHEAT = 3,
    PAL_FOE = 4,
    PAL_PEEL = 5,
    PAL_BRASS = 6,
    PAL_FX = 7,
    PAL_OK = 8,
    PAL_ALERT = 9
};

struct Art {
    gs::Mipped mill, sailA, sailB, wheat, path, door;
    gs::Mipped miller, foe[2], peel[2], down, round, flash, mark;
    gs::Image glyph[96];
    int gw[96] = {};
    int gh = 8;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mmaga
