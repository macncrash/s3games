// S3 CISTERN LADD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cisternladd {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WADER = 2,
    PAL_IRON = 3,
    PAL_BRASS = 4,
    PAL_MOSS = 5,
    PAL_WOOD = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_OK = 9,
    PAL_VAULT = 10,
    PAL_GOLD = 11,
    PAL_DIM = 12
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, sluice, post, motor, bell;
    gs::Mipped lamp, flame[2], weed, barrel, drip[2];
    gs::Mipped vent, mist, shadow, signal;
    gs::Mipped glyph[96];
    int font[96] = {};
    int cope = 1, block = 1, blockB = 1, pier = 1, pierB = 1;
    int water = 1, waterB = 1, deep = 1, slit = 1, slitLit = 1;
    int dripT = 1, dripB = 1, rib = 1, arch = 1, niche = 1, nicheLit = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cisternladd
