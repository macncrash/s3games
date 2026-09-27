// S3 BARGE SLIP sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bargeslip {

enum Pal {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_PIER = 2,
    PAL_MARK = 3,
    PAL_WIN = 4,
    PAL_ALERT = 5,
    PAL_BANNER = 6,
    PAL_LAMP = 7,
    PAL_MAP = 8,
    PAL_SLIP = 12,
    PAL_SHORE = 13
};

struct Art {
    gs::Mipped hull[8];
    gs::Mipped pierH, pierV, cleat, lamp, buoy, flag, foam, pin, panel, dot;
    gs::Mipped title, berthed, inSlip, tide, scraped, missed, leg, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bargeslip
