// S3 SKIFF BUOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace skiff {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_CAN = 2,
    PAL_NUN = 3,
    PAL_BALL = 4,
    PAL_WOOD = 5,
    PAL_FOAM = 6,
    PAL_GULL = 7,
    PAL_END = 8,
    PAL_MAP = 9,
    PAL_WAVE = 10,
    PAL_BANNER = 11,
    PAL_WIN = 12,
    PAL_ALERT = 13,
    PAL_ROAD = 14
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped can, nun, ball;
    gs::Mipped pile, day, finger, shed, rock;
    gs::Mipped gull[2];
    gs::Mipped foam, ring, pin, dash, panel, dot, flag;
    gs::Mipped title, missed, legFail, sameDock, endHeld, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skiff
