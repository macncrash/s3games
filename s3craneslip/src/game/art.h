// S3 CRANESLIP pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace slip {

enum Pal {
    PAL_WHITE = 0,
    PAL_HARBOR = 1,
    PAL_CRANE = 2,
    PAL_HOOK = 3,
    PAL_BARGE = 4,
    PAL_PILE = 5,
    PAL_WATER = 6,
    PAL_CABLE = 7,
    PAL_GULL = 8,
    PAL_AMBER = 9,
    PAL_RED = 10,
    PAL_GREEN = 11,
    PAL_SIGN = 12
};

struct Art {
    gs::Mipped trolley, hook, link, barge, pile, water, gull, lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace slip
