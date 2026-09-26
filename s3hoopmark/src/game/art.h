// S3 HOOPMARK pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace hoopmark {

enum Pal {
    PAL_PARK = 0,
    PAL_GLASS = 1,
    PAL_IRON = 2,
    PAL_BALL = 3,
    PAL_YOU = 4,
    PAL_INK = 5,
    PAL_GOLD = 6,
    PAL_RED = 7,
    PAL_GREEN = 8,
    PAL_JUDGE = 9,
    PAL_METER = 10,
    PAL_LAMP = 11,
    PAL_COIN = 12
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
    gs::Image coin;
    gs::Image ring;
    gs::Image title;
    gs::Image swish;
    gs::Image count;
    gs::Image bank;
    gs::Image rimWord;
    gs::Image shortWord;
    gs::Image longWord;
    gs::Image airWord;
    gs::Image liftWord;
    gs::Image leaveWord;
    gs::Image finished;
    gs::Image stillOpen;
    gs::Image pauseWord;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hoopmark
