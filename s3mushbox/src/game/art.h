// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace mushbox {

enum Pal {
    PAL_HUD = 0,
    PAL_MUSH = 1,
    PAL_BOX = 2,
    PAL_ICE = 3,
    PAL_GOLD = 4
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    gs::Mipped mush[3];
    gs::Mipped box;
    gs::Mipped plank;
    gs::Mipped spark;
    gs::Mipped shadow;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mushbox
