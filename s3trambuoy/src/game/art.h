// S3 TRAMBUOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tram {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TRAM = 1,
    PAL_DOCK = 2,
    PAL_BUOY0 = 3,
    PAL_BUOY1 = 4,
    PAL_BUOY2 = 5,
    PAL_WAKE = 6,
    PAL_MARK = 7,
    PAL_BANNER = 8,
    PAL_WIN = 9,
    PAL_ALERT = 10,
};

struct Art {
    gs::Mipped hull[8];
    gs::Mipped dock;
    gs::Mipped buoy;
    gs::Mipped wake;
    gs::Mipped ring;
    gs::Mipped title;
    gs::Mipped home;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tram
