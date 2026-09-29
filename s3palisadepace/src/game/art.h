// S3 PALISADE PACE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace palisadepace {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_TIMBER = 4,
    PAL_FIGURE = 5,
    PAL_HOLD = 6,
    PAL_LIVE = 7,
    PAL_PINE = 8,
    PAL_FX = 9,
    PAL_CLOTH = 10,
    PAL_IRON = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped walk[2];
    gs::Mipped fallen;
    gs::Mipped stake;
    gs::Mipped gate;
    gs::Mipped banner;
    gs::Mipped torch;
    gs::Mipped pine;
    gs::Mipped drum;
    gs::Mipped musket;
    gs::Mipped bead;
    gs::Mipped pip;
    gs::Mipped flash;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped stripe;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace palisadepace
