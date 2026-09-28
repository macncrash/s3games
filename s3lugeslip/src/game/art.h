// S3 LUGE SLIP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace luge {

enum Pal : int {
    PAL_HUD = 0,
    PAL_POD = 1,
    PAL_TIMBER = 2,
    PAL_POST = 3,
    PAL_END = 4,
    PAL_SPRAY = 5,
    PAL_BIRD = 6,
    PAL_SNOW = 7,
    PAL_LAMP = 8,
    PAL_SHED = 9,
    PAL_BANNER = 10,
    PAL_WIN = 11,
    PAL_ALERT = 12,
    PAL_ICE = 13,
    PAL_TIDE = 14,
    PAL_MARK = 15
};

struct Art {
    gs::Mipped pod[8];
    gs::Mipped plank, cap, post, cleat, shed, lamp;
    gs::Mipped stripe, flag, crack, spray, bird[2];
    gs::Mipped title, slipWord, berthed, missed, tide, wall, paused, endMark;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace luge
