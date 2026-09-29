// S3 VIADUCT PACE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace viaductpace {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_FIGURE = 5,
    PAL_HOLD = 6,
    PAL_LIVE = 7,
    PAL_IRON = 8,
    PAL_FX = 9,
    PAL_TRAIN = 10,
    PAL_METAL = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped walk[2];
    gs::Mipped fallen;
    gs::Mipped arch;
    gs::Mipped pier;
    gs::Mipped post;
    gs::Mipped lamp;
    gs::Mipped board[3];
    gs::Mipped train;
    gs::Mipped rail;
    gs::Mipped cloud;
    gs::Mipped rifle;
    gs::Mipped bead;
    gs::Mipped pip;
    gs::Mipped flash;
    gs::Mipped spray;
    gs::Mipped shadow;
    gs::Mipped stripe;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace viaductpace
