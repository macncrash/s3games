// S3 WHARFWELL sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace wharf {

enum Pal {
    PAL_TEXT = 0,
    PAL_WOOD = 1,
    PAL_STONE = 2,
    PAL_KEEPER = 3,
    PAL_CRAB = 4,
    PAL_BARREL = 5,
    PAL_FOAM = 6,
    PAL_BRUTE = 7,
    PAL_GOLD = 8,
    PAL_ALERT = 9,
    PAL_GULL = 10
};

struct Art {
    gs::Image deck;
    gs::Image pile;
    gs::Image well;
    gs::Image keeper[3];
    gs::Image crab;
    gs::Image barrel;
    gs::Image brute;
    gs::Image foam;
    gs::Image gull;
    gs::Image glyph[96];
    int advance = 12;
    int gh = 14;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wharf
