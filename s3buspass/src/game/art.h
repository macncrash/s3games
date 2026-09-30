// S3 BUS PASS pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace buspass {

enum Pal {
    PAL_HUD = 0,
    PAL_BUS = 1,
    PAL_SNOW = 2,
    PAL_ROCK = 3,
    PAL_PINE = 4,
    PAL_ROAD = 5,
    PAL_CREW = 6,
    PAL_ALERT = 7
};

struct Art {
    gs::Mipped bus;
    gs::Mipped crew;
    gs::Mipped gate;
    gs::Mipped pine;
    gs::Mipped rock;
    gs::Mipped road;
    gs::Mipped peak;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace buspass
