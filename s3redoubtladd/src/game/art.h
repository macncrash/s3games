// S3 REDOUBT LADD pictures. Drawn into the S3-16 at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace redoubtladd {

enum Pal {
    PAL_HUD = 0,
    PAL_EARTH = 1,
    PAL_HERO = 2,
    PAL_TIMBER = 3,
    PAL_GABION = 4,
    PAL_FLAG = 5,
    PAL_OK = 6,
    PAL_ALERT = 7,
    PAL_SOD = 8,
    PAL_WATER = 9
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, sod, gabion, flag, stake, lamp;
    gs::Image glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace redoubtladd
