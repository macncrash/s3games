// Cliff shelf sprites. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cliffkilo {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_MULE = 4,
    PAL_PAN = 5,
    PAL_WHEEL = 6,
    PAL_ROCK = 7,
    PAL_GATE = 8,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped mule;
    gs::Mipped pan;
    gs::Mipped sheave;
    gs::Mipped hub;
    gs::Mipped crag;
    gs::Mipped scrub;
    gs::Mipped post;
    gs::Mipped ribbon;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cliffkilo
