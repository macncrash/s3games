// S3 MUSH GRASS sprites. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mushgrass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TEAM = 1,
    PAL_CREW = 2,
    PAL_GRASS = 3,
    PAL_FLAG = 4,
    PAL_CABIN = 5,
    PAL_TREE = 6,
    PAL_SNOW = 7,
    PAL_BANNER = 8,
    PAL_WIN = 9,
    PAL_ALERT = 10,
    PAL_TUFT = 11,
    PAL_POST = 12,
    PAL_PUFF = 13,
    PAL_SKY = 14,
    PAL_DOG = 15
};

struct Art {
    gs::Mipped sled[8];
    gs::Mipped tuft, tree, cabin, flag, post, puff;
    gs::Mipped title, sub, landed, beaten, lost, paused;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mushgrass
