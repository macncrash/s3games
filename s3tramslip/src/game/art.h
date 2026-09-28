// S3 TRAM SLIP sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tramslip {

enum Pal {
    PAL_HUD = 0,
    PAL_TRAM = 1,
    PAL_QUAY = 2,
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
    gs::Mipped tram[8];
    gs::Mipped quayH, quayV, cleat, lamp, buoy, flag, foam, pin, panel, dot, rail, bell;
    gs::Mipped title, berthed, inSlip, tide, scraped, missed, leg, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tramslip
