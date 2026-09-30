// Pictures for the beacon door. Drawn into VRAM and sprite ROM at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bdoor {

enum Pal {
    PAL_HUD = 0,
    PAL_HEAD = 1,
    PAL_DOOR = 2,
    PAL_LAMP = 3,
    PAL_ARM = 4,
    PAL_WARN = 5,
    PAL_GALE = 6
};

struct Art {
    gs::Mipped slab;
    gs::Mipped arm;
    gs::Mipped gale;
    gs::Mipped lamp;
    gs::Mipped chev;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bdoor
