// S3 KARTBOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kartbox {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_KART = 4,
    PAL_WHEEL = 5,
    PAL_BOX = 6,
    PAL_YARD = 7,
    PAL_SKY = 8,
    PAL_CONE = 9,
    PAL_TAPE = 10,
    PAL_DUST = 11
};

struct Art {
    gs::Mipped body;
    gs::Mipped wheel;
    gs::Mipped post;
    gs::Mipped slab;
    gs::Mipped stripe;
    gs::Mipped cone;
    gs::Mipped cloud;
    gs::Mipped dust;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kartbox
