// S3 BOARD GOLD — cabinet art drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace boardgold {

enum Pal {
    PAL_CAB = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_CORD = 3,
    PAL_INK = 4,
    PAL_TITLE = 5,
    PAL_WIN = 6,
    PAL_BAD = 7,
    PAL_HINT = 8
};

struct Art {
    gs::Image cabinet;
    gs::Image lamp;
    gs::Image plug;
    gs::Image bead;
    gs::Image plate;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace boardgold
