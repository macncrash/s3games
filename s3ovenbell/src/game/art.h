// S3 OVENBELL pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace ovenbell {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_OK = 3,
    PAL_OVEN = 4,
    PAL_LOAF = 5,
    PAL_PALE = 6,
    PAL_CRUST = 7,
    PAL_CHAR = 8,
    PAL_FIRE = 9,
    PAL_PEEL = 10,
    PAL_BELL = 11,
    PAL_CLAP = 12
};

struct Art {
    gs::Image arch;
    gs::Image loaf;
    gs::Image bell;
    gs::Image clapper;
    gs::Image flame;
    gs::Image peel;
    gs::Image pip;
    gs::Image bar;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ovenbell
