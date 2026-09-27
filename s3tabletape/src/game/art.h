// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace tabletape {

enum Pal {
    PAL_WHITE = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_DIM = 3,
    PAL_MINT = 4,
    PAL_YOU = 5,
    PAL_THEM = 6,
    PAL_PUCK = 7,
    PAL_FELT = 8,
    PAL_TAPE = 9,
    PAL_WOOD = 10
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    gs::Mipped malletYou, malletThem, puck, shadow, bar, slip;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tabletape
