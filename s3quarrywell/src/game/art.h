#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace quarry {

enum Pal {
    PAL_HUD = 0,
    PAL_WELL = 1,
    PAL_MAN = 2,
    PAL_ROCK = 3,
    PAL_CART = 4,
    PAL_BOULDER = 5,
    PAL_FX = 6,
    PAL_CLIFF = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Image glyph[96];
    gs::Image well;
    gs::Image man;
    gs::Image rock;
    gs::Image cart;
    gs::Image boulder;
    gs::Image timber;
    gs::Image puff;
    gs::Image pip;
    gs::Image cliff;
    gs::Image lamp;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace quarry
