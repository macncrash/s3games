// S3 FOUNDRY MAGA pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace foundrymaga {

enum Pal {
    PAL_HUD = 0,
    PAL_BRICK = 1,
    PAL_IRON = 2,
    PAL_EMBER = 3,
    PAL_BRASS = 4,
    PAL_FOE = 5,
    PAL_PEEL = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_OK = 9,
    PAL_SOOT = 10,
    PAL_SLAG = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped furnace, chimney, crucible, flame[2], apron[2], peel[2], down, brass, chev, flash, lip, ingot;
    gs::Image glyph[96];
    int gw[96] = {};
    int gh = 8;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace foundrymaga
