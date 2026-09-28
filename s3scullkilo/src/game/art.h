// S3 SCULL KILO pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kilo {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_HULL = 2,
    PAL_OAR = 3,
    PAL_WHEEL = 4,
    PAL_CART = 5,
    PAL_MILL = 6,
    PAL_BANK = 7,
    PAL_MARK = 8,
    PAL_WATER = 12
};

struct Art {
    gs::Image glyph[96];
    gs::Mipped hull;
    gs::Mipped oar[4];
    gs::Mipped wheel[3];
    gs::Mipped mill;
    gs::Mipped reed;
    gs::Mipped tree;
    gs::Mipped post;
    gs::Mipped flag;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kilo
