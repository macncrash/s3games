// S3 CRANEPLAT pictures. Drawn into VRAM and sprite ROM at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace craneplat {

enum Pal {
    PAL_WHITE = 0,
    PAL_YARD = 1,
    PAL_CRANE = 2,
    PAL_HOOK = 3,
    PAL_WOOD = 4,
    PAL_CABLE = 5,
    PAL_RIVAL = 6,
    PAL_AMBER = 7,
    PAL_RED = 8,
    PAL_GREEN = 9,
    PAL_BANNER = 10
};

struct Art {
    gs::Mipped trolley, hook, link, crate, rival, lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace craneplat
