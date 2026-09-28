// S3 LOT PURSE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lotp {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_RED = 2,
    PAL_VAN = 3,
    PAL_TRUCK = 4,
    PAL_WAGON = 5,
    PAL_LOT = 6,
    PAL_FX = 7,
    PAL_WRECK = 8
};

struct Art {
    gs::Mipped sedan, van, truck, wagon, wreck;
    gs::Mipped stripe, lamp, spark, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lotp
