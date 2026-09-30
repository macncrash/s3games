// S3 GRANARY CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gcler {

enum Pal {
    PAL_TEXT = 0,
    PAL_TIMBER = 1,
    PAL_KEEPER = 2,
    PAL_SACK = 3,
    PAL_GRAIN = 4,
    PAL_STRAW = 5,
    PAL_FLOOR = 6,
    PAL_BEAM = 7,
    PAL_GOOD = 8,
    PAL_ALERT = 9,
    PAL_LOFT = 10
};

struct Art {
    gs::Mipped keeper[2];
    gs::Mipped scoop;
    gs::Mipped sack;
    gs::Mipped spill;
    gs::Mipped bale;
    gs::Mipped chaff;
    gs::Mipped post;
    gs::Mipped loft;
    gs::Mipped clock;
    gs::Mipped shadow;
    gs::Mipped mote;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gcler
