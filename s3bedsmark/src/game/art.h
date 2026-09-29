// S3 BEDSMARK pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bedsmark {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_WARN = 2,
    PAL_GOOD = 3,
    PAL_WOOD = 5,
    PAL_DRY = 6,
    PAL_WET = 7,
    PAL_PLANT = 8,
    PAL_MAN = 9,
    PAL_SUN = 10,
    PAL_YARD = 13,
    PAL_WATER = 14
};

struct Art {
    gs::Mipped frame, soil, stake, stakeUp;
    gs::Mipped plant[3][3];
    gs::Mipped man[2];
    gs::Mipped drop, spark, sun, cloud;
    gs::Mipped title, done;
    int grass = 0;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bedsmark
