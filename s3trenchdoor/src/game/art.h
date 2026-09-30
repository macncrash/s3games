// Pictures for the trench gate. Drawn into VRAM and sprite ROM at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace trench {

enum Pal {
    PAL_HUD = 0,
    PAL_SCENE = 1,
    PAL_GATE = 2,
    PAL_SKY = 3,
    PAL_SOLDIER = 4,
    PAL_WARN = 5,
    PAL_FOE = 6,
    PAL_FLARE = 7
};

struct Art {
    gs::Mipped gate;
    gs::Mipped shoulder;
    gs::Mipped foe;
    gs::Mipped flare;
    gs::Mipped chev;
    gs::Mipped post;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace trench
