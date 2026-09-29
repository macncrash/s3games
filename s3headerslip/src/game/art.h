// S3 HEADER SLIP sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace headerslip {

enum Pal {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_RIVAL = 2,
    PAL_PIER = 3,
    PAL_MARK = 4,
    PAL_WIN = 5,
    PAL_ALERT = 6,
    PAL_BANNER = 7,
    PAL_WIND = 8,
    PAL_WATER = 12,
    PAL_SHORE = 13
};

struct Art {
    gs::Mipped hull[8];
    gs::Mipped vane[8];
    gs::Mipped pier, post, buoy, foam, cleat;
    gs::Mipped title, take, berthed, crew, tide, wall, aground;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace headerslip
