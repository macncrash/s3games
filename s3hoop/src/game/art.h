// S3 HOOP pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace hoop {

enum Pal {
    PAL_PARK = 0,
    PAL_GLASS = 1,
    PAL_IRON = 2,
    PAL_BALL = 3,
    PAL_YOU = 4,
    PAL_LANE = 5,
    PAL_INK = 6,
    PAL_GOLD = 7,
    PAL_RED = 8,
    PAL_GREEN = 9,
    PAL_JUDGE = 10,
    PAL_METER = 11,
    PAL_LAMP = 12,
    PAL_DOT = 13
};

struct Art {
    gs::Image board;
    gs::Image pole;
    gs::Image rim;
    gs::Image net[2];
    gs::Image ball[2];
    gs::Image shooter;
    gs::Image glass;
    gs::Image pip;
    gs::Image bracket;
    gs::Image blot[4];
    gs::Image shadow;
    gs::Image trees;
    gs::Image lamp;
    gs::Image moon;
    gs::Image title;
    gs::Image swish;
    gs::Image count;
    gs::Image bank;
    gs::Image rimWord;
    gs::Image shortWord;
    gs::Image longWord;
    gs::Image airWord;
    gs::Image youWin;
    gs::Image laneWin;
    gs::Image pause;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hoop
