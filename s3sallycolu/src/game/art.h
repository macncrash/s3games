// S3 SALLY COLUMN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sally {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WAGON = 2,
    PAL_FLAG = 3,
    PAL_TORCH = 4,
    PAL_DUST = 5,
    PAL_GOOD = 6,
    PAL_ALERT = 7,
    PAL_GOLD = 8,
    PAL_ROAD = 12,
    PAL_BAND = 13
};

struct Art {
    gs::Mipped wagon;
    gs::Mipped flag;
    gs::Mipped jamb;
    gs::Mipped torch;
    gs::Mipped post;
    gs::Mipped dust;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sally
