// S3 HEADER LANE pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace headerlane {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_SAIL = 2,
    PAL_BUOY = 3,
    PAL_MARK = 4,
    PAL_SIGN = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_SEA = 12
};

struct Art {
    gs::Mipped hull, sail, jib, shade;
    gs::Mipped buoy, mark, flag;
    gs::Mipped title, sub, took, missed, start, header;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace headerlane
