// S3 PLOW GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace plowgrass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_PLOW = 1,
    PAL_GRASS = 2,
    PAL_CREW = 3,
    PAL_POST = 4,
    PAL_DUST = 5,
    PAL_DIRT = 6
};

struct Art {
    gs::Mipped plow[8];
    gs::Mipped sod;
    gs::Mipped tuft;
    gs::Mipped post;
    gs::Mipped crew;
    gs::Mipped dust;
    int font[96] = {};
    int dirtTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace plowgrass
