// S3 FOUNDRY CLER pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace fcler {

enum Pal {
    PAL_TEXT = 0,
    PAL_IRON = 1,
    PAL_EMBER = 2,
    PAL_SLAG = 3,
    PAL_BRICK = 4,
    PAL_CREW = 5,
    PAL_CLOCK = 6,
    PAL_GOOD = 7,
    PAL_ALERT = 8,
    PAL_SOOT = 9
};

struct Art {
    gs::Mipped crew[2];
    gs::Mipped rake;
    gs::Mipped slag;
    gs::Mipped furnace;
    gs::Mipped clock;
    gs::Mipped ladle;
    gs::Mipped link;
    gs::Mipped spark;
    gs::Mipped bin;
    int font[96] = {};
    int floor = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fcler
