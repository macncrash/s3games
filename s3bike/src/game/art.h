// S3 BIKE pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bike {

enum Pal {
    PAL_INK = 0,
    PAL_ALERT = 1,
    PAL_BIKE = 2,
    PAL_LOW = 3,
    PAL_HIGH = 4,
    PAL_CITY = 5,
    PAL_ROAD = 6,
    PAL_SIGN = 7,
    PAL_DUST = 8,
    PAL_TITLE = 9
};

struct Art {
    gs::Mipped bike[2];
    gs::Mipped duck;
    gs::Mipped crash;
    gs::Mipped wheel[4];
    gs::Mipped lamp;
    gs::Mipped shadow;
    gs::Mipped puff;
    gs::Mipped post[3];
    gs::Mipped gantry;
    gs::Image title;
    gs::Image sub;
    gs::Image rule;
    gs::Image wire;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bike
