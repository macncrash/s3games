// S3 BUNKER LADD pictures. Drawn into the S3-16 at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bunkerladd {

enum Pal {
    PAL_HUD = 0,
    PAL_CONC = 1,
    PAL_HERO = 2,
    PAL_IRON = 3,
    PAL_RUST = 4,
    PAL_LAMP = 5,
    PAL_OK = 6,
    PAL_ALERT = 7,
    PAL_SAND = 8,
    PAL_PIT = 9
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, slab, trolley, lamp, hatch, bag, slit;
    gs::Image glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bunkerladd
