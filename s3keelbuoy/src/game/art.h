// S3 KEELBUOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keelbuoy {

enum Pal : int {
    PAL_HUD = 0,
    PAL_BOAT = 1,
    PAL_MARK0 = 2,
    PAL_MARK1 = 3,
    PAL_MARK2 = 4,
    PAL_DOCK = 5,
    PAL_FOAM = 6,
    PAL_GULL = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
};

struct Art {
    gs::Mipped boat[8];
    gs::Mipped buoy[3];
    gs::Mipped dock;
    gs::Mipped foam;
    gs::Mipped gull[2];
    gs::Mipped title;
    gs::Mipped done;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keelbuoy
