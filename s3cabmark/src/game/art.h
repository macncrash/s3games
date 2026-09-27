// S3 CAB MARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cabmark {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CAB = 1,
    PAL_BLOCK = 2,
    PAL_WALK = 3,
    PAL_MARK = 4,
    PAL_END = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_ROAD = 8,
    PAL_SIGN = 9
};

struct Art {
    gs::Mipped cab[16];
    gs::Mipped shade;
    gs::Mipped block[3];
    gs::Mipped lamp;
    gs::Mipped asphalt, walk, mark, ring, endbar, tree;
    gs::Mipped title, set, missed, hold, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cabmark
