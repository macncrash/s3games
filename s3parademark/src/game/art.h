// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace parademark {

enum Pal {
    PAL_INK = 0,
    PAL_ME = 1,
    PAL_RED = 2,
    PAL_BLUE = 3,
    PAL_GOLD = 4,
    PAL_FLAG = 5,
    PAL_PAPER = 6,
    PAL_CROWD = 7
};

struct Art {
    int font[96] = {};
    gs::Mipped me, float_, flag, mark, confetti, shadow, lamp;
    gs::Image logo, done, miss;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace parademark
