// S3 TRAP sprites, drawn at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace trap {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_JET = 4,
    PAL_SEA = 5,
    PAL_CITY = 6,
    PAL_FX = 7,
    PAL_DECK = 12,
    PAL_SIDE = 13
};

struct Art {
    gs::Mipped jet[3];
    gs::Mipped flame;
    gs::Mipped island, wire, mast;
    gs::Mipped building, wall, boxcar, derrick, bridge, ice, spire, pole;
    gs::Mipped chev, donut, tri;
    gs::Mipped ball;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace trap
