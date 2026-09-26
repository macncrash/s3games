// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace fairchime {

enum Pal {
    PAL_INK = 0,
    PAL_WORD = 1,
    PAL_ALERT = 2,
    PAL_GREEN = 3,
    PAL_BRASS = 4,
    PAL_CREAM = 5,
    PAL_KID = 6,
    PAL_WOOD = 7,
    PAL_AWN = 8,
    PAL_RED = 9,
    PAL_BLUE = 10,
    PAL_CLOCK = 11,
    PAL_HAND = 12,
    PAL_PINK = 13,
    PAL_BULB = 14,
    PAL_NIGHT = 15
};

struct Art {
    int font[96] = {};
    gs::Image title, twelve, face, halo, cap, pip, chevron;
    gs::Image hand[3][60];
    gs::Mipped kid[2];
    gs::Mipped bottle, ring, bell, awning, board, counter, gate;
    gs::Mipped balloon, bulb, pennant, moon, star, burst, shadow;
    gs::Mipped hub, car, stand;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fairchime
