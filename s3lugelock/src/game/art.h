// Pictures for the lock chute. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lugelock {

enum Pal {
    PAL_HUD = 0,
    PAL_RIDER = 1,
    PAL_STEEL = 2,
    PAL_POST = 3,
    PAL_FX = 4,
    PAL_TITLE = 5,
    PAL_GOLD = 7,
    PAL_ALERT = 8,
    PAL_SIGN = 9,
    PAL_ICE = 12
};

struct Art {
    gs::Mipped flat, tuck, lean;
    gs::Mipped post, leaf, beam, sign;
    gs::Mipped spark, shadow;
    gs::Image title, sub;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lugelock
