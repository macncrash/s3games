// S3 RICKSHAW PASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CAB = 1,
    PAL_ROCK = 2,
    PAL_PINE = 3,
    PAL_GATE = 4,
    PAL_STORM = 5,
    PAL_ROAD = 12,
    PAL_SNOW = 13,
};

struct Art {
    gs::Mipped cab;
    gs::Mipped puller;
    gs::Mipped rock;
    gs::Mipped pine;
    gs::Mipped gate;
    gs::Mipped banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pass
