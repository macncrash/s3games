// S3 HARBOR MAGA pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace harbormaga {

enum Pal {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_WAKE = 2,
    PAL_BRASS = 3,
    PAL_PIER = 4,
    PAL_LIGHT = 5,
    PAL_RED = 6,
    PAL_CREAM = 7,
    PAL_WATER = 12
};

struct Art {
    gs::Mipped cutter;
    gs::Mipped wreck;
    gs::Mipped shed;
    gs::Mipped light;
    gs::Mipped buoy;
    gs::Mipped crane;
    gs::Mipped sight;
    gs::Mipped round;
    gs::Mipped spent;
    gs::Mipped splash;
    gs::Image glyph[96];
    int cellW = 6;
    int cellH = 8;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace harbormaga
