// S3 TOWER PURSE sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tpurs {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_RED = 2,
    PAL_TEAL = 3,
    PAL_STONE = 4,
    PAL_LAMP = 5,
    PAL_FX = 6,
    PAL_SPARK = 7,
    PAL_YARD = 12
};

struct Art {
    gs::Mipped cab;
    gs::Mipped crawler;
    gs::Mipped hauler;
    gs::Mipped scout;
    gs::Mipped tower;
    gs::Mipped lamp;
    gs::Mipped puff;
    gs::Mipped spark;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tpurs
