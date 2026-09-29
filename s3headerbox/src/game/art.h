// S3 HEADER BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace headerbox {

enum Pal {
    PAL_HUD = 0,
    PAL_BOAT = 1,
    PAL_SAIL = 2,
    PAL_BOX = 3,
    PAL_POST = 4,
    PAL_WATER = 5
};

struct Art {
    gs::Mipped boat[16];
    gs::Mipped box;
    gs::Mipped post;
    gs::Mipped wake;
    int font[96] = {};
    int waterTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace headerbox
