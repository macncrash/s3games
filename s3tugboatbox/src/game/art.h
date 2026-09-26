// S3 TUGBOAT BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tugbox {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TUG = 1,
    PAL_QUAY = 2,
    PAL_MARK = 3,
    PAL_END = 4,
    PAL_FOAM = 5,
    PAL_SMOKE = 6,
    PAL_GULL = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_BANNER = 10,
    PAL_LAMP = 11,
    PAL_CH = 12
};

struct Art {
    gs::Mipped tug[16];
    gs::Mipped shade;
    gs::Mipped quay, shed, bulk, lamp;
    gs::Mipped post, hbar, vbar, hatch;
    gs::Mipped buoyR, buoyG, boom;
    gs::Mipped foam, smoke, gull[2], pin;
    gs::Mipped title, stopped, missed, outside, stoppedShort, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tugbox
