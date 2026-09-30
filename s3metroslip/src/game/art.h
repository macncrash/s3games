// S3 METRO SLIP sprites. Painted into VDP ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace metroslip {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_WAKE = 2,
    PAL_PILE = 3,
    PAL_WOOD = 4,
    PAL_MARK = 5,
    PAL_GULL = 6,
    PAL_LAMP = 7,
    PAL_TIDE = 8,
    PAL_MAP = 9,
    PAL_WIN = 10,
    PAL_ALERT = 11,
    PAL_FIELD = 12,
    PAL_SLIP = 13,
    PAL_BANNER = 14,
    PAL_WATER = 15
};

struct Art {
    gs::Mipped hull, wake, pile, lamp, flag, dash, fender, gull[2], dot, pin;
    gs::Mipped title, berthed, inSlip, missed, tide, legFail, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace metroslip
