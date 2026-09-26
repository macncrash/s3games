// S3 DEPOT PACE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace depotpace {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_BRICK = 4,
    PAL_FIGURE = 5,
    PAL_HOLD = 6,
    PAL_LIVE = 7,
    PAL_WOOD = 8,
    PAL_FX = 9,
    PAL_SOOT = 10,
    PAL_METAL = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped walk[2];
    gs::Mipped fallen;
    gs::Mipped shed;
    gs::Mipped girder;
    gs::Mipped sign;
    gs::Mipped clock[2];
    gs::Mipped lamp;
    gs::Mipped crate[3];
    gs::Mipped boxcar;
    gs::Mipped loco;
    gs::Mipped drum;
    gs::Mipped sack;
    gs::Mipped post;
    gs::Mipped stack;
    gs::Mipped tower;
    gs::Mipped rifle;
    gs::Mipped bead;
    gs::Mipped pip;
    gs::Mipped flash;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped stripe;
    gs::Mipped steam;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace depotpace
