// Orchard pictures. Drawn into VRAM and sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace orchard {

enum Pal {
    PAL_HUD = 0,
    PAL_FIELD = 1,
    PAL_TREE = 2,
    PAL_CLUTTER = 3,
    PAL_HERO = 4,
    PAL_BANNER = 5
};

struct Art {
    gs::Image tree;
    gs::Image hero;
    gs::Image pile[4];
    gs::Image spark;
    gs::Image title;
    gs::Image sub;
    gs::Image win;
    gs::Image lose;
    int font[96] = {};
    int grass[4] = {};
    int soil = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace orchard
