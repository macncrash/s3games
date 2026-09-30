// S3 WHARF LADD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace wharfladd {

enum Pal {
    PAL_HUD = 0,
    PAL_WATER = 1,
    PAL_TIMBER = 2,
    PAL_COAT = 3,
    PAL_IRON = 4,
    PAL_LAMP = 5,
    PAL_CRATE = 6,
    PAL_GULL = 7,
    PAL_ALERT = 8,
    PAL_OK = 9,
    PAL_GOLD = 10,
    PAL_DIM = 11,
    PAL_ROPE = 12
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, dolly, gull, lamp, coil, piling;
    gs::Mipped glyph[96];
    int font[96] = {};
    int plank = 1, plankB = 1, beam = 1, nail = 1;
    int water = 1, waterB = 1, sky = 1, cloud = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wharfladd
