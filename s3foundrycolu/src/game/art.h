// S3 FOUNDRY COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace foundry {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_COKE = 4,
    PAL_LADLE = 5,
    PAL_SLAG = 6,
    PAL_STACK = 7,
    PAL_GATE = 8,
    PAL_CHIMNEY = 9,
    PAL_FX = 10,
    PAL_GLOW = 11,
    PAL_ROAD = 12,
    PAL_CRUCIBLE = 13,
    PAL_INGOT = 14
};

struct Art {
    gs::Mipped coke, ladle, slag;
    gs::Mipped stack, gate, chimney, crucible, ingot;
    gs::Mipped soot, shadow, plume;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace foundry
