// S3 CISTERN PURSE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace purse {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WATER = 2,
    PAL_YOU = 3,
    PAL_RIVAL = 4,
    PAL_HEAVY = 5,
    PAL_SKIT = 6,
    PAL_FX = 7,
    PAL_OK = 8,
    PAL_ALERT = 9
};

struct Art {
    gs::Mipped basin, ring, lip;
    gs::Mipped cart[2], heavy[2], skit[2], wreck, spark, lamp;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace purse
