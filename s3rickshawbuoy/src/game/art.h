// S3 RICKSHAW BUOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rick {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SHAW = 1,
    PAL_NUN = 2,
    PAL_CAN = 3,
    PAL_DOCK = 4,
    PAL_DUST = 5,
    PAL_BIRD = 6,
    PAL_CRATE = 7,
    PAL_ROAD = 8,
    PAL_STALL = 9,
    PAL_BANNER = 10,
    PAL_WIN = 11,
    PAL_ALERT = 12,
    PAL_SMOKE = 13,
    PAL_MAP = 14,
    PAL_LAMP = 15
};

struct Art {
    gs::Mipped shaw[8];
    gs::Mipped buoy[3];
    gs::Mipped quay, quayB, stall, pile, sign, lampPost;
    gs::Mipped wrong, crate, bird[2], dust, smoke, ring, lamp, pin, dot, panel;
    gs::Mipped title, sub, same, missed, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rick
