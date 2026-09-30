// S3 SALLY BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sallybann {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_COAT = 2,
    PAL_BANNER = 3,
    PAL_BOLT = 4,
    PAL_GROUND = 5,
    PAL_GOOD = 6,
    PAL_ALERT = 7,
    PAL_GOLD = 8
};

struct Art {
    gs::Mipped runner;
    gs::Mipped banner;
    gs::Mipped jamb;
    gs::Mipped lintel;
    gs::Mipped bolt;
    gs::Mipped sod;
    gs::Mipped puff;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sallybann
