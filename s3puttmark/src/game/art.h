// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace puttmark {

enum Pal {
    PAL_INK = 0,
    PAL_BRASS = 1,
    PAL_GREEN = 2,
    PAL_BAD = 3,
    PAL_BALL = 4,
    PAL_COIN = 5,
    PAL_CUP = 6,
    PAL_FLAG = 7,
    PAL_GRASS = 8,
    PAL_TREE = 9,
    PAL_SUN = 10,
    PAL_HAND = 11,
    PAL_AIM = 12,
    PAL_LOGO = 13,
    PAL_PUTTER = 14
};

struct Art {
    int font[96] = {};
    gs::Mipped ball, shadow, coin, cup;
    gs::Mipped flag[2], flagDown;
    gs::Mipped tuft, tree, sun, putter, glove, chevron, dot;
    gs::Image logo, finished, open;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace puttmark
