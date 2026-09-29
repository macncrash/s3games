// S3 MAIL VAN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace van {

enum Pal : int {
    PAL_HUD = 0,
    PAL_VAN = 1,
    PAL_NUN = 2,
    PAL_CAN = 3,
    PAL_DOCK = 4,
    PAL_WAKE = 5,
    PAL_BIRD = 6,
    PAL_REED = 7,
    PAL_SACK = 8,
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
    gs::Mipped nun, nun3, can;
    gs::Mipped quay, shed, pile, flag;
    gs::Mipped reed, sack, bird[2];
    gs::Mipped wake, ring, lamp, pin;
    gs::Mipped title, round, same, made, missed, wrong, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace van
