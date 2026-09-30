// S3 RAILKILO pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace railkilo {

enum Pal {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_SPEEDER = 2,
    PAL_CREW = 3,
    PAL_WHEEL = 4,
    PAL_GANTRY = 5,
    PAL_YARD = 6,
    PAL_RAIL = 7,
    PAL_CLOCK = 8,
    PAL_TITLE = 9
};

struct Art {
    gs::Mipped ride[2];
    gs::Mipped duck;
    gs::Mipped spill;
    gs::Mipped scrap[2];
    gs::Mipped gantry;
    gs::Mipped crew;
    gs::Mipped clock[6];
    gs::Mipped post;
    gs::Image title;
    gs::Image sub;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace railkilo
