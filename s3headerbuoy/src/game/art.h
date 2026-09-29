// S3 HEADER BUOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace headerbuoy {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_BOAT = 4,
    PAL_BUOY = 5,
    PAL_DOCK = 6,
    PAL_SAIL = 7,
    PAL_WAKE = 8
};

struct Art {
    gs::Mipped boat[16];
    gs::Mipped buoy;
    gs::Mipped dock;
    gs::Mipped wake;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace headerbuoy
