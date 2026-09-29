// S3 QUARRY LADD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace quarryladd {

enum Pal {
    PAL_HUD = 0,
    PAL_ROCK = 1,
    PAL_MAN = 2,
    PAL_IRON = 3,
    PAL_DUST = 4,
    PAL_WATER = 5,
    PAL_WOOD = 6,
    PAL_SKIP = 7,
    PAL_ALERT = 8,
    PAL_OK = 9,
    PAL_GOLD = 11,
    PAL_DIM = 12
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, skip, dust, hook, cable, lamp, clock, boulder, ripple;
    gs::Mipped glyph[96];
    int font[96] = {};
    int rock = 1, rockB = 1, strata = 1, lip = 1, grate = 1;
    int water = 1, pit = 1, farWall = 1, farLip = 1, star = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace quarryladd
