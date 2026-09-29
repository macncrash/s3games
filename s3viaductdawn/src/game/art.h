// S3 VIADUCT DAWN pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace viaduct {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_MAN = 2,
    PAL_FIRE = 3,
    PAL_EMBER = 4,
    PAL_IRON = 5,
    PAL_CASK = 6,
    PAL_MOON = 7,
    PAL_GOLD = 8,
    PAL_WATER = 9,
    PAL_ALERT = 10,
    PAL_SPARK = 11,
    PAL_PIER = 14,
    PAL_HINT = 15
};

struct Art {
    gs::Mipped man[2];
    gs::Mipped basket;
    gs::Mipped flame[2];
    gs::Mipped cask;
    gs::Mipped pier;
    gs::Mipped span;
    gs::Mipped spark;
    gs::Mipped moon;
    gs::Mipped glyph[96];
    int font[96] = {};
    int deck = 0;
    int joint = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace viaduct
