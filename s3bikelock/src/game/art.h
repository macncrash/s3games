// S3 BIKE LOCK pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bikelock {

enum Pal {
    PAL_HUD = 0,
    PAL_BIKE = 1,
    PAL_GATE = 2,
    PAL_STONE = 3,
    PAL_WATER = 4,
    PAL_TREE = 5,
    PAL_ALERT = 6
};

struct Art {
    gs::Mipped bike;
    gs::Mipped duck;
    gs::Mipped gate;
    gs::Mipped stone;
    gs::Mipped water;
    gs::Mipped tree;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bikelock
