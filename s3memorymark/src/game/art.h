// S3 MEMORYMARK pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace memorymark {

enum Pal {
    PAL_BACK = 0,
    PAL_FACE = 1,
    PAL_STAMP = 2,
    PAL_HAND = 3,
    PAL_CURSOR = 4,
    PAL_INK = 5,
    PAL_GOLD = 6,
    PAL_ALERT = 7,
    PAL_OK = 8
};

struct Art {
    gs::Image back;
    gs::Image face[4];
    gs::Image stamp;
    gs::Image hand;
    gs::Image cursor;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace memorymark
