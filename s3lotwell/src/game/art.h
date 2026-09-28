// S3 LOT WELL pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lotwell {

enum Pal {
    PAL_HUD = 0,
    PAL_DIM = 1,
    PAL_ALERT = 2,
    PAL_OK = 3,
    PAL_GOLD = 4,
    PAL_WATCH = 5,
    PAL_RAM = 6,
    PAL_WELL = 7,
    PAL_PROP = 8,
    PAL_LAMP = 9,
    PAL_FX = 10,
    PAL_LOT = 14
};

struct Art {
    gs::Mipped watch;
    gs::Mipped ram;
    gs::Mipped well;
    gs::Mipped lamp;
    gs::Mipped stall;
    gs::Mipped puff;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
    int asphalt = 0;
    int stripe = 0;
    int curb = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lotwell
