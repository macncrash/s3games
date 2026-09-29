// Pictures drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mushturn {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_TEAM = 2,
    PAL_PINE = 3,
    PAL_SIGN = 4,
    PAL_ARCH = 5,
    PAL_SNOW = 12
};

struct Art {
    gs::Image glyph[96];
    gs::Mipped musher;
    gs::Mipped dog[3];
    gs::Mipped sled;
    gs::Mipped spruce;
    gs::Mipped chevron;
    gs::Mipped arch;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mushturn
