// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace puttgold {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_GREEN = 3,
    PAL_RED = 4,
    PAL_BALL = 5,
    PAL_WOOD = 6,
    PAL_HEDGE = 7,
    PAL_TREE = 8,
    PAL_SUN = 9,
    PAL_AIM = 10,
    PAL_LOGO = 11
};

struct Art {
    int font[96] = {};
    gs::Mipped ball, shadow, cup, hedge, wood, tuft, tree, sun, dot;
    gs::Mipped flag[2];
    gs::Mipped logo, doubled, fell, num2, num1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace puttgold
