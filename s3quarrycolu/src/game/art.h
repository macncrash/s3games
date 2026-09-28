// S3 QUARRY COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace qcol {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_DUMP = 4,
    PAL_ROCK = 5,
    PAL_CRUSH = 6,
    PAL_BOARD = 7,
    PAL_COAT = 8,
    PAL_DUST = 9,
    PAL_SIGN = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped dumper, flag, board, marshal;
    gs::Mipped cliff, crusher, hopper, boulder;
    gs::Mipped dust, shadow, block;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace qcol
