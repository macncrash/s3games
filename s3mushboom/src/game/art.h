// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace mushboom {

enum Pal {
    PAL_HUD = 0,
    PAL_MUSH = 1,
    PAL_DRIVE = 2,
    PAL_BOOM = 3,
    PAL_WATER = 4,
    PAL_SNOW = 5
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    gs::Mipped mush[2];
    gs::Mipped drive;
    gs::Mipped plank;
    gs::Mipped post;
    gs::Mipped water;
    gs::Mipped snow;
    gs::Mipped shadow;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mushboom
