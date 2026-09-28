#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cranebox {

enum Pal {
    PAL_WHITE = 0,
    PAL_YARD = 1,
    PAL_CRANE = 2,
    PAL_BOX = 3,
    PAL_AMBER = 4,
    PAL_RED = 5,
    PAL_GREEN = 6
};

struct Art {
    gs::Mipped crane, wheel, post, stripe, shed, lamp;
    int font[96] = {};
    int dirt = 1;
    int curb = 2;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cranebox
