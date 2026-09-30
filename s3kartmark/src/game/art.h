// S3 KARTMARK pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kartmark {

enum Pal : int {
    PAL_HUD = 0,
    PAL_KART = 1,
    PAL_DRIVER = 2,
    PAL_ASPHALT = 3,
    PAL_MARK = 4,
    PAL_CONE = 5,
    PAL_STAND = 6,
    PAL_WHEEL = 7,
    PAL_BANNER = 8,
};

struct Art {
    gs::Mipped kart;
    gs::Mipped wheel[2];
    gs::Mipped driver;
    gs::Mipped paint;
    gs::Mipped cone;
    gs::Mipped stand;
    gs::Mipped lamp;
    gs::Mipped banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kartmark
