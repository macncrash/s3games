// Pictures drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mushlock {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_TEAM = 2,
    PAL_WOOD = 3,
    PAL_STONE = 4,
    PAL_FLAG = 5,
    PAL_PINE = 6,
    PAL_SNOW = 12
};

struct Art {
    gs::Image glyph[96];
    gs::Mipped sled;
    gs::Mipped dog[3];
    gs::Mipped leaf;
    gs::Mipped pier;
    gs::Mipped pine;
    gs::Mipped post;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mushlock
