// S3 ORCHARD LADD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace orchardladd {

enum Pal {
    PAL_HUD = 0,
    PAL_ORCH = 1,
    PAL_PICK = 2,
    PAL_WOOD = 3,
    PAL_APPLE = 4,
    PAL_WASP = 5,
    PAL_LEAF = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_OK = 9,
    PAL_SKY = 10,
    PAL_GOLD = 11,
    PAL_DIM = 12,
    PAL_BARN = 13,
    PAL_DITCH = 14
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, barrel, wasp, apple, tree, hive, crate;
    gs::Mipped glyph[96];
    int font[96] = {};
    int grass = 1, soil = 1, soilB = 1, leaf = 1, leafB = 1;
    int board = 1, boardB = 1, crateT = 1, pit = 1;
    int sky = 1, blossom = 1, barn = 1, barnB = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace orchardladd
