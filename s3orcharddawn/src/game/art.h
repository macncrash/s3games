// S3 ORCHARD DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace orcharddawn {

enum Pal {
    PAL_HUD = 0,
    PAL_GRASS = 1,
    PAL_BARK = 2,
    PAL_LEAF = 3,
    PAL_MAN = 4,
    PAL_FIRE = 5,
    PAL_APPLE = 6,
    PAL_WIND = 7,
    PAL_IRON = 8,
    PAL_MOON = 9,
    PAL_SUN = 10,
    PAL_GOLD = 11,
    PAL_ALERT = 12,
    PAL_EMBER = 13,
    PAL_SMOKE = 14,
    PAL_PIP = 15
};

struct Art {
    gs::Mipped tree;
    gs::Mipped man[2];
    gs::Mipped pot;
    gs::Mipped flame[2];
    gs::Mipped apple;
    gs::Mipped leaf;
    gs::Mipped spark;
    gs::Mipped shade;
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped glyph[96];
    int font[96] = {};
    int bar = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace orcharddawn
