// S3 KILN SEVEN pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kilnseven {

enum Pal {
    PAL_HUD = 0,
    PAL_CLAY = 1,
    PAL_RIVAL = 2,
    PAL_BRICK = 3,
    PAL_FIRE = 4,
    PAL_ASH = 5,
    PAL_GOOD = 6,
    PAL_BAD = 7,
    PAL_INK = 8
};

struct Art {
    gs::Image kiln;
    gs::Image kilnFar;
    gs::Image pot;
    gs::Image crack;
    gs::Image flame;
    gs::Image cone;
    gs::Image bar;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kilnseven
