// S3 KEEL LANE sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keellane {

enum Pal {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_SAIL = 2,
    PAL_MARK = 3,
    PAL_WIN = 4,
    PAL_ALERT = 5,
    PAL_SIGN = 6,
    PAL_CREW = 7,
    PAL_LANE = 12
};

struct Art {
    gs::Mipped hull, sail, boom, wake, buoy, gate;
    gs::Mipped title, held, left, crew, stay, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keellane
