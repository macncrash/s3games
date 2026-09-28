// S3 JUGGLE TAPE pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace juggletape {

enum Pal {
    PAL_BODY = 0,
    PAL_GLOVE = 1,
    PAL_RED = 2,
    PAL_GOLD = 3,
    PAL_BLUE = 4,
    PAL_STAGE = 5,
    PAL_REEL = 6,
    PAL_DRAWER = 7,
    PAL_INK = 8,
    PAL_MARK = 9,
    PAL_ALERT = 10,
    PAL_TITLE = 11,
    PAL_WIN = 12
};

struct Art {
    gs::Image juggler;
    gs::Image glove;
    gs::Image red;
    gs::Image gold;
    gs::Image blue;
    gs::Image floor;
    gs::Image reel;
    gs::Image drawer;
    gs::Image lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace juggletape
