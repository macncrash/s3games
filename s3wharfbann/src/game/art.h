// S3 WHARF BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace wharfbann {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_SAILOR = 2,
    PAL_BANNER = 3,
    PAL_BARREL = 4,
    PAL_WATER = 5,
    PAL_HARBOR = 6,
    PAL_ROPE = 7
};

struct Art {
    gs::Mipped stand, walkA, walkB, leap;
    gs::Mipped banner, staff, bollard, barrel, plank, rope, buoy, gull, crate;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wharfbann
