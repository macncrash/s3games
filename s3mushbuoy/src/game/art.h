// S3 MUSH BUOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mush {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TEAM = 1,
    PAL_CREW = 2,
    PAL_RED = 3,
    PAL_GREEN = 4,
    PAL_AMBER = 5,
    PAL_DOCK = 6,
    PAL_SNOW = 7,
    PAL_TREE = 8,
    PAL_BANNER = 9,
    PAL_WIN = 10,
    PAL_ALERT = 11,
    PAL_SHED = 12,
    PAL_ICE = 13,
    PAL_FLAG = 14,
    PAL_LAMP = 15
};

struct Art {
    gs::Mipped sled[8];
    gs::Mipped buoy, dock, shed, tree, puff, lamp, flag;
    gs::Mipped title, sub, home, beat, missed, clock, paused;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mush
