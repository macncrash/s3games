// S3 CRANEPASS pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pass {

enum Pal {
    PAL_WHITE = 0,
    PAL_LAND = 1,
    PAL_CRANE = 2,
    PAL_HOOK = 3,
    PAL_ROCK = 4,
    PAL_LOG = 5,
    PAL_JEEP = 6,
    PAL_SLAB = 7,
    PAL_CABLE = 8,
    PAL_RIVAL = 9,
    PAL_AMBER = 10,
    PAL_RED = 11,
    PAL_GREEN = 12,
    PAL_SNOW = 13,
    PAL_SIGN = 14
};

struct Art {
    gs::Mipped trolley, hook, link, rock, log, jeep, slab, rival, flake, gate, arrow;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pass
