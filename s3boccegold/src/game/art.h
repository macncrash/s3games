#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace boccegold {

enum Pal {
    PAL_COURT = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_RIVAL = 3,
    PAL_JACK = 4,
    PAL_INK = 5,
    PAL_TITLE = 6,
    PAL_WIN = 7,
    PAL_HINT = 8,
    PAL_AIM = 9
};

struct Art {
    gs::Image court;
    gs::Image bowl;
    gs::Image jack;
    gs::Image banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace boccegold
