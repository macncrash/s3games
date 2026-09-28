// S3 PLOWSLIP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace plow {

enum Pal {
    PAL_HUD = 0,
    PAL_PIER = 1,
    PAL_PLOW = 2,
    PAL_CREW = 3,
    PAL_BUOY = 4,
    PAL_WAKE = 5,
    PAL_MUD = 6,
    PAL_FOAM = 7
};

struct Art {
    gs::Mipped plow[5];
    gs::Mipped crewBoat;
    gs::Mipped pier;
    gs::Mipped bulk;
    gs::Mipped buoy;
    gs::Mipped crew;
    gs::Mipped wake;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace plow
