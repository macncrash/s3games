// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace hornmark {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_BRASS = 2,
    PAL_COAT = 3,
    PAL_SKY = 4,
    PAL_PINE = 5,
    PAL_NOTE = 6,
    PAL_DIM = 7,
    PAL_FACE = 8,
    PAL_MOON = 9,
    PAL_STAFF = 10,
    PAL_BREATH = 11,
    PAL_TITLE = 12,
    PAL_BAD = 13,
    PAL_HILL = 14,
    PAL_SHADE = 15
};

struct Art {
    int font[96] = {};
    gs::Image player;
    gs::Image horn;
    gs::Image bell;
    gs::Image note;
    gs::Image staff;
    gs::Image pine;
    gs::Image moon;
    gs::Image breath;
    gs::Image stamp;
    gs::Image rock;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hornmark
