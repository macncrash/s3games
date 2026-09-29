// S3 BEDSBELL pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bedsbell {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_OK = 3,
    PAL_SOIL = 4,
    PAL_LEAF = 5,
    PAL_CAN = 6,
    PAL_BELL = 7,
    PAL_MAN = 8,
    PAL_BAR = 9
};

struct Art {
    int font[96] = {};
    gs::Image bed = {};
    gs::Image sprout = {};
    gs::Image bloom = {};
    gs::Image can = {};
    gs::Image pour = {};
    gs::Image bell = {};
    gs::Image clapper = {};
    gs::Image gardener = {};
    gs::Image solid = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bedsbell
