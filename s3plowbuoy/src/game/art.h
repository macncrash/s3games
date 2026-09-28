// S3 PLOW BUOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace plow {

enum Pal : int {
    PAL_HUD = 0,
    PAL_PLOW = 1,
    PAL_NUN = 2,
    PAL_CAN = 3,
    PAL_DOCK = 4,
    PAL_SPRAY = 5,
    PAL_ICE = 6,
    PAL_POST = 7,
    PAL_CREW = 8,
    PAL_OTHER = 9,
    PAL_BANNER = 10,
    PAL_WIN = 11,
    PAL_ALERT = 12,
    PAL_FIELD = 13,
    PAL_LAMP = 14,
    PAL_BLADE = 15
};

struct Art {
    gs::Mipped plow[8];
    gs::Mipped nun, nun3, can;
    gs::Mipped quay, shed, pile, flag, lamp, crate;
    gs::Mipped spray, ring, pin;
    gs::Mipped title, round, same, made, missed, wrong, crew, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace plow
