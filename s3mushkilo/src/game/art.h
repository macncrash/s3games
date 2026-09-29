// S3 MUSHKILO sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mush {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_MUSH = 4,
    PAL_WHEEL = 5,
    PAL_CART = 6,
    PAL_BIKE = 7,
    PAL_POST = 8,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped mush[2];
    gs::Mipped wheel;
    gs::Mipped cart;
    gs::Mipped bike;
    gs::Mipped post;
    gs::Mipped tree;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mush
