// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace cueseven {

enum Pal {
    PAL_INK = 0,
    PAL_FELT = 1,
    PAL_RAIL = 2,
    PAL_POCKET = 3,
    PAL_CUE = 4,
    PAL_WHITE = 5,
    PAL_OBJECT = 6,
    PAL_WORD = 7,
    PAL_GOOD = 8,
    PAL_BAD = 9,
    PAL_METER = 10,
    PAL_SHADOW = 11,
    PAL_YOU = 12
};

struct Art {
    int font[96] = {};
    gs::Mipped ball;
    gs::Mipped cue;
    gs::Image blot;
    gs::Image shadow;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cueseven
