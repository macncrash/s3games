// S3 KEELGRASS pictures. Drawn into VRAM and sprite ROM at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keel {

enum Pal {
    PAL_HUD = 0,
    PAL_BOAT = 1,
    PAL_WATER = 2,
    PAL_GRASS = 3,
    PAL_STONE = 4,
    PAL_SPRAY = 5
};

struct Art {
    gs::Mipped boat;
    gs::Mipped keel;
    gs::Mipped water;
    gs::Mipped grass;
    gs::Mipped stone;
    gs::Mipped spray;
    gs::Mipped flag;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keel
