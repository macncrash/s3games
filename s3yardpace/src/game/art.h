// S3 YARD PACE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace yardpace {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_WOOD = 4,
    PAL_FIGURE = 5,
    PAL_HOLD = 6,
    PAL_LIVE = 7,
    PAL_IRON = 8,
    PAL_FX = 9,
    PAL_SKY = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped hand[2];
    gs::Mipped fallen;
    gs::Mipped crib;
    gs::Mipped balk;
    gs::Mipped stake;
    gs::Mipped beam;
    gs::Mipped hoist;
    gs::Mipped coil;
    gs::Mipped keg;
    gs::Mipped horse;
    gs::Mipped post;
    gs::Mipped lantern;
    gs::Mipped barrow;
    gs::Mipped pennant[2];
    gs::Mipped bead;
    gs::Mipped rifle;
    gs::Mipped pip;
    gs::Mipped flash;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped stripe;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace yardpace
