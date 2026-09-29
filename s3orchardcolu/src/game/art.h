// Orchard column sprites. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace orchard {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_CAR = 4,
    PAL_VAN = 5,
    PAL_LORRY = 6,
    PAL_TREE = 7,
    PAL_BIN = 8,
    PAL_APPLE = 9,
    PAL_FX = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped car, van, lorry;
    gs::Mipped tree, bin, apple, dust, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace orchard
