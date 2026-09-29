// S3 MAIL VAN TURN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mailvanturn {

enum Pal {
    PAL_HUD = 0,
    PAL_VAN = 1,
    PAL_BOX = 2,
    PAL_HOUSE = 3,
    PAL_LAMP = 4,
    PAL_SIGN = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped van, wheel, shade, box, house, lamp, chevron, post;
    gs::Mipped title, job, upright, tipped, missed, clock, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mailvanturn
