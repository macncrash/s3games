// S3 RICKSHAW TURN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rickshawturn {

enum Pal {
    PAL_HUD = 0,
    PAL_CAB = 1,
    PAL_WHEEL = 2,
    PAL_POST = 3,
    PAL_LAMP = 4,
    PAL_SIGN = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped cab, wheel, rider, lamp, chevron, post, shadow, flag;
    gs::Mipped title, job, made, tipped, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rickshawturn
