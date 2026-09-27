// S3 ALLEY DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace alleydawn {

enum Pal {
    PAL_HUD = 0,
    PAL_BRICK = 1,
    PAL_WET = 2,
    PAL_MAN = 3,
    PAL_FIRE = 4,
    PAL_EMBER = 5,
    PAL_IRON = 6,
    PAL_TARP = 7,
    PAL_CAN = 8,
    PAL_SMOKE = 9,
    PAL_MOON = 10,
    PAL_SUN = 11,
    PAL_GOLD = 12,
    PAL_ALERT = 13,
    PAL_PIP = 14,
    PAL_LAMP = 15
};

struct Art {
    gs::Mipped man[2];
    gs::Mipped pot;
    gs::Mipped flame[2];
    gs::Mipped tarp;
    gs::Mipped can[2];
    gs::Mipped spark;
    gs::Mipped moon;
    gs::Mipped glyph[96];
    int font[96] = {};
    int brick = 0;
    int mortar = 0;
    int wet = 0;
    int stripe = 0;
    int dark = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace alleydawn
