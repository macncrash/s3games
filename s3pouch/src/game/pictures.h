// S3 POUCH pictures. Drawn into VRAM and sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace pouch {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CITY = 1,
    PAL_HERO = 2,
    PAL_RED = 3,
    PAL_BLUE = 4,
    PAL_TAXI = 5,
    PAL_VAN = 6,
    PAL_VAN2 = 7,
    PAL_BUS = 8,
    PAL_BUS2 = 9,
    PAL_PROP = 10,
    PAL_TITLE = 11,
    PAL_ALARM = 12,
    PAL_SAFE = 13,
    PAL_GOLD = 14,
    PAL_FX = 15
};

struct Art {
    gs::Mipped hero[2];
    gs::Mipped bare[2];
    gs::Mipped satchel;
    gs::Mipped sedan, van, bus;
    gs::Mipped lamp, tree, shadow, puff;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pouch
