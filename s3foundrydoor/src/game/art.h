// S3 FOUNDRY DOOR pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace foundrydoor {

enum Pal {
    PAL_TEXT = 0,
    PAL_EMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_IRON = 4,
    PAL_FIGURE = 5,
    PAL_HEAT = 6,
    PAL_SOOT = 7,
    PAL_FX = 8,
    PAL_BRICK = 9,
    PAL_SLAG = 10
};

struct Art {
    gs::Mipped founder[2];
    gs::Mipped door;
    gs::Mipped flame[2];
    gs::Mipped ladle;
    gs::Mipped hook;
    gs::Mipped spark;
    gs::Mipped notch;
    gs::Mipped slab;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace foundrydoor
