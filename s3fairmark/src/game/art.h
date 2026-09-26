// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace fairmark {

enum Pal {
    PAL_INK = 0,
    PAL_BRASS = 1,
    PAL_GOOD = 2,
    PAL_BAD = 3,
    PAL_WOOD = 4,
    PAL_RED = 5,
    PAL_GOLD = 6,
    PAL_BLUE = 7,
    PAL_RING = 8,
    PAL_HAND = 9,
    PAL_BULB = 10,
    PAL_AWN = 11,
    PAL_NIGHT = 12,
    PAL_PAPER = 13,
    PAL_KID = 14
};

struct Art {
    int font[96] = {};
    gs::Mipped bottle, ring, coin, hand, star, bulb, pennant, dot;
    gs::Mipped post, shelf, awning, cloth, wheel, moon, kid, shadow;
    gs::Image logo, finished, open, sign;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fairmark
