// S3 QUARRY BANN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace qbann {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_BANNER = 4,
    PAL_ROCK = 5,
    PAL_MILL = 6,
    PAL_CREW = 7,
    PAL_DUST = 8,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped crew, banner, rock, cliff, mill, hopper, post, dust, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace qbann
