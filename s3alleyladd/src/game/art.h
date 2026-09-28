// S3 ALLEY LADD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace alleyladd {

enum Pal {
    PAL_HUD = 0,
    PAL_ALLEY = 1,
    PAL_HAND = 2,
    PAL_IRON = 3,
    PAL_NEON = 4,
    PAL_STEAM = 5,
    PAL_WOOD = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_OK = 9,
    PAL_FAR = 10,
    PAL_GOLD = 11,
    PAL_DIM = 12
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, bin, steam, lid, bulb, pipe;
    gs::Mipped glyph[96];
    int font[96] = {};
    int cope = 1, brick = 1, brickB = 1, sill = 1, sillB = 1;
    int asphalt = 1, asphaltB = 1, pit = 1, pane = 1, paneLit = 1;
    int star = 1, starB = 1, wall = 1, cornice = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace alleyladd
