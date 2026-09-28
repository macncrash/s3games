// S3 SCULL sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scull {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SHELL = 1,
    PAL_RIVAL = 2,
    PAL_BUOY = 3,
    PAL_DOCK = 4,
    PAL_SHORE = 5,
    PAL_FOAM = 6,
    PAL_MARK = 7,
    PAL_WATER = 12
};

struct Art {
    gs::Mipped shell[16];  // 8 headings × 2 oar phases, bow is north at heading 0
    gs::Mipped buoy[2];
    gs::Mipped dock;
    gs::Mipped reed;
    gs::Mipped foam;
    gs::Mipped chevron;
    int font[96] = {};
    int waterTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scull
