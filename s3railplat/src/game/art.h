// S3 RAIL PLAT pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace railplat {

enum Pal : int {
    PAL_TEXT = 0,
    PAL_DIM = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_AMBER = 4,
    PAL_COACH = 5,
    PAL_PLAT = 6,
    PAL_LAND = 7,
    PAL_LAMP = 8,
    PAL_FX = 9
};

struct Art {
    gs::Image coach[2];
    gs::Image plat, rail, lamp, tree, hill, cloud, puff, buffer;
    gs::Image logo, tag, banLevel, banShort, banPast, banCrew;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace railplat
