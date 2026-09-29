// Pictures drawn at boot. The gold stamp is the only finished mark.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace anvilmark {

enum Pal {
    PAL_HUD = 0,
    PAL_IRON = 1,
    PAL_FIRE = 2,
    PAL_GOLD = 3,
    PAL_WOOD = 4,
    PAL_SPARK = 5,
    PAL_NIGHT = 6,
    PAL_SMITH = 7
};

struct Art {
    gs::Mipped anvil;
    gs::Mipped hammer;
    gs::Mipped bar;
    gs::Mipped notch;
    gs::Mipped spark;
    gs::Mipped smith;
    gs::Image title;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace anvilmark
