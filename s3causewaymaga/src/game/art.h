// S3 CAUSEWAY MAGA pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cwmaga {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_FOE = 2,
    PAL_SKIFF = 3,
    PAL_BRASS = 4,
    PAL_FX = 5,
    PAL_OK = 6,
    PAL_ALERT = 7,
    PAL_STONE = 8
};

struct Art {
    gs::Mipped gunner[2];
    gs::Mipped runner[2];
    gs::Mipped skiff;
    gs::Mipped lamp;
    gs::Mipped brass;
    gs::Mipped flash;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cwmaga
