// Pictures for the granary yard. Drawn into the VDP at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace granary {

enum Pal {
    PAL_HUD = 0,
    PAL_FARM = 1,
    PAL_BANN = 2,
    PAL_BARN = 3,
    PAL_YARD = 4,
    PAL_CROW = 5,
    PAL_GROUND = 6
};

struct Art {
    gs::Mipped farmer[2];
    gs::Mipped banner;
    gs::Mipped barn;
    gs::Mipped crow;
    gs::Mipped sack;
    int font[96] = {};
    int wheat = 1;
    int dirt = 2;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace granary
