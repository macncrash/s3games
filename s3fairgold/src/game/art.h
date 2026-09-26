// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace fairgold {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_GOOD = 3,
    PAL_BAD = 4,
    PAL_WOOD = 5,
    PAL_RED = 6,
    PAL_BLUE = 7,
    PAL_RING = 8,
    PAL_KID = 9,
    PAL_BULB = 10,
    PAL_AWN = 11,
    PAL_NIGHT = 12,
    PAL_PAPER = 13,
    PAL_GATE = 14,
    PAL_LOGO = 15
};

struct Art {
    int font[96] = {};
    gs::Mipped bottle, ring, kid[2], pennant, bulb, post, shelf, awning, cloth;
    gs::Mipped wheel, moon, star, shade, balloon, gondola, bar, dot;
    gs::Image logo, doubled, stay, sign, one, two;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fairgold
