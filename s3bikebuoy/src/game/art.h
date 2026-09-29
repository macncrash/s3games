// S3 BIKE BUOY sprites. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bike {

enum Pal {
    PAL_HUD = 0,
    PAL_BIKE = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_GOLD = 4,
    PAL_WOOD = 5,
    PAL_WAKE = 6,
    PAL_SHED = 7,
    PAL_WATER = 12
};

struct Art {
    gs::Mipped bike[8];
    gs::Mipped buoy;
    gs::Mipped quay;
    gs::Mipped pile;
    gs::Mipped shed;
    gs::Mipped lamp;
    gs::Mipped wake;
    gs::Mipped flag;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bike
