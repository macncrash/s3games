// S3 CISTERN MAGA pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cmaga {

enum Pal {
    PAL_HUD = 0,
    PAL_LIME = 1,
    PAL_WATER = 2,
    PAL_VAULT = 3,
    PAL_RAIDER = 4,
    PAL_WADER = 5,
    PAL_BRASS = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_OK = 9
};

struct Art {
    gs::Mipped block, lip, water, stair, lamp, raider[2], wader[2], sunk, brass, notch, splash, drip;
    gs::Image glyph[96];
    int gw[96] = {};
    int gh = 8;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cmaga
