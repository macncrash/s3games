// S3 PLOW pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace plow {

enum Pal {
    PAL_INK = 0,
    PAL_TRUCK = 1,
    PAL_SIGN = 2,
    PAL_TREE = 3,
    PAL_DRIFT = 4,
    PAL_ROCK = 5,
    PAL_FX = 6,
    PAL_TITLE = 7,
    PAL_MOUNT = 8,
    PAL_AMBER = 9,
    PAL_ALARM = 10,
    PAL_CUT = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped truckDown, truckUp;
    gs::Mipped drift[2];
    gs::Mipped spruce[3];
    gs::Mipped rock[2];
    gs::Mipped post;
    gs::Mipped gateRoll, gateSummit;
    gs::Mipped flake, shadow;
    gs::Image titleCard, titleSub;
    gs::Image count[4];
    gs::Image plowed;
    gs::Image winText, loseStorm, loseSnow, loseBuried;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace plow
