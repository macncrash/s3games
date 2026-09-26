// S3 SKIFF GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace skiffgrass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_TUFT = 2,
    PAL_REED = 3,
    PAL_WOOD = 4,
    PAL_MARK = 5,
    PAL_FOAM = 6,
    PAL_GULL = 7,
    PAL_BUOY = 8,
    PAL_MAP = 9,
    PAL_WIN = 10,
    PAL_ALERT = 11,
    PAL_FIELD = 12,
    PAL_ENDF = 13,
    PAL_BANNER = 14
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped tuft, reed, post, flag, dash, shed, buoy;
    gs::Mipped gull[2];
    gs::Mipped foam, ring, dot, pin, panel;
    gs::Mipped title, fullStop, onGrass, missed, offGrass, legFail, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skiffgrass
