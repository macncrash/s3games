// S3 VIADUCT LADD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace viaductladd {

enum Pal {
    PAL_HUD = 0,
    PAL_BRICK = 1,
    PAL_DUSK = 2,
    PAL_COAT = 3,
    PAL_BRASS = 4,
    PAL_GOLD = 5,
    PAL_LAMP = 6,
    PAL_CROW = 7,
    PAL_ALERT = 8,
    PAL_OK = 9,
    PAL_ARCH = 10,
    PAL_GORGE = 11,
    PAL_DIM = 12
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, crow, lamp, signal, pier;
    gs::Mipped glyph[96];
    int font[96] = {};
    int sky = 1, cloud = 1, brick = 1, brickB = 1, cap = 1, joint = 1;
    int voussoir = 1, gorge = 1, gorgeB = 1, rail = 1, arch = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace viaductladd
