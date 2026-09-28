// Pictures for S3 JUGGLECHIME. Drawn into VRAM at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace jugglechime {

enum Pal {
    PAL_INK = 1,
    PAL_GOLD = 2,
    PAL_CREAM = 3,
    PAL_ALERT = 4,
    PAL_WIN = 5,
    PAL_BODY = 6,
    PAL_GLOVE = 7,
    PAL_CLOCK = 8,
    PAL_BELL = 9,
    PAL_STAGE = 10,
    PAL_TITLE = 11
};

struct Art {
    gs::Image juggler;
    gs::Image glove;
    gs::Image cream;
    gs::Image gold;
    gs::Image bell;
    gs::Image face;
    gs::Image pip;
    gs::Image floor;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace jugglechime
