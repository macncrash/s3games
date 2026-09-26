// S3 WICKET GOLD pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace wicketgold {

enum Pal {
    PAL_CLOUD = 0,
    PAL_PITCH = 1,
    PAL_TREE = 2,
    PAL_BAT = 3,
    PAL_BOWL = 4,
    PAL_CREAM = 5,
    PAL_GOLD = 6,
    PAL_STUMP = 7,
    PAL_ROPE = 8,
    PAL_ROPEGOLD = 9,
    PAL_CROWD = 10,
    PAL_HOUSE = 11,
    PAL_INK = 12,
    PAL_GOLDTEXT = 13,
    PAL_GREEN = 14,
    PAL_RED = 15
};

struct Art {
    gs::Image batsman[4];
    gs::Image bowler[3];
    gs::Image umpire[3];
    gs::Image ball[2];
    gs::Image stump[2];
    gs::Image bail;
    gs::Image pitch;
    gs::Image rope;
    gs::Image tree;
    gs::Image crowd;
    gs::Image house;
    gs::Image cloud;
    gs::Image sun;
    gs::Image star;
    gs::Image blot;
    gs::Image plate;
    gs::Image wordWicket;
    gs::Image wordGold;
    gs::Image wordDouble;
    gs::Image wordNot;
    gs::Image wordRope;
    gs::Image wordBowled;
    gs::Image wordCream;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wicketgold
