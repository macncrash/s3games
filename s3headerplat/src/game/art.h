// S3 HEADER PLAT sprites. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace headerplat {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CAR = 1,
    PAL_DOCK = 2,
    PAL_POST = 3,
    PAL_TREE = 4,
    PAL_SIGN = 5,
    PAL_ROAD = 12,
};

struct Art {
    gs::Mipped car;
    gs::Mipped dock;
    gs::Mipped post;
    gs::Mipped tree;
    gs::Mipped sign;
    gs::Mipped title;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace headerplat
