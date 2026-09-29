// S3 MUSH LANE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mushlane {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TEAM = 1,
    PAL_PINE = 2,
    PAL_STAKE = 3,
    PAL_BANNER = 4,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped dogs, sled, shade;
    gs::Mipped pine, stake, arch;
    gs::Mipped title, stay, held, left, late, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mushlane
