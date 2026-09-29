// Palisade column sprites. Drawn into the VDP at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pcol {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_WAGON = 4,
    PAL_CART = 5,
    PAL_WOOD = 6,
    PAL_BOOM = 7,
    PAL_TREE = 8,
    PAL_FX = 9,
    PAL_SENTRY = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped wagon, cart, stake, boom, tip, sentry;
    gs::Mipped tree, dust;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pcol
