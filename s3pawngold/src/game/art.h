// S3 PAWN GOLD — board and pawn art drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pawngold {

enum Pal {
    PAL_BOARD = 0,
    PAL_GOLD = 1,
    PAL_IVORY = 2,
    PAL_MARK = 3,
    PAL_INK = 4,
    PAL_TITLE = 5,
    PAL_WIN = 6,
    PAL_BAD = 7,
    PAL_HINT = 8
};

struct Art {
    gs::Image board;
    gs::Image pawn;
    gs::Image crown;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pawngold
