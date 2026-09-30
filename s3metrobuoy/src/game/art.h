// S3 METRO BUOY sprites. Everything is painted at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace metrobuoy {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_AMBER = 2,
    PAL_TEAL = 3,
    PAL_PIER = 4,
    PAL_WAKE = 5,
    PAL_GULL = 6,
    PAL_CITY = 7,
    PAL_CRATE = 8,
    PAL_OTHER = 9,
    PAL_BANNER = 10,
    PAL_WIN = 11,
    PAL_ALERT = 12,
    PAL_WATER = 13,
    PAL_LAMP = 14,
    PAL_POST = 15
};

struct Art {
    gs::Mipped hull[8];
    gs::Mipped amber, teal, violet;
    gs::Mipped pier, canopy, lamp, flag;
    gs::Mipped block, crate, gull[2];
    gs::Mipped wake, ring, pin;
    gs::Mipped title, round, home, made, missed, wrong, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace metrobuoy
