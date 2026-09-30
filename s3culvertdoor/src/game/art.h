// Pictures for one culvert door, drawn into the S3-16 at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace culvertdoor {

enum Pal {
    PAL_HUD = 0,
    PAL_PIPE = 1,
    PAL_DOOR = 2,
    PAL_BODY = 3,
    PAL_WATER = 4,
    PAL_WARN = 5,
    PAL_FIG = 6
};

struct Art {
    gs::Mipped leaf;
    gs::Mipped shoulder;
    gs::Mipped figure;
    gs::Mipped chev;
    gs::Mipped sheet;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace culvertdoor
