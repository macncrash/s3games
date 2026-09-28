// S3 MARKETCHIME pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace marketchime {

enum Pal {
    PAL_HUD = 0,
    PAL_STALL = 1,
    PAL_CLERK = 2,
    PAL_CUST = 3,
    PAL_GOODS = 4,
    PAL_COIN = 5,
    PAL_CLOCK = 6,
    PAL_OK = 7,
    PAL_WARN = 8,
    PAL_BAD = 9,
    PAL_DIM = 10
};

struct Art {
    gs::Mipped awning, post, counter, crate;
    gs::Mipped clerk, buyer, bell, clock;
    gs::Mipped coin[4], good[3];
    gs::Image solid;
    gs::Image title, sub, chime, leave, gone, exact;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace marketchime
