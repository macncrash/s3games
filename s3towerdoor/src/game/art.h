// S3 TOWER DOOR pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tower {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WOOD = 2,
    PAL_GUARD = 3,
    PAL_FOE = 4,
    PAL_MOON = 5,
    PAL_GOLD = 6,
    PAL_IRON = 7
};

struct Art {
    gs::Image tower;
    gs::Image doorL, doorR;
    gs::Image guard;
    gs::Image climber;
    gs::Image moon;
    gs::Image flag;
    gs::Image bar;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tower
