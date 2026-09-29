// Mail van kilometer sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mailkilo {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_VAN = 4,
    PAL_WHEEL = 5,
    PAL_SACK = 6,
    PAL_BOX = 7,
    PAL_LAMP = 8,
    PAL_POST = 9,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped hood, wheel, sack, box, lamp, post, banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mailkilo
