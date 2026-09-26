// S3 SKIFF TURN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace skiffturn {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_REED = 4,
    PAL_WOOD = 5,
    PAL_FOAM = 6,
    PAL_BIRD = 7,
    PAL_ALERT = 8,
    PAL_WIN = 9,
    PAL_BANNER = 10,
    PAL_TAG = 11,
    PAL_CREEK = 12,  // road generator: peat water, mud verge, grass bank
    PAL_MARK = 13,
    PAL_SKY = 14
};

struct Art {
    gs::Mipped stern[7];
    gs::Mipped wreck;
    gs::Mipped buoy;
    gs::Mipped board[3];
    gs::Mipped reed, heron, shack, dock;
    gs::Mipped gull[2];
    gs::Mipped foam, spray, shadow;
    gs::Mipped cloud, sun;
    gs::Mipped title, steady, tipped, missed, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skiffturn
