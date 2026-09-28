// S3 MILLWELL pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mill {

enum Pal {
    PAL_HUD = 0,
    PAL_MILL = 1,
    PAL_WELL = 2,
    PAL_MAN = 3,
    PAL_CROW = 4,
    PAL_BOAR = 5,
    PAL_RAIDER = 6,
    PAL_FX = 7,
    PAL_YARD = 12
};

struct Art {
    gs::Mipped mill[2];
    gs::Mipped well;
    gs::Mipped man[2];
    gs::Mipped crow;
    gs::Mipped boar;
    gs::Mipped raider;
    gs::Mipped sack;
    gs::Mipped puff;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mill
