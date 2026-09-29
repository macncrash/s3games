#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace craneboom {

enum Pal {
    PAL_WHITE = 0,
    PAL_YARD = 1,
    PAL_CRANE = 2,
    PAL_BOOM = 3,
    PAL_DRIVE = 4,
    PAL_AMBER = 5,
    PAL_RED = 6,
    PAL_GREEN = 7
};

struct Art {
    gs::Mipped cab, boom, drive, plank, saddle, pier, hook, link, shed;
    int font[96] = {};
    int dirt = 1;
    int curb = 2;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace craneboom
