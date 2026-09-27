// S3 HARBOR WELL pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace well {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_KEEPER = 4,
    PAL_STONE = 5,
    PAL_WOOD = 6,
    PAL_SEA = 7,
    PAL_SPRAY = 8,
    PAL_NIGHT = 9,
    PAL_WATER = 12
};

struct Art {
    gs::Mipped well;
    gs::Mipped roof;
    gs::Mipped keeper[2];
    gs::Mipped beam;
    gs::Mipped spray;
    gs::Mipped gull;
    gs::Mipped lamp;
    gs::Mipped crack;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
    int quay = 1;
    int lip = 2;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace well
