// S3 CONVOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace convoy {

enum Pal {
    PAL_WHITE = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_TRUCK = 4,
    PAL_JEEP = 5,
    PAL_RAID = 6,
    PAL_HAZ = 7,
    PAL_FX = 8,
    PAL_PROP = 9,
    PAL_CLOUD = 10,
    PAL_MESA = 11,
    PAL_ROAD = 12,
    PAL_DIM = 14
};

struct Art {
    gs::Mipped truck, jeep, bike, wreck, mine;
    gs::Mipped boom[3];
    gs::Mipped shot, shadow, puff, cactus, butte, arch, arrow, reticle, quad;
    gs::Mipped font[96];
    gs::Mipped title, sub1, sub2, help1, help2, roll;
    gs::Mipped arrived, ends, paused;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace convoy
