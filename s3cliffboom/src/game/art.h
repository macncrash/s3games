// Sprites and the cliff picture. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cliffboom {

enum Pal {
    PAL_WHITE = 0,
    PAL_CLIFF = 1,
    PAL_CAR = 2,
    PAL_BOOM = 3,
    PAL_DUST = 4,
    PAL_AMBER = 5,
    PAL_RED = 6,
    PAL_GREEN = 7,
    PAL_BANNER = 8
};

struct Art {
    gs::Mipped car[8];
    gs::Mipped boom;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cliffboom
