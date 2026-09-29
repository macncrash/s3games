// S3 RICKSHAW LANE sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rickshawlane {

enum Pal {
    PAL_HUD = 0,
    PAL_CAB = 1,
    PAL_WHEEL = 2,
    PAL_STALL = 3,
    PAL_LAMP = 4,
    PAL_SIGN = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped cab, wheel, rider, lamp, stall, arch, shadow;
    gs::Mipped title, held, left, missed, stay, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rickshawlane
