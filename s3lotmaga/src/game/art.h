// S3 LOT MAGA pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lot {

enum Pal {
    PAL_HUD = 0,
    PAL_LOT = 1,
    PAL_CAR = 2,
    PAL_STORE = 3,
    PAL_BRASS = 4,
    PAL_COMMIT = 5,
    PAL_PEEL = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_OK = 9
};

struct Art {
    gs::Mipped stall, car, lamp, door, clerk, commit[2], peel[2], down, brass, chev, flash, bag;
    gs::Image glyph[96];
    int gw[96] = {};
    int gh = 8;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lot
