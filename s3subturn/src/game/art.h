// S3 SUB TURN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace subturn {

enum Pal {
    PAL_HUD = 0,
    PAL_SUB = 1,
    PAL_TRENCH = 2,
    PAL_KELP = 3,
    PAL_WIN = 4,
    PAL_ALERT = 5,
    PAL_BANNER = 6,
    PAL_CREW = 7,
    PAL_MARK = 8
};

struct Art {
    gs::Mipped sub[8];
    gs::Mipped sand, kelp, wreck, buoy, flag, bubble;
    gs::Mipped title, cleared, tipped, crew, trench, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace subturn
