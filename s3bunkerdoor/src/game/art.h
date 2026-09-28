// Pictures for the bunker door. Drawn into VRAM and sprite ROM at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace door {

enum Pal {
    PAL_HUD = 0,
    PAL_ROOM = 1,
    PAL_DOOR = 2,
    PAL_NIGHT = 3,
    PAL_ARM = 4,
    PAL_WARN = 5,
    PAL_FIG = 6
};

struct Art {
    gs::Mipped slab;
    gs::Mipped arm;
    gs::Mipped figure;
    gs::Mipped moon;
    gs::Mipped chev;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace door
