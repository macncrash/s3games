// S3 SKIFF LANE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace skifflane {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_REED = 4,
    PAL_WOOD = 5,
    PAL_FOAM = 6,
    PAL_GULL = 7,
    PAL_ALERT = 8,
    PAL_WIN = 9,
    PAL_BANNER = 10,
    PAL_TAG = 11,
    PAL_LANE = 12,  // road generator: the channel
    PAL_MARK = 13,
    PAL_FLAG = 14,
    PAL_SKY = 15
};

struct Art {
    gs::Mipped stern[3];
    gs::Mipped buoy, post, reed, shack, dock;
    gs::Mipped foam, flag, bar, can, shadow, sun, cloud;
    gs::Mipped gull[2];
    gs::Mipped title, held, whole, left, missed, paused, end;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skifflane
