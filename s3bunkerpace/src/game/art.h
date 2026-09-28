// S3 BUNKER PACE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bunkerpace {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_CONCRETE = 4,
    PAL_FIGURE = 5,
    PAL_BAG = 6,
    PAL_FX = 7,
    PAL_NIGHT = 8
};

struct Art {
    gs::Mipped sentry[2];
    gs::Mipped downed;
    gs::Mipped pillar;
    gs::Mipped lintel;
    gs::Mipped bag;
    gs::Mipped lamp;
    gs::Mipped bead;
    gs::Mipped flash;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bunkerpace
