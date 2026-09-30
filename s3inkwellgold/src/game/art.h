// S3 INKWELL GOLD — desk art drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace inkwellgold {

enum Pal {
    PAL_DESK = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_QUILL = 3,
    PAL_INK = 4,
    PAL_TITLE = 5,
    PAL_WIN = 6,
    PAL_BAD = 7,
    PAL_HINT = 8
};

struct Art {
    gs::Image desk;
    gs::Image well;
    gs::Image pool;
    gs::Image quill;
    gs::Image drop;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace inkwellgold
