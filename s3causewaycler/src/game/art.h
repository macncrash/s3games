// S3 CAUSEWAY CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ccler {

enum Pal {
    PAL_TEXT = 0,
    PAL_STONE = 1,
    PAL_KEEPER = 2,
    PAL_WOOD = 3,
    PAL_NET = 4,
    PAL_BUOY = 5,
    PAL_WATER = 6,
    PAL_LAMP = 7,
    PAL_GOOD = 8,
    PAL_ALERT = 9
};

struct Art {
    gs::Mipped keeper[2];
    gs::Mipped rake;
    gs::Mipped timber;
    gs::Mipped crate;
    gs::Mipped net;
    gs::Mipped barrel;
    gs::Mipped buoy;
    gs::Mipped rope;
    gs::Mipped slab;
    gs::Mipped post;
    gs::Mipped lamp;
    gs::Mipped clock;
    gs::Mipped shadow;
    gs::Mipped puff;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ccler
