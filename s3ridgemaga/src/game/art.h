// S3 RIDGE MAGA sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rmaga {

enum Pal {
    PAL_INK = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_YOU = 4,
    PAL_RAID = 5,
    PAL_PEEL = 6,
    PAL_STONE = 7,
    PAL_FX = 8,
    PAL_MOUNT = 9,
    PAL_CLOTH = 10,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped you[2];
    gs::Mipped raider[2];
    gs::Mipped peel[2];
    gs::Mipped fallen;
    gs::Mipped cairn, stake, post, banner;
    gs::Mipped mount[2];
    gs::Mipped sun, cloud, dust, flash, shadow;
    gs::Mipped round, spent, bead;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rmaga
