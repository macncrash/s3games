// S3 BIKETURN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace biketurn {

enum Pal {
    PAL_HUD = 0,
    PAL_BIKE = 1,
    PAL_SIGN = 2,
    PAL_TITLE = 3,
    PAL_DUST = 4,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped bike[5];
    gs::Mipped tumble;
    gs::Mipped chevL;
    gs::Mipped chevR;
    gs::Image title;
    gs::Image sub;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace biketurn
