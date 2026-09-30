// S3 BUSBUOY pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace buoy {

enum Pal : int {
    PAL_HUD = 0,
    PAL_BUS = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_GOLD = 4,
    PAL_DOCK = 5,
    PAL_WAKE = 6,
    PAL_ROCK = 7,
    PAL_SHED = 8,
    PAL_OK = 9,
};

struct Art {
    gs::Image font[96];
    gs::Image bus[5];  // N NE E SE S; west headings flip
    gs::Image mark;
    gs::Image dock;
    gs::Image shed;
    gs::Image wake;
    gs::Image rock;
    gs::Image lamp;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace buoy
