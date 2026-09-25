// S3 TANK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tank {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_PLAYER = 4,
    PAL_RIVAL = 5,
    PAL_FX = 6,
    PAL_BLDG = 7,
    PAL_LAMP = 8,
    PAL_SMOKE = 9,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped rear[2];
    gs::Mipped front;
    gs::Mipped bldg[3];
    gs::Mipped lamp;
    gs::Mipped hydrant;
    gs::Mipped shell;
    gs::Mipped flash;
    gs::Mipped smoke;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tank
