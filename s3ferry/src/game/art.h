// S3 FERRY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ferry {

enum Pal : int {
    PAL_HUD = 0,
    PAL_FERRY = 1,
    PAL_PIER = 2,
    PAL_SHED = 3,
    PAL_FOAM = 4,
    PAL_GULL = 5,
    PAL_BUOY = 6,
    PAL_CLOCK = 7,
    PAL_GUIDE = 8,
    PAL_ALERT = 9,
    PAL_WIN = 10,
    PAL_BANNER = 11,
    PAL_DIM = 12,
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped pier;
    gs::Mipped apron;
    gs::Mipped shed;
    gs::Mipped bridge;
    gs::Mipped buoy[2];
    gs::Mipped clock[8];
    gs::Mipped foam;
    gs::Mipped gull[2];
    gs::Mipped guide;
    gs::Mipped title;
    gs::Mipped docked;
    gs::Mipped tideOut;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ferry
