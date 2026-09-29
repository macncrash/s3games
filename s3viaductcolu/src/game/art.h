// S3 VIADUCT COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace vcol {

enum Pal {
    PAL_TEXT = 0,
    PAL_MIST = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_LORRY = 4,
    PAL_STONE = 5,
    PAL_LAMP = 6,
    PAL_CHAIN = 7,
    PAL_COAT = 8,
    PAL_SMOKE = 9,
    PAL_WATER = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped lorry, pennant, chain, sentry;
    gs::Mipped pier, lamp, rail;
    gs::Mipped smoke, shadow, stripe;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace vcol
