// S3 TRENCH MAGA pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tmaga {

enum Pal {
    PAL_HUD = 0,
    PAL_BAG = 1,
    PAL_MUD = 2,
    PAL_BRASS = 3,
    PAL_FOE = 4,
    PAL_PEEL = 5,
    PAL_FX = 6,
    PAL_ALERT = 7,
    PAL_OK = 8,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped bag, plank, post, wire, foe[2], peel[2], down, brass, notch, flash, stab;
    gs::Image glyph[96];
    int gw[96] = {};
    int gh = 8;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tmaga
