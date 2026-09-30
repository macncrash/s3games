// S3 METRO LANE pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace metro {

enum Pal {
    PAL_HUD = 0,
    PAL_CAR = 1,
    PAL_LAMP = 2,
    PAL_PILLAR = 3,
    PAL_TITLE = 4,
    PAL_ALERT = 5,
    PAL_STOP = 6,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped car[3];
    gs::Mipped shadow;
    gs::Mipped lamp;
    gs::Mipped pillar;
    gs::Mipped stop;
    gs::Image title;
    gs::Image sub;
    gs::Image rule;
    gs::Image made;
    gs::Image out;
    gs::Image missed;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace metro
