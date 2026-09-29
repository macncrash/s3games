// Pictures for the viaduct watch. Drawn into the VDP at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace maga {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_REAL = 2,
    PAL_FEINT = 3,
    PAL_GUN = 4,
    PAL_FX = 5,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped arch;
    gs::Mipped real;
    gs::Mipped feint;
    gs::Mipped gun;
    gs::Mipped flash;
    gs::Mipped chev;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace maga
