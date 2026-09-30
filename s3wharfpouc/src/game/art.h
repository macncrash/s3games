// S3 WHARF POUC sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace wharf {

enum Pal {
    PAL_HUD = 0,
    PAL_SKY = 1,
    PAL_PLANK = 2,
    PAL_POUCH = 3,
    PAL_PLAYER = 4,
    PAL_IRON = 5,
    PAL_WOOD = 6,
    PAL_LAMP = 7,
    PAL_ALERT = 8,
    PAL_WATER = 9,
    PAL_SHED = 10,
    PAL_GO = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped stand, runA, runB, duck, leap;
    gs::Mipped pouch[2];
    gs::Mipped barrel, hook, mast, shed, piling, gull[2];
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wharf
