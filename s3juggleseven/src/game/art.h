// S3 JUGGLE SEVEN pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace juggleseven {

enum Pal {
    PAL_BODY = 0,
    PAL_GLOVE = 1,
    PAL_RED = 2,
    PAL_BLUE = 3,
    PAL_STAGE = 4,
    PAL_RIVAL = 5,
    PAL_INK = 6,
    PAL_MARK = 7,
    PAL_ALERT = 8,
    PAL_TITLE = 9,
    PAL_WIN = 10
};

struct Art {
    gs::Image juggler;
    gs::Image rival;
    gs::Image glove;
    gs::Image red;
    gs::Image blue;
    gs::Image floor;
    gs::Image booth;
    gs::Image lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace juggleseven
