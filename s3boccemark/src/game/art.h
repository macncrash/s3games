#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace boccemark {

enum Pal {
    PAL_COURT = 0,
    PAL_YOU = 1,
    PAL_THEM = 2,
    PAL_MARK = 3,
    PAL_INK = 4,
    PAL_GOLD = 5,
    PAL_TITLE = 6,
    PAL_WIN = 7,
    PAL_HINT = 8,
    PAL_AIM = 9
};

struct Art {
    gs::Image court;
    gs::Image ball;
    gs::Image mark;
    gs::Image title;
    gs::Image win;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace boccemark
