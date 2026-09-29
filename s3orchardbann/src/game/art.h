// S3 ORCHARD BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace orchardbann {

enum Pal {
    PAL_HUD = 0,
    PAL_LEAF = 1,
    PAL_PICK = 2,
    PAL_BANNER = 3,
    PAL_SCARE = 4,
    PAL_WOOD = 5,
    PAL_APPLE = 6,
    PAL_EARTH = 7
};

struct Art {
    gs::Mipped stand, walkA, walkB, swing;
    gs::Mipped scare[2];
    gs::Mipped banner;
    gs::Mipped tree;
    gs::Mipped apple;
    gs::Mipped gate;
    gs::Mipped basket;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace orchardbann
