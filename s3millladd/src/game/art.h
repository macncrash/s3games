// S3 MILL LADD pictures. Drawn into the S3-16 at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace millladd {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_HERO = 2,
    PAL_IRON = 3,
    PAL_GRAIN = 4,
    PAL_LAMP = 5,
    PAL_OK = 6,
    PAL_ALERT = 7,
    PAL_WATER = 8,
    PAL_STONE = 9
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, plank, hopper, sack, wheel, hatch, stone, lamp;
    gs::Image glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace millladd
