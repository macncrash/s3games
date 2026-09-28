// S3 PLOW LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace plowlock {

enum Pal : int {
    PAL_HUD = 0,
    PAL_PLOW = 1,
    PAL_GATE = 2,
    PAL_BANK = 3,
    PAL_END = 4,
    PAL_SNOW = 5,
    PAL_POST = 6
};

struct Art {
    gs::Mipped plow[8];
    gs::Mipped gate;
    gs::Mipped bank;
    gs::Mipped post;
    gs::Mipped banner;
    int font[96] = {};
    int snowTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace plowlock
