// S3 HARBOR COLUMN pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace hcol {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_LORRY = 4,
    PAL_TENDER = 5,
    PAL_CHAIN = 6,
    PAL_CRANE = 7,
    PAL_SHED = 8,
    PAL_BUOY = 9,
    PAL_FX = 10,
    PAL_LINK = 11,
    PAL_ROAD = 12,
    PAL_LAMP = 13,
    PAL_GULL = 14
};

struct Art {
    gs::Mipped lorry, tender;
    gs::Mipped post, link, lamp;
    gs::Mipped crane, shed, buoy, gull;
    gs::Mipped puff, shadow, cloud;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hcol
