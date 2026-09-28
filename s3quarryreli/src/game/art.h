// S3 QUARRY RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace quarry {

enum Pal {
    PAL_HUD = 0,
    PAL_FACE = 1,
    PAL_YOU = 2,
    PAL_FOE = 3,
    PAL_SLAB = 4,
    PAL_FX = 5,
    PAL_BELL = 6,
    PAL_ALERT = 7,
    PAL_OK = 8,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped face, crane, cage, pick, flash, dust;
    gs::Mipped climber[2], cart[2], slab[2];
    gs::Mipped bell, worker;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace quarry
