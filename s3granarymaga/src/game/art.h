// S3 GRANARY MAGA pictures. Drawn into the VDP at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace grmaga {

enum Pal {
    PAL_HUD = 0,
    PAL_WATCH = 1,
    PAL_RAIDER = 2,
    PAL_SACK = 3,
    PAL_GRAIN = 4,
    PAL_FLASH = 5,
    PAL_HELD = 6,
    PAL_LOST = 7,
    PAL_TIMBER = 8
};

struct Art {
    gs::Mipped watcher[2];
    gs::Mipped raider[2];
    gs::Mipped sack;
    gs::Mipped silo;
    gs::Mipped door;
    gs::Mipped round;
    gs::Mipped flash;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace grmaga
