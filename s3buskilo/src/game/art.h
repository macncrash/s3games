// S3 BUSKILO pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kilo {

enum Pal {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_BUS = 2,
    PAL_CREW = 3,
    PAL_WHEEL = 4,
    PAL_HANG = 5,
    PAL_CITY = 6,
    PAL_ROAD = 7,
    PAL_CLOCK = 8,
    PAL_TITLE = 9
};

struct Art {
    gs::Mipped ride[2];
    gs::Mipped duck;
    gs::Mipped spill;
    gs::Mipped wheel[2];
    gs::Mipped hang;
    gs::Mipped crew;
    gs::Mipped clock[8];
    gs::Mipped post;
    gs::Image title;
    gs::Image sub;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kilo
