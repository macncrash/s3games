// S3 SUBSLIP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace slip {

enum Pal {
    PAL_HUD = 0,
    PAL_SEA = 1,
    PAL_SUB = 2,
    PAL_QUAY = 3,
    PAL_BUOY = 4,
    PAL_LAMP = 5,
    PAL_KELP = 6,
    PAL_FX = 7
};

struct Art {
    gs::Mipped sub;
    gs::Mipped prop;
    gs::Mipped piling;
    gs::Mipped quay;
    gs::Mipped buoy;
    gs::Mipped bubble;
    gs::Mipped kelp;
    gs::Mipped gull;
    gs::Mipped lamp;
    gs::Image skyline;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace slip
