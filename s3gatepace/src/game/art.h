// S3 GATE PACE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pace {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_FIGURE = 5,
    PAL_HOLD = 6,
    PAL_LIVE = 7,
    PAL_TREE = 8,
    PAL_FX = 9,
    PAL_NIGHT = 10,
    PAL_METAL = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped walk[2];
    gs::Mipped fallen;
    gs::Mipped pier;
    gs::Mipped block;
    gs::Mipped keystone;
    gs::Mipped bollard;
    gs::Mipped tree;
    gs::Mipped bead;
    gs::Mipped barrel;
    gs::Mipped lamp;
    gs::Mipped pip;
    gs::Mipped flash;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped stripe;
    gs::Mipped moon;
    gs::Mipped cloud;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pace
