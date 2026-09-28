// S3 BUNKER RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bunker {

enum Pal {
    PAL_HUD = 0,
    PAL_DOOR = 1,
    PAL_YOU = 2,
    PAL_FOE = 3,
    PAL_PLATE = 4,
    PAL_FX = 5,
    PAL_BELL = 6,
    PAL_ALERT = 7,
    PAL_OK = 8
};

struct Art {
    gs::Mipped door, bar, rifle, sight, flash, dust;
    gs::Mipped walker[2], sprinter[2], plate[2];
    gs::Mipped bell, rope, helm;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bunker
