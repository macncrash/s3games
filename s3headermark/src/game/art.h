// S3 HEADERMARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace headermark {

enum Pal {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_MARK = 2,
    PAL_BUOY = 3,
    PAL_WAKE = 4,
    PAL_WATER = 5
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped mark;
    gs::Mipped buoy;
    gs::Mipped wake;
    int font[96] = {};
    int waterTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace headermark
