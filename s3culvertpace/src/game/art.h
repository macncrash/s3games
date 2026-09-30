// Pictures for one culvert, drawn into the S3-16 at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace culvertpace {

enum Pal {
    PAL_TEXT = 0,
    PAL_PIPE = 1,
    PAL_MOSS = 2,
    PAL_WATER = 3,
    PAL_FIGURE = 4,
    PAL_ALERT = 5,
    PAL_GOOD = 6,
    PAL_FLASH = 7,
    PAL_SIGHT = 8,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped ring;
    gs::Mipped walker;
    gs::Mipped fallen;
    gs::Mipped drip;
    gs::Mipped flash;
    gs::Mipped sight;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace culvertpace
