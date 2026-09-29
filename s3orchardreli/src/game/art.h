// Orchard sprites. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace orchard {

enum Pal {
    PAL_HUD = 0,
    PAL_TREE = 1,
    PAL_KEEP = 2,
    PAL_CROW = 3,
    PAL_PICK = 4,
    PAL_BELL = 5,
    PAL_BASK = 6,
    PAL_FLOOR = 12
};

struct Art {
    gs::Mipped tree;
    gs::Mipped keep[2];
    gs::Mipped crow[2];
    gs::Mipped picker;
    gs::Mipped basket;
    gs::Mipped bell;
    gs::Mipped rope;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace orchard
