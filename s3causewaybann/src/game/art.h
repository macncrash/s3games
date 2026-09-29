// S3 CAUSEWAY BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace causewaybann {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_COAT = 2,
    PAL_BANNER = 3,
    PAL_WATCH = 4,
    PAL_WATER = 5,
    PAL_WOOD = 6,
    PAL_LAMP = 7
};

struct Art {
    gs::Mipped stand, walkA, walkB, leap;
    gs::Mipped watch[2];
    gs::Mipped banner;
    gs::Mipped slab;
    gs::Mipped pier;
    gs::Mipped wave;
    gs::Mipped lamp;
    gs::Mipped gate;
    gs::Mipped boat;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace causewaybann
