// S3 TILEMARK pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace tilemark {

enum Pal {
    PAL_TILE = 0,
    PAL_MARK = 1,
    PAL_STAMP = 2,
    PAL_CURSOR = 3,
    PAL_INK = 4,
    PAL_GOLD = 5,
    PAL_ALERT = 6,
    PAL_OK = 7
};

struct Art {
    gs::Image face[4];
    gs::Image plate;
    gs::Image stamp;
    gs::Image cursor;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tilemark
