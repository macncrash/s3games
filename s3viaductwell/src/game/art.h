// S3 VIADUCT WELL sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace well {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_FOE = 2,
    PAL_RAM = 3,
    PAL_WELL = 4,
    PAL_STONE = 5,
    PAL_SHOT = 6,
    PAL_OK = 7,
    PAL_BAD = 8
};

struct Art {
    gs::Mipped sentry[2];
    gs::Mipped raider[2];
    gs::Mipped rammer[2];
    gs::Mipped well;
    gs::Mipped pier;
    gs::Mipped bolt;
    gs::Mipped crack;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace well
