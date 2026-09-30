// S3 FOUNDRY LADD sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace foundryladd {

enum Pal {
    PAL_HUD = 0,
    PAL_BRICK = 1,
    PAL_HAND = 2,
    PAL_IRON = 3,
    PAL_HEAT = 4,
    PAL_LADLE = 5,
    PAL_SLAG = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_OK = 9,
    PAL_SMOKE = 10,
    PAL_GOLD = 11,
    PAL_DIM = 12
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, ladle, furnace, stack, ingot, spark, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
    int brick = 1, grate = 1, ember = 1, soot = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace foundryladd
