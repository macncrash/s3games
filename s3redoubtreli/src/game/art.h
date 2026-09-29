// S3 REDOUBT RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace redoubt {

enum Pal {
    PAL_HUD = 0,
    PAL_EARTH = 1,
    PAL_YOU = 2,
    PAL_FOE = 3,
    PAL_SHIELD = 4,
    PAL_FX = 5,
    PAL_BELL = 6,
    PAL_ALERT = 7,
    PAL_OK = 8,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped berm, gun, flash, stake;
    gs::Mipped walker[2], shield[2];
    gs::Mipped bell, rope, frame;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace redoubt
