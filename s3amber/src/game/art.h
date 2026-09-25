// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace amber {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_LAW = 4,
    PAL_RUN = 5,
    PAL_BRAKE = 6,
    PAL_COP = 7,
    PAL_SIGNAL = 8,
    PAL_LAMP_R = 9,
    PAL_LAMP_A = 10,
    PAL_LAMP_G = 11,
    PAL_ROAD = 12,
    PAL_BLOCK = 13,
    PAL_FX = 14,
    PAL_DIM = 15
};

struct Art {
    int font[96] = {};
    gs::Mipped title, clear, over;
    gs::Mipped car[4];  // 0 north, 1 east, 2 south, 3 west
    gs::Mipped cop[2];
    gs::Mipped signal, lamp;
    gs::Mipped white, amberLine, asphalt, pad, shadow, ring, bracket;
    gs::Mipped block[4];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace amber
