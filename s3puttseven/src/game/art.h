// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace puttseven {

enum Pal {
    PAL_INK = 0,
    PAL_YOU = 1,
    PAL_THEM = 2,
    PAL_BALL = 3,
    PAL_CUP = 4,
    PAL_FLAG = 5,
    PAL_HEDGE = 6,
    PAL_WOOD = 7,
    PAL_SAND = 8,
    PAL_TREE = 9,
    PAL_SUN = 10,
    PAL_AIM = 11,
    PAL_LOGO = 12,
    PAL_ALARM = 13
};

struct Art {
    int font[96] = {};
    gs::Mipped ball, shadow, cup, hedge, wood, sand, tuft, tree, sun, dot, peg, golfer;
    gs::Mipped flag[2];
    gs::Mipped logo, win, lose;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace puttseven
