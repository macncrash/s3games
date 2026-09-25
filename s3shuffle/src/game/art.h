// S3 SHUFFLE sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace shuffle {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_YOU = 2,
    PAL_HOUSE = 3,
    PAL_WAX = 4,
    PAL_Z3 = 5,
    PAL_Z2 = 6,
    PAL_Z1 = 7,
    PAL_LINE = 8,
    PAL_RAIL = 9,
    PAL_GUTTER = 10,
    PAL_LOGO = 11,
    PAL_LAMP = 12,
    PAL_AIM = 13,
    PAL_BAD = 14
};

struct Art {
    gs::Mipped puck;
    gs::Mipped shadow;
    gs::Mipped lamp;
    gs::Mipped bottle;
    gs::Mipped glass;
    gs::Mipped chev;
    gs::Mipped dot;
    gs::Mipped digit[3];
    gs::Mipped logo;
    gs::Mipped win;
    int wax = 1;
    int line = 1;
    int hRail = 1;
    int vRail = 1;
    int wood = 1;
    int gutter = 1;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace shuffle
