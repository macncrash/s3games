// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace spanwell {

enum Pal {
    PAL_HUD = 0,
    PAL_SKY = 1,
    PAL_SEA = 2,
    PAL_WOOD = 3,
    PAL_STONE = 4,
    PAL_WELL = 5,
    PAL_KEEPER = 6,
    PAL_WAVE = 7,
    PAL_GOLD = 8,
    PAL_WARN = 9,
    PAL_ROPE = 10,
    PAL_FOAM = 11
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    gs::Mipped plank, post, well, roof, bucket, keeper, brace, wave, foam;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace spanwell
