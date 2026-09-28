// S3 LANTERN SEVEN pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace lanternseven {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GREEN = 3,
    PAL_DIM = 4,
    PAL_YOU = 5,
    PAL_THEM = 6,
    PAL_LAMP = 7,
    PAL_FLAME = 8,
    PAL_POST = 9,
    PAL_MOON = 10,
    PAL_SOLID = 11
};

struct Art {
    gs::Mipped you;
    gs::Mipped lamp;
    gs::Mipped flame;
    gs::Mipped post;
    gs::Mipped moon;
    gs::Mipped solid;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lanternseven
