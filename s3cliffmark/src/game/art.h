// Cliff-mark sprites. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cliffmark {

enum Pal {
    PAL_HUD = 0,
    PAL_CART = 1,
    PAL_POST = 2,
    PAL_CRAG = 3,
    PAL_PAINT = 4,
    PAL_RIBBON = 5,
    PAL_SHELF = 12,
    PAL_MARK = 13
};

struct Art {
    gs::Mipped cart;
    gs::Mipped post;
    gs::Mipped crag;
    gs::Mipped paint;
    gs::Mipped ribbon;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cliffmark
