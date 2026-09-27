// S3 HARBOR DOOR sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace harbordoor {

enum Pal {
    PAL_HUD = 0,
    PAL_IRON = 1,
    PAL_GUN = 2,
    PAL_SKIFF = 3,
    PAL_RAM = 4,
    PAL_SHOT = 5,
    PAL_FOAM = 6,
    PAL_GOLD = 7,
    PAL_ALERT = 8,
    PAL_WATER = 12
};

struct Art {
    gs::Mipped door;
    gs::Mipped gun;
    gs::Mipped skiff;
    gs::Mipped ram;
    gs::Mipped bolt;
    gs::Mipped splash;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace harbordoor
