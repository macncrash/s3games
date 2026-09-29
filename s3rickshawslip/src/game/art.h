// S3 RICKSHAW SLIP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rickslip {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SHAW = 1,
    PAL_PIER = 2,
    PAL_LAMP = 3,
    PAL_MARK = 4,
    PAL_DUST = 5,
    PAL_BIRD = 6,
    PAL_TIDE = 7,
    PAL_CANOPY = 8,
    PAL_SHED = 9,
    PAL_MAP = 10,
    PAL_WIN = 11,
    PAL_ALERT = 12,
    PAL_SLIP = 13,
    PAL_QUAY = 14,
    PAL_BANNER = 15
};

struct Art {
    gs::Mipped shaw[8];
    gs::Mipped pile, lamp, cleat, shed, tuft, flag;
    gs::Mipped gull[2];
    gs::Mipped dust, dash, pin, endMark;
    gs::Mipped title, berthed, inSlip, missed, tide, scraped, leg, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rickslip
