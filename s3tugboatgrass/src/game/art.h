// S3 TUGBOAT GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tuggrass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TUG = 1,
    PAL_QUAY = 2,
    PAL_MARK = 3,
    PAL_FOAM = 4,
    PAL_SMOKE = 5,
    PAL_GULL = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_BANNER = 9,
    PAL_REED = 10,
    PAL_TUFT = 11,
    PAL_FIELD = 12,
    PAL_LIP = 13,
    PAL_WOOD = 14
};

struct Art {
    gs::Mipped tug[16];
    gs::Mipped shade;
    gs::Mipped quay, shed, bulk, post, lamp;
    gs::Mipped buoyR, buoyG;
    gs::Mipped reed, tuft, flag, dash;
    gs::Mipped foam, smoke, gull[2];
    gs::Mipped pin, dot, panel;
    gs::Mipped title, fullStop, onGrass;
    gs::Mipped ranOff, offGrass, shortStop, notFull, timed, legFail, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tuggrass
