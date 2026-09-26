// S3 SKIFF KILO sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kilo {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_MILL = 2,
    PAL_PADDLE = 3,
    PAL_BANK = 4,
    PAL_HOUSE = 5,
    PAL_CLOCK = 6,
    PAL_FOAM = 7,
    PAL_BUOY = 8,
    PAL_GULL = 9,
    PAL_BANNER = 10,
    PAL_WIN = 11,
    PAL_ALERT = 12,
    PAL_TAG = 13,
    PAL_CREW = 14,
    PAL_SPAR = 15
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped mill[8];
    gs::Mipped paddle[8];
    gs::Mipped hand[8];
    gs::Mipped bank, reed, house, clock, dock, buoy, post, foam, line, gull[2];
    gs::Mipped title, clean, took, touched, beached, paused, kilo;
    gs::Mipped m250, m500, m750, m1000, crewTag;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kilo
