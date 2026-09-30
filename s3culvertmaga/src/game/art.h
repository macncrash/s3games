// S3 CULVERT MAGA pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cmaga {

enum Pal {
    PAL_HUD = 0,
    PAL_PIPE = 1,
    PAL_MOSS = 2,
    PAL_WATER = 3,
    PAL_LAMP = 4,
    PAL_SHADE = 5,
    PAL_BRASS = 6,
    PAL_ALERT = 7,
    PAL_OK = 8
};

struct Art {
    gs::Mipped ring, rib, drip, water, grate;
    gs::Mipped lamp[2], shade[2], down, brass, muzzle;
    gs::Image glyph[96];
    int gw[96] = {};
    int gh = 8;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cmaga
