// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace fairseven {

enum Pal {
    PAL_INK = 0,
    PAL_TITLE = 1,
    PAL_GOOD = 2,
    PAL_BAD = 3,
    PAL_PAPER = 4,
    PAL_CREAM = 5,
    PAL_GOLD = 6,
    PAL_BELL = 7,
    PAL_YOU = 8,
    PAL_THEM = 9,
    PAL_HEADY = 10,
    PAL_HEADT = 11,
    PAL_PIPY = 12,
    PAL_PIPT = 13,
    PAL_PIPD = 14,
    PAL_BOOTH = 15
};

struct Art {
    int font[96] = {};
    gs::Mipped bottle, ring, bell, head, pip, bulb, pennant;
    gs::Mipped shelf, awning, cloth, post, wheel, gondola, moon, shade, dot;
    gs::Image fair, seven, late, sign, one, two, three;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fairseven
