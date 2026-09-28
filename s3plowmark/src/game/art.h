// S3 PLOWMARK pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace plow {

enum Pal {
    PAL_HUD = 0,
    PAL_TRUCK = 1,
    PAL_SNOW = 2,
    PAL_PINE = 3,
    PAL_MARK = 4,
    PAL_BARN = 5,
    PAL_FLAKE = 6
};

struct Art {
    gs::Mipped truck;
    gs::Mipped blade[5];
    gs::Mipped paint;
    gs::Mipped stake;
    gs::Mipped pine;
    gs::Mipped barn;
    gs::Mipped fence;
    gs::Mipped spray;
    gs::Mipped flake;
    gs::Mipped word;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace plow
