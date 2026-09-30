// S3 PAWN SEVEN pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pawnseven {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_BOARD = 4,
    PAL_WHITE = 5,
    PAL_BLACK = 6,
    PAL_GOLD = 7
};

struct Art {
    gs::Mipped white;
    gs::Mipped black;
    gs::Mipped flag;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pawnseven
