// S3 WICKET pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace wicket {

enum Pal {
    PAL_CLOUD = 0,
    PAL_PITCH = 1,
    PAL_TREE = 2,
    PAL_BAT = 3,
    PAL_BOWL = 4,
    PAL_BALL = 5,
    PAL_STUMP = 6,
    PAL_ROPE = 7,
    PAL_CROWD = 8,
    PAL_SHADE = 9,
    PAL_INK = 10,
    PAL_GOLD = 11,
    PAL_GREEN = 12,
    PAL_RED = 13,
    PAL_TITLE = 14,
    PAL_HOUSE = 15
};

struct Art {
    gs::Image batsman[4];
    gs::Image bowler[3];
    gs::Image ball[2];
    gs::Image stump[2];
    gs::Image bail;
    gs::Image pitch;
    gs::Image rope;
    gs::Image tree;
    gs::Image crowd;
    gs::Image house;
    gs::Image screen;
    gs::Image cloud;
    gs::Image sun;
    gs::Image shadow;
    gs::Image blot;
    gs::Image title;
    gs::Image sixBalls;
    gs::Image hitWicket;
    gs::Image clearRope;
    gs::Image bowledWord;
    gs::Image ours;
    gs::Image gone;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wicket
