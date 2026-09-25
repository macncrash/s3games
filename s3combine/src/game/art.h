// S3 COMBINE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace combine {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_RIG = 4,
    PAL_WHEAT = 5,
    PAL_TREE = 6,
    PAL_CLOUD = 7,
    PAL_YARD = 8,
    PAL_LAMP = 9,
    PAL_SUN = 10,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped body;
    gs::Mipped header;
    gs::Mipped fill;
    gs::Mipped bat;
    gs::Mipped ear;
    gs::Mipped chaff;
    gs::Mipped lampOn;
    gs::Mipped lampOff;
    gs::Mipped tree;
    gs::Mipped bale;
    gs::Mipped leg;
    gs::Mipped shadow;
    gs::Mipped sun;
    gs::Mipped cloud;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace combine
