// S3 MILL COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace millc {

enum Pal {
    PAL_TEXT = 0,
    PAL_WHEAT = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_LORRY = 4,
    PAL_CART = 5,
    PAL_MILL = 6,
    PAL_SAIL = 7,
    PAL_TREE = 8,
    PAL_DUST = 9,
    PAL_TIMBER = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped lorry, cart, mill, sailPlus, sailCross, cap;
    gs::Mipped timber, stripe, tree, dust;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace millc
