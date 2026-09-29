// S3 SOLITAIRE GOLD — felt and cards drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace solitaire {

enum Pal {
    PAL_FELT = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_INK = 3,
    PAL_TITLE = 4,
    PAL_WIN = 5,
    PAL_BAD = 6,
    PAL_HINT = 7,
    PAL_PIP = 8
};

struct Art {
    gs::Image felt;
    gs::Image card;
    gs::Image coin;
    gs::Image oval;
    gs::Image stack;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace solitaire
