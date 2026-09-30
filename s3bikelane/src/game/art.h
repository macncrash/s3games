// S3 BIKE LANE pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lane {

enum Pal {
    PAL_HUD = 0,
    PAL_BIKE = 1,
    PAL_CONE = 2,
    PAL_TREE = 3,
    PAL_TITLE = 4,
    PAL_ALERT = 5,
    PAL_GATE = 6,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped bike[3];
    gs::Mipped shadow;
    gs::Mipped cone;
    gs::Mipped tree;
    gs::Mipped gate;
    gs::Image title;
    gs::Image sub;
    gs::Image rule;
    gs::Image made;
    gs::Image out;
    gs::Image missed;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lane
