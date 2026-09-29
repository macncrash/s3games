// Yard, beds, and wall clock. Drawn into the VDP at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bedschime {

enum Pal {
    PAL_YARD = 0,
    PAL_BED = 1,
    PAL_WET = 2,
    PAL_PLANT = 3,
    PAL_CAN = 4,
    PAL_CLOCK = 5,
    PAL_INK = 6,
    PAL_TITLE = 7,
    PAL_WIN = 8,
    PAL_DEAD = 9,
    PAL_HINT = 10,
    PAL_DROP = 11
};

constexpr int kBeds = 6;

struct Art {
    gs::Image bed;
    gs::Image soil;
    gs::Image plant;
    gs::Image can;
    gs::Image clock;
    gs::Image hand;
    gs::Image drop;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bedschime
