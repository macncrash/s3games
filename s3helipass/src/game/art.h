// S3 HELIPASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace helipass {

enum Pal {
    PAL_HUD = 0,
    PAL_SHIP = 1,
    PAL_ROCK = 2,
    PAL_SNOW = 3,
    PAL_FX = 4
};

struct Art {
    gs::Mipped body;
    gs::Mipped rotor[3];
    gs::Mipped rock;
    gs::Mipped snow;
    gs::Mipped pad;
    gs::Mipped cloud;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace helipass
