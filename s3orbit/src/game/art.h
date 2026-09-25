// S3 ORBIT sprites and the station picture. Everything is drawn at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace orbit {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_SCENE = 4,
    PAL_SHIP = 5,
    PAL_ARM = 6,
    PAL_FIRE = 7,
    PAL_HOT = 8,
    PAL_OK = 9
};

struct Art {
    gs::Mipped ship, plume, puff, link, joint, collar, chevron, dot, lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace orbit
