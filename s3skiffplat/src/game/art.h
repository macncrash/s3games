// S3 SKIFF PLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace skiffplat {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_WOOD = 2,
    PAL_PILE = 3,
    PAL_FOLK = 4,
    PAL_FOAM = 5,
    PAL_GULL = 6,
    PAL_LAMP = 7,
    PAL_ALERT = 8,
    PAL_WIN = 9,
    PAL_BANNER = 10,
    PAL_BUOY = 11,
    PAL_GRASS = 12,
    PAL_SHOAL = 13,
    PAL_WAVE = 14,
    PAL_TAG = 15
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped deck;
    gs::Mipped pile, post, shed, lamp, staff, pip;
    gs::Mipped buoy, reed, flag, rope, coil;
    gs::Mipped hand[2];
    gs::Mipped gull[2];
    gs::Mipped foam, pin;
    gs::Mipped track, bubble;
    gs::Mipped title, level, withPlat, missed, tideOut, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skiffplat
