// S3 CRANE MARK sprites and the yard picture. Everything is drawn at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cranemark {

enum Pal {
    PAL_WHITE = 0,
    PAL_YARD = 1,
    PAL_CRANE = 2,
    PAL_HOOK = 3,
    PAL_WOOD = 4,
    PAL_CABLE = 5,
    PAL_END = 6,
    PAL_BANNER = 7,
    PAL_AMBER = 8,
    PAL_RED = 9,
    PAL_GREEN = 10,
    PAL_SET = 11
};

struct Art {
    gs::Mipped trolley, hook, link, shadow, pennant, gull, banner;
    gs::Mipped crate;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cranemark
