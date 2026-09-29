// Orchard pursuit sprites. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace orchardpurs {

enum Pal {
    PAL_HUD = 0,
    PAL_TREE = 1,
    PAL_TRACTOR = 2,
    PAL_SPRAY = 3,
    PAL_HARVEST = 4,
    PAL_APPLE = 5,
    PAL_SHED = 6,
    PAL_LEAF = 7,
    PAL_WRECK = 8
};

struct Art {
    gs::Mipped tree;
    gs::Mipped tractor;
    gs::Mipped sprayer;
    gs::Mipped harvest;
    gs::Mipped apple;
    gs::Mipped mist;
    gs::Mipped shed;
    gs::Mipped leaf;
    gs::Mipped spark;
    gs::Mipped titleA;
    gs::Mipped titleB;
    gs::Mipped sub;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace orchardpurs
