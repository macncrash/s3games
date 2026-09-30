// S3 KART KILO sprites. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kartkilo {

enum Pal : int {
    PAL_HUD = 0,
    PAL_KART = 1,
    PAL_RIVAL = 2,
    PAL_WHEEL = 3,
    PAL_ROAD = 4,
    PAL_BANNER = 5,
    PAL_WIN = 6,
    PAL_ALERT = 7,
    PAL_PUFF = 8,
    PAL_POST = 9
};

struct Art {
    gs::Mipped kart;
    gs::Mipped rival;
    gs::Mipped wheel[4];
    gs::Mipped road;
    gs::Mipped ribbon;
    gs::Mipped puff;
    gs::Mipped title;
    gs::Mipped clean;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kartkilo
