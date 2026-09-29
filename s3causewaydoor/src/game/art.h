// S3 CAUSEWAY DOOR sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace causewaydoor {

enum Pal {
    PAL_HUD = 0,
    PAL_IRON = 1,
    PAL_COAT = 2,
    PAL_SEA = 3,
    PAL_STONE = 4,
    PAL_LAMP = 5,
    PAL_CHAIN = 6,
    PAL_SKY = 7
};

struct Art {
    gs::Mipped leanL, leanR, brace, shoulder;
    gs::Mipped door, chain, lamp, slab, pile, wave, gull, bolt;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace causewaydoor
