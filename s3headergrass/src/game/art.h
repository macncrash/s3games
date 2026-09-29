// S3 HEADER GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace headergrass {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_BOAT = 4,
    PAL_MEADOW = 5,
    PAL_FLAG = 6,
    PAL_WAKE = 7,
    PAL_SHORE = 8
};

struct Art {
    gs::Mipped boat[16];
    gs::Mipped meadow;
    gs::Mipped tuft;
    gs::Mipped flag;
    gs::Mipped wake;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace headergrass
