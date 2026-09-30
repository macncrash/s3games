// S3 WHARF PACE sprites. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace wharfpace {

enum Pal {
    PAL_TEXT = 0,
    PAL_LAMP = 1,
    PAL_ALERT = 2,
    PAL_LIVE = 3,
    PAL_TIMBER = 4,
    PAL_COAT = 5,
    PAL_ROPE = 6,
    PAL_GULL = 7,
    PAL_CRATE = 8,
    PAL_FX = 9,
    PAL_SKY = 10,
    PAL_PILE = 11,
    PAL_DECK = 12
};

struct Art {
    gs::Mipped keeper[2];
    gs::Mipped downed;
    gs::Mipped pile;
    gs::Mipped lamp;
    gs::Mipped gull[2];
    gs::Mipped crate;
    gs::Mipped coil;
    gs::Mipped sight;
    gs::Mipped flash;
    gs::Mipped splash;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wharfpace
