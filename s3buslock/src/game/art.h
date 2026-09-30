// S3 BUS LOCK pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace buslock {

enum Pal {
    PAL_HUD = 0,
    PAL_BUS = 1,
    PAL_GATE = 2,
    PAL_STONE = 3,
    PAL_WATER = 4,
    PAL_BANK = 5,
    PAL_CREW = 6,
    PAL_ALERT = 7
};

struct Art {
    gs::Mipped bus;
    gs::Mipped crew;
    gs::Mipped gate;
    gs::Mipped stone;
    gs::Mipped water;
    gs::Mipped lamp;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace buslock
