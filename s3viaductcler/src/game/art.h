// S3 VIADUCT CLER sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cler {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_FIGURE = 5,
    PAL_CRATE = 6,
    PAL_BARREL = 7,
    PAL_FX = 8,
    PAL_IRON = 9,
    PAL_DECK = 10,
    PAL_PIER = 11,
    PAL_ROAD = 12,
    PAL_CLOCK = 13,
    PAL_LAMP = 14
};

struct Art {
    gs::Mipped walker[2];
    gs::Mipped pole;
    gs::Mipped pier;
    gs::Mipped deck;
    gs::Mipped crate;
    gs::Mipped barrel;
    gs::Mipped beam;
    gs::Mipped lamp;
    gs::Mipped clock;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cler
