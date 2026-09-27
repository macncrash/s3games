// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace golfgold {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_GRASS = 3,
    PAL_BALL = 4,
    PAL_MAN = 5,
    PAL_SKY = 6,
    PAL_TREE = 7,
    PAL_CUP = 8,
    PAL_WORD = 9,
    PAL_GOOD = 10,
    PAL_BAD = 11,
    PAL_METER = 12,
    PAL_FLAG = 13,
    PAL_SAND = 14,
    PAL_SHADOW = 15
};

struct Art {
    int font[96] = {};
    gs::Mipped ball;
    gs::Mipped golfer[2];
    gs::Mipped flag;
    gs::Mipped tree;
    gs::Mipped cup;
    gs::Image blot;
    gs::Image shadow;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace golfgold
