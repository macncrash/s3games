// Pictures drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mushpass {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_TEAM = 2,
    PAL_ROCK = 3,
    PAL_FLAG = 4,
    PAL_PINE = 5,
    PAL_STORM = 6,
    PAL_SNOW = 12
};

struct Art {
    gs::Image glyph[96];
    gs::Mipped sled;
    gs::Mipped dog[3];
    gs::Mipped peak;
    gs::Mipped cairn;
    gs::Mipped banner;
    gs::Mipped flake;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mushpass
