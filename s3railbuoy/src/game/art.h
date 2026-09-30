// S3 RAIL BUOY pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace railbuoy {

enum Pal : int {
    PAL_TEXT = 0,
    PAL_DIM = 1,
    PAL_BUOY = 2,
    PAL_CAR = 3,
    PAL_DOCK = 4,
    PAL_FAR = 5,
    PAL_RAIL = 6,
    PAL_FOAM = 7,
    PAL_AMBER = 8
};

struct Art {
    gs::Image car[8];
    gs::Image buoy;
    gs::Image dock;
    gs::Image far;
    gs::Image sleeper;
    gs::Image foam;
    gs::Image font[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace railbuoy
