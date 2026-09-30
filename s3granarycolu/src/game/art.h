// Granary column sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace granary {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_CART = 4,
    PAL_BIKE = 5,
    PAL_WAGON = 6,
    PAL_BARN = 7,
    PAL_SACK = 8,
    PAL_WHEAT = 9,
    PAL_FX = 10,
    PAL_SILO = 11,
    PAL_ROAD = 12,
    PAL_CHUTE = 13
};

struct Art {
    gs::Mipped cart, bike, wagon;
    gs::Mipped barn, silo, sack, wheat, chute;
    gs::Mipped dust, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace granary
