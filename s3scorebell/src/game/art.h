// S3 SCORE BELL pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scorebell {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_PAPER = 2,
    PAL_INK = 3,
    PAL_BRASS = 4,
    PAL_ROOM = 5,
    PAL_DEAD = 6
};

struct Art {
    gs::Mipped desk, sheet, clef, note, bar, bell, clapper, stand;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scorebell
