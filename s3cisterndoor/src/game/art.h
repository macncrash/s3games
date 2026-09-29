// Cistern door pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cistern {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WATER = 2,
    PAL_WOOD = 3,
    PAL_BODY = 4,
    PAL_ALARM = 5
};

struct Art {
    gs::Image arch;
    gs::Image water;
    gs::Image door;
    gs::Image iron;
    gs::Image body;
    gs::Image shoulder;
    gs::Image arrow;
    gs::Image drip;
    gs::Image bar;
    gs::Image word;
    gs::Image glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cistern
