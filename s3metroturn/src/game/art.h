// S3 METRO TURN sprites. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace metroturn {

enum Pal {
    PAL_HUD = 0,
    PAL_TRAIN = 1,
    PAL_LAMP = 2,
    PAL_SIGN = 3,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped car;
    gs::Mipped lamp;
    gs::Mipped chev;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace metroturn
