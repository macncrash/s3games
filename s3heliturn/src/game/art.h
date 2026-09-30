// S3 HELITURN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace heliturn {

enum Pal {
    PAL_HUD = 0,
    PAL_HELI = 1,
    PAL_SIGN = 2,
    PAL_TITLE = 3,
    PAL_RIDGE = 4,
    PAL_PAD = 5
};

struct Art {
    gs::Mipped heli[5][2];
    gs::Mipped wreck;
    gs::Mipped chevL;
    gs::Mipped chevR;
    gs::Mipped ridge;
    gs::Mipped pad;
    gs::Image title;
    gs::Image sub;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace heliturn
