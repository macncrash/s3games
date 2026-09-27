// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace tablegold {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_YOU = 3,
    PAL_THEM = 4,
    PAL_PUCK = 5,
    PAL_WORD = 6,
    PAL_GOOD = 7,
    PAL_BAD = 8,
    PAL_ICE = 9,
    PAL_SHADOW = 10
};

struct Art {
    int font[96] = {};
    gs::Mipped malletYou;
    gs::Mipped malletThem;
    gs::Mipped puck;
    gs::Image shadow;
    gs::Image blot;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tablegold
