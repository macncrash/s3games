// S3 REDOUBT MAGA pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rmaga {

enum Pal {
    PAL_HUD = 0,
    PAL_EARTH = 1,
    PAL_GAB = 2,
    PAL_FLAG = 3,
    PAL_STORM = 4,
    PAL_SCOUT = 5,
    PAL_YOU = 6,
    PAL_BRASS = 7,
    PAL_FX = 8,
    PAL_ALERT = 9,
    PAL_OK = 10
};

struct Art {
    gs::Mipped gabion, flag, chest, you[2], storm[2], scout[2], down, brass, puff, chev;
    gs::Image glyph[96];
    int gw[96] = {};
    int gh = 8;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rmaga
