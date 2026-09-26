// S3 RIDGE RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace reli {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_SENTRY = 4,
    PAL_SKIRM = 5,
    PAL_PORTER = 6,
    PAL_SCOUT = 7,
    PAL_FX = 8,
    PAL_BELL = 9,
    PAL_STONE = 10,
    PAL_RELIEF = 11,
    PAL_FIELD = 12,
    PAL_MOUNT = 13
};

struct Art {
    gs::Mipped sentry[2];
    gs::Mipped skirm[2];
    gs::Mipped porter[2];
    gs::Mipped scout[2];
    gs::Mipped bolt, glint, dust, shadow;
    gs::Mipped bell, yoke, rope, pad;
    gs::Mipped cairn, banner, lantern;
    gs::Mipped peakL, peakR, moon, cloud, star;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace reli
