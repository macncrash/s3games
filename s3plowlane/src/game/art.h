// S3 PLOWLANE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace plow {

enum Pal {
    PAL_HUD = 0,
    PAL_WARN = 1,
    PAL_GOOD = 2,
    PAL_PLOW = 3,
    PAL_POST = 4,
    PAL_GATE = 5,
    PAL_FIELD = 6,
    PAL_TITLE = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped plow;
    gs::Mipped share;
    gs::Mipped post;
    gs::Mipped bale;
    gs::Mipped gate;
    gs::Image title;
    gs::Image sub;
    gs::Image tag;
    gs::Image won;
    gs::Image lost;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace plow
