// Pictures for S3 BOCCECHIME. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace boccechime {

enum Pal {
    PAL_COURT = 0,
    PAL_BOWL = 1,
    PAL_PALL = 2,
    PAL_CLOCK = 3,
    PAL_INK = 4,
    PAL_TITLE = 5,
    PAL_WIN = 6,
    PAL_DEAD = 7,
    PAL_HINT = 8,
    PAL_GOLD = 9
};

struct Art {
    gs::Image court;
    gs::Image bowl;
    gs::Image pallino;
    gs::Image clock;
    gs::Image banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace boccechime
