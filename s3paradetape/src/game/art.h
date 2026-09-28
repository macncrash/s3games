// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace paradetape {

enum Pal {
    PAL_INK = 0,
    PAL_YOU = 1,
    PAL_CROWD = 2,
    PAL_RED = 3,
    PAL_GOLD = 4,
    PAL_BLUE = 5,
    PAL_PAPER = 6,
    PAL_CURB = 7
};

struct Art {
    int font[96] = {};
    gs::Mipped you, float_, flag, curb, drum, shadow, confetti;
    gs::Image logo, done, open;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace paradetape
