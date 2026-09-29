// S3 CAUSEWAY LADD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace causewayladd {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_SEA = 2,
    PAL_COAT = 3,
    PAL_RUST = 4,
    PAL_GOLD = 5,
    PAL_LAMP = 6,
    PAL_GULL = 7,
    PAL_ALERT = 8,
    PAL_OK = 9,
    PAL_FAR = 10,
    PAL_FOAM = 11,
    PAL_DIM = 12
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, gull, lamp, beacon, post;
    gs::Mipped glyph[96];
    int font[96] = {};
    int sky = 1, haze = 1, stone = 1, stoneB = 1, cap = 1, joint = 1;
    int pile = 1, water = 1, waterB = 1, foam = 1, rail = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace causewayladd
