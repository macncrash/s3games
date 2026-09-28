// S3 SCULL LANE sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sculllane {

enum Pal {
    PAL_HUD = 0,
    PAL_SHELL = 1,
    PAL_OAR = 2,
    PAL_BUOY = 3,
    PAL_WIN = 4,
    PAL_ALERT = 5,
    PAL_SIGN = 6,
    PAL_CREW = 7,
    PAL_LANE = 12
};

struct Art {
    gs::Mipped shell, oar, buoy, flag, splash, cox;
    gs::Mipped title, held, left, crew, stay, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sculllane
