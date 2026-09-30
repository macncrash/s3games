// Pictures drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kartlock {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_KART = 2,
    PAL_STEEL = 3,
    PAL_FLAG = 4,
    PAL_TREE = 5,
    PAL_ROAD = 12
};

struct Art {
    gs::Image glyph[96];
    gs::Mipped kart;
    gs::Mipped gate;
    gs::Mipped post;
    gs::Mipped tree;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kartlock
