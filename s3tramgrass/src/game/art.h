// S3 TRAM GRASS sprites. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tramgrass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TRAM = 1,
    PAL_POLE = 2,
    PAL_WIRE = 3,
    PAL_WIN = 4,
    PAL_ALERT = 5,
    PAL_BANNER = 6,
    PAL_SPARK = 7,
    PAL_SIGN = 8,
    PAL_RAIL = 12,
    PAL_FIELD = 13
};

struct Art {
    gs::Mipped tram[8];
    gs::Mipped shade;
    gs::Mipped pole, tuft, board, spark;
    gs::Mipped title, fullStop, onGrass;
    gs::Mipped past, leftMeadow, onRails, wheels, clocked, failed, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tramgrass
