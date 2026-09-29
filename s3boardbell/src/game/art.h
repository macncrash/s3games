// Night desk art for S3 BOARD BELL. Drawn into the VDP at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace boardbell {

enum Pal {
    PAL_DESK = 0,
    PAL_LAMP = 1,
    PAL_PLUG = 2,
    PAL_BELL = 3,
    PAL_INK = 4,
    PAL_TITLE = 5,
    PAL_WIN = 6,
    PAL_DEAD = 7,
    PAL_HINT = 8,
    PAL_CORD = 9
};

constexpr int JACKS = 6;

struct Art {
    gs::Image desk;
    gs::Image lamp;
    gs::Image plug;
    gs::Image bell;
    gs::Image badge;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace boardbell
