// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace paradeseven {

enum Pal {
    PAL_INK = 0,
    PAL_YOU = 1,
    PAL_THEM = 2,
    PAL_RED = 3,
    PAL_BLUE = 4,
    PAL_GOLD = 5,
    PAL_FLAG = 6,
    PAL_PAPER = 7
};

struct Art {
    int font[96] = {};
    gs::Mipped you, them, float_, flag, curb, confetti, shadow, lamp;
    gs::Image logo, done, miss;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace paradeseven
