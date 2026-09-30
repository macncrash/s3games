// S3 BEACON LADD pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace beaconladd {

enum Pal {
    PAL_HUD = 0,
    PAL_KEEPER = 1,
    PAL_STONE = 2,
    PAL_LAMP = 3,
    PAL_IRON = 4,
    PAL_NIGHT = 5,
    PAL_SEA = 6
};

struct Art {
    gs::Mipped keeper[2];
    gs::Mipped climb;
    gs::Mipped beacon;
    gs::Mipped beam[3];
    gs::Mipped cliff;
    gs::Mipped rung;
    gs::Mipped star;
    gs::Mipped lamp;
    gs::Image wordOne;
    gs::Image wordFar;
    gs::Image wordGo;
    gs::Image wordDone;
    gs::Image wordHint;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace beaconladd
