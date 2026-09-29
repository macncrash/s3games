// S3 CISTERN RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cistern {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WATER = 2,
    PAL_YOU = 3,
    PAL_FOE = 4,
    PAL_PLATE = 5,
    PAL_FX = 6,
    PAL_BELL = 7,
    PAL_OK = 8,
    PAL_ALERT = 9
};

struct Art {
    gs::Mipped basin, ring, sentry[2], raider[2], runner[2], plate[2];
    gs::Mipped bell, rope, bucket, flash, reed;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cistern
