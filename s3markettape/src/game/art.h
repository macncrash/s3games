// S3 MARKETTAPE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace markettape {

enum Pal {
    PAL_HUD = 0,
    PAL_STALL = 1,
    PAL_CLERK = 2,
    PAL_APPLE = 3,
    PAL_PLUM = 4,
    PAL_LOAF = 5,
    PAL_BUN = 6,
    PAL_PEAR = 7,
    PAL_FIG = 8,
    PAL_TAPE = 9,
    PAL_OK = 10,
    PAL_BAD = 11,
    PAL_WOOD = 12,
    PAL_INK = 13
};

struct Art {
    gs::Mipped awning, clerk, crate, drawer, apple, plum, loaf, bun, pear, fig, arrow;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace markettape
