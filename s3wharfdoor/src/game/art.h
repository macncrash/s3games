// Wharf pictures. Drawn into VRAM and sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace wharf {

enum Pal {
    PAL_HUD = 0,
    PAL_QUAY = 1,
    PAL_DOOR = 2,
    PAL_CREW = 3,
    PAL_LAMP = 4,
    PAL_CRATE = 5,
    PAL_WARN = 6,
    PAL_BIRD = 7,
    PAL_WATER = 12
};

struct Art {
    gs::Mipped door;
    gs::Mipped crew;
    gs::Mipped lamp;
    gs::Mipped crate;
    gs::Mipped gull;
    gs::Mipped pile;
    gs::Mipped chev;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wharf
