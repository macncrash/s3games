// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace pinsgold {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_BALL = 3,
    PAL_BOWLER = 4,
    PAL_AMBER = 5,
    PAL_GREEN = 6,
    PAL_RED = 7,
    PAL_LAMP = 8,
    PAL_DECK = 9,
    PAL_GLOW = 10,
    PAL_LANE = 12
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    gs::Mipped pin, pinFlat;
    gs::Mipped ball[4];
    gs::Mipped bowler[4];
    gs::Mipped arrow, dot, foul, shadow, glow, lamp, curtain, machine;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pinsgold
