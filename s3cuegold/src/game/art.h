// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace cuegold {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_FELT = 3,
    PAL_CUEBALL = 4,
    PAL_CUE = 5,
    PAL_RAIL = 6,
    PAL_POCKET = 7,
    PAL_WORD = 8,
    PAL_GOOD = 9,
    PAL_BAD = 10,
    PAL_METER = 11,
    PAL_SHADOW = 12
};

struct Art {
    int font[96] = {};
    gs::Mipped ball;
    gs::Mipped cue;
    gs::Image blot;
    gs::Image shadow;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cuegold
