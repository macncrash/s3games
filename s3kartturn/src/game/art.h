// S3 KARTTURN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kartturn {

enum Pal {
    PAL_HUD = 0,
    PAL_KART = 1,
    PAL_SIGN = 2,
    PAL_TITLE = 3,
    PAL_GATE = 4,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped kart[5];
    gs::Mipped tipped;
    gs::Mipped chevL;
    gs::Mipped chevR;
    gs::Mipped gate;
    gs::Image title;
    gs::Image sub;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kartturn
