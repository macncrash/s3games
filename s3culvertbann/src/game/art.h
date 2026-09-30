// S3 CULVERT BANN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace culvertbann {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_COAT = 2,
    PAL_BANNER = 3,
    PAL_WATER = 4,
    PAL_RAT = 5,
    PAL_IRON = 6,
    PAL_MOSS = 7
};

struct Art {
    gs::Mipped stand, walkA, walkB;
    gs::Mipped banner;
    gs::Mipped rat;
    gs::Mipped rib;
    gs::Mipped slab;
    gs::Mipped water;
    gs::Mipped grate;
    gs::Mipped drip;
    gs::Mipped lamp;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace culvertbann
