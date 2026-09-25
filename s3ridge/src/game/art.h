// S3 RIDGE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ridge {

enum Pal {
    PAL_INK = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_YOU = 4,
    PAL_RAID = 5,
    PAL_BRUTE = 6,
    PAL_RUN = 7,
    PAL_FX = 8,
    PAL_STONE = 9,
    PAL_MOUNT = 10,
    PAL_BANNER = 11,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped you[2];
    gs::Mipped raider[2];
    gs::Mipped brute[2];
    gs::Mipped runner[2];
    gs::Mipped bolt, dust, flash, shadow, sun, cloud;
    gs::Mipped post, cairn, banner, mouth;
    gs::Mipped mount[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ridge
