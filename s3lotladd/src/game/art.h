// S3 LOT LADD sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lotladd {

enum Pal {
    PAL_HUD = 0,
    PAL_LOT = 1,
    PAL_HAND = 2,
    PAL_IRON = 3,
    PAL_SODIUM = 4,
    PAL_CAR = 5,
    PAL_SIGN = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_OK = 9,
    PAL_NIGHT = 10,
    PAL_GOLD = 11,
    PAL_DIM = 12
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, car, lamp, tire, moon, flag, office, shadow, dust;
    gs::Mipped glyph[96];
    int font[96] = {};
    int asphalt = 1, stall = 1, star = 1, brick = 1, win = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lotladd
