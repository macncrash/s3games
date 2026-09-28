// S3 LUGE TURN sprites. Drawn into VDP RAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace luge {

enum Pal {
    PAL_HUD = 0,
    PAL_ICE = 1,
    PAL_POD = 2,
    PAL_GATE = 3,
    PAL_PINE = 4,
    PAL_FX = 5,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped pod[3];
    gs::Mipped gate;
    gs::Mipped pine;
    gs::Mipped spray;
    gs::Mipped title;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace luge
