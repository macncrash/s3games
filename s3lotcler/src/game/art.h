// S3 LOT CLER pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lotcler {

enum Pal {
    PAL_INK = 0,
    PAL_YARD = 1,
    PAL_RIG = 2,
    PAL_TIRE = 3,
    PAL_DRUM = 4,
    PAL_WOOD = 5,
    PAL_LEAF = 6,
    PAL_CONE = 7,
    PAL_SACK = 8,
    PAL_POLE = 9,
    PAL_DUST = 10,
    PAL_GOOD = 11,
    PAL_HOT = 12
};

struct Art {
    gs::Mipped rig[2];
    gs::Mipped tire;
    gs::Mipped drum;
    gs::Mipped pallet;
    gs::Mipped leaves;
    gs::Mipped cone;
    gs::Mipped sack;
    gs::Mipped plank;
    gs::Mipped pole;
    gs::Mipped stall;
    gs::Mipped fence;
    gs::Mipped clock;
    gs::Mipped shadow;
    gs::Mipped dust;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lotcler
