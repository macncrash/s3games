// S3 HELIBUOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace buoy {

enum Pal : int {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_WIN = 2,
    PAL_BANNER = 3,
    PAL_HELI = 4,
    PAL_DOCK = 5,
    PAL_BUOY = 6,
    PAL_BUOY2 = 7,
    PAL_BUOY3 = 8,
    PAL_WAKE = 9,
    PAL_SHORE = 10,
    PAL_FLAG = 11,
    PAL_ROTOR = 12,
    PAL_SUN = 13,
    PAL_MARK = 14,
    PAL_DIM = 15,
};

struct Art {
    gs::Mipped heli;
    gs::Mipped rotor;
    gs::Mipped shadow;
    gs::Mipped dock;
    gs::Mipped buoy;
    gs::Mipped flag;
    gs::Mipped wake;
    gs::Mipped shore;
    gs::Mipped gull;
    gs::Mipped title;
    gs::Mipped docked;
    gs::Mipped late;
    gs::Mipped dipped;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace buoy
