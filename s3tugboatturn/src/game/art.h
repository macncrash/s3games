// S3 TUGBOAT TURN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tugturn {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TUG = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_PIER = 4,
    PAL_STEEL = 5,
    PAL_FOAM = 6,
    PAL_BIRD = 7,
    PAL_ALERT = 8,
    PAL_WIN = 9,
    PAL_BANNER = 10,
    PAL_TAG = 11,
    PAL_HARBOR = 12,  // road generator: fairway water, timber verge, concrete quay
    PAL_RIVAL = 13,
    PAL_SKY = 14,
    PAL_MARK = 15
};

struct Art {
    gs::Mipped stern[7];
    gs::Mipped wreck;
    gs::Mipped buoy;
    gs::Mipped board[3];
    gs::Mipped shed, crane, lamp, bollard;
    gs::Mipped gull[2];
    gs::Mipped foam, spray, smoke, shadow;
    gs::Mipped cloud, sun;
    gs::Mipped title, steady, tipped, beaten, missed, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tugturn
