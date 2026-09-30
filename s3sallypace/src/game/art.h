#pragma once
#include "console/system.h"

namespace sally {

enum {
    PAL_INK = 1,
    PAL_GOLD = 2,
    PAL_BAD = 3,
    PAL_GOOD = 4,
    PAL_YOU = 5,
    PAL_FOE = 6,
    PAL_FX = 7,
    PAL_SUN = 8,
    PAL_GND = 9,
};

struct Art {
    gs::Image body;
    gs::Image gun;
    gs::Image smoke;
    gs::Image sun;
    gs::Image tree;
    gs::Image pip;
    int fontBase = 1;

    void build(gs::VDP& vdp);
};

}  // namespace sally
