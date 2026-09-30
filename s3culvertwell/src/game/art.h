// S3 CULVERTWELL sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace culvertwell {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_KEEPER = 2,
    PAL_FOE = 3,
    PAL_WELL = 4,
    PAL_BARREL = 5,
    PAL_RAM = 6,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped keeper;
    gs::Mipped swing;
    gs::Mipped sapper;
    gs::Mipped barrel;
    gs::Mipped ram;
    gs::Mipped well[3];
    gs::Mipped archL;
    gs::Mipped archR;
    gs::Mipped lintel;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace culvertwell
