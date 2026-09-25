// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace mine {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_GOOD = 2,
    PAL_BAD = 3,
    PAL_WOOD = 4,
    PAL_ORE = 5,
    PAL_CART = 6,
    PAL_BOLT = 7,
    PAL_RIB = 8,
    PAL_CEIL = 9,
    PAL_EXIT = 10,
    PAL_DUST = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    int rock[4] = {};
    int beam = 0;
    gs::Mipped cart[2];
    gs::Mipped prop;
    gs::Mipped ore;
    gs::Mipped rib;
    gs::Mipped cap;
    gs::Mipped bolt;
    gs::Mipped spark;
    gs::Mipped reticle;
    gs::Mipped shadow;
    gs::Mipped dust;
    gs::Mipped exit;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mine
