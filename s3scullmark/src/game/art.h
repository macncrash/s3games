// S3 SCULLMARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scull {

enum Pal {
    PAL_HUD = 0,
    PAL_SKY = 1,
    PAL_WOOD = 2,
    PAL_BOAT = 3,
    PAL_BANK = 4,
    PAL_MARK = 5,
    PAL_FOAM = 6
};

struct Art {
    gs::Mipped hull;
    gs::Mipped rower;
    gs::Mipped oar[9];
    gs::Mipped paint;
    gs::Mipped buoy;
    gs::Mipped tree;
    gs::Mipped reed;
    gs::Mipped ripple;
    gs::Mipped setWord;
    gs::Mipped endWord;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scull
