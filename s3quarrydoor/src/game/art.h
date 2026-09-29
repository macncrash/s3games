// Pictures for the quarry gate. Drawn into VRAM and sprite ROM at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace quarry {

enum Pal {
    PAL_HUD = 0,
    PAL_PIT = 1,
    PAL_GATE = 2,
    PAL_TRUCK = 3,
    PAL_CREW = 4,
    PAL_WARN = 5,
    PAL_DUST = 6
};

struct Art {
    gs::Mipped gate;
    gs::Mipped shore;
    gs::Mipped crew;
    gs::Mipped truck;
    gs::Mipped dust;
    gs::Mipped chev;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace quarry
