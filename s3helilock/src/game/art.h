// S3 HELILOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace helilock {

enum Pal {
    PAL_HUD = 0,
    PAL_SHIP = 1,
    PAL_GATE = 2,
    PAL_WORLD = 3,
    PAL_FX = 4
};

struct Art {
    gs::Mipped body;
    gs::Mipped rotor[3];
    gs::Mipped gate;
    gs::Mipped wall;
    gs::Mipped water;
    gs::Mipped pad;
    gs::Mipped cloud;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace helilock
