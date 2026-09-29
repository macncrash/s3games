// S3 CHOIRBELL sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace choir {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_GLASS = 2,
    PAL_ROBE = 3,
    PAL_ROBE2 = 4,
    PAL_BELL = 5,
    PAL_SKIN = 6,
    PAL_NOTE = 7
};

struct Art {
    gs::Mipped arch;
    gs::Mipped glass;
    gs::Mipped column;
    gs::Mipped pew;
    gs::Mipped floor;
    gs::Mipped robe;
    gs::Mipped robeAlt;
    gs::Mipped head;
    gs::Mipped book;
    gs::Mipped bell;
    gs::Mipped clapper;
    gs::Mipped note;
    gs::Mipped candle;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace choir
