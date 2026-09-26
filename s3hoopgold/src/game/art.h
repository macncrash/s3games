// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace hoopgold {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_BALL = 3,
    PAL_YOU = 4,
    PAL_BOARD = 5,
    PAL_IRON = 6,
    PAL_NET = 7,
    PAL_GOOD = 8,
    PAL_BAD = 9,
    PAL_COURT = 10,
    PAL_METER = 11,
    PAL_LAMP = 12,
    PAL_FENCE = 13,
    PAL_WORD = 14,
    PAL_SHADOW = 15
};

struct Art {
    int font[96] = {};
    gs::Mipped ball[2];
    gs::Mipped player[2];
    gs::Mipped rim, net[2], board, pole, chip, fence, sun;
    gs::Image shadow, blot, bracket;
    gs::Image hoop, doubled, noDouble, swish, count, bank;
    gs::Image rimWord, shortWord, longWord, airWord, notWord, x2, x1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hoopgold
