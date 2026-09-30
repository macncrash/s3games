// S3 SALLY MAGA sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sallymaga {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_MAN = 2,
    PAL_FALL = 3,
    PAL_BRASS = 4,
    PAL_FIRE = 5,
    PAL_CREAM = 6,
    PAL_RED = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped raider;
    gs::Mipped fallen;
    gs::Mipped tower;
    gs::Mipped arch;
    gs::Mipped torch;
    gs::Mipped banner;
    gs::Mipped sight;
    gs::Mipped puff;
    gs::Mipped round;
    gs::Mipped spent;
    gs::Image glyph[96];
    int cellW = 6;
    int cellH = 7;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sallymaga
