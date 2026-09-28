// S3 KEYSTAPE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keys {

enum Pal { PAL_HUD = 0, PAL_DESK = 1, PAL_KEY = 2, PAL_TAPE = 3, PAL_DOOR = 4 };

struct Art {
    gs::Mipped desk;
    gs::Mipped drawer;
    gs::Mipped tape;
    gs::Mipped bow;
    gs::Mipped blade;
    gs::Mipped tooth[3];
    gs::Mipped pip;
    gs::Mipped cursor;
    gs::Mipped door;
    gs::Mipped lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keys
